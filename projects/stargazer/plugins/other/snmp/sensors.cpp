#include "asn1/INTEGER.h"

#include "stg/user.h"
#include "stg/user_property.h"

#include "sensors.h"

void Int2OS(ObjectSyntax_t * dst, long src)
{
dst->present = ObjectSyntax_PR_simple;
SimpleSyntax_t * simpleSyntax = &dst->choice.simple;
simpleSyntax->present = SimpleSyntax_PR_number;
asn_long2INTEGER(&simpleSyntax->choice.number, src);
}

bool ConnectedUsersSensor::GetValue(ObjectSyntax_t * objectSyntax)
{
int handle = users.OpenSearch();
if (!handle)
    return false;

USER_PTR user;
size_t count = 0;
while (!users.SearchNext(handle, &user))
    {
    if (user->GetConnected())
        ++count;
    }

users.CloseSearch(handle);

Int2OS(objectSyntax, count);
return true;
}

bool AuthorizedUsersSensor::GetValue(ObjectSyntax_t * objectSyntax)
{
int handle = users.OpenSearch();
if (!handle)
    return false;

USER_PTR user;
size_t count = 0;
while (!users.SearchNext(handle, &user))
    {
    if (user->GetAuthorized())
        ++count;
    }

users.CloseSearch(handle);

Int2OS(objectSyntax, count);
return true;
}

bool AlwaysOnlineUsersSensor::GetValue(ObjectSyntax_t * objectSyntax)
{
int handle = users.OpenSearch();
if (!handle)
    return false;

USER_PTR user;
size_t count = 0;
while (!users.SearchNext(handle, &user))
    {
    if (user->GetProperty().alwaysOnline)
        ++count;
    }

users.CloseSearch(handle);

Int2OS(objectSyntax, count);
return true;
}

bool NoCashUsersSensor::GetValue(ObjectSyntax_t * objectSyntax)
{
int handle = users.OpenSearch();
if (!handle)
    return false;

USER_PTR user;
size_t count = 0;
while (!users.SearchNext(handle, &user))
    {
    if (user->GetProperty().cash < 0)
        ++count;
    }

users.CloseSearch(handle);

Int2OS(objectSyntax, count);
return true;
}

bool DisabledDetailStatsUsersSensor::GetValue(ObjectSyntax_t * objectSyntax)
{
int handle = users.OpenSearch();
if (!handle)
    return false;

USER_PTR user;
size_t count = 0;
while (!users.SearchNext(handle, &user))
    {
    if (user->GetProperty().disabledDetailStat)
        ++count;
    }

users.CloseSearch(handle);

Int2OS(objectSyntax, count);
return true;
}

bool DisabledUsersSensor::GetValue(ObjectSyntax_t * objectSyntax)
{
int handle = users.OpenSearch();
if (!handle)
    return false;

USER_PTR user;
size_t count = 0;
while (!users.SearchNext(handle, &user))
    {
    if (user->GetProperty().disabled)
        ++count;
    }

users.CloseSearch(handle);

Int2OS(objectSyntax, count);
return true;
}

bool PassiveUsersSensor::GetValue(ObjectSyntax_t * objectSyntax)
{
int handle = users.OpenSearch();
if (!handle)
    return false;

USER_PTR user;
size_t count = 0;
while (!users.SearchNext(handle, &user))
    {
    if (user->GetProperty().passive)
        ++count;
    }

users.CloseSearch(handle);

Int2OS(objectSyntax, count);
return true;
}

bool CreditUsersSensor::GetValue(ObjectSyntax_t * objectSyntax)
{
int handle = users.OpenSearch();
if (!handle)
    return false;

USER_PTR user;
size_t count = 0;
while (!users.SearchNext(handle, &user))
    {
    if (user->GetProperty().credit > 0)
        ++count;
    }

users.CloseSearch(handle);

Int2OS(objectSyntax, count);
return true;
}

bool FreeMbUsersSensor::GetValue(ObjectSyntax_t * objectSyntax)
{
int handle = users.OpenSearch();
if (!handle)
    return false;

USER_PTR user;
size_t count = 0;
while (!users.SearchNext(handle, &user))
    {
    if (user->GetProperty().freeMb > 0)
        ++count;
    }

users.CloseSearch(handle);

Int2OS(objectSyntax, count);
return true;
}