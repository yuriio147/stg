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
 *    Date: 27.10.2002
 */

/*
 *    Author : Boris Mikhailenko <stg34@stargazer.dp.ua>
 */

/*
 $Revision: 1.101 $
 $Date: 2010/11/03 10:50:03 $
 $Author: faust $
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h>
#include <unistd.h> // access

#include <cassert>

#include "user.h"
#include "common.h"
#include "settings.h"
#include "script_executer.h"
#include "tariff.h"
#include "tariffs.h"
#include "admin.h"

USER::USER(const SETTINGS * s,
           const BASE_STORE * st,
           const TARIFFS * t,
           const ADMIN & a,
           const map<uint32_t, user_iter> * ipIdx)
    : property(s),
      WriteServLog(GetStgLogger()),
      login(),
      id(0),
      __connected(0),
      connected(__connected),
      userIDGenerator(),
      __currIP(0),
      currIP(__currIP),
      lastIPForDisconnect(0),
      pingTime(0),
      sysAdmin(a),
      store(st),
      tariffs(t),
      tariff(tariffs->GetNoTariff()),
      cash(property.cash),
      up(property.up),
      down(property.down),
      lastCashAdd(property.lastCashAdd),
      passiveTime(property.passiveTime),
      lastCashAddTime(property.lastCashAddTime),
      freeMb(property.freeMb),
      lastActivityTime(property.lastActivityTime),
      password(property.password),
      passive(property.passive),
      disabled(property.disabled),
      disabledDetailStat(property.disabledDetailStat),
      alwaysOnline(property.alwaysOnline),
      tariffName(property.tariffName),
      nextTariff(property.nextTariff),
      address(property.address),
      note(property.note),
      group(property.group),
      email(property.email),
      phone(property.phone),
      realName(property.realName),
      credit(property.credit),
      creditExpire(property.creditExpire),
      ips(property.ips),
      userdata0(property.userdata0),
      userdata1(property.userdata1),
      userdata2(property.userdata2),
      userdata3(property.userdata3),
      userdata4(property.userdata4),
      userdata5(property.userdata5),
      userdata6(property.userdata6),
      userdata7(property.userdata7),
      userdata8(property.userdata8),
      userdata9(property.userdata9),
      passiveNotifier(this),
      tariffNotifier(this),
      cashNotifier(this),
      ipNotifier(this)
{
settings = s;
ipIndex = ipIdx;

password = "*_EMPTY_PASSWORD_*";
tariffName = NO_TARIFF_NAME;
connected = 0;
traffStatInUse = 0;
traffStat = &traffStatInternal[0];
tariff = tariffs->GetNoTariff();
ips = StrToIPS("*");
deleted = false;
lastWriteStat = stgTime + random() % settings->GetStatWritePeriod();
lastWriteDeatiledStat = stgTime;
lastSwapDeatiledStat = stgTime;

property.tariffName.AddBeforeNotifier(&tariffNotifier);
property.passive.AddBeforeNotifier(&passiveNotifier);
property.cash.AddBeforeNotifier(&cashNotifier);
currIP.AddAfterNotifier(&ipNotifier);

lastScanMessages = 0;

writeFreeMbTraffCost = settings->GetWriteFreeMbTraffCost();

pthread_mutexattr_t attr;
pthread_mutexattr_init(&attr);
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
pthread_mutex_init(&mutex, &attr);
}
//-----------------------------------------------------------------------------
USER::USER(const USER & u)
    : property(u.settings),
      WriteServLog(GetStgLogger()),
      login(u.login),
      id(u.id),
      __connected(u.__connected),
      connected(__connected),
      __currIP(u.__currIP),
      currIP(__currIP),
      lastIPForDisconnect(0),
      pingTime(u.pingTime),
      sysAdmin(u.sysAdmin),
      store(u.store),
      tariffs(u.tariffs),
      tariff(u.tariff),
      cash(property.cash),
      up(property.up),
      down(property.down),
      lastCashAdd(property.lastCashAdd),
      passiveTime(property.passiveTime),
      lastCashAddTime(property.lastCashAddTime),
      freeMb(property.freeMb),
      lastActivityTime(property.lastActivityTime),
      password(property.password),
      passive(property.passive),
      disabled(property.disabled),
      disabledDetailStat(property.disabledDetailStat),
      alwaysOnline(property.alwaysOnline),
      tariffName(property.tariffName),
      nextTariff(property.nextTariff),
      address(property.address),
      note(property.note),
      group(property.group),
      email(property.email),
      phone(property.phone),
      realName(property.realName),
      credit(property.credit),
      creditExpire(property.creditExpire),
      ips(property.ips),
      userdata0(property.userdata0),
      userdata1(property.userdata1),
      userdata2(property.userdata2),
      userdata3(property.userdata3),
      userdata4(property.userdata4),
      userdata5(property.userdata5),
      userdata6(property.userdata6),
      userdata7(property.userdata7),
      userdata8(property.userdata8),
      userdata9(property.userdata9),
      passiveNotifier(this),
      tariffNotifier(this),
      cashNotifier(this),
      ipNotifier(this)
{
if (&u == this)
    return;

connected = 0;
traffStatInUse = 0;

ipIndex = u.ipIndex;

deleted = u.deleted;
traffStat = &traffStatInternal[traffStatInUse % 2];
traffStatToWrite = &traffStatInternal[(traffStatInUse +1) % 2];

lastWriteStat = u.lastWriteStat;
lastWriteDeatiledStat = u.lastWriteDeatiledStat;
lastSwapDeatiledStat = u.lastSwapDeatiledStat;

settings = u.settings;

property.tariffName.AddBeforeNotifier(&tariffNotifier);
property.passive.AddBeforeNotifier(&passiveNotifier);
property.cash.AddBeforeNotifier(&cashNotifier);
currIP.AddAfterNotifier(&ipNotifier);

lastScanMessages = 0;

writeFreeMbTraffCost = settings->GetWriteFreeMbTraffCost();

pthread_mutexattr_t attr;
pthread_mutexattr_init(&attr);
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
pthread_mutex_init(&mutex, &attr);
}
//-----------------------------------------------------------------------------
USER::~USER()
{
property.passive.DelBeforeNotifier(&passiveNotifier);
property.tariffName.DelBeforeNotifier(&tariffNotifier);
pthread_mutex_destroy(&mutex);
}
//-----------------------------------------------------------------------------
void USER::SetLogin(string const & l)
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
assert(login.empty() && "Login is already set");
login = l;
id = userIDGenerator.GetNextID();
}
//-----------------------------------------------------------------------------
int USER::ReadConf()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
USER_CONF uc;

if (store->RestoreUserConf(&uc, login))
    {
    WriteServLog("Cannot read conf for user %s.", login.c_str());
    WriteServLog("%s", store->GetStrError().c_str());
    printfd(__FILE__, "Cannot read conf for user %s.\n", login.c_str());
    printfd(__FILE__, "%s\n", store->GetStrError().c_str());
    return -1;
    }

password = uc.password;
passive = uc.passive;
disabled = uc.disabled;
disabledDetailStat = uc.disabledDetailStat;
alwaysOnline = uc.alwaysOnline;
tariffName = uc.tariffName;
address = uc.address;
phone = uc.phone;
email = uc.email;
note = uc.note;
realName = uc.realName;
group = uc.group;
credit = uc.credit;
nextTariff = uc.nextTariff;
userdata0 = uc.userdata[0];
userdata1 = uc.userdata[1];
userdata2 = uc.userdata[2];
userdata3 = uc.userdata[3];
userdata4 = uc.userdata[4];
userdata5 = uc.userdata[5];
userdata6 = uc.userdata[6];
userdata7 = uc.userdata[7];
userdata8 = uc.userdata[8];
userdata9 = uc.userdata[9];

creditExpire = uc.creditExpire;
ips = uc.ips;

tariff = tariffs->FindByName(tariffName);
if (tariff == NULL)
    {
    WriteServLog("Cannot read user %s. Tariff %s not exist.",
                 login.c_str(), property.tariffName.Get().c_str());
    return -1;
    }

return 0;
}
//-----------------------------------------------------------------------------
int USER::ReadStat()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
USER_STAT us;

if (store->RestoreUserStat(&us, login))
    {
    WriteServLog("Cannot read stat for user %s.", login.c_str());
    WriteServLog("%s", store->GetStrError().c_str());
    printfd(__FILE__, "Cannot read stat for user %s.\n", login.c_str());
    printfd(__FILE__, "%s\n", store->GetStrError().c_str());
    return -1;
    }

up = us.up;
down = us.down;
cash = us.cash;
freeMb = us.freeMb;
lastCashAdd = us.lastCashAdd;
lastCashAddTime = us.lastCashAddTime;
passiveTime = us.passiveTime;
lastActivityTime = us.lastActivityTime;

return 0;
}
//-----------------------------------------------------------------------------
int USER::WriteConf()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
USER_CONF uc;

uc.password = password;
uc.passive = passive;
uc.disabled = disabled;
uc.disabledDetailStat = disabledDetailStat;
uc.alwaysOnline = alwaysOnline;
uc.tariffName = tariffName;
uc.address = address;
uc.phone = phone;
uc.email = email;
uc.note = note;
uc.realName = realName;
uc.group = group;
uc.credit = credit;
uc.nextTariff = nextTariff;
uc.userdata[0] = userdata0;
uc.userdata[1] = userdata1;
uc.userdata[2] = userdata2;
uc.userdata[3] = userdata3;
uc.userdata[4] = userdata4;
uc.userdata[5] = userdata5;
uc.userdata[6] = userdata6;
uc.userdata[7] = userdata7;
uc.userdata[8] = userdata8;
uc.userdata[9] = userdata9;

uc.creditExpire = creditExpire;
uc.ips = ips;

printfd(__FILE__, "USER::WriteConf()\n");

if (store->SaveUserConf(uc, login))
    {
    WriteServLog("Cannot write conf for user %s.", login.c_str());
    WriteServLog("%s", store->GetStrError().c_str());
    printfd(__FILE__, "Cannot write conf for user %s.\n", login.c_str());
    printfd(__FILE__, "%s\n", store->GetStrError().c_str());
    return -1;
    }

return 0;
}
//-----------------------------------------------------------------------------
int USER::WriteStat()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
USER_STAT us;

us.up = up;
us.down = down;
us.cash = cash;
us.freeMb = freeMb;
us.lastCashAdd = lastCashAdd;
us.lastCashAddTime = lastCashAddTime;
us.passiveTime = passiveTime;
us.lastActivityTime = lastActivityTime;

printfd(__FILE__, "USER::WriteStat()\n");

if (store->SaveUserStat(us, login))
    {
    WriteServLog("Cannot write stat for user %s.", login.c_str());
    WriteServLog("%s", store->GetStrError().c_str());
    printfd(__FILE__, "Cannot write stat for user %s.\n", login.c_str());
    printfd(__FILE__, "%s\n", store->GetStrError().c_str());
    return -1;
    }

lastWriteStat = stgTime;

return 0;
}
//-----------------------------------------------------------------------------
int USER::WriteMonthStat()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
USER_STAT us;
struct tm * t1;
time_t tt = stgTime - 3600;
t1 = localtime(&tt);

us.up = up;
us.down = down;
us.cash = cash;
us.freeMb = freeMb;
us.lastCashAdd = lastCashAdd;
us.lastCashAddTime = lastCashAddTime;
us.passiveTime = passiveTime;
us.lastActivityTime = lastActivityTime;

if (store->SaveMonthStat(us, t1->tm_mon, t1->tm_year, login))
    {
    WriteServLog("Cannot write month stat for user %s.", login.c_str());
    WriteServLog("%s", store->GetStrError().c_str());
    printfd(__FILE__, "Cannot write month stat for user %s.\n", login.c_str());
    printfd(__FILE__, "%s\n", store->GetStrError().c_str());
    return -1;
    }

return 0;
}
//-----------------------------------------------------------------------------
int USER::Authorize(uint32_t ip, const string &, uint32_t dirs, const BASE_AUTH * auth)
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
/*
 *  Authorize user. It only means that user will be authorized. Nothing more.
 *  User can be connected or disconnected while authorized.
 *  Example: user is authorized but disconnected due to 0 money or blocking
 */

/*
 * Prevent double authorization by identical authorizers
 */
if (authorizedBy.find(auth) != authorizedBy.end())
    return 0;

if (!ip)
    return -1;

for (int i = 0; i < DIR_NUM; i++)
    {
    enabledDirs[i] = dirs & (1 << i);
    }

if (authorizedBy.size())
    {
    if (currIP != ip)
        {
        //  We are already authorized, but with different IP address
        errorStr = "User " + login + " alredy authorized with IP address " + inet_ntostring(ip);
        return -1;
        }

    map<uint32_t, user_iter>::const_iterator ci = ipIndex->find(ip);
    if (ci != ipIndex->end())
        {
        //  Address is already present in IP-index
        //  If it's not our IP - throw an error
        if (&(*ci->second) != this)
            {
            errorStr = "IP address " + inet_ntostring(ip) + " alredy in use";
            return -1;
            }
        }
    }
else
    {
    if (ipIndex->find(ip) != ipIndex->end())
        {
        //  Address is already present in IP-index
        errorStr = "IP address " + inet_ntostring(ip) + " alredy in use";
        return -1;
        }

    if (ips.ConstData().IsIPInIPS(ip))
        {
        currIP = ip;
        lastIPForDisconnect = currIP;
        }
    else
        {
        printfd(__FILE__, " user %s: ips = %s\n", login.c_str(), ips.ConstData().GetIpStr().c_str());
        errorStr = "IP address " + inet_ntostring(ip) + " not belong user " + login;
        return -1;
        }
    }

authorizedBy.insert(auth);

ScanMessage();

return 0;
}
//-----------------------------------------------------------------------------
void USER::Unauthorize(const BASE_AUTH * auth)
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
/*
 *  Authorizer tries to unauthorize user, that was not authorized by it
 */
if (!authorizedBy.erase(auth))
    return;

if (authorizedBy.empty())
    {
    lastIPForDisconnect = currIP;
    currIP = 0; // DelUser in traffcounter
    return;
    }
}
//-----------------------------------------------------------------------------
bool USER::IsAuthorizedBy(const BASE_AUTH * auth) const
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
//  Is this user authorized by specified authorizer?
return authorizedBy.find(auth) != authorizedBy.end();
}
//-----------------------------------------------------------------------------
void USER::Connect(bool fakeConnect)
{
/*
 *  Connect user to Internet. This function is differ from Authorize() !!!
 */

STG_LOCKER lock(&mutex, __FILE__, __LINE__);

if (!fakeConnect)
    {
    string scriptOnConnect = settings->GetScriptDir() + "/OnConnect";

    if (access(scriptOnConnect.c_str(), X_OK) == 0)
        {
        char dirsStr[DIR_NUM + 1];
        dirsStr[DIR_NUM] = 0;
        for (int i = 0; i < DIR_NUM; i++)
            {
            dirsStr[i] = enabledDirs[i] ? '1' : '0';
            }

        string scriptOnConnectParams;
        strprintf(&scriptOnConnectParams,
                "%s \"%s\" \"%s\" \"%f\" \"%d\" \"%s\"",
                scriptOnConnect.c_str(),
                login.c_str(),
                inet_ntostring(currIP).c_str(),
                (double)cash,
                id,
                dirsStr);

        ScriptExec(scriptOnConnectParams);
        }
    else
        {
        WriteServLog("Script %s cannot be executed. File not found.", scriptOnConnect.c_str());
        }

    connected = true;
    }

if (store->WriteUserConnect(login, currIP))
    {
    WriteServLog("Cannot write connect for user %s.", login.c_str());
    WriteServLog("%s", store->GetStrError().c_str());
    }

if (!fakeConnect)
    lastIPForDisconnect = currIP;

//printfd(__FILE__, "Connect. user name \'%s\' ip=%s\n", login.c_str(), inet_ntostring(currIP).c_str());
/*if (settings->GetLogUserConnectDisconnect())
    WriteServLog("User \'%s\', %s: Connect.", login.c_str(), inet_ntostring(currIP).c_str());*/
}
//-----------------------------------------------------------------------------
void USER::Disconnect(bool fakeDisconnect, const std::string & reason)
{
/*
 *  Disconnect user from Internet. This function is differ from UnAuthorize() !!!
 */

STG_LOCKER lock(&mutex, __FILE__, __LINE__);

if (!lastIPForDisconnect)
    {
    printfd(__FILE__, "lastIPForDisconnect\n");
    return;
    }

if (!fakeDisconnect)
    {
    string scriptOnDisonnect = settings->GetScriptDir() + "/OnDisconnect";

    if (access(scriptOnDisonnect.c_str(), X_OK) == 0)
        {
        char dirsStr[DIR_NUM + 1];
        dirsStr[DIR_NUM] = 0;
        for (int i = 0; i < DIR_NUM; i++)
            {
            dirsStr[i] = enabledDirs[i] ? '1' : '0';
            }

        string scriptOnDisonnectParams;
        strprintf(&scriptOnDisonnectParams,
                "%s \"%s\" \"%s\" \"%f\" \"%d\" \"%s\"",
                scriptOnDisonnect.c_str(),
                login.c_str(),
                inet_ntostring(lastIPForDisconnect).c_str(),
                (double)cash,
                id,
                dirsStr);

        ScriptExec(scriptOnDisonnectParams);
        }
    else
        {
        WriteServLog("Script OnDisconnect cannot be executed. File not found.");
        }

    connected = false;
    }

if (store->WriteUserDisconnect(login, up, down, sessionUpload, sessionDownload, cash, freeMb, reason))
    {
    WriteServLog("Cannot write disconnect for user %s.", login.c_str());
    WriteServLog("%s", store->GetStrError().c_str());
    }

//printfd(__FILE__, "Disconnect. User name \'%s\' ip=%s reason: '%s'\n", login.c_str(), inet_ntostring(lastIPForDisconnect).c_str(), reason.c_str());
/*if (settings->GetLogUserConnectDisconnect())
    WriteServLog("User \'%s\', %s: Disconnect.", login.c_str(), inet_ntostring(lastIPForDisconnect).c_str());*/

if (!fakeDisconnect)
    lastIPForDisconnect = 0;

DIR_TRAFF zeroSesssion;

sessionUpload = zeroSesssion;
sessionDownload = zeroSesssion;
}
//-----------------------------------------------------------------------------
void USER::PrintUser() const
{
//return;
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
cout << "============================================================" << endl;
cout << "id=" << id << endl;
cout << "login=" << login << endl;
cout << "password=" << password << endl;
cout << "passive=" << passive << endl;
cout << "disabled=" << disabled << endl;
cout << "disabledDetailStat=" << disabledDetailStat << endl;
cout << "alwaysOnline=" << alwaysOnline << endl;
cout << "tariffName=" << tariffName << endl;
cout << "address=" << address << endl;
cout << "phone=" << phone << endl;
cout << "email=" << email << endl;
cout << "note=" << note << endl;
cout << "realName=" <<realName << endl;
cout << "group=" << group << endl;
cout << "credit=" << credit << endl;
cout << "nextTariff=" << nextTariff << endl;
cout << "userdata0" << userdata0 << endl;
cout << "userdata1" << userdata1 << endl;
cout << "creditExpire=" << creditExpire << endl;
cout << "ips=" << ips << endl;
cout << "------------------------" << endl;
cout << "up=" << up << endl;
cout << "down=" << down << endl;
cout << "cash=" << cash << endl;
cout << "freeMb=" << freeMb << endl;
cout << "lastCashAdd=" << lastCashAdd << endl;
cout << "lastCashAddTime=" << lastCashAddTime << endl;
cout << "passiveTime=" << passiveTime << endl;
cout << "lastActivityTime=" << lastActivityTime << endl;
cout << "============================================================" << endl;
}
//-----------------------------------------------------------------------------
void USER::Run()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

if (stgTime - lastWriteStat > settings->GetStatWritePeriod())
    {
    printfd(__FILE__, "USER::WriteStat user=%s\n", GetLogin().c_str());
    WriteStat();
    }
if (creditExpire.ConstData() && creditExpire.ConstData() < stgTime)
    {
    WriteServLog("User: %s. Credit expired.", login.c_str());
    credit = 0;
    creditExpire = 0;
    WriteConf();
    }

if (passive.ConstData()
    && (stgTime % 30 == 0)
    && (passiveTime.ModificationTime() != stgTime))
    {
    passiveTime = passiveTime + (stgTime - passiveTime.ModificationTime());
    printfd(__FILE__, "===== %s: passiveTime=%d =====\n", login.c_str(), passiveTime.ConstData());
    }

if (!authorizedBy.empty())
    {
    if (connected)
        {
        lastActivityTime = *const_cast<time_t *>(&stgTime);
        }
    if (!connected && IsInetable())
        {
        Connect();
        }
    if (connected && !IsInetable())
        {
        if (disabled)
            Disconnect(false, "disabled");
        else if (passive)
            Disconnect(false, "passive");
        else
            Disconnect(false, "no cash");
        }

    if (stgTime - lastScanMessages > 10)
        {
        ScanMessage();
        lastScanMessages = stgTime;
        }
    }
else
    {
    if (connected)
        {
        Disconnect(false, "not authorized");
        }
    }

}
//-----------------------------------------------------------------------------
void USER::UpdatePingTime(time_t t)
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
//printfd(__FILE__, "UpdatePingTime(%d) %s\n", t, login.c_str());
if (t)
    pingTime = t;
else
    pingTime = stgTime;
}
//-----------------------------------------------------------------------------
bool USER::IsInetable()
{
//STG_LOCKER lock(&mutex, __FILE__, __LINE__);

if (disabled || passive)
    return false;

if (settings->GetFreeMbAllowInet())
    {
    if (freeMb >= 0)
        return true;
    }

if (settings->GetShowFeeInCash())
    {
    return (cash >= -credit);
    }

return (cash - tariff->GetFee() >= -credit);
}
//-----------------------------------------------------------------------------
string USER::GetEnabledDirs()
{
//STG_LOCKER lock(&mutex, __FILE__, __LINE__);

string dirs = "";
for(int i = 0; i < DIR_NUM; i++)
    dirs += enabledDirs[i] ? "1" : "0";
return dirs;
}
//-----------------------------------------------------------------------------
#ifdef TRAFF_STAT_WITH_PORTS
void USER::AddTraffStatU(int dir, uint32_t ip, uint16_t port, uint32_t len)
#else
void USER::AddTraffStatU(int dir, uint32_t ip, uint32_t len)
#endif
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

if (!connected)
    return;

double cost = 0;
DIR_TRAFF dt(up);

int64_t traff = tariff->GetTraffByType(up.ConstData()[dir], down.ConstData()[dir]);
int64_t threshold = tariff->GetThreshold(dir) * 1024 * 1024;

dt[dir] += len;

int tt = tariff->GetTraffType();
if (tt == TRAFF_UP ||
    tt == TRAFF_UP_DOWN ||
    // Check NEW traff data
    (tt == TRAFF_MAX && dt[dir] > down.ConstData()[dir]))
    {
    double dc = 0;
    if (traff < threshold &&
        traff + len >= threshold)
        {
        // cash = partBeforeThreshold * priceBeforeThreshold +
        //        partAfterThreshold * priceAfterThreshold
        int64_t before = threshold - traff; // Chunk part before threshold
        int64_t after = len - before; // Chunk part after threshold
        dc = tariff->GetPriceWithTraffType(up.ConstData()[dir], // Traff before chunk
                                           down.ConstData()[dir],
                                           dir,
                                           stgTime) * before +
             tariff->GetPriceWithTraffType(dt[dir], // Traff after chunk
                                           down.ConstData()[dir],
                                           dir,
                                           stgTime) * after;
        }
    else
        {
        dc = tariff->GetPriceWithTraffType(up.ConstData()[dir],
                                           down.ConstData()[dir],
                                           dir,
                                           stgTime) * len;
        }

    if (freeMb.ConstData() <= 0) // FreeMb is exhausted
        cost = dc;
    else if (freeMb.ConstData() < dc) // FreeMb is partially exhausted
        cost = dc - freeMb.ConstData();

    // Direct access to internal data structures via friend-specifier
    property.stat.freeMb -= dc;
    property.stat.cash -= cost;
    cash.ModifyTime();
    freeMb.ModifyTime();
    }

up = dt;
sessionUpload[dir] += len;

//Add detailed stat

if (!writeFreeMbTraffCost && freeMb.ConstData() >= 0)
    cost = 0;

#ifdef TRAFF_STAT_WITH_PORTS
IP_DIR_PAIR idp(ip, dir, port);
#else
IP_DIR_PAIR idp(ip, dir);
#endif

map<IP_DIR_PAIR, STAT_NODE>::iterator lb;
lb = traffStat->lower_bound(idp);
if (lb == traffStat->end())
    {
    traffStat->insert(lb,
                      pair<IP_DIR_PAIR, STAT_NODE>(idp,
                                                   STAT_NODE(len, 0, cost)));
    }
else
    if (lb->first.dir == dir && lb->first.ip == ip)
        {
        lb->second.cash += cost;
        lb->second.up += len;
        }
    else
        {
        traffStat->insert(lb,
                          pair<IP_DIR_PAIR, STAT_NODE>(idp,
                                                       STAT_NODE(len, 0, cost)));
        }
}
//-----------------------------------------------------------------------------
#ifdef TRAFF_STAT_WITH_PORTS
void USER::AddTraffStatD(int dir, uint32_t ip, uint16_t port, uint32_t len)
#else
void USER::AddTraffStatD(int dir, uint32_t ip, uint32_t len)
#endif
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

if (!connected)
    return;

double cost = 0;
DIR_TRAFF dt(down);

int64_t traff = tariff->GetTraffByType(up.ConstData()[dir], down.ConstData()[dir]);
int64_t threshold = tariff->GetThreshold(dir) * 1024 * 1024;

dt[dir] += len;

int tt = tariff->GetTraffType();
if (tt == TRAFF_DOWN ||
    tt == TRAFF_UP_DOWN ||
    // Check NEW traff data
    (tt == TRAFF_MAX && up.ConstData()[dir] <= dt[dir]))
    {
    double dc = 0;
    if (traff < threshold &&
        traff + len >= threshold)
        {
        // cash = partBeforeThreshold * priceBeforeThreshold +
        //        partAfterThreshold * priceAfterThreshold
        int64_t before = threshold - traff; // Chunk part before threshold
        int64_t after = len - before; // Chunk part after threshold
        dc = tariff->GetPriceWithTraffType(up.ConstData()[dir],
                                           down.ConstData()[dir], // Traff before chunk
                                           dir,
                                           stgTime) * before +
             tariff->GetPriceWithTraffType(up.ConstData()[dir],
                                           dt[dir], // Traff after chunk
                                           dir,
                                           stgTime) * after;
        }
    else
        {
        dc = tariff->GetPriceWithTraffType(up.ConstData()[dir],
                                           down.ConstData()[dir],
                                           dir,
                                           stgTime) * len;
        }

    if (freeMb.ConstData() <= 0) // FreeMb is exhausted
        cost = dc;
    else if (freeMb.ConstData() < dc) // FreeMb is partially exhausted
        cost = dc - freeMb.ConstData();

    property.stat.freeMb -= dc;
    property.stat.cash -= cost;
    cash.ModifyTime();
    freeMb.ModifyTime();
    }

down = dt;
sessionDownload[dir] += len;

//Add detailed stat

if (!writeFreeMbTraffCost && freeMb.ConstData() >= 0)
    cost = 0;

#ifdef TRAFF_STAT_WITH_PORTS
IP_DIR_PAIR idp(ip, dir, port);
#else
IP_DIR_PAIR idp(ip, dir);
#endif

map<IP_DIR_PAIR, STAT_NODE>::iterator lb;
lb = traffStat->lower_bound(idp);
if (lb == traffStat->end())
    {
    traffStat->insert(lb,
                      pair<IP_DIR_PAIR, STAT_NODE>(idp,
                                                   STAT_NODE(0, len, cost)));
    }
else
    if (lb->first.dir == dir && lb->first.ip == ip)
        {
        lb->second.cash += cost;
        lb->second.down += len;
        }
    else
        {
        traffStat->insert(lb,
                          pair<IP_DIR_PAIR, STAT_NODE>(idp,
                                                       STAT_NODE(0, len, cost)));
        }
}
//-----------------------------------------------------------------------------
void USER::AddCurrIPBeforeNotifier(PROPERTY_NOTIFIER_BASE<uint32_t> * n)
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
currIP.AddBeforeNotifier(n);
}
//-----------------------------------------------------------------------------
void USER::DelCurrIPBeforeNotifier(PROPERTY_NOTIFIER_BASE<uint32_t> * n)
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
currIP.DelBeforeNotifier(n);
}
//-----------------------------------------------------------------------------
void USER::AddCurrIPAfterNotifier(PROPERTY_NOTIFIER_BASE<uint32_t> * n)
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
currIP.AddAfterNotifier(n);
}
//-----------------------------------------------------------------------------
void USER::DelCurrIPAfterNotifier(PROPERTY_NOTIFIER_BASE<uint32_t> * n)
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
currIP.DelAfterNotifier(n);
}
//-----------------------------------------------------------------------------
void USER::OnAdd()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

string scriptOnAdd = settings->GetScriptDir() + "/OnUserAdd";

if (access(scriptOnAdd.c_str(), X_OK) == 0)
    {
    string scriptOnAddParams;
    strprintf(&scriptOnAddParams,
            "%s \"%s\"",
            scriptOnAdd.c_str(),
            login.c_str());

    ScriptExec(scriptOnAddParams);
    }
else
    {
    WriteServLog("Script %s cannot be executed. File not found.", scriptOnAdd.c_str());
    }
}
//-----------------------------------------------------------------------------
void USER::OnDelete()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

string scriptOnDel = settings->GetScriptDir() + "/OnUserDel";

if (access(scriptOnDel.c_str(), X_OK) == 0)
    {
    string scriptOnDelParams;
    strprintf(&scriptOnDelParams,
            "%s \"%s\"",
            scriptOnDel.c_str(),
            login.c_str());

    ScriptExec(scriptOnDelParams);
    }
else
    {
    WriteServLog("Script %s cannot be executed. File not found.", scriptOnDel.c_str());
    }

Run();
}
//-----------------------------------------------------------------------------
void USER::ResetDetailStat()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

traffStatToWrite->erase(traffStatToWrite->begin(), traffStatToWrite->end());
}
//-----------------------------------------------------------------------------
int USER::WriteDetailStat()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

printfd(__FILE__, "USER::WriteDetailedStat(): size = %d\n", traffStatToWrite->size());

if (traffStatToWrite->size() && !disabledDetailStat)
    {
    if (store->WriteDetailedStat(traffStatToWrite, lastWriteDeatiledStat, login))
        {
        WriteServLog("Cannot write detail stat for user %s.", login.c_str());
        WriteServLog("%s", store->GetStrError().c_str());
        }
    }
lastWriteDeatiledStat = lastSwapDeatiledStat;
return 0;
}
//-----------------------------------------------------------------------------
int USER::SwapDetailStat()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

lastSwapDeatiledStat = stgTime;
traffStatToWrite = &traffStatInternal[traffStatInUse % 2];
traffStat = &traffStatInternal[++traffStatInUse % 2];

return 0;
}
//-----------------------------------------------------------------------------
double USER::GetPassiveTimePart() const
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

static int daysInMonth[12] =
{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

struct tm * tms;
time_t t = stgTime;
tms = localtime(&t);

time_t secMonth = daysInMonth[(tms->tm_mon + 11) % 12] * 24 * 3600; // Previous month

if (tms->tm_year % 4 == 0 && tms->tm_mon == 1)
    {
    // Leap year
    secMonth += 24 * 3600;
    }

int dt = secMonth - passiveTime;

if (dt < 0)
    dt = 0;

return double(dt) / (secMonth);
}
//-----------------------------------------------------------------------------
void USER::SetPassiveTimeAsNewUser()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

time_t t;
struct tm * tm;
t = stgTime;
tm = localtime(&t);
int daysCurrMon = DaysInCurrentMonth();
double pt = (tm->tm_mday - 1) / (double)daysCurrMon;

passiveTime = (time_t)(pt * 24 * 3600 * daysCurrMon);
}
//-----------------------------------------------------------------------------
void USER::MidnightResetSessionStat()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

if (connected)
    {
    Disconnect(true, "fake");
    Connect(true);
    }
}
//-----------------------------------------------------------------------------
void USER::ProcessNewMonth()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);
//  Reset traff
if (connected)
    {
    Disconnect(true, "fake");
    }
DIR_TRAFF zeroTarff;

WriteMonthStat();

up = zeroTarff;
down = zeroTarff;

if (connected)
    {
    Connect(true);
    }

//  Set new tariff
if (nextTariff.ConstData() != "")
    {
    const TARIFF * nt;
    nt = tariffs->FindByName(nextTariff);
    if (nt == NULL)
        {
        WriteServLog("Cannot change tariff for user %s. Tariff %s not exist.",
                     login.c_str(), property.tariffName.Get().c_str());
        }
    else
        {
        property.tariffName.Set(nextTariff, sysAdmin, login, store);
        tariff = nt;
        }
    ResetNextTariff();
    WriteConf();
    }
}
//-----------------------------------------------------------------------------
void USER::ProcessDayFeeSpread()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

if (passive.ConstData())
    return;

double f = tariff->GetFee() / DaysInCurrentMonth();

if (f == 0.0)
    return;

double c = cash;
property.cash.Set(c - f, sysAdmin, login, store, "Subscriber fee charge");
ResetPassiveTime();
}
//-----------------------------------------------------------------------------
void USER::ProcessDayFee()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

double passiveTimePart = 1.0;
if (!settings->GetFullFee())
    {
    passiveTimePart = GetPassiveTimePart();
    }
else
    {
    if (passive.ConstData())
        {
        printfd(__FILE__, "Don't charge fee `cause we are passive\n");
        return;
        }
    }
double f = tariff->GetFee() * passiveTimePart;

ResetPassiveTime();

if (f == 0.0)
    return;

double c = cash;
printfd(__FILE__, "login: %8s   Fee=%f PassiveTimePart=%f fee=%f\n",
        login.c_str(),
        tariff->GetFee(),
        passiveTimePart,
        f);
property.cash.Set(c - f, sysAdmin, login, store, "Subscriber fee charge");
}
//-----------------------------------------------------------------------------
void USER::SetPrepaidTraff()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

property.freeMb.Set(tariff->GetFree(), sysAdmin, login, store, "Prepaid traffic");
}
//-----------------------------------------------------------------------------
int USER::AddMessage(STG_MSG * msg)
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

if (SendMessage(*msg) == 0)
    {
    if (msg->header.repeat > 0)
        {
        msg->header.repeat--;
        #ifndef DEBUG
        //TODO: gcc v. 4.x generate ICE on x86_64
        msg->header.lastSendTime = time(NULL);
        #else
        msg->header.lastSendTime = stgTime;
        #endif
        if (store->AddMessage(msg, login))
            {
            errorStr = store->GetStrError();
            STG_LOGGER & WriteServLog = GetStgLogger();
            WriteServLog("Error adding message %s", errorStr.c_str());
            WriteServLog("%s", store->GetStrError().c_str());
            return -1;
            }
        }
    }
else
    {
    if (store->AddMessage(msg, login))
        {
        errorStr = store->GetStrError();
        STG_LOGGER & WriteServLog = GetStgLogger();
        WriteServLog("Error adding message %s", errorStr.c_str());
        WriteServLog("%s", store->GetStrError().c_str());
        return -1;
        }
    }
return 0;
}
//-----------------------------------------------------------------------------
int USER::SendMessage(const STG_MSG & msg)
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

if (authorizedBy.empty())
    {
    return -1;
    }

int ret = -1;
set<const BASE_AUTH*>::iterator it;

it = authorizedBy.begin();
while (it != authorizedBy.end())
    {
    if ((*it)->SendMessage(msg, currIP) == 0)
        ret = 0;
    ++it;
    }
return ret;
}
//-----------------------------------------------------------------------------
int USER::ScanMessage()
{
STG_LOCKER lock(&mutex, __FILE__, __LINE__);

vector<STG_MSG_HDR> hdrsList;

if (store->GetMessageHdrs(&hdrsList, login))
    {
    printfd(__FILE__, "Error GetMessageHdrs %s\n", store->GetStrError().c_str());
    return -1;
    }

for (unsigned i = 0; i < hdrsList.size(); i++)
    {

    if (hdrsList[i].lastSendTime + hdrsList[i].repeatPeriod * 60 < (unsigned)stgTime)
        {
        STG_MSG msg;
        if (store->GetMessage(hdrsList[i].id, &msg, login) == 0)
            {
            if (SendMessage(msg) == 0)
                {
                msg.header.repeat--;
                if (msg.header.repeat < 0)
                    {
                    printfd(__FILE__, "DelMessage\n");
                    store->DelMessage(hdrsList[i].id, login);
                    }
                else
                    {
                    #ifndef DEBUG
                    //TODO: gcc v. 4.x generate ICE on x86_64
                    msg.header.lastSendTime = time(NULL);
                    #else
                    msg.header.lastSendTime = stgTime;
                    #endif
                    if (store->EditMessage(msg, login))
                        {
                        printfd(__FILE__, "EditMessage Error %s\n", store->GetStrError().c_str());
                        }
                    }
                }
            }
        else
            {
            WriteServLog("Cannot get message for user %s.", login.c_str());
            WriteServLog("%s", store->GetStrError().c_str());
            }
        }
    }
return 0;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHG_PASSIVE_NOTIFIER::Notify(const int & oldPassive, const int & newPassive)
{
if (newPassive && !oldPassive)
    user->property.cash.Set(user->cash - user->tariff->GetPassiveCost(),
                            user->sysAdmin,
                            user->login,
                            user->store,
                            "Freeze");
}
//-----------------------------------------------------------------------------
void CHG_TARIFF_NOTIFIER::Notify(const string &, const string & newTariff)
{
user->tariff = user->tariffs->FindByName(newTariff);
}
//-----------------------------------------------------------------------------
void CHG_CASH_NOTIFIER::Notify(const double & oldCash, const double & newCash)
{
user->lastCashAddTime = *const_cast<time_t *>(&stgTime);
user->lastCashAdd = newCash - oldCash;
}
//-----------------------------------------------------------------------------
void CHG_IP_NOTIFIER::Notify(const uint32_t & from, const uint32_t & to)
{
    printfd(__FILE__, "Change IP from %s to %s\n", inet_ntostring(from).c_str(), inet_ntostring(to).c_str());
    if (from != 0)
        user->Disconnect(false, "Change IP");
    if (to != 0)
        user->Connect(false);
}
//-----------------------------------------------------------------------------