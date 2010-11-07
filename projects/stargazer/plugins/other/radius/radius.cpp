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
 *    Author : Maxim Mamontov <faust@stargazer.dp.ua>
 */

/*
 *  This file contains a realization of radius data access plugin for Stargazer
 *
 *  $Revision: 1.14 $
 *  $Date: 2009/12/13 14:17:13 $
 *
 */

#include <algorithm>
#include <signal.h>

#include "radius.h"
#include "common.h"

extern volatile const time_t stgTime;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class RAD_CREATOR
{
private:
    RADIUS * rad;

public:
    RAD_CREATOR()
        : rad(new RADIUS())
        {
        };
    ~RAD_CREATOR()
        {
        delete rad;
        };

    RADIUS * GetPlugin()
        {
        return rad;
        };
};
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
RAD_CREATOR radc;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BASE_PLUGIN * GetPlugin()
{
return radc.GetPlugin();
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
uint16_t RAD_SETTINGS::GetPort() const
{
return port;
}
//-----------------------------------------------------------------------------
uint32_t RAD_SETTINGS::GetServerIP() const
{
return serverIP;
}
//-----------------------------------------------------------------------------
int RAD_SETTINGS::GetPassword(string * password) const
{
*password = RAD_SETTINGS::password;
return 0;
}
//-----------------------------------------------------------------------------
int RAD_SETTINGS::GetAuthServices(list<string> * svcs) const
{
*svcs = authServices;
return 0;
}
//-----------------------------------------------------------------------------
int RAD_SETTINGS::GetAcctServices(list<string> * svcs) const
{
*svcs = acctServices;
return 0;
}
//-----------------------------------------------------------------------------
int RAD_SETTINGS::ParseIP(const string & str, uint32_t * IP)
{
*IP = inet_addr(str.c_str());
return *IP == INADDR_NONE ? -1 : 0;
}
//-----------------------------------------------------------------------------
int RAD_SETTINGS::ParseIntInRange(const string & str, int min, int max, int * val)
{
if (str2x(str.c_str(), *val))
    {
    errorStr = "Incorrect value \'" + str + "\'.";
    return -1;
    }
if (*val < min || *val > max)
    {
    errorStr = "Value \'" + str + "\' out of range.";
    return -1;
    }
return 0;
}
//-----------------------------------------------------------------------------
int RAD_SETTINGS::ParseServices(const vector<string> & str, list<string> * lst)
{
    copy(str.begin(), str.end(), back_inserter(*lst));
    list<string>::iterator it(find(lst->begin(),
                                   lst->end(),
                                   "empty"));
    if (it != lst->end())
        *it = "";

    return 0;
}
//-----------------------------------------------------------------------------
int RAD_SETTINGS::ParseSettings(const MODULE_SETTINGS & s)
{
int p;
PARAM_VALUE pv;
vector<PARAM_VALUE>::const_iterator pvi;
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
pv.param = "ServerIP";
pvi = find(s.moduleParams.begin(), s.moduleParams.end(), pv);
if (pvi == s.moduleParams.end())
    {
    serverIP = 0;
    }
else
    {
    if (ParseIP(pvi->value[0], &serverIP))
        {
        serverIP = 0;
        }
    }
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
pv.param = "AuthServices";
pvi = find(s.moduleParams.begin(), s.moduleParams.end(), pv);
if (pvi != s.moduleParams.end())
    {
    ParseServices(pvi->value, &authServices);
    }
///////////////////////////
pv.param = "AcctServices";
pvi = find(s.moduleParams.begin(), s.moduleParams.end(), pv);
if (pvi != s.moduleParams.end())
    {
    ParseServices(pvi->value, &acctServices);
    }

return 0;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
RADIUS::RADIUS()
{
isRunning = false;
}
//-----------------------------------------------------------------------------
void RADIUS::SetUsers(USERS * u)
{
users = u;
}
//-----------------------------------------------------------------------------
void RADIUS::SetStgSettings(const SETTINGS * s)
{
stgSettings = s;
}
//-----------------------------------------------------------------------------
void RADIUS::SetSettings(const MODULE_SETTINGS & s)
{
settings = s;
}
//-----------------------------------------------------------------------------
void RADIUS::SetStore(BASE_STORE * s)
{
store = s;
}
//-----------------------------------------------------------------------------
int RADIUS::ParseSettings()
{
int ret = radSettings.ParseSettings(settings);
if (ret)
    errorStr = radSettings.GetStrError();
return ret;
}
//-----------------------------------------------------------------------------
bool RADIUS::IsRunning()
{
return isRunning;
}
//-----------------------------------------------------------------------------
const string RADIUS::GetVersion() const
{
return "RADIUS data access plugin v 0.6";
}
//-----------------------------------------------------------------------------
uint16_t RADIUS::GetStartPosition() const
{
// Start before any authorizers!!!
return 20;
}
//-----------------------------------------------------------------------------
uint16_t RADIUS::GetStopPosition() const
{
return 20;
}
//-----------------------------------------------------------------------------
void RADIUS::SetUserNotifier(user_iter)
{
}
//-----------------------------------------------------------------------------
void RADIUS::UnSetUserNotifier(user_iter)
{
}
//-----------------------------------------------------------------------------
int RADIUS::PrepareNet()
{
sock = socket(AF_INET, SOCK_DGRAM, 0);

if (sock < 0)
    {
    errorStr = "Cannot create socket.";
    printfd(__FILE__, "Cannot create socket\n");
    return -1;
    }

inAddr.sin_family = AF_INET;
inAddr.sin_port = htons(port);
inAddr.sin_addr.s_addr = inet_addr("0.0.0.0");

if (bind(sock, (struct sockaddr*)&inAddr, sizeof(inAddr)) < 0)
    {
    errorStr = "RADIUS: Bind failed.";
    printfd(__FILE__, "Cannot bind socket\n");
    return -1;
    }

return 0;
}
//-----------------------------------------------------------------------------
int RADIUS::FinalizeNet()
{
close(sock);
return 0;
}
//-----------------------------------------------------------------------------
int RADIUS::Start()
{
string password;

radSettings.GetPassword(&password);
port = radSettings.GetPort();
serverIP = radSettings.GetServerIP();
radSettings.GetAuthServices(&authServices);
radSettings.GetAcctServices(&acctServices);

InitEncrypt(&ctx, password);

nonstop = true;

if (PrepareNet())
    {
    return -1;
    }

if (!isRunning)
    {
    if (pthread_create(&thread, NULL, Run, this))
        {
        errorStr = "Cannot create thread.";
        printfd(__FILE__, "Cannot create thread\n");
        return -1;
        }
    }

errorStr = "";
return 0;
}
//-----------------------------------------------------------------------------
int RADIUS::Stop()
{
if (!IsRunning())
    return 0;

nonstop = false;

map<string, RAD_SESSION>::iterator it;
for (it = sessions.begin(); it != sessions.end(); ++it)
    {
    user_iter ui;
    if (users->FindByName(it->second.userName, &ui))
        {
        ui->Unauthorize(this);
        }
    }
sessions.erase(sessions.begin(), sessions.end());

FinalizeNet();

if (isRunning)
    {
    //5 seconds to thread stops itself
    for (int i = 0; i < 25 && isRunning; i++)
        {
        usleep(200000);
        }

    //after 5 seconds waiting thread still running. now killing it
    if (isRunning)
        {
        if (pthread_kill(thread, SIGINT))
            {
            errorStr = "Cannot kill thread.";
            printfd(__FILE__, "Cannot kill thread\n");
            return -1;
            }
        printfd(__FILE__, "RADIUS::Stop killed Run\n");
        }
    }

return 0;
}
//-----------------------------------------------------------------------------
void * RADIUS::Run(void * d)
{
RADIUS * rad = (RADIUS *)d;
RAD_PACKET packet;

rad->isRunning = true;

while (rad->nonstop)
    {
    if (!rad->WaitPackets(rad->sock))
        {
        continue;
        }
    if (rad->RecvData(&packet))
        {
        printfd(__FILE__, "RADIUS::Run Error on RecvData\n");
        }
    else
        {
        if (rad->ProcessData(&packet))
            {
            packet.packetType = RAD_REJECT_PACKET;
            }
        rad->Send(packet);
        }
    }

rad->isRunning = false;

return NULL;
}
//-----------------------------------------------------------------------------
int RADIUS::RecvData(RAD_PACKET * packet)
{
    int8_t buf[RAD_MAX_PACKET_LEN];
    outerAddrLen = sizeof(struct sockaddr_in);
    int dataLen = recvfrom(sock, buf, RAD_MAX_PACKET_LEN, 0, (struct sockaddr *)&outerAddr, &outerAddrLen);
    if (dataLen > 0) {
        Decrypt(&ctx, (char *)packet, (const char *)buf, dataLen / 8);
    }
    if (strncmp((char *)packet->magic, RAD_ID, RAD_MAGIC_LEN))
        {
        printfd(__FILE__, "RADIUS::RecvData Error magic. Wanted: '%s', got: '%s'\n", RAD_ID, packet->magic);
        return -1;
        }
    return 0;
}
//-----------------------------------------------------------------------------
int RADIUS::Send(const RAD_PACKET & packet)
{
int res, len = sizeof(RAD_PACKET);
char buf[1032];

Encrypt(&ctx, buf, (char *)&packet, len / 8);
res = sendto(sock, buf, len, 0, (struct sockaddr *)&outerAddr, outerAddrLen);

return 0;
}
//-----------------------------------------------------------------------------
int RADIUS::ProcessData(RAD_PACKET * packet)
{
//struct in_addr addr = {packet->ip};
if (strncmp((const char *)packet->protoVer, "01", 2))
    {
    printfd(__FILE__, "RADIUS::ProcessData packet.protoVer incorrect\n");
    return -1;
    }
switch (packet->packetType)
    {
    case RAD_AUTZ_PACKET:
        return ProcessAutzPacket(packet);
    case RAD_AUTH_PACKET:
        return ProcessAuthPacket(packet);
    case RAD_POST_AUTH_PACKET:
        return ProcessPostAuthPacket(packet);
    case RAD_ACCT_START_PACKET:
        return ProcessAcctStartPacket(packet);
    case RAD_ACCT_STOP_PACKET:
        return ProcessAcctStopPacket(packet);
    case RAD_ACCT_UPDATE_PACKET:
        return ProcessAcctUpdatePacket(packet);
    case RAD_ACCT_OTHER_PACKET:
        return ProcessAcctOtherPacket(packet);
    default:
        printfd(__FILE__, "RADIUS::ProcessData Unsupported packet type: %d\n", packet->packetType);
        return -1;
    };
return 0;
}
//-----------------------------------------------------------------------------
int RADIUS::ProcessAutzPacket(RAD_PACKET * packet)
{
USER_CONF conf;

if (!IsAllowedService((char *)packet->service))
    {
    printfd(__FILE__, "RADIUS::ProcessAutzPacket service '%s' is not allowed to authorize\n", packet->service);
    packet->packetType = RAD_REJECT_PACKET;
    return 0;
    }

if (store->RestoreUserConf(&conf, (char *)packet->login))
    {
    packet->packetType = RAD_REJECT_PACKET;
    printfd(__FILE__, "RADIUS::ProcessAutzPacket cannot restore conf for user '%s'\n", packet->login);
    return 0;
    }

// At this point service can be authorized at least
// So we send a plain-text password

packet->packetType = RAD_ACCEPT_PACKET;
strncpy((char *)packet->password, conf.password.c_str(), RAD_PASSWORD_LEN);

return 0;
}
//-----------------------------------------------------------------------------
int RADIUS::ProcessAuthPacket(RAD_PACKET * packet)
{
user_iter ui;

if (!CanAcctService((char *)packet->service))
    {

    // There are no sense to check for allowed service
    // It has allready checked at previous stage (authorization)

    printfd(__FILE__, "RADIUS::ProcessAuthPacket service '%s' neednot stargazer authentication\n", (char *)packet->service);
    packet->packetType = RAD_ACCEPT_PACKET;
    return 0;
    }

// At this point we have an accountable service
// All other services got a password if allowed or rejected

if (!FindUser(&ui, (char *)packet->login))
    {
    packet->packetType = RAD_REJECT_PACKET;
    printfd(__FILE__, "RADIUS::ProcessAuthPacket user '%s' not found\n", (char *)packet->login);
    return 0;
    }

if (ui->IsInetable())
    {
    packet->packetType = RAD_ACCEPT_PACKET;
    }
else
    {
    packet->packetType = RAD_REJECT_PACKET;
    }

packet->packetType = RAD_ACCEPT_PACKET;
return 0;
}
//-----------------------------------------------------------------------------
int RADIUS::ProcessPostAuthPacket(RAD_PACKET * packet)
{
user_iter ui;

if (!CanAcctService((char *)packet->service))
    {

    // There are no sense to check for allowed service
    // It has allready checked at previous stage (authorization)

    packet->packetType = RAD_ACCEPT_PACKET;
    return 0;
    }

if (!FindUser(&ui, (char *)packet->login))
    {
    packet->packetType = RAD_REJECT_PACKET;
    printfd(__FILE__, "RADIUS::ProcessPostAuthPacket user '%s' not found\n", (char *)packet->login);
    return 0;
    }

// I think that only Framed-User services has sense to be accountable
// So we have to supply a Framed-IP

USER_IPS ips = ui->property.ips;
packet->packetType = RAD_ACCEPT_PACKET;

// Additional checking for Framed-User service

if (!strncmp((char *)packet->service, "Framed-User", RAD_SERVICE_LEN))
    packet->ip = ips[0].ip;
else
    packet->ip = 0;

return 0;
}
//-----------------------------------------------------------------------------
int RADIUS::ProcessAcctStartPacket(RAD_PACKET * packet)
{
user_iter ui;

if (!FindUser(&ui, (char *)packet->login))
    {
    packet->packetType = RAD_REJECT_PACKET;
    printfd(__FILE__, "RADIUS::ProcessAcctStartPacket user '%s' not found\n", (char *)packet->login);
    return 0;
    }

// At this point we have to unauthorize user only if it is an accountable service

if (CanAcctService((char *)packet->service))
    {
    if (sessions.find((const char *)packet->sessid) != sessions.end())
        {
        printfd(__FILE__, "RADIUS::ProcessAcctStartPacket session already started!\n");
        packet->packetType = RAD_REJECT_PACKET;
        return -1;
        }
    USER_IPS ips = ui->property.ips;
    if (ui->Authorize(ips[0].ip, "", 0xffFFffFF, this))
        {
        printfd(__FILE__, "RADIUS::ProcessAcctStartPacket cannot authorize user '%s'\n", packet->login);
        packet->packetType = RAD_REJECT_PACKET;
        return -1;
        }
    sessions[(const char *)packet->sessid].userName = (const char *)packet->login;
    sessions[(const char *)packet->sessid].serviceType = (const char *)packet->service;
    for_each(sessions.begin(), sessions.end(), SPrinter());
    }
else
    {
    printfd(__FILE__, "RADIUS::ProcessAcctStartPacket service '%s' can not be accounted\n", (char *)packet->service);
    }

packet->packetType = RAD_ACCEPT_PACKET;
return 0;
}
//-----------------------------------------------------------------------------
int RADIUS::ProcessAcctStopPacket(RAD_PACKET * packet)
{
map<string, RAD_SESSION>::iterator sid;

if ((sid = sessions.find((const char *)packet->sessid)) == sessions.end())
    {
    printfd(__FILE__, "RADIUS::ProcessAcctStopPacket session had not started yet\n");
    packet->packetType = RAD_REJECT_PACKET;
    return -1;
    }

user_iter ui;

if (!FindUser(&ui, sid->second.userName))
    {
    packet->packetType = RAD_REJECT_PACKET;
    printfd(__FILE__, "RADIUS::ProcessPostAuthPacket user '%s' not found\n", sid->second.userName.c_str());
    return 0;
    }

sessions.erase(sid);

ui->Unauthorize(this);

packet->packetType = RAD_ACCEPT_PACKET;
return 0;
}
//-----------------------------------------------------------------------------
int RADIUS::ProcessAcctUpdatePacket(RAD_PACKET * packet)
{
// Fake. May be used later
packet->packetType = RAD_ACCEPT_PACKET;
return 0;
}
//-----------------------------------------------------------------------------
int RADIUS::ProcessAcctOtherPacket(RAD_PACKET * packet)
{
// Fake. May be used later
packet->packetType = RAD_ACCEPT_PACKET;
return 0;
}
//-----------------------------------------------------------------------------
void RADIUS::InitEncrypt(BLOWFISH_CTX * ctx, const string & password)
{
unsigned char keyL[RAD_PASSWORD_LEN];  // Пароль для шифровки
memset(keyL, 0, RAD_PASSWORD_LEN);
strncpy((char *)keyL, password.c_str(), RAD_PASSWORD_LEN);
Blowfish_Init(ctx, keyL, RAD_PASSWORD_LEN);
}
//-----------------------------------------------------------------------------
void RADIUS::Encrypt(BLOWFISH_CTX * ctx, char * dst, const char * src, int len8)
{
// len8 - длина в 8-ми байтовых блоках
if (dst != src)
    memcpy(dst, src, len8 * 8);

for (int i = 0; i < len8; i++)
    Blowfish_Encrypt(ctx, (uint32_t *)(dst + i*8), (uint32_t *)(dst + i*8 + 4));
}
//-----------------------------------------------------------------------------
void RADIUS::Decrypt(BLOWFISH_CTX * ctx, char * dst, const char * src, int len8)
{
// len8 - длина в 8-ми байтовых блоках
if (dst != src)
    memcpy(dst, src, len8 * 8);

for (int i = 0; i < len8; i++)
    Blowfish_Decrypt(ctx, (uint32_t *)(dst + i*8), (uint32_t *)(dst + i*8 + 4));
}
//-----------------------------------------------------------------------------
void RADIUS::PrintServices(const list<string> & svcs)
{
    for_each(svcs.begin(), svcs.end(), Printer());
}
//-----------------------------------------------------------------------------
bool RADIUS::FindUser(user_iter * ui, const std::string & login) const
{
if (users->FindByName(login, ui))
    {
    return false;
    }
return true;
}
//-----------------------------------------------------------------------------
bool RADIUS::CanAuthService(const std::string & svc) const
{
    return find(authServices.begin(), authServices.end(), svc) != authServices.end();
}
//-----------------------------------------------------------------------------
bool RADIUS::CanAcctService(const std::string & svc) const
{
    return find(acctServices.begin(), acctServices.end(), svc) != acctServices.end();
}
//-----------------------------------------------------------------------------
bool RADIUS::IsAllowedService(const std::string & svc) const
{
    return CanAuthService(svc) || CanAcctService(svc);
}
//-----------------------------------------------------------------------------
bool RADIUS::WaitPackets(int sd) const
{
fd_set rfds;
FD_ZERO(&rfds);
FD_SET(sd, &rfds);

struct timeval tv;
tv.tv_sec = 0;
tv.tv_usec = 500000;

int res = select(sd + 1, &rfds, NULL, NULL, &tv);
if (res == -1) // Error
    {
    if (errno != EINTR)
        {
        printfd(__FILE__, "Error on select: '%s'\n", strerror(errno));
        }
    return false;
    }

if (res == 0) // Timeout
    {
    return false;
    }

return true;
}