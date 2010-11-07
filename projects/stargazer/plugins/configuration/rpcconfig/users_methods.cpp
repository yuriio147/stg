#include "users_methods.h"

#include "rpcconfig.h"
#include "user_helper.h"
#include "user_ips.h"
#include "utils.h"

#include "common.h"

//------------------------------------------------------------------------------

void METHOD_USER_GET::execute(xmlrpc_c::paramList const & paramList,
                              xmlrpc_c::value *   const   retvalPtr)
{
std::string cookie = paramList.getString(0);
std::string login = paramList.getString(1);
std::string enc;
paramList.verifyEnd(2);

std::map<std::string, xmlrpc_c::value> structVal;
ADMIN_INFO adminInfo;

if (config->GetAdminInfo(cookie, &adminInfo))
    {
    structVal["result"] = xmlrpc_c::value_boolean(false);
    *retvalPtr = xmlrpc_c::value_struct(structVal);
    return;
    }

user_iter u;

if (users->FindByName(login, &u))
    {
    structVal["result"] = xmlrpc_c::value_boolean(false);
    *retvalPtr = xmlrpc_c::value_struct(structVal);
    return;
    }

USER_HELPER uhelper(u);

if (!adminInfo.priviledges.userConf || !adminInfo.priviledges.userPasswd)
    {
    uhelper.GetUserInfo(retvalPtr, true);
    return;
    }

uhelper.GetUserInfo(retvalPtr);
}

//------------------------------------------------------------------------------

void METHOD_USER_ADD::execute(xmlrpc_c::paramList const & paramList,
                              xmlrpc_c::value *   const   retvalPtr)
{
std::string cookie = paramList.getString(0);
std::string login = paramList.getString(1);
std::string enc;
paramList.verifyEnd(2);

ADMIN_INFO adminInfo;

if (config->GetAdminInfo(cookie, &adminInfo))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

ADMIN admin;

if (admins->FindAdmin(adminInfo.admin, &admin))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

user_iter u;

if (users->FindByName(login, &u))
    {
    if (users->Add(login, admin))
        {
        *retvalPtr = xmlrpc_c::value_boolean(false);
        return;
        }

    *retvalPtr = xmlrpc_c::value_boolean(true);
    return;
    }
    
*retvalPtr = xmlrpc_c::value_boolean(false);
return;
}

//------------------------------------------------------------------------------

void METHOD_USER_DEL::execute(xmlrpc_c::paramList const & paramList,
                              xmlrpc_c::value *   const   retvalPtr)
{
std::string cookie = paramList.getString(0);
std::string login = paramList.getString(1);
std::string enc;
paramList.verifyEnd(2);

ADMIN_INFO adminInfo;

if (config->GetAdminInfo(cookie, &adminInfo))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

ADMIN admin;

if (admins->FindAdmin(adminInfo.admin, &admin))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

user_iter u;

if (users->FindByName(login, &u))
    {
    users->Del(login, admin);
    *retvalPtr = xmlrpc_c::value_boolean(true);
    return;
    }

*retvalPtr = xmlrpc_c::value_boolean(false);
return;
}

//------------------------------------------------------------------------------

void METHOD_USERS_GET::execute(xmlrpc_c::paramList const & paramList,
                               xmlrpc_c::value *   const   retvalPtr)
{
std::string cookie = paramList.getString(0);
std::string enc;
paramList.verifyEnd(1);

std::map<std::string, xmlrpc_c::value> structVal;
std::vector<xmlrpc_c::value> retval;
ADMIN_INFO adminInfo;

if (config->GetAdminInfo(cookie, &adminInfo))
    {
    structVal["result"] = xmlrpc_c::value_boolean(false);
    *retvalPtr = xmlrpc_c::value_struct(structVal);
    return;
    }

bool hidePassword = !adminInfo.priviledges.userConf ||
                    !adminInfo.priviledges.userPasswd;

user_iter u;

int h = users->OpenSearch();
if (!h)
    {
    printfd(__FILE__, "users->OpenSearch() error\n");
    users->CloseSearch(h);
    return;
    }

while (1)
    {
    if (users->SearchNext(h, &u))
        {
        break;
        }

    xmlrpc_c::value info;

    USER_HELPER uhelper(u);

    uhelper.GetUserInfo(&info, hidePassword);

    retval.push_back(info);
    }

*retvalPtr = xmlrpc_c::value_array(retval);
}

//------------------------------------------------------------------------------

void METHOD_USER_CHG::execute(xmlrpc_c::paramList const & paramList,
                              xmlrpc_c::value *   const   retvalPtr)
{
std::string cookie = paramList.getString(0);
std::string login = paramList.getString(1);
xmlrpc_c::value_struct info(paramList.getStruct(2));
std::string enc;
paramList.verifyEnd(3);

ADMIN_INFO adminInfo;

if (config->GetAdminInfo(cookie, &adminInfo))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

ADMIN admin;

if (admins->FindAdmin(adminInfo.admin, &admin))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

user_iter u;

if (users->FindByName(login, &u))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

USER_HELPER uhelper(u);

if (!adminInfo.priviledges.userConf || !adminInfo.priviledges.userPasswd)
    {
    uhelper.SetUserInfo(info, admin, login, *store);
    }
else
    {
    uhelper.SetUserInfo(info, admin, login, *store);
    }

u->WriteConf();
u->WriteStat();

*retvalPtr = xmlrpc_c::value_boolean(true);
}

//------------------------------------------------------------------------------

void METHOD_USER_CASH_ADD::execute(xmlrpc_c::paramList const & paramList,
                                   xmlrpc_c::value *   const   retvalPtr)
{
std::string cookie = paramList.getString(0);
std::string login = paramList.getString(1);
double amount = paramList.getDouble(2);
std::string comment = IconvString(paramList.getString(3), "UTF-8", "KOI8-R");
std::string enc;
paramList.verifyEnd(4);

ADMIN_INFO adminInfo;

if (config->GetAdminInfo(cookie, &adminInfo))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

ADMIN admin;

if (admins->FindAdmin(adminInfo.admin, &admin))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

user_iter u;

if (users->FindByName(login, &u))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

double cash = u->property.cash.Get();
cash += amount;

if (!u->property.cash.Set(cash, admin, login, store, comment))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

u->WriteStat();

*retvalPtr = xmlrpc_c::value_boolean(true);
}

//------------------------------------------------------------------------------

void METHOD_USER_CASH_SET::execute(xmlrpc_c::paramList const & paramList,
                                   xmlrpc_c::value *   const   retvalPtr)
{
std::string cookie = paramList.getString(0);
std::string login = paramList.getString(1);
double cash = paramList.getDouble(2);
std::string comment = IconvString(paramList.getString(3), "UTF-8", "KOI8-R");
std::string enc;
paramList.verifyEnd(4);

ADMIN_INFO adminInfo;

if (config->GetAdminInfo(cookie, &adminInfo))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

ADMIN admin;

if (admins->FindAdmin(adminInfo.admin, &admin))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

user_iter u;

if (users->FindByName(login, &u))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

if (!u->property.cash.Set(cash, admin, login, store, comment))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

u->WriteStat();

*retvalPtr = xmlrpc_c::value_boolean(true);
}

//------------------------------------------------------------------------------

void METHOD_USER_TARIFF_CHANGE::execute(xmlrpc_c::paramList const & paramList,
                                        xmlrpc_c::value *   const   retvalPtr)
{
std::string cookie = paramList.getString(0);
std::string login = paramList.getString(1);
std::string tariff = paramList.getString(2);
bool delayed = paramList.getBoolean(3);
std::string comment = IconvString(paramList.getString(4), "UTF-8", "KOI8-R");
std::string enc;
paramList.verifyEnd(5);

ADMIN_INFO adminInfo;

if (config->GetAdminInfo(cookie, &adminInfo))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

ADMIN admin;

if (admins->FindAdmin(adminInfo.admin, &admin))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

user_iter u;

if (users->FindByName(login, &u))
    {
    *retvalPtr = xmlrpc_c::value_boolean(false);
    return;
    }

if (tariffs->FindByName(tariff))
    {
    if (delayed)
        {
        if (u->property.nextTariff.Set(tariff,
                                       admin,
                                       login,
                                       store))
            {
            u->WriteConf();
            *retvalPtr = xmlrpc_c::value_boolean(true);
            return;
            }
        }
    if (u->property.tariffName.Set(tariff,
                                   admin,
                                   login,
                                   store))
        {
        u->WriteConf();
        *retvalPtr = xmlrpc_c::value_boolean(true);
        return;
        }
    }

*retvalPtr = xmlrpc_c::value_boolean(false);
}

//------------------------------------------------------------------------------

void METHOD_GET_ONLINE_IPS::execute(xmlrpc_c::paramList const & paramList,
                                    xmlrpc_c::value *   const   retvalPtr)
{
std::string cookie = paramList.getString(0);
std::vector<xmlrpc_c::value> subnetsStr = paramList.getArray(1);
paramList.verifyEnd(2);

std::vector<IP_MASK> subnets;

std::vector<xmlrpc_c::value>::iterator it;

for (it = subnetsStr.begin(); it != subnetsStr.end(); ++it)
    {
    IP_MASK ipm;
    if (ParseNet(xmlrpc_c::value_string(*it), ipm))
        {
        printfd(__FILE__, "METHOD_GET_ONLINE_IPS::execute(): Failed to parse subnet ('%s')\n", std::string(xmlrpc_c::value_string(*it)).c_str());
        }
    else
        {
        subnets.push_back(ipm);
        }
    }

std::map<std::string, xmlrpc_c::value> structVal;
ADMIN_INFO adminInfo;

if (config->GetAdminInfo(cookie, &adminInfo))
    {
    structVal["result"] = xmlrpc_c::value_boolean(false);
    *retvalPtr = xmlrpc_c::value_struct(structVal);
    return;
    }

std::vector<xmlrpc_c::value> ips;

user_iter u;

int handle = users->OpenSearch();
if (!handle)
    {
    printfd(__FILE__, "users->OpenSearch() error\n");
    users->CloseSearch(handle);
    return;
    }

while (1)
    {
    if (users->SearchNext(handle, &u))
        {
        break;
        }

    if (u->GetAuthorized())
        {
        uint32_t ip = u->GetCurrIP();

        std::vector<IP_MASK>::iterator it;
        for (it = subnets.begin(); it != subnets.end(); ++it)
            {
            if ((it->ip & it->mask) == (ip & it->mask))
                {
                ips.push_back(xmlrpc_c::value_string(inet_ntostring(u->GetCurrIP())));
                break;
                }
            }
        }
    }

structVal["ips"] = xmlrpc_c::value_array(ips);

*retvalPtr = xmlrpc_c::value_struct(structVal);
}

bool METHOD_GET_ONLINE_IPS::ParseNet(const std::string & net, IP_MASK & ipm) const
{
size_t pos = net.find_first_of('/');

if (pos == std::string::npos)
    {
    printfd(__FILE__, "METHOD_GET_ONLINE_IPS::ParseNet(): Network address is not in CIDR-notation\n");
    return true;
    }

int res = inet_pton(AF_INET, net.substr(0, pos).c_str(), &ipm.ip);

if (res < 0)
    {
    printfd(__FILE__, "METHOD_GET_ONLINE_IPS::ParseNet(): '%s'\n", strerror(errno));
    return true;
    }
else if (res == 0)
    {
    printfd(__FILE__, "METHOD_GET_ONLINE_IPS::ParseNet(): Invalid network address\n", strerror(errno));
    return true;
    }

if (str2x(net.substr(pos + 1, net.length() - pos - 1), ipm.mask))
    {
    printfd(__FILE__, "METHOD_GET_ONLINE_IPS::ParseNet(): Invalid network mask\n");
    return true;
    }
if (ipm.mask > 32)
    {
    printfd(__FILE__, "METHOD_GET_ONLINE_IPS::ParseNet(): Network mask is out of range\n");
    return true;
    }
ipm.mask = htonl(0xffFFffFF << (32 - ipm.mask));

return false;
}