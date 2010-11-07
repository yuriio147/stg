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
Date: 18.09.2002
*/

/*
* Author : Boris Mikhailenko <stg34@stg.dp.ua>
*/

/*
$Revision: 1.19 $
$Date: 2009/03/24 11:20:15 $
$Author: faust $
*/

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include <net/bpf.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "ether_cap.h"
#include "common.h"
#include "raw_ip_packet.h"

using namespace std;

//#define CAP_DEBUG 1
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
class BPF_CAP_CREATOR
{
private:
    BPF_CAP * bpfc;

public:
    BPF_CAP_CREATOR()
        : bpfc(new BPF_CAP())
        {
        };
    ~BPF_CAP_CREATOR()
        {
        delete bpfc;
        };

    BPF_CAP * GetCapturer()
    {
    return bpfc;
    };
};
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BPF_CAP_CREATOR bcc;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BASE_PLUGIN * GetPlugin()
{
return bcc.GetCapturer();
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int BPF_CAP_SETTINGS::ParseSettings(const MODULE_SETTINGS & s)
{
//char sep[]= ", \t\n\r";
//char *s;
string ifaces;
//char * str;
//char *p;

iface.erase(iface.begin(), iface.end());

//PARAM_VALUE pv;
//pv.param = "WorkDir";
//vector<PARAM_VALUE>::const_iterator pvi;

if (s.moduleParams.empty())
    {
    errorStr = "Parameter \'iface\' not found.";
    printfd(__FILE__, "Parameter 'iface' not found\n");
    return -1;
    }

for (unsigned i = 0; i < s.moduleParams.size(); i++)
    {
    if (s.moduleParams[i].param != "iface")
        {
        errorStr = "Parameter \'" + s.moduleParams[i].param + "\' unrecognized.";
        printfd(__FILE__, "Invalid parameter: '%s'\n", s.moduleParams[i].param.c_str());
        return -1;
        }
    for (unsigned j = 0; j < s.moduleParams[i].value.size(); j++)
        {
        iface.push_back(s.moduleParams[i].value[j]);
        }
    }

/*if (cf.ReadString("Iface", &ifaces, "NoIface") < 0)
    {
    errorStr = "Cannot read parameter \'Iface\' from " + cf.GetFileName();
    return -1;
    }

str = new char[ifaces.size() + 1];
strcpy(str, ifaces.c_str());
p = str;

while((s = strtok(p, sep)))
    {
    printfd(__FILE__, "iface[] = %s\n", s);
    p = NULL;
    iface.push_back(s);
    //strncpy(iface[i++], s, DEV_NAME_LEN);
    //devNum++;
    }

delete[] str;
if (!ifaces.size())
    {
    errorStr = "Error read parameter \'Iface\' from " + cf.GetFileName();
    return -1;
    }*/

return 0;
}
//-----------------------------------------------------------------------------
string BPF_CAP_SETTINGS::GetIface(unsigned int num)
{
if (num >= iface.size())
    {
    return "";
    }
return iface[num];
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const string BPF_CAP::GetVersion() const
{
return "bpf_cap v.1.0";
}
//-----------------------------------------------------------------------------
BPF_CAP::BPF_CAP()
{
isRunning = false;
nonstop = false;
}
//-----------------------------------------------------------------------------
void BPF_CAP::SetSettings(const MODULE_SETTINGS & s)
{
settings = s;
}
//-----------------------------------------------------------------------------
int BPF_CAP::ParseSettings()
{
int ret = capSettings.ParseSettings(settings);
if (ret)
    {
    errorStr = capSettings.GetStrError();
    return ret;
    }
return 0;
}
//-----------------------------------------------------------------------------
void BPF_CAP::SetTraffcounter(TRAFFCOUNTER * tc)
{
traffCnt = tc;
}
//-----------------------------------------------------------------------------
const string & BPF_CAP::GetStrError() const
{
return errorStr;
}
//-----------------------------------------------------------------------------
int BPF_CAP::Start()
{
if (isRunning)
    return 0;

if (BPFCapOpen() < 0)
    {
    //errorStr = "Cannot open bpf device!";
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
int BPF_CAP::Stop()
{
if (!isRunning)
    return 0;

BPFCapClose();

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
    //TODO pthread_cancel()
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
bool BPF_CAP::IsRunning()
{
return isRunning;
}
//-----------------------------------------------------------------------------
void * BPF_CAP::Run(void * d)
{
BPF_CAP * dc = (BPF_CAP *)d;
dc->isRunning = true;

uint8_t hdr[96]; //68 + 14 + 4(size) + 9(SYS_IFACE) + 1(align to 4) = 96

RAW_PACKET *  rpp = (RAW_PACKET *)&hdr[14];
memset(hdr, 0, sizeof(hdr));

rpp->dataLen = -1;
char * iface;

while (dc->nonstop)
    {
    dc->BPFCapRead((char*)&hdr, 68 + 14, &iface);

    if (!(hdr[12] == 0x8 && hdr[13] == 0x0))
    {
        continue;
    }

    dc->traffCnt->Process(*rpp);
    }

dc->isRunning = false;
return NULL;
}
//-----------------------------------------------------------------------------
uint16_t BPF_CAP::GetStartPosition() const
{
return 0;
}
//-----------------------------------------------------------------------------
uint16_t BPF_CAP::GetStopPosition() const
{
return 0;
}
//-----------------------------------------------------------------------------
int BPF_CAP::BPFCapOpen()
{
//for (int i = 0; i < settings->devNum; i++)
int i = 0;
BPF_DATA bd;
pollfd pd;

while ((bd.iface = capSettings.GetIface(i)) != "")
    {
    bpfData.push_back(bd);
    if (BPFCapOpen(&bpfData[i]) < 0)
    {
    return -1;
    }

    pd.events = POLLIN;
    pd.fd = bpfData[i].fd;
    polld.push_back(pd);
    i++;
    }

return 0;
}
//-----------------------------------------------------------------------------
//int BPF_CAP::BPFCapOpen(string ifaceToOpen)
int BPF_CAP::BPFCapOpen(BPF_DATA * bd)
{
char devbpf[20];
int i = 0;
int l = BUFF_LEN;
int im = 1;
struct ifreq ifr;

do  {
    sprintf(devbpf, "/dev/bpf%d", i);
    i++;
    bd->fd = open(devbpf, O_RDONLY);
    //cd[n].fd = open(devbpf, O_RDONLY);
    } while(bd->fd < 0 && errno == EBUSY);
      //while(cd[n].fd < 0 && errno == EBUSY);

//if (cd[n].fd < 0)
if (bd->fd < 0)
    {
    errorStr = "Can't capture packets. Open bpf device for " + bd->iface + " error.";
    printfd(__FILE__, "Cannot open BPF device\n");
    return -1;
    }

//strncpy(ifr.ifr_name, settings->iface[n], sizeof(ifr.ifr_name));
strncpy(ifr.ifr_name, bd->iface.c_str(), sizeof(ifr.ifr_name));

//if (ioctl(cd[n].fd, BIOCSBLEN, (caddr_t)&l) < 0)
if (ioctl(bd->fd, BIOCSBLEN, (caddr_t)&l) < 0)
    {
    errorStr = bd->iface + " BIOCSBLEN " + string(strerror(errno));
    printfd(__FILE__, "ioctl failed: '%s'\n", errorStr.c_str());
    return -1;
    }

//if (ioctl(cd[n].fd, BIOCSETIF, (caddr_t)&ifr) < 0 )
if (ioctl(bd->fd, BIOCSETIF, (caddr_t)&ifr) < 0)
    {
    errorStr = bd->iface + " BIOCSETIF " + string(strerror(errno));
    printfd(__FILE__, "ioctl failed: '%s'\n", errorStr.c_str());
    return -1;
    }

//if (ioctl(cd[n].fd, BIOCIMMEDIATE, &im) < 0 )
if (ioctl(bd->fd, BIOCIMMEDIATE, &im) < 0)
    {
    errorStr = bd->iface + " BIOCIMMEDIATE " + string(strerror(errno));
    printfd(__FILE__, "ioctl failed: '%s'\n", errorStr.c_str());
    return -1;
    }

return bd->fd;
//return 0;
}
//-----------------------------------------------------------------------------
int BPF_CAP::BPFCapClose()
{
for (unsigned int i = 0; i < bpfData.size(); i++)
  close(bpfData[i].fd);
return 0;
}
//-----------------------------------------------------------------------------
int BPF_CAP::BPFCapRead(char * buffer, int blen, char ** capIface)
{
poll(&polld[0], polld.size(), -1);

for (unsigned int i = 0; i < polld.size(); i++)
    {
    if (polld[i].revents & POLLIN)
        {
        BPFCapRead(buffer, blen, capIface, &bpfData[i]);
        polld[i].revents = 0;
        return 0;
        }
    }
return 0;
}
//-----------------------------------------------------------------------------
int BPF_CAP::BPFCapRead(char * buffer, int blen, char **, BPF_DATA * bd)
{
if (bd->canRead)
    {
    bd->r = read(bd->fd, bd->buffer, BUFF_LEN);
    if (bd->r < 0)
        {
        //printfd(__FILE__, " error read\n");
        usleep(20000);
        }

    bd->p = bd->buffer;
    bd->bh = (struct bpf_hdr*)bd->p;
    bd->canRead = 0;
    }

if(bd->r > bd->sum)
    {
    memcpy(buffer, (char*)(bd->p) + bd->bh->bh_hdrlen, blen);
    //strncpy(iface, settings->iface[n], 9);
    //*iface = settings->iface[n];

    bd->sum += BPF_WORDALIGN(bd->bh->bh_hdrlen + bd->bh->bh_caplen);
    bd->p = bd->p + BPF_WORDALIGN(bd->bh->bh_hdrlen + bd->bh->bh_caplen);
    bd->bh = (struct bpf_hdr*)bd->p;
    }

if(bd->r <= bd->sum)
    {
    bd->canRead = 1;
    bd->sum = 0;
    }

return 0;
}
//-----------------------------------------------------------------------------
