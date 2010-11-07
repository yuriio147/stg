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
 */

 /*
 $Revision: 1.31 $
 $Date: 2010/11/03 10:26:30 $
 $Author: faust $
 */

#ifndef common_h
#define common_h

#include <ctime>
#include <string>

#include "os_int.h"
#include "stg_const.h"

#define STAT_TIME_3         (1)
#define STAT_TIME_2         (2)
#define STAT_TIME_1         (3)
#define STAT_TIME_1_2       (4)
#define STAT_TIME_1_4       (5)
#define STAT_TIME_1_6       (6)

#define FN_STR_LEN      (NAME_MAX)

#define ST_F    0
#define ST_B    1
#define ST_KB   2
#define ST_MB   3

//-----------------------------------------------------------------------------
const char    * IntToKMG(long long a, int statType = ST_F);
const char    * LogDate(time_t t);
int             ParesTimeStat(const char * str);
int             IsTimeStat(struct tm * t, int statTime);
/*bool            IsDigit(char c);
bool            IsAlpha(char c);*/
int             strtodouble2(const char * s, double &a);
int             printfd(const char * __file__, const char * fmt, ...);
void            Encode12(char * dst, const char * src, size_t srcLen);
void            Decode21(char * dst, const char * src);

void            Encode12str(std::string & dst, const std::string & src);
void            Decode21str(std::string & dst, const std::string & src);

int             ParseIPString(const char * str, uint32_t * ips, int maxIP);
void            KOIToWin(const char * s1, char * s2, int l);
void            WinToKOI(const char * s1, char * s2, int l);
void            KOIToWin(const std::string & s1, std::string * s2);
void            WinToKOI(const std::string & s1, std::string * s2);
int             DaysInMonth(unsigned year, unsigned mon);
int             DaysInCurrentMonth();
int             Min8(int a);
//char          * inet_ntostr(unsigned long);
std::string     inet_ntostring(uint32_t);
uint32_t        inet_strington(const std::string & value);
int             strprintf(std::string * str, const char * fmt, ...);
int             ParseTariffTimeStr(const char * str, int &h1, int &m1, int &h2, int &m2);
uint32_t        CalcMask(uint32_t msk);
void            TouchFile(const std::string & fileName);
#ifdef WIN32
void            EncodeStr(char * str, unsigned long serial, int useHDD);
void            DecodeStr(char * str, unsigned long serial, int useHDD);
#endif //WIN32
void            SwapBytes(uint16_t & value);
void            SwapBytes(uint32_t & value);
void            SwapBytes(uint64_t & value);
void            SwapBytes(int16_t & value);
void            SwapBytes(int32_t & value);
void            SwapBytes(int64_t & value);

std::string &   TrimL(std::string & val);
std::string &   TrimR(std::string & val);
std::string &   Trim(std::string & val);

std::string     IconvString(const std::string & source, const std::string & from, const std::string & to);

//-----------------------------------------------------------------------------
template <typename varT>
int str2x(const std::string & str, varT & x)
{
    int pos = 0;
    int minus = 1;

    if (str.empty())
        return -1;

    if (str[0] == '+')
        pos++;

    if (str[0] == '-')
    {
        pos++;
        minus = -1;
    }

    if ((str[pos] < '0' || str[pos] > '9'))
        return -1;

    x = str[pos++] - '0';

    for (unsigned i = pos; i < str.size(); i++)
    {
        if ((str[i] < '0' || str[i] > '9'))
            return -1;

        x *= 10;
        x += str[i] - '0';
    }

    x*= minus;

    return 0;
}
//-----------------------------------------------------------------------------
template <typename varT>
const std::string & x2str(varT x, std::string & s)
{
    varT xx = x;
    int pos = 1;

    x /= 10;
    while (x != 0)
    {
        x /= 10;
        pos++;
    }

    if (xx < 0)
    {
        pos++;
        s.resize(pos, 0);
        s[0] = '-';
    }
    else if (xx > 0)
    {
        s.resize(pos, 0);
    }
    else
    {
        s.resize(1, 0);
        s[0] = '0';
        return s;
    }

    x = xx;

    while (x != 0)
    {
        if (x < 0)
            s[--pos] = -(x % 10) + '0';
        else
            s[--pos] = x % 10 + '0';

        x /= 10;
    }

    return s;
}
//-----------------------------------------------------------------------------
template <typename varT>
const std::string & unsigned2str(varT x, std::string & s)
{
    varT xx = x;
    int pos = 1;

    x /= 10;
    while (x != 0)
    {
        x /= 10;
        pos++;
    }

    if (xx > 0)
    {
        s.resize(pos, 0);
    }
    else
    {
        s.resize(1, 0);
        s[0] = '0';
        return s;
    }

    x = xx;

    while (x != 0)
    {
        s[--pos] = x % 10 + '0';

        x /= 10;
    }

    return s;
}
//-----------------------------------------------------------------------------
int str2x(const std::string & str, int & x);
int str2x(const std::string & str, unsigned & x);
int str2x(const std::string & str, long long & x);
int str2x(const std::string & str, unsigned long long & x);
//-----------------------------------------------------------------------------
const std::string & x2str(unsigned x, std::string & s);
const std::string & x2str(unsigned long long x, std::string & s);
//-----------------------------------------------------------------------------
char * stg_strptime(const char *, const char *, struct tm *);
time_t stg_timegm(struct tm *);

#endif