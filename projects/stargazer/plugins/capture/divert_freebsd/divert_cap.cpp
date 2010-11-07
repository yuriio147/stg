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
* Author : Boris Mikhailenko <stg34@stg.dp.ua>
*/

/*
$Revision: 1.13 $
$Date: 2010/09/10 06:43:03 $
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include <sys/uio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "common.h"
#include "divert_cap.h"

#define BUFF_LEN (16384) /* max mtu -> lo=16436  TODO why?*/

//-----------------------------------------------------------------------------
struct DIVERT_DATA
{
int sock;
short int port;
unsigned char buffer[BUFF_LEN];
char iface[10];
};
//-----------------------------------------------------------------------------
pollfd pollddiv;
DIVERT_DATA cddiv;  //capture data
//-----------------------------------------------------------------------------
class DIVERT_CAP_CREATOR
{
private:
    DIVERT_CAP * divc;

public:
    DIVERT_CAP_CREATOR()
        : divc(new DIVERT_CAP())
        {
        };
    ~DIVERT_CAP_CREATOR()
        {
        delete divc;
        };

    DIVERT_CAP * GetCapturer()
    {
    return divc;
    };
};
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DIVERT_CAP_CREATOR dcc;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BASE_PLUGIN * GetPlugin()
{
return dcc.GetCapturer();
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const string DIVERT_CAP::GetVersion() const
{
return "Divert_cap v.1.0";
}
//-----------------------------------------------------------------------------
DIVERT_CAP::DIVERT_CAP()
{
isRunning = false;
nonstop = false;
}
//-----------------------------------------------------------------------------
void DIVERT_CAP::SetTraffcounter(TRAFFCOUNTER * tc)
{
traffCnt = tc;
}
//-----------------------------------------------------------------------------
const string & DIVERT_CAP::GetStrError() const
{
return errorStr;
}
//-----------------------------------------------------------------------------
int DIVERT_CAP::Start()
{
if (isRunning)
    return 0;

if (DivertCapOpen() < 0)
    {
    errorStr = "Cannot open socket!";
    printfd(__FILE__, "Cannot open socket\n");
    return -1;
    }

nonstop = true;

if (pthread_create(&thread, NULL, Run, this) == 0)
    {
    return 0;
    }

errorStr = "Cannot create thread.";
printfd(__FILE__, "Cannot create thread\n");
return -1;
}
//-----------------------------------------------------------------------------
int DIVERT_CAP::Stop()
{
if (!isRunning)
    return 0;

DivertCapClose();

nonstop = false;

//5 seconds to thread stops itself
int i;
for (i = 0; i < 25; i++)
    {
    if (!isRunning)
        break;

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
    }

return 0;
}
//-----------------------------------------------------------------------------
bool DIVERT_CAP::IsRunning()
{
return isRunning;
}
//-----------------------------------------------------------------------------
void * DIVERT_CAP::Run(void * d)
{
DIVERT_CAP * dc = (DIVERT_CAP *)d;
dc->isRunning = true;

/*struct ETH_IP
{
uint16_t    ethHdr[8];
RAW_PACKET  rp;
char        padding[4];
char        padding1[8];
};

ETH_IP * ethIP;

char ethip[sizeof(ETH_IP)];

//memset(&ethIP, 0, sizeof(ethIP));
memset(&ethip, 0, sizeof(ETH_IP));

ethIP = (ETH_IP *)&ethip;
ethIP->rp.dataLen = -1;
*/
//char * iface = NULL;
char buffer[64];
while (dc->nonstop)
    {
    RAW_PACKET rp;
    dc->DivertCapRead(buffer, 64, NULL);

    //printf("%x %x %x %x \n", buffer[0], buffer[4], buffer[8], buffer[12]);
    //printf("%x %x %x %x \n", buffer[16], buffer[20], buffer[24], buffer[28]);
    //printf("%x %x %x %x \n", buffer[32], buffer[36], buffer[40], buffer[44]);

    if (buffer[12] != 0x8)
        continue;

    memcpy(rp.pckt, &buffer[14], pcktSize);

    //dc->traffCnt->Process(*((RAW_PACKET*)( &buffer[14] ))); // - too dirty!
    dc->traffCnt->Process(rp);
    }

dc->isRunning = false;
return NULL;
}
//-----------------------------------------------------------------------------
uint16_t DIVERT_CAP::GetStartPosition() const
{
return 10;
}
//-----------------------------------------------------------------------------
uint16_t DIVERT_CAP::GetStopPosition() const
{
return 10;
}
//-----------------------------------------------------------------------------
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
//-----------------------------------------------------------------------------
int DIVERT_CAP::DivertCapOpen()
{
memset(&pollddiv, 0, sizeof(pollddiv));
memset(&cddiv, 0, sizeof(DIVERT_DATA));

strcpy(cddiv.iface, "foo");
cddiv.port = port;

DivertCapOpen(0);
pollddiv.events = POLLIN;
pollddiv.fd = cddiv.sock;

return 0;
}
//-----------------------------------------------------------------------------
int DIVERT_CAP::DivertCapOpen(int)
{
int ret;
cddiv.sock = socket(PF_INET, SOCK_RAW, IPPROTO_DIVERT);
if (cddiv.sock < 0)
    {
    errorStr = "Create divert socket error.";
    printfd(__FILE__, "Cannot create divert socket\n");
    return -1;
    }

struct sockaddr_in divAddr;

memset(&divAddr, 0, sizeof(divAddr));

divAddr.sin_family = AF_INET;
divAddr.sin_port = htons(cddiv.port);
divAddr.sin_addr.s_addr = INADDR_ANY;

ret = bind(cddiv.sock, (struct sockaddr *)&divAddr, sizeof(divAddr));

if (ret < 0)
    {
    errorStr = "Bind divert socket error.";
    printfd(__FILE__, "Cannot bind divert socket\n");
    return -1;
    }

return cddiv.sock;
}
//-----------------------------------------------------------------------------
int DIVERT_CAP::DivertCapRead(char * b, int blen, char ** iface)
{
poll(&pollddiv, 1, -1);

if (pollddiv.revents & POLLIN)
    {
    DivertCapRead(b, blen, iface, 0);
    pollddiv.revents = 0;
    return 0;
    }

return 0;
}
//-----------------------------------------------------------------------------
int DIVERT_CAP::DivertCapRead(char * b, int blen, char ** iface, int)
{
static char buf[BUFF_LEN];
static struct sockaddr_in divertaddr;
static int bytes;
static socklen_t divertaddrSize = sizeof(divertaddr);

if ((bytes = recvfrom (cddiv.sock, buf, BUFF_LEN,
                       0, (struct sockaddr*) &divertaddr, &divertaddrSize)) > 50)
    {
    memcpy(b + 14, buf, blen - 14);
    b[12] = 0x8;

    if (iface)
        *iface = cddiv.iface;

    sendto(cddiv.sock, buf, bytes, 0, (struct sockaddr*)&divertaddr, divertaddrSize);
    }

return 0;
}
//-----------------------------------------------------------------------------
int DIVERT_CAP::DivertCapClose()
{
close(cddiv.sock);
return 0;
}
//-----------------------------------------------------------------------------
int DIVERT_CAP::ParseSettings()
{
int p;
PARAM_VALUE pv;
vector<PARAM_VALUE>::const_iterator pvi;

pv.param = "Port";
pvi = find(settings.moduleParams.begin(), settings.moduleParams.end(), pv);
if (pvi == settings.moduleParams.end())
    {
    port = 15701;
    return 0;
    }

if (ParseIntInRange(pvi->value[0], 1, 65535, &p))
    {
    errorStr = "Cannot parse parameter \'Port\': " + errorStr;
    printfd(__FILE__, "Cannot parse parameter 'Port'\n");
    return -1;
    }

port = p;

return 0;
}
//-----------------------------------------------------------------------------
int DIVERT_CAP::ParseIntInRange(const string & str, int min, int max, int * val)
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
void DIVERT_CAP::SetSettings(const MODULE_SETTINGS & s)
{
settings = s;
}
//-----------------------------------------------------------------------------
