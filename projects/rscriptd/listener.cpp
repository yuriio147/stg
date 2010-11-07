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

#include <arpa/inet.h>
#include <sys/uio.h> // readv
#include <sys/types.h> // for historical versions of BSD
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <csignal>
#include <ctime>
#include <cstring>
#include <sstream>
#include <algorithm>

#include "listener.h"
#include "script_executer.h"
#include "stg_locker.h"
#include "common.h"

//-----------------------------------------------------------------------------
LISTENER::LISTENER()
    : WriteServLog(GetStgLogger()),
      port(0),
      running(false),
      receiverStopped(true),
      processorStopped(true),
      userTimeout(0),
      listenSocket(0)
{
version = "rscriptd listener v.1.2";

pthread_mutex_init(&mutex, NULL);
}
//-----------------------------------------------------------------------------
void LISTENER::SetPassword(const string & p)
{
password = p;
printfd(__FILE__, "Encryption initiated with password \'%s\'\n", password.c_str());
InitEncrypt(&ctxS, password);
}
//-----------------------------------------------------------------------------
bool LISTENER::Start()
{
printfd(__FILE__, "LISTENER::Start()\n");
running = true;

if (PrepareNet())
    {
    return true;
    }

if (receiverStopped)
    {
    if (pthread_create(&receiverThread, NULL, Run, this))
        {
        errorStr = "Cannot create thread.";
        return true;
        }
    }

if (processorStopped)
    {
    if (pthread_create(&processorThread, NULL, RunProcessor, this))
        {
        errorStr = "Cannot create thread.";
        return true;
        }
    }

errorStr = "";

return false;
}
//-----------------------------------------------------------------------------
bool LISTENER::Stop()
{
running = false;

printfd(__FILE__, "LISTENER::Stop()\n");

usleep(500000);

if (!processorStopped)
    {
    //5 seconds to thread stops itself
    for (int i = 0; i < 25 && !processorStopped; i++)
        {
        usleep(200000);
        }

    //after 5 seconds waiting thread still running. now killing it
    if (!processorStopped)
        {
        //TODO pthread_cancel()
        if (pthread_kill(processorThread, SIGINT))
            {
            errorStr = "Cannot kill thread.";
            return true;
            }
        printfd(__FILE__, "LISTENER killed Timeouter\n");
        }
    }

if (!receiverStopped)
    {
    //5 seconds to thread stops itself
    for (int i = 0; i < 25 && !receiverStopped; i++)
        {
        usleep(200000);
        }

    //after 5 seconds waiting thread still running. now killing it
    if (!receiverStopped)
        {
        //TODO pthread_cancel()
        if (pthread_kill(receiverThread, SIGINT))
            {
            errorStr = "Cannot kill thread.";
            return true;
            }
        printfd(__FILE__, "LISTENER killed Run\n");
        }
    }

pthread_join(receiverThread, NULL);
pthread_join(processorThread, NULL);

pthread_mutex_destroy(&mutex);

FinalizeNet();

std::for_each(users.begin(), users.end(), DisconnectUser(*this));

printfd(__FILE__, "LISTENER::Stoped successfully.\n");

return false;
}
//-----------------------------------------------------------------------------
void * LISTENER::Run(void * d)
{
LISTENER * ia = static_cast<LISTENER *>(d);

ia->Runner();

return NULL;
}
//-----------------------------------------------------------------------------
void LISTENER::Runner()
{
receiverStopped = false;

while (running)
    {
    RecvPacket();
    }

receiverStopped = true;
}
//-----------------------------------------------------------------------------
void * LISTENER::RunProcessor(void * d)
{
LISTENER * ia = static_cast<LISTENER *>(d);

ia->ProcessorRunner();

return NULL;
}
//-----------------------------------------------------------------------------
void LISTENER::ProcessorRunner()
{
processorStopped = false;

while (running)
    {
    usleep(500000);
    if (!pending.empty())
        ProcessPending();
    ProcessTimeouts();
    }

processorStopped = true;
}
//-----------------------------------------------------------------------------
bool LISTENER::PrepareNet()
{
listenSocket = socket(AF_INET, SOCK_DGRAM, 0);

if (listenSocket < 0)
    {
    errorStr = "Cannot create socket.";
    return true;
    }

printfd(__FILE__, "Port: %d\n", port);

struct sockaddr_in listenAddr;
listenAddr.sin_family = AF_INET;
listenAddr.sin_port = htons(port);
listenAddr.sin_addr.s_addr = inet_addr("0.0.0.0");

if (bind(listenSocket, (struct sockaddr*)&listenAddr, sizeof(listenAddr)) < 0)
    {
    errorStr = "LISTENER: Bind failed.";
    return true;
    }

printfd(__FILE__, "LISTENER::PrepareNet() >>>> Start successfull.\n");

return false;
}
//-----------------------------------------------------------------------------
bool LISTENER::FinalizeNet()
{
close(listenSocket);

return false;
}
//-----------------------------------------------------------------------------
bool LISTENER::RecvPacket()
{
struct iovec iov[2];

char buffer[RS_MAX_PACKET_LEN];
RS_PACKET_HEADER packetHead;

iov[0].iov_base = reinterpret_cast<char *>(&packetHead);
iov[0].iov_len = sizeof(packetHead);
iov[1].iov_base = buffer;
iov[1].iov_len = sizeof(buffer);

size_t dataLen = 0;
while (dataLen < sizeof(buffer))
    {
    if (!WaitPackets(listenSocket))
        {
        if (!running)
            return false;
        continue;
        }
    int portion = readv(listenSocket, iov, 2);
    if (portion < 0)
        {
        return true;
        }
    dataLen += portion;
    }

if (CheckHeader(packetHead))
    {
    printfd(__FILE__, "Invalid packet or incorrect protocol version!\n");
    return true;
    }

std::string userLogin((char *)packetHead.login);
PendingData data;
data.login = userLogin;
data.ip = ntohl(packetHead.ip);
data.id = ntohl(packetHead.id);

if (packetHead.packetType == RS_ALIVE_PACKET)
    {
    data.type = PendingData::ALIVE;
    }
else if (packetHead.packetType == RS_CONNECT_PACKET)
    {
    data.type = PendingData::CONNECT;
    if (GetParams(buffer, data))
        {
        return true;
        }
    }
else if (packetHead.packetType == RS_DISCONNECT_PACKET)
    {
    data.type = PendingData::DISCONNECT;
    if (GetParams(buffer, data))
        {
        return true;
        }
    }

STG_LOCKER lock(&mutex, __FILE__, __LINE__);
pending.push_back(data);

return false;
}
//-----------------------------------------------------------------------------
bool LISTENER::GetParams(char * buffer, UserData & data)
{
RS_PACKET_TAIL packetTail;

Decrypt(&ctxS, (char *)&packetTail, buffer, sizeof(packetTail) / 8);

if (strncmp((char *)packetTail.magic, RS_ID, RS_MAGIC_LEN))
    {
    printfd(__FILE__, "Invalid crypto magic\n");
    return true;
    }

std::stringstream params;
params << data.login << " "
       << inet_ntostring(data.ip) << " "
       << ntohl(data.id) << " "
       << (char *)packetTail.params;

data.params = params.str();

return false;
}
//-----------------------------------------------------------------------------
void LISTENER::ProcessPending()
{
printfd(__FILE__, "Pending data size: %d\n", pending.size());
std::list<PendingData>::iterator it(pending.begin());
while (it != pending.end())
    {
    std::vector<AliveData>::iterator uit(
            std::lower_bound(
                users.begin(),
                users.end(),
                it->login)
            );
    if (it->type == PendingData::CONNECT)
        {
        if (uit == users.end() || uit->login != it->login)
            {
            // Add new user
            Connect(*it);
            users.insert(uit, AliveData(static_cast<UserData>(*it)));
            }
        else if (uit->login == it->login)
            {
            // Update already existing user
            time(&uit->lastAlive);
            uit->params = it->params;
            }
        }
    else if (it->type == PendingData::ALIVE)
        {
        if (uit != users.end() && uit->login == it->login)
            {
            // Update existing user
            time(&uit->lastAlive);
            }
        }
    else if (it->type == PendingData::DISCONNECT)
        {
        if (uit != users.end() && uit->login == it->login.c_str())
            {
            // Disconnect existing user
            Disconnect(*uit);
            users.erase(uit);
            }
        }

    STG_LOCKER lock(&mutex, __FILE__, __LINE__);
    pending.erase(it++);
    }
}
//-----------------------------------------------------------------------------
void LISTENER::ProcessTimeouts()
{
const std::vector<AliveData>::iterator it(
        std::stable_partition(
            users.begin(),
            users.end(),
            IsNotTimedOut(userTimeout)
        )
    );

if (it != users.end())
    {
    printfd(__FILE__, "Total users: %d, users to disconnect: %d\n", users.size(), std::distance(it, users.end()));

    std::for_each(
            it,
            users.end(),
            DisconnectUser(*this)
        );

    users.erase(it, users.end());
    }
}
//-----------------------------------------------------------------------------
bool LISTENER::Connect(const UserData & data) const
{
printfd(__FILE__, "Connect %s\n", data.login.c_str());
if (access(scriptOnConnect.c_str(), X_OK) == 0)
    {
    if (ScriptExec(scriptOnConnect + " " + data.params))
        {
        WriteServLog("Script %s cannot be executed for an unknown reason.", scriptOnConnect.c_str());
        return true;
        }
    }
else
    {
    WriteServLog("Script %s cannot be executed. File not found.", scriptOnConnect.c_str());
    return true;
    }
return false;
}
//-----------------------------------------------------------------------------
bool LISTENER::Disconnect(const UserData & data) const
{
printfd(__FILE__, "Disconnect %s\n", data.login.c_str());
if (access(scriptOnDisconnect.c_str(), X_OK) == 0)
    {
    if (ScriptExec(scriptOnDisconnect + " " + data.params))
        {
        WriteServLog("Script %s cannot be executed for an unknown reson.", scriptOnDisconnect.c_str());
        return true;
        }
    }
else
    {
    WriteServLog("Script %s cannot be executed. File not found.", scriptOnDisconnect.c_str());
    return true;
    }
return false;
}
//-----------------------------------------------------------------------------
void LISTENER::InitEncrypt(BLOWFISH_CTX * ctx, const string & password)
{
unsigned char keyL[PASSWD_LEN];
memset(keyL, 0, PASSWD_LEN);
strncpy((char *)keyL, password.c_str(), PASSWD_LEN);
Blowfish_Init(ctx, keyL, PASSWD_LEN);
}
//-----------------------------------------------------------------------------
void LISTENER::Decrypt(BLOWFISH_CTX * ctx, char * dst, const char * src, int len8)
{
if (dst != src)
    memcpy(dst, src, len8 * 8);

for (int i = 0; i < len8; i++)
    Blowfish_Decrypt(ctx, (uint32_t *)(dst + i * 8), (uint32_t *)(dst + i * 8 + 4));
}
//-----------------------------------------------------------------------------
bool LISTENER::CheckHeader(const RS_PACKET_HEADER & header) const
{
if (strncmp((char *)header.magic, RS_ID, RS_MAGIC_LEN))
    {
    return true;
    }
if (strncmp((char *)header.protoVer, "02", RS_PROTO_VER_LEN))
    {
    return true;
    }
return false;
}
//-----------------------------------------------------------------------------
bool LISTENER::WaitPackets(int sd) const
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
//-----------------------------------------------------------------------------