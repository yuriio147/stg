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
 $Revision: 1.15 $
 $Date: 2010/10/04 20:17:12 $
 $Author: faust $
 */

#include "stg/common.h"
#include "admins_impl.h"
#include "admin_impl.h"

#include <cerrno>
#include <cassert>
#include <algorithm>

//-----------------------------------------------------------------------------
ADMINS_IMPL::ADMINS_IMPL(STORE * st)
    : ADMINS(),
      stg(PRIV(0xFFFF), "@stargazer", ""),
      noAdmin(PRIV(0xFFFF), "NO-ADMIN", ""),
      data(),
      store(st),
      WriteServLog(GetStgLogger()),
      searchDescriptors(),
      handle(0),
      mutex(),
      strError()
{
pthread_mutex_init(&mutex, NULL);
Read();
}
//-----------------------------------------------------------------------------
int ADMINS_IMPL::Add(const std::string & login, const ADMIN * admin)
{
STG_LOCKER lock(&mutex);
const PRIV * priv = admin->GetPriv();

if (!priv->adminChg)
    {
    std::string s = admin->GetLogStr() + " Add administrator \'" + login + "\'. Access denied.";
    strError = "Access denied.";
    WriteServLog(s.c_str());
    return -1;
    }

ADMIN_IMPL adm(PRIV(0), login, "");
admin_iter ai(find(data.begin(), data.end(), adm));

if (ai != data.end())
    {
    strError = "Administrator \'" + login + "\' cannot not be added. Administrator already exist.";
    WriteServLog("%s %s", admin->GetLogStr().c_str(), strError.c_str());

    return -1;
    }

data.push_back(adm);

if (store->AddAdmin(login) == 0)
    {
    WriteServLog("%s Administrator \'%s\' added.",
                 admin->GetLogStr().c_str(), login.c_str());
    return 0;
    }

strError = "Administrator \'" + login + "\' was not added. Error: " + store->GetStrError();
WriteServLog("%s %s", admin->GetLogStr().c_str(), strError.c_str());

return -1;
}
//-----------------------------------------------------------------------------
int ADMINS_IMPL::Del(const std::string & login, const ADMIN * admin)
{
STG_LOCKER lock(&mutex);
const PRIV * priv = admin->GetPriv();

if (!priv->adminChg)
    {
    std::string s = admin->GetLogStr() + " Delete administrator \'" + login + "\'. Access denied.";
    strError = "Access denied.";
    WriteServLog(s.c_str());
    return -1;
    }

admin_iter ai(find(data.begin(), data.end(), ADMIN_IMPL(PRIV(0), login, "")));

if (ai == data.end())
    {
    strError = "Administrator \'" + login + "\' cannot be deleted. Administrator does not exist.";
    WriteServLog("%s %s", admin->GetLogStr().c_str(), strError.c_str());
    return -1;
    }

std::map<int, const_admin_iter>::iterator si;
si = searchDescriptors.begin();
while (si != searchDescriptors.end())
    {
    if (si->second == ai)
        (si->second)++;
    ++si;
    }

data.remove(*ai);
if (store->DelAdmin(login) < 0)
    {
    strError = "Administrator \'" + login + "\' was not deleted. Error: " + store->GetStrError();
    WriteServLog("%s %s", admin->GetLogStr().c_str(), strError.c_str());

    return -1;
    }

WriteServLog("%s Administrator \'%s\' deleted.", admin->GetLogStr().c_str(), login.c_str());
return 0;
}
//-----------------------------------------------------------------------------
int ADMINS_IMPL::Change(const ADMIN_CONF & ac, const ADMIN * admin)
{
STG_LOCKER lock(&mutex);
const PRIV * priv = admin->GetPriv();

if (!priv->adminChg)
    {
    std::string s = admin->GetLogStr() + " Change administrator \'" + ac.login + "\'. Access denied.";
    strError = "Access denied.";
    WriteServLog(s.c_str());
    return -1;
    }

admin_iter ai(find(data.begin(), data.end(), ADMIN_IMPL(PRIV(0), ac.login, "")));

if (ai == data.end())
    {
    strError = "Administrator \'" + ac.login + "\' cannot be changed " + ". Administrator does not exist.";
    WriteServLog("%s %s", admin->GetLogStr().c_str(), strError.c_str());
    return -1;
    }

*ai = ac;
if (store->SaveAdmin(ac))
    {
    WriteServLog("Cannot write admin %s.", ac.login.c_str());
    WriteServLog("%s", store->GetStrError().c_str());
    return -1;
    }

WriteServLog("%s Administrator \'%s\' changed.",
             admin->GetLogStr().c_str(), ac.login.c_str());

return 0;
}
//-----------------------------------------------------------------------------
int ADMINS_IMPL::Read()
{
STG_LOCKER lock(&mutex);
std::vector<std::string> adminsList;
if (store->GetAdminsList(&adminsList) < 0)
    {
    WriteServLog(store->GetStrError().c_str());
    return -1;
    }

for (unsigned int i = 0; i < adminsList.size(); i++)
    {
    ADMIN_CONF ac(PRIV(0), adminsList[i], "");

    if (store->RestoreAdmin(&ac, adminsList[i]))
        {
        WriteServLog(store->GetStrError().c_str());
        return -1;
        }

    data.push_back(ADMIN_IMPL(ac));
    }
return 0;
}
//-----------------------------------------------------------------------------
bool ADMINS_IMPL::Find(const std::string & l, ADMIN ** admin)
{
assert(admin != NULL && "Pointer to admin is not null");

STG_LOCKER lock(&mutex);
if (data.empty())
    {
    printfd(__FILE__, "No admin in system!\n");
    *admin = &noAdmin;
    return false;
    }

admin_iter ai(find(data.begin(), data.end(), ADMIN_IMPL(PRIV(0), l, "")));

if (ai != data.end())
    {
    *admin = &(*ai);
    return false;
    }

return true;
}
//-----------------------------------------------------------------------------
bool ADMINS_IMPL::Exists(const std::string & login) const
{
STG_LOCKER lock(&mutex);
if (data.empty())
    {
    printfd(__FILE__, "no admin in system!\n");
    return true;
    }

const_admin_iter ai(find(data.begin(), data.end(), ADMIN_IMPL(PRIV(0), login, "")));

if (ai != data.end())
    return true;

return false;
}
//-----------------------------------------------------------------------------
bool ADMINS_IMPL::Correct(const std::string & login, const std::string & password, ADMIN ** admin)
{
STG_LOCKER lock(&mutex);
if (data.empty())
    {
    printfd(__FILE__, "no admin in system!\n");
    return true;
    }

admin_iter ai(find(data.begin(), data.end(), ADMIN_IMPL(PRIV(0), login, "")));

if (ai == data.end())
    {
    return false;
    }

if (ai->GetPassword() != password)
    {
    return false;
    }

*admin = &(*ai);

return true;
}
//-----------------------------------------------------------------------------
int ADMINS_IMPL::OpenSearch() const
{
STG_LOCKER lock(&mutex);
handle++;
searchDescriptors[handle] = data.begin();
return handle;
}
//-----------------------------------------------------------------------------
int ADMINS_IMPL::SearchNext(int h, ADMIN_CONF * ac) const
{
STG_LOCKER lock(&mutex);
if (searchDescriptors.find(h) == searchDescriptors.end())
    {
    WriteServLog("ADMINS. Incorrect search handle.");
    return -1;
    }

if (searchDescriptors[h] == data.end())
    return -1;

ADMIN_IMPL a = *searchDescriptors[h]++;

*ac = a.GetConf();

return 0;
}
//-----------------------------------------------------------------------------
int ADMINS_IMPL::CloseSearch(int h) const
{
STG_LOCKER lock(&mutex);
if (searchDescriptors.find(h) != searchDescriptors.end())
    {
    searchDescriptors.erase(searchDescriptors.find(h));
    return 0;
    }

WriteServLog("ADMINS. Incorrect search handle.");
return -1;
}
//-----------------------------------------------------------------------------
