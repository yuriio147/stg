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
 $Author: faust $
 $Revision: 1.12 $
 $Date: 2009/06/08 10:02:28 $
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#include <langinfo.h>
#include <iostream>
#include <iconv.h>

#include "stg/common.h"
#include "sg_error_codes.h"
#include "common_sg.h"
#include "version_sg.h"

using namespace std;

const int usageConf = 0;
const int usageInfo = 1;

const int TO_KOI8 = 0;
const int FROM_KOI8 = 1;
//-----------------------------------------------------------------------------
struct GetUserData
{
    GetUserData(REQUEST & req, bool res) : request(req), result(res) {}
    REQUEST & request;
    bool result;
};
//---------------------------------------------------------------------------
struct HelpParams
{
    string setActionName;
    string getActionName;
    string valueName;
    string valueParam;
};
//---------------------------------------------------------------------------
void Usage(int usageType)
{
printf("Sgconf version: %s\n\n", VERSION_SG);

char action[4];
if (usageType == usageConf)
    strcpy(action, "set");
else
    strcpy(action, "get");

printf("To add or to set cash use:\n");
printf("sgconf set -s <server> -p <port> -a <admin> -w <admin_pass> -u <user> -c <add_cash[:log message]>\n");
printf("sgconf set -s <server> -p <port> -a <admin> -w <admin_pass> -u <user> -v <set_cash[:log message]>\n");
printf("To get cash use:\n");
printf("sgconf get -s <server> -p <port> -a <admin> -w <admin_pass> -u <user> -c\n\n");

HelpParams hp[] =
{
    {"set tariff",              "get tariff",           "-t",   "<tariff:now|delayed>"},
    {"set credit",              "get credit",           "-r",   "<credit>"},
    {"set credit expire",       "get credit expire",    "-E",   "<credit_expire_date>"},
    {"set password",            "get password",         "-o",   "<new_password>"},
    {"set prepaid traffic",     "get prepaid traffic",  "-e",   "<prepaid>"},
    {"set IP-addresses",        "get IP-addresses",     "-I",   "<*|ip_addr[,ip_addr...]>"},
    {"set name",                "get name",             "-A",   "<name>"},
    {"set note",                "get note",             "-N",   "<note>"},
    {"set street address",      "get street address",   "-D",   "<address>"},
    {"set email",               "get email",            "-L",   "<email>"},
    {"set phone",               "get phone",            "-P",   "<phone>"},
    {"set group",               "get group",            "-G",   "<group>"},
    {"set/unset down",          "get down",             "-d",   "<0/1>"},
    {"set/unset \'passive\'",   "get \'passive\'",      "-i",   "<0/1>"},
    {"set/unset \'disableDetailStat\'",   "get \'disableDetailStat\'",      "--disable-stat",   "<0/1>"},
    {"set/unset \'alwaysOnline\'",   "get \'alwaysOnline\'",      "--always-online",   "<0/1>"},
};

for (unsigned i = 0; i < sizeof(hp) / sizeof(HelpParams); i++)
    {
    printf("To %s use:\n", hp[i].setActionName.c_str());
    printf("sgconf set -s <server> -p <port> -a <admin> -w <admin_pass> -u <user> %s %s\n",
           hp[i].valueName.c_str(), hp[i].valueParam.c_str());
    printf("To %s use:\n", hp[i].getActionName.c_str());
    printf("sgconf get -s <server> -p <port> -a <admin> -w <admin_pass> -u <user> %s\n\n",
           hp[i].valueName.c_str());
    }

printf("To set user\'s upload traffic value use:\n");
printf("sgconf set -s <server> -p <port> -a <admin> -w <admin_pass> -u <user> --u0 <traff> [--u1<traff> ...]\n");
printf("To get user\'s upload traffic value use:\n");
printf("sgconf get -s <server> -p <port> -a <admin> -w <admin_pass> -u <user> --u0 [--u1 ...]\n\n");

printf("To set user\'s download traffic value use:\n");
printf("sgconf set -s <server> -p <port> -a <admin> -w <admin_pass> -u <user> --d0 <traff> [--d1<traff> ...]\n");
printf("To get user\'s download traffic value use:\n");
printf("sgconf get -s <server> -p <port> -a <admin> -w <admin_pass> -u <user> --d0 [--d1 ...]\n\n");

printf("To set userdata<0...9> use:\n");
printf("sgconf set -s <server> -p <port> -a <admin> -w <admin_pass> -u <user> --ud0 <userdata> [--ud1<userdata> ...]\n");
printf("To get userdata<0...9> use:\n");
printf("sgconf get -s <server> -p <port> -a <admin> -w <admin_pass> -u <user> --ud0 [--ud1 ...]\n\n");

printf("To get user's authorizers list use:\n");
printf("sgconf get -s <server> -p <port> -a <admin> -w <admin_pass> -u <user> --authorized-by\n\n");

printf("To send message use:\n");
printf("sgconf set -s <server> -p <port> -a <admin> -w <admin_pass> -u <user> -m <message>\n\n");

printf("To create user use:\n");
printf("sgconf set -s <server> -p <port> -a <admin> -w <admin_pass> -u <user> -n\n\n");

printf("To delete user use:\n");
printf("sgconf set -s <server> -p <port> -a <admin> -w <admin_pass> -u <user> -l\n\n");
}
//---------------------------------------------------------------------------
void UsageConf()
{
Usage(usageConf);
}
//---------------------------------------------------------------------------
void UsageInfo()
{
Usage(usageInfo);
}
//---------------------------------------------------------------------------
int CheckLogin(const char * login)
{
for (int i = 0; i < (int)strlen(login); i++)
    {
    if (!(( login[i] >= 'a' && login[i] <= 'z')
        || (login[i] >= 'A' && login[i] <= 'Z')
        || (login[i] >= '0' && login[i] <= '9')
        ||  login[i] == '.'
        ||  login[i] == '_'
        ||  login[i] == '-'))
        {
        return 1;
        }
    }
return 0;
}
//-----------------------------------------------------------------------------
short int ParseServerPort(const char * p)
{
int port;
if (str2x(p, port) != 0)
    {
    printf("Incorresct server port %s\n", p);
    exit(NETWORK_ERR_CODE);
    }
return (short)port;
}
//-----------------------------------------------------------------------------
char * ParseAdminLogin(char * adm)
{
if (CheckLogin(adm))
    {
    printf("Incorresct admin login %s\n", adm);
    exit(PARAMETER_PARSING_ERR_CODE);
    }
return adm;
}
//-----------------------------------------------------------------------------
char * ParsePassword(char * pass)
{
if (strlen(pass) >= ADM_PASSWD_LEN)
    {
    printf("Password too big %s\n", pass);
    exit(PARAMETER_PARSING_ERR_CODE);
    }

return pass;
}
//-----------------------------------------------------------------------------
char * ParseUser(char * usr)
{
if (CheckLogin(usr))
    {
    printf("Incorresct user login %s\n", usr);
    exit(PARAMETER_PARSING_ERR_CODE);
    }
return usr;
}
//-----------------------------------------------------------------------------
void ConvertKOI8(const string & src, string * dst, int encType)
{
iconv_t cd;
char * ob = new char[src.size() * 2 + 1];
char * ib = new char[src.size() + 1];

strcpy(ib, src.c_str());

char * outbuf = ob;
char * inbuf = ib;

setlocale(LC_ALL, "");

char charsetF[100];
char charsetT[100];

if (encType == TO_KOI8)
    {
    strcpy(charsetF, nl_langinfo(CODESET));
    strcpy(charsetT, "koi8-ru");
    }
else
    {
    strcpy(charsetT, nl_langinfo(CODESET));
    strcpy(charsetF, "koi8-ru");
    }

size_t nconv = 1;

size_t insize = strlen(ib);
size_t outsize = insize * 2 + 1;

insize = src.size();

cd = iconv_open(charsetT, charsetF);
if (cd == (iconv_t) -1)
    {
    if (errno != EINVAL)
        printf("error iconv_open\n");
    else
        {
        printf("Warning: iconv from %s to %s failed\n", charsetF, charsetT);
        *dst = src;
        return;
        }

    exit(ICONV_ERR_CODE);
    }

#if defined(FREE_BSD) || defined(FREE_BSD5)
nconv = iconv(cd, (const char **)&inbuf, &insize, &outbuf, &outsize);
#else
nconv = iconv(cd, &inbuf, &insize, &outbuf, &outsize);
#endif
//printf("charsetT=%s charsetF=%s\n", charsetT, charsetF);
//printf("ib=%s ob=%s\n", ib, ob);
//printf("nconv=%d outsize=%d\n", nconv, outsize);
if (nconv == (size_t) -1)
    {
    if (errno != EINVAL)
        {
        printf("iconv error\n");
        exit(ICONV_ERR_CODE);
        }
    }

*outbuf = L'\0';

iconv_close(cd);
*dst = ob;

delete[] ob;
delete[] ib;
}
//-----------------------------------------------------------------------------
void ConvertFromKOI8(const string & src, string * dst)
{
ConvertKOI8(src, dst, FROM_KOI8);
}
//-----------------------------------------------------------------------------
void ConvertToKOI8(const string & src, string * dst)
{
ConvertKOI8(src, dst, TO_KOI8);
}
//-----------------------------------------------------------------------------
int RecvSetUserAnswer(const char * ans, void * d)
{
GetUserData * data = static_cast<GetUserData *>(d);

data->result = (strcasecmp("Ok", ans) == 0);

return 0;
}
//-----------------------------------------------------------------------------
struct StringReqParams
{
    string name;
    RESETABLE<string> reqParam;
    const string * value;
};
//-----------------------------------------------------------------------------
void GetUserCallback(const PARSER_GET_USER::INFO & info, void * d)
{
GetUserData * data = static_cast<GetUserData *>(d);

if (info.login == "")
    {
    data->result = false;
    return;
    }

if (!data->request.cash.res_empty())
    cout << "cash = " << info.cash << endl;

if (!data->request.credit.res_empty())
    cout << "credit = " << info.credit << endl;

if (!data->request.creditExpire.res_empty())
    {
    char buf[32];
    struct tm brokenTime;
    time_t tt = info.creditExpire;

    brokenTime.tm_wday = 0;
    brokenTime.tm_yday = 0;
    brokenTime.tm_isdst = 0;
    brokenTime.tm_hour = 0;
    brokenTime.tm_min = 0;
    brokenTime.tm_sec = 0;

    gmtime_r(&tt, &brokenTime);

    strftime(buf, 32, "%Y-%m-%d", &brokenTime);

    cout << "creditExpire = " << buf << endl;
    }

if (!data->request.down.res_empty())
    cout << "down = " << info.down << endl;

if (!data->request.passive.res_empty())
    cout << "passive = " << info.passive << endl;

if (!data->request.disableDetailStat.res_empty())
    cout << "disableDetailStat = " << info.disableDetailStat << endl;

if (!data->request.alwaysOnline.res_empty())
    cout << "alwaysOnline = " << info.alwaysOnline << endl;

if (!data->request.prepaidTraff.res_empty())
    cout << "prepaidTraff = " << info.prepaidTraff << endl;

for (int i = 0; i < DIR_NUM; i++)
    {
    if (!data->request.sessionUpload[i].res_empty())
        cout << "session upload for dir " << i << " = " << info.stat.su[i] << endl;
    if (!data->request.sessionDownload[i].res_empty())
        cout << "session download for dir " << i << "=" << info.stat.sd[i] << endl;
    }

for (int i = 0; i < DIR_NUM; i++)
    {
    if (!data->request.monthUpload[i].res_empty())
        cout << "month upload for dir " << i << " = " << info.stat.mu[i] << endl;
    if (!data->request.monthDownload[i].res_empty())
        cout << "month download for dir " << i << " = " << info.stat.md[i] << endl;
    }

for (int i = 0; i < USERDATA_NUM; i++)
    {
    if (!data->request.userData[i].res_empty())
        {
        string str;
        ConvertFromKOI8(info.userData[i], &str);
        cout << "user data " << i << " = " << str << endl;
        }
    }

StringReqParams strReqParams[] =
{
    {"note",     data->request.note,        &info.note},
    {"name",     data->request.name,        &info.name},
    {"address",  data->request.address,     &info.address},
    {"email",    data->request.email,       &info.email},
    {"phone",    data->request.phone,       &info.phone},
    {"group",    data->request.group,       &info.group},
    {"tariff",   data->request.tariff,      &info.tariff},
    {"password", data->request.usrPasswd,   &info.password},
    {"ip",       data->request.ips,         &info.ips} // IP-address of user
};
for (unsigned i = 0; i < sizeof(strReqParams) / sizeof(StringReqParams); i++)
    {
    if (!strReqParams[i].reqParam.res_empty())
        {
        string str;
        ConvertFromKOI8(*strReqParams[i].value, &str);
        cout << strReqParams[i].name << " = " << str << endl;
        }
    }
data->result = true;
}
//-----------------------------------------------------------------------------
void RecvAuthByData(const PARSER_AUTH_BY::INFO & list, void *)
{
for (std::vector<std::string>::const_iterator it = list.begin(); it != list.end(); ++it)
    cout << *it << "\n";
cout << endl;
}
//-----------------------------------------------------------------------------
int ProcessSetUser(const std::string &server,
                   int port,
                   const std::string &admLogin,
                   const std::string &admPasswd,
                   const std::string &str,
                   void * data,
                   bool isMessage)
{
SERVCONF sc;

bool result = false;

sc.SetServer(server.c_str());
sc.SetPort(port);
sc.SetAdmLogin(admLogin.c_str());
sc.SetAdmPassword(admPasswd.c_str());

REQUEST request;
GetUserData cbdata(request, false);

if (isMessage)
    {
    sc.SetSendMessageCb(RecvSetUserAnswer, &cbdata);
    sc.MsgUser(str.c_str());
    }
else
    {
    sc.SetChgUserCb(RecvSetUserAnswer, &cbdata);
    sc.ChgUser(str.c_str());
    }

if (result)
    {
    printf("Ok\n");
    return 0;
    }
else
    {
    printf("Error\n");
    return -1;
    }

return 0;
}
//-----------------------------------------------------------------------------
int ProcessGetUser(const std::string &server,
                   int port,
                   const std::string &admLogin,
                   const std::string &admPasswd,
                   const std::string &login,
                   REQUEST & request)
{
SERVCONF sc;

sc.SetServer(server.c_str());
sc.SetPort(port);
sc.SetAdmLogin(admLogin.c_str());
sc.SetAdmPassword(admPasswd.c_str());

// TODO Good variable name :)
GetUserData data(request, false);

sc.SetGetUserCallback(GetUserCallback, &data);
sc.GetUser(login.c_str());

if (data.result)
    {
    printf("Ok\n");
    return 0;
    }
else
    {
    printf("Error\n");
    return -1;
    }

return 0;
}
//-----------------------------------------------------------------------------
int ProcessAuthBy(const std::string &server,
                  int port,
                  const std::string &admLogin,
                  const std::string &admPasswd,
                  const std::string &login,
                  REQUEST & request)
{
SERVCONF sc;

sc.SetServer(server.c_str());
sc.SetPort(port);
sc.SetAdmLogin(admLogin.c_str());
sc.SetAdmPassword(admPasswd.c_str());

sc.SetAuthByCallback(RecvAuthByData, NULL);
sc.AuthBy(login.c_str());

if (sc.GetError())
    {
    printf("Error\n");
    return -1;
    }

printf("Ok\n");
return 0;
}
//-----------------------------------------------------------------------------
