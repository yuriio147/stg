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
Date: 16.05.2008
*/

/*
* Author : Maxim Mamontov <faust@stg.dp.ua>
*/

/*
$Revision: 1.5 $
$Date: 2009/12/13 12:56:07 $
$Author: faust $
*/
#ifndef __CAP_NF_H__
#define __CAP_NF_H__

#include <string>
#include <pthread.h>

#include "os_int.h"
#include "base_plugin.h"

#define VERSION "CAP_NF v. 0.4"
#define START_POS 0
#define STOP_POS 0

struct NF_HEADER
{
    uint16_t version;   // Protocol version
    uint16_t count;     // Flows count
    uint32_t uptime;    // System uptime
    uint32_t timestamp; // UNIX timestamp
    uint32_t nsecs;     // Residual nanoseconds
    uint32_t flowSeq;   // Sequence counter
    uint8_t  eType;     // Engine type
    uint8_t  eID;       // Engine ID
    uint16_t sInterval; // Sampling mode and interval
} __attribute__ ((packed));

struct NF_DATA
{
    uint32_t srcAddr;   // Flow source address
    uint32_t dstAddr;   // Flow destination address
    uint32_t nextHop;   // IP addres on next hop router
    uint16_t inSNMP;    // SNMP index of input iface
    uint16_t outSNMP;   // SNMP index of output iface
    uint32_t packets;   // Packets in flow
    uint32_t octets;    // Total number of bytes in flow
    uint32_t timeStart; // Uptime on first packet in flow
    uint32_t timeFinish;// Uptime on last packet in flow
    uint16_t srcPort;   // Flow source port
    uint16_t dstPort;   // Flow destination port
    uint8_t  pad1;      // 1-byte padding
    uint8_t  TCPFlags;  // Cumulative OR of TCP flags
    uint8_t  proto;     // IP protocol type (tcp, udp, etc.)
    uint8_t  tos;       // IP Type of Service (ToS)
    uint16_t srcAS;     // Source BGP autonomous system number
    uint16_t dstAS;     // Destination BGP autonomus system number
    uint8_t  srcMask;   // Source address mask in "slash" notation
    uint8_t  dstMask;   // Destination address mask in "slash" notation
    uint16_t pad2;      // 2-byte padding
} __attribute__ ((packed));

#define BUF_SIZE (sizeof(NF_HEADER) + 30 * sizeof(NF_DATA))

class NF_CAP : public BASE_PLUGIN 
{
public:
    NF_CAP();
    ~NF_CAP();

    void            SetUsers(USERS *) {};
    void            SetTariffs(TARIFFS *) {};
    void            SetAdmins(ADMINS *) {};
    void            SetTraffcounter(TRAFFCOUNTER * tc) { traffCnt = tc; };
    void            SetStore(BASE_STORE *) {};
    void            SetStgSettings(const SETTINGS *) {};
    void            SetSettings(const MODULE_SETTINGS & s) { settings = s; };
    int             ParseSettings();

    int             Start();
    int             Stop();
    int             Reload() { return 0; };
    bool            IsRunning() { return runningTCP || runningUDP; };
    const string  & GetStrError() const { return errorStr; };
    const string    GetVersion() const { return VERSION; };
    uint16_t        GetStartPosition() const { return START_POS; };
    uint16_t        GetStopPosition() const { return STOP_POS; };

private:
    TRAFFCOUNTER * traffCnt;
    MODULE_SETTINGS settings;
    pthread_t tidTCP;
    pthread_t tidUDP;
    bool runningTCP;
    bool runningUDP;
    bool stoppedTCP;
    bool stoppedUDP;
    uint16_t portT;
    uint16_t portU;
    int sockTCP;
    int sockUDP;
    mutable std::string errorStr;

    static void * RunUDP(void *);
    static void * RunTCP(void *);
    void ParseBuffer(uint8_t *, int);

    bool OpenTCP();
    bool OpenUDP();
    void CloseTCP() { close(sockTCP); };
    void CloseUDP() { close(sockUDP); };

    bool WaitPackets(int sd) const;
};

extern "C" BASE_PLUGIN * GetPlugin();

#endif