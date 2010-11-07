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
$Revision: 1.30 $
$Date: 2010/03/04 12:29:06 $
$Author: faust $
*/

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "ao.h"
#include "../../../user.h"
#include "../../../eventloop.h"

class AO_CREATOR
{
private:
    AUTH_AO * ao;

public:
    AO_CREATOR()
        : ao(new AUTH_AO())
        {
        };
    ~AO_CREATOR()
        {
        delete ao;
        };

    AUTH_AO * GetPlugin()
        {
        return ao;
        };
};
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
AO_CREATOR aoc;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// ����� ��� ������ ����� � ������ �������������
template <typename varType>
class IS_CONTAINS_USER: public binary_function<varType, user_iter, bool>
{
public:
    bool operator()(varType notifier, user_iter user) const
        {
        return notifier.GetUser() == user;
        };
};
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BASE_PLUGIN * GetPlugin()
{
return aoc.GetPlugin();
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const string AUTH_AO::GetVersion() const
{
return "Always Online authorizator v.1.0";
}
//-----------------------------------------------------------------------------
AUTH_AO::AUTH_AO()
{
isRunning = false;
}
//-----------------------------------------------------------------------------
void AUTH_AO::SetUsers(USERS * u)
{
users = u;
}
//-----------------------------------------------------------------------------
void AUTH_AO::SetSettings(const MODULE_SETTINGS & s)
{
settings = s;
}
//-----------------------------------------------------------------------------
int AUTH_AO::ParseSettings()
{
return 0;
}
//-----------------------------------------------------------------------------
const string & AUTH_AO::GetStrError() const
{
return errorStr;
}
//-----------------------------------------------------------------------------
int AUTH_AO::Start()
{
GetUsers();

list<user_iter>::iterator users_iter;

onAddUserNotifier.SetAuthorizator(this);
onDelUserNotifier.SetAuthorizator(this);
users->AddNotifierUserAdd(&onAddUserNotifier);
users->AddNotifierUserDel(&onDelUserNotifier);

users_iter = usersList.begin();
while (users_iter != usersList.end())
    {
    UpdateUserAuthorization(*users_iter);
    ++users_iter;
    }
isRunning = true;
return 0;
}
//-----------------------------------------------------------------------------
int AUTH_AO::Stop()
{
if (!isRunning)
    return 0;

users->DelNotifierUserAdd(&onAddUserNotifier);
users->DelNotifierUserDel(&onDelUserNotifier);

list<user_iter>::iterator users_iter;
users_iter = usersList.begin();
while (users_iter != usersList.end())
    {
    Unauthorize(*users_iter);
    UnSetUserNotifiers(*users_iter);
    ++users_iter;
    }
isRunning = false;
return 0;
}
//-----------------------------------------------------------------------------
bool AUTH_AO::IsRunning()
{
return isRunning;
}
//-----------------------------------------------------------------------------
uint16_t AUTH_AO::GetStartPosition() const
{
return 70;
}
//-----------------------------------------------------------------------------
uint16_t AUTH_AO::GetStopPosition() const
{
return 70;
}
//-----------------------------------------------------------------------------
void AUTH_AO::SetUserNotifiers(user_iter u)
{
// ---------- AlwaysOnline -------------------
CHG_BEFORE_NOTIFIER<int> BeforeChgAONotifier;
CHG_AFTER_NOTIFIER<int>  AfterChgAONotifier;

BeforeChgAONotifier.SetAuthorizator(this);
BeforeChgAONotifier.SetUser(u);
BeforeChgAONotifierList.push_front(BeforeChgAONotifier);

AfterChgAONotifier.SetAuthorizator(this);
AfterChgAONotifier.SetUser(u);
AfterChgAONotifierList.push_front(AfterChgAONotifier);

u->property.alwaysOnline.AddBeforeNotifier(&(*BeforeChgAONotifierList.begin()));
u->property.alwaysOnline.AddAfterNotifier(&(*AfterChgAONotifierList.begin()));
// ---------- AlwaysOnline end ---------------

// ---------- IP -------------------
CHG_BEFORE_NOTIFIER<USER_IPS> BeforeChgIPNotifier;
CHG_AFTER_NOTIFIER<USER_IPS>  AfterChgIPNotifier;

BeforeChgIPNotifier.SetAuthorizator(this);
BeforeChgIPNotifier.SetUser(u);
BeforeChgIPNotifierList.push_front(BeforeChgIPNotifier);

AfterChgIPNotifier.SetAuthorizator(this);
AfterChgIPNotifier.SetUser(u);
AfterChgIPNotifierList.push_front(AfterChgIPNotifier);

u->property.ips.AddBeforeNotifier(&(*BeforeChgIPNotifierList.begin()));
u->property.ips.AddAfterNotifier(&(*AfterChgIPNotifierList.begin()));
// ---------- IP end ---------------
}
//-----------------------------------------------------------------------------
void AUTH_AO::UnSetUserNotifiers(user_iter u)
{
// ---      AlwaysOnline        ---
IS_CONTAINS_USER<CHG_BEFORE_NOTIFIER<int> > IsContainsUserAOB;
IS_CONTAINS_USER<CHG_AFTER_NOTIFIER<int> > IsContainsUserAOA;

list<CHG_BEFORE_NOTIFIER<int> >::iterator aoBIter;
list<CHG_AFTER_NOTIFIER<int> >::iterator  aoAIter;

aoBIter = find_if(BeforeChgAONotifierList.begin(),
                  BeforeChgAONotifierList.end(),
                  bind2nd(IsContainsUserAOB, u));

if (aoBIter != BeforeChgAONotifierList.end())
    {
    aoBIter->GetUser()->property.alwaysOnline.DelBeforeNotifier(&(*aoBIter));
    BeforeChgAONotifierList.erase(aoBIter);
    }

aoAIter = find_if(AfterChgAONotifierList.begin(),
                  AfterChgAONotifierList.end(),
                  bind2nd(IsContainsUserAOA, u));

if (aoAIter != AfterChgAONotifierList.end())
    {
    aoAIter->GetUser()->property.alwaysOnline.DelAfterNotifier(&(*aoAIter));
    AfterChgAONotifierList.erase(aoAIter);
    }
// ---      AlwaysOnline end    ---

// ---          IP              ---
IS_CONTAINS_USER<CHG_BEFORE_NOTIFIER<USER_IPS> > IsContainsUserIPB;
IS_CONTAINS_USER<CHG_AFTER_NOTIFIER<USER_IPS> >  IsContainsUserIPA;

list<CHG_BEFORE_NOTIFIER<USER_IPS> >::iterator ipBIter;
list<CHG_AFTER_NOTIFIER<USER_IPS> >::iterator  ipAIter;

ipBIter = find_if(BeforeChgIPNotifierList.begin(),
                  BeforeChgIPNotifierList.end(),
                  bind2nd(IsContainsUserIPB, u));

if (ipBIter != BeforeChgIPNotifierList.end())
    {
    ipBIter->GetUser()->property.ips.DelBeforeNotifier(&(*ipBIter));
    BeforeChgIPNotifierList.erase(ipBIter);
    }

ipAIter = find_if(AfterChgIPNotifierList.begin(),
                  AfterChgIPNotifierList.end(),
                  bind2nd(IsContainsUserIPA, u));

if (ipAIter != AfterChgIPNotifierList.end())
    {
    ipAIter->GetUser()->property.ips.DelAfterNotifier(&(*ipAIter));
    AfterChgIPNotifierList.erase(ipAIter);
    }
// ---          IP end          ---
}
//-----------------------------------------------------------------------------
void AUTH_AO::GetUsers()
{
user_iter u;
int h = users->OpenSearch();
if (!h)
    {
    printfd(__FILE__, "users->OpenSearch() error\n");
    return;
    }

while (1)
    {
    if (users->SearchNext(h, &u))
        {
        break;
        }
    usersList.push_back(u);
    SetUserNotifiers(u);
    }

users->CloseSearch(h);
}
//-----------------------------------------------------------------------------
void AUTH_AO::Unauthorize(user_iter u) const
{
u->Unauthorize(this);
}
//-----------------------------------------------------------------------------
void AUTH_AO::UpdateUserAuthorization(user_iter u) const
{
if (u->property.alwaysOnline)
    {
    USER_IPS ips = u->property.ips;
    if (ips.OnlyOneIP())
        {
        if (u->Authorize(ips[0].ip, "", 0xFFffFFff, this) == 0)
            {
            }
        }
    }
}
//-----------------------------------------------------------------------------
void AUTH_AO::AddUser(user_iter u)
{
SetUserNotifiers(u);
usersList.push_back(u);
UpdateUserAuthorization(u);
}
//-----------------------------------------------------------------------------
void AUTH_AO::DelUser(user_iter u)
{
Unauthorize(u);
UnSetUserNotifiers(u);

list<user_iter>::iterator users_iter;
users_iter = usersList.begin();

while (users_iter != usersList.end())
    {
    if (u == *users_iter)
        {
        usersList.erase(users_iter);
        break;
        }
    ++users_iter;
    }
}
//-----------------------------------------------------------------------------
int AUTH_AO::SendMessage(const STG_MSG &, uint32_t) const
{
errorStr = "Authorization modele \'AlwaysOnline\' does not support sending messages";
return -1;
}
//-----------------------------------------------------------------------------
template <typename varParamType>
void CHG_BEFORE_NOTIFIER<varParamType>::Notify(const varParamType &, const varParamType &)
{
EVENT_LOOP_SINGLETON::GetInstance().Enqueue(*auth, &AUTH_AO::Unauthorize, user);
}
//-----------------------------------------------------------------------------
template <typename varParamType>
void CHG_AFTER_NOTIFIER<varParamType>::Notify(const varParamType &, const varParamType &)
{
EVENT_LOOP_SINGLETON::GetInstance().Enqueue(*auth, &AUTH_AO::UpdateUserAuthorization, user);
}
//-----------------------------------------------------------------------------
