/*
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *    Author : Boris Mikhailenko <stg34@stargazer.dp.ua>
 *    Author : Maxim Mamontov <faust@stargazer.dp.ua>
 */

#include <sys/time.h>

#include <csignal>
#include <cassert>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <algorithm>

#include "stg/common.h"
#include "stg/locker.h"
#include "stg/users.h"
#include "stg/user_property.h"
#include "stg/plugin_creator.h"
#include "stg/logger.h"
#include "rscript.h"
#include "ur_functor.h"
#include "send_functor.h"

extern volatile const time_t stgTime;

#define RS_MAX_ROUTERS  (100)

using RS::REMOTE_SCRIPT;

namespace {

template<typename T>
struct USER_IS
{
    USER_IS(USER_PTR u) : user(u) {}
    bool operator()(const T & notifier) { return notifier.GetUser() == user; }

    USER_PTR user;
};

} // namespace anonymous

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
PLUGIN_CREATOR<REMOTE_SCRIPT> rsc;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
PLUGIN * GetPlugin()
{
return rsc.GetPlugin();
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
RS::SETTINGS::SETTINGS()
    : sendPeriod(0),
      port(0),
      errorStr(),
      netRouters(),
      userParams(),
      password(),
      subnetFile()
{
}
//-----------------------------------------------------------------------------
int RS::SETTINGS::ParseSettings(const MODULE_SETTINGS & s)
{
int p;
PARAM_VALUE pv;
vector<PARAM_VALUE>::const_iterator pvi;
netRouters.clear();
///////////////////////////
pv.param = "Port";
pvi = find(s.moduleParams.begin(), s.moduleParams.end(), pv);
if (pvi == s.moduleParams.end())
    {
    errorStr = "Parameter \'Port\' not found.";
    printfd(__FILE__, "Parameter 'Port' not found\n");
    return -1;
    }
if (ParseIntInRange(pvi->value[0], 2, 65535, &p))
    {
    errorStr = "Cannot parse parameter \'Port\': " + errorStr;
    printfd(__FILE__, "Cannot parse parameter 'Port'\n");
    return -1;
    }
port = p;
///////////////////////////
pv.param = "SendPeriod";
pvi = find(s.moduleParams.begin(), s.moduleParams.end(), pv);
if (pvi == s.moduleParams.end())
    {
    errorStr = "Parameter \'SendPeriod\' not found.";
    printfd(__FILE__, "Parameter 'SendPeriod' not found\n");
    return -1;
    }

if (ParseIntInRange(pvi->value[0], 5, 600, &sendPeriod))
    {
    errorStr = "Cannot parse parameter \'SendPeriod\': " + errorStr;
    printfd(__FILE__, "Cannot parse parameter 'SendPeriod'\n");
    return -1;
    }
///////////////////////////
pv.param = "UserParams";
pvi = find(s.moduleParams.begin(), s.moduleParams.end(), pv);
if (pvi == s.moduleParams.end())
    {
    errorStr = "Parameter \'UserParams\' not found.";
    printfd(__FILE__, "Parameter 'UserParams' not found\n");
    return -1;
    }
userParams = pvi->value;
///////////////////////////
pv.param = "Password";
pvi = find(s.moduleParams.begin(), s.moduleParams.end(), pv);
if (pvi == s.moduleParams.end())
    {
    errorStr = "Parameter \'Password\' not found.";
    printfd(__FILE__, "Parameter 'Password' not found\n");
    return -1;
    }
password = pvi->value[0];
///////////////////////////
pv.param = "SubnetFile";
pvi = find(s.moduleParams.begin(), s.moduleParams.end(), pv);
if (pvi == s.moduleParams.end())
    {
    errorStr = "Parameter \'SubnetFile\' not found.";
    printfd(__FILE__, "Parameter 'SubnetFile' not found\n");
    return -1;
    }
subnetFile = pvi->value[0];

NRMapParser nrMapParser;

if (!nrMapParser.ReadFile(subnetFile))
    {
    netRouters = nrMapParser.GetMap();
    }
else
    {
    GetStgLogger()("mod_rscript: error opening subnets file '%s'", subnetFile.c_str());
    }

return 0;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
REMOTE_SCRIPT::REMOTE_SCRIPT()
    : ctx(),
      ipNotifierList(),
      connNotifierList(),
      authorizedUsers(),
      errorStr(),
      rsSettings(),
      settings(),
      sendPeriod(15),
      halfPeriod(8),
      nonstop(false),
      isRunning(false),
      users(NULL),
      netRouters(),
      thread(),
      mutex(),
      sock(0),
      onAddUserNotifier(*this),
      onDelUserNotifier(*this),
      logger(GetPluginLogger(GetStgLogger(), "rscript"))
{
pthread_mutex_init(&mutex, NULL);
}
//-----------------------------------------------------------------------------
REMOTE_SCRIPT::~REMOTE_SCRIPT()
{
pthread_mutex_destroy(&mutex);
}
//-----------------------------------------------------------------------------
void * REMOTE_SCRIPT::Run(void * d)
{
sigset_t signalSet;
sigfillset(&signalSet);
pthread_sigmask(SIG_BLOCK, &signalSet, NULL);

REMOTE_SCRIPT * rs = static_cast<REMOTE_SCRIPT *>(d);

rs->isRunning = true;

while (rs->nonstop)
    {
    rs->PeriodicSend();
    sleep(2);
    }

rs->isRunning = false;
return NULL;
}
//-----------------------------------------------------------------------------
int REMOTE_SCRIPT::ParseSettings()
{
int ret = rsSettings.ParseSettings(settings);
if (ret)
    errorStr = rsSettings.GetStrError();

sendPeriod = rsSettings.GetSendPeriod();
halfPeriod = sendPeriod / 2;

return ret;
}
//-----------------------------------------------------------------------------
int REMOTE_SCRIPT::Start()
{
netRouters = rsSettings.GetSubnetsMap();

InitEncrypt(&ctx, rsSettings.GetPassword());

users->AddNotifierUserAdd(&onAddUserNotifier);
users->AddNotifierUserDel(&onDelUserNotifier);

nonstop = true;

if (GetUsers())
    {
    return -1;
    }

if (PrepareNet())
    {
    return -1;
    }

if (!isRunning)
    {
    if (pthread_create(&thread, NULL, Run, this))
        {
        errorStr = "Cannot create thread.";
	logger("Cannot create thread.");
        printfd(__FILE__, "Cannot create thread\n");
        return -1;
        }
    }

errorStr = "";
return 0;
}
//-----------------------------------------------------------------------------
int REMOTE_SCRIPT::Stop()
{
if (!IsRunning())
    return 0;

nonstop = false;

std::for_each(
        authorizedUsers.begin(),
        authorizedUsers.end(),
        DisconnectUser(*this)
        );

FinalizeNet();

if (isRunning)
    {
    //5 seconds to thread stops itself
    for (int i = 0; i < 25 && isRunning; i++)
        {
        struct timespec ts = {0, 200000000};
        nanosleep(&ts, NULL);
        }
    }

users->DelNotifierUserDel(&onDelUserNotifier);
users->DelNotifierUserAdd(&onAddUserNotifier);

if (isRunning)
    {
    logger("Cannot stop thread.");
    return -1;
    }

return 0;
}
//-----------------------------------------------------------------------------
int REMOTE_SCRIPT::Reload()
{
NRMapParser nrMapParser;

if (nrMapParser.ReadFile(rsSettings.GetMapFileName()))
    {
    errorStr = nrMapParser.GetErrorStr();
    logger("Map file reading error: %s", errorStr.c_str());
    return -1;
    }

    {
    STG_LOCKER lock(&mutex, __FILE__, __LINE__);

    printfd(__FILE__, "REMOTE_SCRIPT::Reload()\n");

    netRouters = nrMapParser.GetMap();
    }

std::for_each(authorizedUsers.begin(),
              authorizedUsers.end(),
              UpdateRouter(*this));

return 0;
}
//-----------------------------------------------------------------------------
bool REMOTE_SCRIPT::PrepareNet()
{
sock = socket(AF_INET, SOCK_DGRAM, 0);

if (sock < 0)
    {
    errorStr = "Cannot create socket.";
    logger("Canot create a socket: %s", strerror(errno));
    printfd(__FILE__, "Cannot create socket\n");
    return true;
    }

return false;
}
//-----------------------------------------------------------------------------
bool REMOTE_SCRIPT::FinalizeNet()
{
close(sock);
return false;
}
//-----------------------------------------------------------------------------
void REMOTE_SCRIPT::PeriodicSend()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

map<uint32_t, RS::USER>::iterator it(authorizedUsers.begin());
while (it != authorizedUsers.end())
    {
    if (difftime(stgTime, it->second.lastSentTime) - (rand() % halfPeriod) > sendPeriod)
        {
        Send(it->second);
        }
    ++it;
    }
}
//-----------------------------------------------------------------------------
#ifdef NDEBUG
bool REMOTE_SCRIPT::PreparePacket(char * buf, size_t, RS::USER & rsu, bool forceDisconnect) const
#else
bool REMOTE_SCRIPT::PreparePacket(char * buf, size_t bufSize, RS::USER & rsu, bool forceDisconnect) const
#endif
{
RS::PACKET_HEADER packetHead;

memset(packetHead.padding, 0, sizeof(packetHead.padding));
strcpy((char*)packetHead.magic, RS_ID);
packetHead.protoVer[0] = '0';
packetHead.protoVer[1] = '2';
if (forceDisconnect)
    {
    packetHead.packetType = RS_DISCONNECT_PACKET;
    printfd(__FILE__, "RSCRIPT: force disconnect for '%s'\n", rsu.user->GetLogin().c_str());
    }
else
    {
    if (rsu.shortPacketsCount % MAX_SHORT_PCKT == 0)
        {
        //SendLong
        packetHead.packetType = rsu.user->IsInetable() ? RS_CONNECT_PACKET : RS_DISCONNECT_PACKET;
        if (rsu.user->IsInetable())
            printfd(__FILE__, "RSCRIPT: connect for '%s'\n", rsu.user->GetLogin().c_str());
        else
            printfd(__FILE__, "RSCRIPT: disconnect for '%s'\n", rsu.user->GetLogin().c_str());
        }
    else
        {
        //SendShort
        packetHead.packetType = rsu.user->IsInetable() ? RS_ALIVE_PACKET : RS_DISCONNECT_PACKET;
        if (rsu.user->IsInetable())
            printfd(__FILE__, "RSCRIPT: alive for '%s'\n", rsu.user->GetLogin().c_str());
        else
            printfd(__FILE__, "RSCRIPT: disconnect for '%s'\n", rsu.user->GetLogin().c_str());
        }
    }
rsu.shortPacketsCount++;
rsu.lastSentTime = stgTime;

packetHead.ip = htonl(rsu.ip);
packetHead.id = htonl(rsu.user->GetID());
strncpy((char*)packetHead.login, rsu.user->GetLogin().c_str(), RS_LOGIN_LEN);
packetHead.login[RS_LOGIN_LEN - 1] = 0;

memcpy(buf, &packetHead, sizeof(packetHead));

if (packetHead.packetType == RS_ALIVE_PACKET)
    {
    return false;
    }

RS::PACKET_TAIL packetTail;

memset(packetTail.padding, 0, sizeof(packetTail.padding));
strcpy((char*)packetTail.magic, RS_ID);
vector<string>::const_iterator it;
std::string params;
for(it = rsSettings.GetUserParams().begin();
    it != rsSettings.GetUserParams().end();
    ++it)
    {
    std::string parameter(GetUserParam(rsu.user, *it));
    if (params.length() + parameter.length() > RS_PARAMS_LEN - 1)
        break;
    params += parameter + " ";
    }
strncpy((char *)packetTail.params, params.c_str(), RS_PARAMS_LEN);
packetTail.params[RS_PARAMS_LEN - 1] = 0;

assert(sizeof(packetHead) + sizeof(packetTail) <= bufSize && "Insufficient buffer space");

Encrypt(&ctx, buf + sizeof(packetHead), (char *)&packetTail, sizeof(packetTail) / 8);

return false;
}
//-----------------------------------------------------------------------------
bool REMOTE_SCRIPT::Send(RS::USER & rsu, bool forceDisconnect) const
{
char buffer[RS_MAX_PACKET_LEN];

memset(buffer, 0, sizeof(buffer));

if (PreparePacket(buffer, sizeof(buffer), rsu, forceDisconnect))
    {
    printfd(__FILE__, "REMOTE_SCRIPT::Send() - Invalid packet length!\n");
    return true;
    }

std::for_each(
        rsu.routers.begin(),
        rsu.routers.end(),
        PacketSender(sock, buffer, sizeof(buffer), htons(rsSettings.GetPort()))
        );

return false;
}
//-----------------------------------------------------------------------------
bool REMOTE_SCRIPT::SendDirect(RS::USER & rsu, uint32_t routerIP, bool forceDisconnect) const
{
char buffer[RS_MAX_PACKET_LEN];

if (PreparePacket(buffer, sizeof(buffer), rsu, forceDisconnect))
    {
    printfd(__FILE__, "REMOTE_SCRIPT::SendDirect() - Invalid packet length!\n");
    return true;
    }

struct sockaddr_in sendAddr;

sendAddr.sin_family = AF_INET;
sendAddr.sin_port = htons(rsSettings.GetPort());
sendAddr.sin_addr.s_addr = routerIP;

int res = sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&sendAddr, sizeof(sendAddr));

if (res < 0)
    logger("sendto error: %s", strerror(errno));

return (res != sizeof(buffer));
}
//-----------------------------------------------------------------------------
bool REMOTE_SCRIPT::GetUsers()
{
USER_PTR u;

int h = users->OpenSearch();
assert(h && "USERS::OpenSearch is always correct");

while (!users->SearchNext(h, &u))
    {
    SetUserNotifiers(u);
    }

users->CloseSearch(h);
return false;
}
//-----------------------------------------------------------------------------
std::vector<uint32_t> REMOTE_SCRIPT::IP2Routers(uint32_t ip)
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
for (size_t i = 0; i < netRouters.size(); ++i)
    {
    if ((ip & netRouters[i].subnetMask) == (netRouters[i].subnetIP & netRouters[i].subnetMask))
        {
        return netRouters[i].routers;
        }
    }
return std::vector<uint32_t>();
}
//-----------------------------------------------------------------------------
string REMOTE_SCRIPT::GetUserParam(USER_PTR u, const string & paramName) const
{
string value = "";
if (strcasecmp(paramName.c_str(), "cash") == 0)
    strprintf(&value, "%f", u->GetProperty().cash.Get());
else
if (strcasecmp(paramName.c_str(), "freeMb") == 0)
    strprintf(&value, "%f", u->GetProperty().freeMb.Get());
else
if (strcasecmp(paramName.c_str(), "passive") == 0)
    strprintf(&value, "%d", u->GetProperty().passive.Get());
else
if (strcasecmp(paramName.c_str(), "disabled") == 0)
    strprintf(&value, "%d", u->GetProperty().disabled.Get());
else
if (strcasecmp(paramName.c_str(), "alwaysOnline") == 0)
    strprintf(&value, "%d", u->GetProperty().alwaysOnline.Get());
else
if (strcasecmp(paramName.c_str(), "tariffName") == 0 ||
    strcasecmp(paramName.c_str(), "tariff") == 0)
    value = "\"" + u->GetProperty().tariffName.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "nextTariff") == 0)
    value = "\"" + u->GetProperty().nextTariff.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "address") == 0)
    value = "\"" + u->GetProperty().address.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "note") == 0)
    value = "\"" + u->GetProperty().note.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "group") == 0)
    value = "\"" + u->GetProperty().group.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "email") == 0)
    value = "\"" + u->GetProperty().email.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "realName") == 0)
    value = "\"" + u->GetProperty().realName.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "credit") == 0)
    strprintf(&value, "%f", u->GetProperty().credit.Get());
else
if (strcasecmp(paramName.c_str(), "userdata0") == 0)
    value = "\"" + u->GetProperty().userdata0.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "userdata1") == 0)
    value = "\"" + u->GetProperty().userdata1.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "userdata2") == 0)
    value = "\"" + u->GetProperty().userdata2.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "userdata3") == 0)
    value = "\"" + u->GetProperty().userdata3.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "userdata4") == 0)
    value = "\"" + u->GetProperty().userdata4.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "userdata5") == 0)
    value = "\"" + u->GetProperty().userdata5.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "userdata6") == 0)
    value = "\"" + u->GetProperty().userdata6.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "userdata7") == 0)
    value = "\"" + u->GetProperty().userdata7.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "userdata8") == 0)
    value = "\"" + u->GetProperty().userdata8.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "userdata9") == 0)
    value = "\"" + u->GetProperty().userdata9.Get() + "\"";
else
if (strcasecmp(paramName.c_str(), "enabledDirs") == 0)
    value = u->GetEnabledDirs();
else
    printfd(__FILE__, "Unknown value name: %s\n", paramName.c_str());
return value;
}
//-----------------------------------------------------------------------------
void REMOTE_SCRIPT::SetUserNotifiers(USER_PTR u)
{
ipNotifierList.push_front(RS::IP_NOTIFIER(*this, u));
connNotifierList.push_front(RS::CONNECTED_NOTIFIER(*this, u));
}
//-----------------------------------------------------------------------------
void REMOTE_SCRIPT::UnSetUserNotifiers(USER_PTR u)
{
ipNotifierList.erase(std::remove_if(ipNotifierList.begin(),
                                    ipNotifierList.end(),
                                    USER_IS<IP_NOTIFIER>(u)),
                     ipNotifierList.end());
connNotifierList.erase(std::remove_if(connNotifierList.begin(),
                                      connNotifierList.end(),
                                      USER_IS<CONNECTED_NOTIFIER>(u)),
                       connNotifierList.end());

}
//-----------------------------------------------------------------------------
void REMOTE_SCRIPT::AddRSU(USER_PTR user)
{
RS::USER rsu(IP2Routers(user->GetCurrIP()), user);
Send(rsu);

STG_LOCKER lock(&mutex, __FILE__, __LINE__);
authorizedUsers.insert(std::make_pair(user->GetCurrIP(), rsu));
}
//-----------------------------------------------------------------------------
void REMOTE_SCRIPT::DelRSU(USER_PTR user)
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
const map<uint32_t, RS::USER>::iterator it(
        authorizedUsers.find(user->GetCurrIP())
        );
if (it != authorizedUsers.end())
    {
    Send(it->second, true);
    authorizedUsers.erase(it);
    }
}
//-----------------------------------------------------------------------------
void RS::IP_NOTIFIER::Notify(const uint32_t & /*oldValue*/, const uint32_t & newValue)
{
if (newValue)
    rs.AddRSU(user);
else
    rs.DelRSU(user);
}
//-----------------------------------------------------------------------------
void RS::CONNECTED_NOTIFIER::Notify(const bool & /*oldValue*/, const bool & newValue)
{
if (newValue)
    rs.AddRSU(user);
else
    rs.DelRSU(user);
}
//-----------------------------------------------------------------------------
void REMOTE_SCRIPT::InitEncrypt(BLOWFISH_CTX * ctx, const string & password) const
{
unsigned char keyL[PASSWD_LEN];  // Пароль для шифровки
memset(keyL, 0, PASSWD_LEN);
strncpy((char *)keyL, password.c_str(), PASSWD_LEN);
Blowfish_Init(ctx, keyL, PASSWD_LEN);
}
//-----------------------------------------------------------------------------
void REMOTE_SCRIPT::Encrypt(BLOWFISH_CTX * ctx, char * dst, const char * src, size_t len8) const
{
if (dst != src)
    memcpy(dst, src, len8 * 8);
for (size_t i = 0; i < len8; ++i)
    Blowfish_Encrypt(ctx, (uint32_t *)(dst + i * 8), (uint32_t *)(dst + i * 8 + 4));
}
//-----------------------------------------------------------------------------
