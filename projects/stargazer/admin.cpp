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
 $Revision: 1.13 $
 $Date: 2010/10/04 20:16:09 $
 $Author: faust $
 */

#include "admin.h"
#include "common.h"

//-----------------------------------------------------------------------------
ADMIN::ADMIN()
    : conf(),
      ip(0),
      WriteServLog(GetStgLogger())
{
}
//-----------------------------------------------------------------------------
ADMIN::ADMIN(const ADMIN_CONF & ac)
    : conf(ac),
      ip(0),
      WriteServLog(GetStgLogger())
{
}
//-----------------------------------------------------------------------------
ADMIN::ADMIN(const PRIV & priv, const std::string & login, const std::string & password)
    : conf(priv, login, password),
      ip(0),
      WriteServLog(GetStgLogger())
{
}
//-----------------------------------------------------------------------------
ADMIN & ADMIN::operator=(const ADMIN & adm)
{
if (&adm == this)
    return *this;

conf = adm.conf;
ip = adm.ip;
return *this;
}
//-----------------------------------------------------------------------------
ADMIN & ADMIN::operator=(const ADMIN_CONF & ac)
{
conf = ac;
return *this;
}
//-----------------------------------------------------------------------------
bool ADMIN::operator==(const ADMIN & rhs) const
{
return conf.login == rhs.GetLogin();
}
//-----------------------------------------------------------------------------
bool ADMIN::operator!=(const ADMIN & rhs) const
{
return conf.login != rhs.GetLogin();
}
//-----------------------------------------------------------------------------
bool ADMIN::operator<(const ADMIN & rhs) const
{
return conf.login < rhs.GetLogin();
}
//-----------------------------------------------------------------------------
bool ADMIN::operator<=(const ADMIN & rhs) const
{
return conf.login <= rhs.GetLogin();
}
//-----------------------------------------------------------------------------
string ADMIN::GetAdminIPStr() const
{
return inet_ntostring(ip);
}
//-----------------------------------------------------------------------------
void ADMIN::PrintAdmin() const
{
printfd(__FILE__, "=======================================\n");
printfd(__FILE__, "login %s\n",     conf.login.c_str());
printfd(__FILE__, "password %s\n",  conf.password.c_str());
printfd(__FILE__, "ChgConf %d\n",   conf.priv.userConf);
printfd(__FILE__, "ChgStat %d\n",   conf.priv.userStat);
printfd(__FILE__, "ChgCash %d\n",   conf.priv.userCash);
printfd(__FILE__, "UsrAddDel %d\n", conf.priv.userAddDel);
printfd(__FILE__, "ChgAdmin %d\n",  conf.priv.adminChg);
printfd(__FILE__, "ChgTariff %d\n", conf.priv.tariffChg);
printfd(__FILE__, "=======================================\n");
}
//-----------------------------------------------------------------------------
const string ADMIN::GetLogStr() const
{
return "Admin \'" + conf.login + "\', " + GetAdminIPStr() + ":";
}
//-----------------------------------------------------------------------------