#ifndef RAW_IP_PACKET_H
#define RAW_IP_PACKET_H

#if defined(FREE_BSD) || defined(FREE_BSD5)
#include <netinet/in_systm.h> // n_long in netinet/ip.h
#endif

#include <netinet/in.h> // for htons
#include <netinet/ip.h> // for struct ip

#include <cstring>

#include "stg_const.h"
#include "common.h"

#define IPv4 (2)

enum { pcktSize = 68 }; //60(max) ip + 8 udp or tcp (part of tcp or udp header to ports)
//-----------------------------------------------------------------------------
struct RAW_PACKET
{
    RAW_PACKET()
        : dataLen(-1)
    {
    memset(pckt, 0, pcktSize);
    }

    RAW_PACKET(const RAW_PACKET & rp)
        : dataLen(rp.dataLen)
    {
    memcpy(pckt, rp.pckt, pcktSize);
    }

uint16_t    GetIPVersion() const;
uint8_t     GetHeaderLen() const;
uint8_t     GetProto() const;
uint32_t    GetLen() const;
uint32_t    GetSrcIP() const;
uint32_t    GetDstIP() const;
uint16_t    GetSrcPort() const;
uint16_t    GetDstPort() const;

bool        operator==(const RAW_PACKET & rvalue) const;
bool        operator!=(const RAW_PACKET & rvalue) const { return !(*this == rvalue); };
bool        operator<(const RAW_PACKET & rvalue) const;

union
    {
    uint8_t pckt[pcktSize]; // Packet header as a raw data
    struct
        {
        struct ip   ipHeader;
        // Only for packets without options field
        uint16_t    sPort;
        uint16_t    dPort;
        } header __attribute__ ((packed));
    };
int32_t dataLen; // IP packet length. Set to -1 to use length field from the header
};
//-----------------------------------------------------------------------------
inline uint16_t RAW_PACKET::GetIPVersion() const
{
return header.ipHeader.ip_v;
}
//-----------------------------------------------------------------------------
inline uint8_t RAW_PACKET::GetHeaderLen() const
{
return header.ipHeader.ip_hl * 4;
}
//-----------------------------------------------------------------------------
inline uint8_t RAW_PACKET::GetProto() const
{
return header.ipHeader.ip_p;
}
//-----------------------------------------------------------------------------
inline uint32_t RAW_PACKET::GetLen() const
{
if (dataLen != -1)
    return dataLen;
return ntohs(header.ipHeader.ip_len);
}
//-----------------------------------------------------------------------------
inline uint32_t RAW_PACKET::GetSrcIP() const
{
return header.ipHeader.ip_src.s_addr;
}
//-----------------------------------------------------------------------------
inline uint32_t RAW_PACKET::GetDstIP() const
{
return header.ipHeader.ip_dst.s_addr;
}
//-----------------------------------------------------------------------------
inline uint16_t RAW_PACKET::GetSrcPort() const
{
if (header.ipHeader.ip_p == 1) // for icmp proto return port 0
    return 0;
return ntohs(*((uint16_t*)(pckt + header.ipHeader.ip_hl * 4)));
}
//-----------------------------------------------------------------------------
inline uint16_t RAW_PACKET::GetDstPort() const
{
if (header.ipHeader.ip_p == 1) // for icmp proto return port 0
    return 0;
return ntohs(*((uint16_t*)(pckt + header.ipHeader.ip_hl * 4 + 2)));
}
//-----------------------------------------------------------------------------
inline bool RAW_PACKET::operator==(const RAW_PACKET & rvalue) const
{
if (header.ipHeader.ip_src.s_addr != rvalue.header.ipHeader.ip_src.s_addr)
    return false;

if (header.ipHeader.ip_dst.s_addr != rvalue.header.ipHeader.ip_dst.s_addr)
    return false;

if (header.ipHeader.ip_p != 1 && rvalue.header.ipHeader.ip_p != 1)
    {
    if (*((uint16_t *)(pckt + header.ipHeader.ip_hl * 4)) !=
        *((uint16_t *)(rvalue.pckt + rvalue.header.ipHeader.ip_hl * 4)))
        return false;

    if (*((uint16_t *)(pckt + header.ipHeader.ip_hl * 4 + 2)) !=
        *((uint16_t *)(rvalue.pckt + rvalue.header.ipHeader.ip_hl * 4 + 2)))
        return false;
    }

if (header.ipHeader.ip_p != rvalue.header.ipHeader.ip_p)
    return false;

return true;
}
//-----------------------------------------------------------------------------
inline bool RAW_PACKET::operator<(const RAW_PACKET & rvalue) const
{
if (header.ipHeader.ip_src.s_addr < rvalue.header.ipHeader.ip_src.s_addr) 
    return true;
if (header.ipHeader.ip_src.s_addr > rvalue.header.ipHeader.ip_src.s_addr) 
    return false;

if (header.ipHeader.ip_dst.s_addr < rvalue.header.ipHeader.ip_dst.s_addr) 
    return true;
if (header.ipHeader.ip_dst.s_addr > rvalue.header.ipHeader.ip_dst.s_addr) 
    return false;

if (header.ipHeader.ip_p != 1 && rvalue.header.ipHeader.ip_p != 1)
    {
    if (*((uint16_t *)(pckt + header.ipHeader.ip_hl * 4)) <
        *((uint16_t *)(rvalue.pckt + rvalue.header.ipHeader.ip_hl * 4))) 
        return true;
    if (*((uint16_t *)(pckt + header.ipHeader.ip_hl * 4)) >
        *((uint16_t *)(rvalue.pckt + rvalue.header.ipHeader.ip_hl * 4))) 
        return false;

    if (*((uint16_t *)(pckt + header.ipHeader.ip_hl * 4 + 2)) <
        *((uint16_t *)(rvalue.pckt + rvalue.header.ipHeader.ip_hl * 4 + 2))) 
        return true;
    if (*((uint16_t *)(pckt + header.ipHeader.ip_hl * 4 + 2)) >
        *((uint16_t *)(rvalue.pckt + rvalue.header.ipHeader.ip_hl * 4 + 2))) 
        return false;
    }

if (header.ipHeader.ip_p < rvalue.header.ipHeader.ip_p) 
    return true;

return false;
}
//-----------------------------------------------------------------------------

#endif