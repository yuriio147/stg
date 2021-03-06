#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <cstring>
#include <cerrno>
#include <ctime>
#include <csignal>
#include <cassert>

#include <vector>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <utility>

#include "stg/common.h"

#include "smux.h"
#include "utils.h"

namespace
{

bool SPrefixLess(const Sensors::value_type & a,
                 const Sensors::value_type & b)
{
return a.first.PrefixLess(b.first);
}

}

extern "C" STG::Plugin* GetPlugin()
{
    static SMUX plugin;
    return &plugin;
}

SMUX_SETTINGS::SMUX_SETTINGS()
    : errorStr(),
      ip(0),
      port(0),
      password()
{}

int SMUX_SETTINGS::ParseSettings(const STG::ModuleSettings & s)
{
STG::ParamValue pv;
std::vector<STG::ParamValue>::const_iterator pvi;
int p;

pv.param = "Port";
pvi = std::find(s.moduleParams.begin(), s.moduleParams.end(), pv);
if (pvi == s.moduleParams.end() || pvi->value.empty())
    {
    errorStr = "Parameter \'Port\' not found.";
    printfd(__FILE__, "Parameter 'Port' not found\n");
    return -1;
    }
if (ParseIntInRange(pvi->value[0], 2, 65535, &p))
    {
    errorStr = "Cannot parse parameter \'Port\': " + errorStr;
    printfd(__FILE__, "Cannot parse parameter 'Port'\n");
    return -1;
    }
port = static_cast<uint16_t>(p);

pv.param = "Password";
pvi = std::find(s.moduleParams.begin(), s.moduleParams.end(), pv);
if (pvi == s.moduleParams.end() || pvi->value.empty())
    {
    errorStr = "Parameter \'Password\' not found.";
    printfd(__FILE__, "Parameter 'Password' not found\n");
    password = "";
    }
else
    {
    password = pvi->value[0];
    }

pv.param = "Server";
pvi = std::find(s.moduleParams.begin(), s.moduleParams.end(), pv);
if (pvi == s.moduleParams.end() || pvi->value.empty())
    {
    errorStr = "Parameter \'Server\' not found.";
    printfd(__FILE__, "Parameter 'Server' not found\n");
    return -1;
    }
ip = inet_strington(pvi->value[0]);

return 0;
}

SMUX::SMUX()
    : users(NULL),
      tariffs(NULL),
      admins(NULL),
      services(NULL),
      corporations(NULL),
      traffcounter(NULL),
      running(false),
      stopped(true),
      needReconnect(false),
      lastReconnectTry(0),
      reconnectTimeout(1),
      sock(-1),
      addUserNotifier(*this),
      delUserNotifier(*this),
      addDelTariffNotifier(*this),
      logger(STG::PluginLogger::get("smux"))
{
pthread_mutex_init(&mutex, NULL);

smuxHandlers[SMUX_PDUs_PR_close] = &SMUX::CloseHandler;
smuxHandlers[SMUX_PDUs_PR_registerResponse] = &SMUX::RegisterResponseHandler;
smuxHandlers[SMUX_PDUs_PR_pdus] = &SMUX::PDUsRequestHandler;
smuxHandlers[SMUX_PDUs_PR_commitOrRollback] = &SMUX::CommitOrRollbackHandler;

pdusHandlers[PDUs_PR_get_request] = &SMUX::GetRequestHandler;
pdusHandlers[PDUs_PR_get_next_request] = &SMUX::GetNextRequestHandler;
pdusHandlers[PDUs_PR_set_request] = &SMUX::SetRequestHandler;
}

SMUX::~SMUX()
{
    {
    Sensors::iterator it;
    for (it = sensors.begin(); it != sensors.end(); ++it)
        delete it->second;
    }
    {
    Tables::iterator it;
    for (it = tables.begin(); it != tables.end(); ++it)
        delete it->second;
    }
printfd(__FILE__, "SMUX::~SMUX()\n");
pthread_mutex_destroy(&mutex);
}

int SMUX::ParseSettings()
{
return smuxSettings.ParseSettings(settings);
}

int SMUX::Start()
{
assert(users != NULL && "users must not be NULL");
assert(tariffs != NULL && "tariffs must not be NULL");
assert(admins != NULL && "admins must not be NULL");
assert(services != NULL && "services must not be NULL");
assert(corporations != NULL && "corporations must not be NULL");
assert(traffcounter != NULL && "traffcounter must not be NULL");

if (PrepareNet())
    needReconnect = true;

// Users
sensors[OID(".1.3.6.1.4.1.38313.1.1.1")] = new TotalUsersSensor(*users);
sensors[OID(".1.3.6.1.4.1.38313.1.1.2")] = new ConnectedUsersSensor(*users);
sensors[OID(".1.3.6.1.4.1.38313.1.1.3")] = new AuthorizedUsersSensor(*users);
sensors[OID(".1.3.6.1.4.1.38313.1.1.4")] = new AlwaysOnlineUsersSensor(*users);
sensors[OID(".1.3.6.1.4.1.38313.1.1.5")] = new NoCashUsersSensor(*users);
sensors[OID(".1.3.6.1.4.1.38313.1.1.6")] = new DisabledDetailStatsUsersSensor(*users);
sensors[OID(".1.3.6.1.4.1.38313.1.1.7")] = new DisabledUsersSensor(*users);
sensors[OID(".1.3.6.1.4.1.38313.1.1.8")] = new PassiveUsersSensor(*users);
sensors[OID(".1.3.6.1.4.1.38313.1.1.9")] = new CreditUsersSensor(*users);
sensors[OID(".1.3.6.1.4.1.38313.1.1.10")] = new FreeMbUsersSensor(*users);
sensors[OID(".1.3.6.1.4.1.38313.1.1.11")] = new TariffChangeUsersSensor(*users);
sensors[OID(".1.3.6.1.4.1.38313.1.1.12")] = new ActiveUsersSensor(*users);
// Tariffs
sensors[OID(".1.3.6.1.4.1.38313.1.2.1")] = new TotalTariffsSensor(*tariffs);
// Admins
sensors[OID(".1.3.6.1.4.1.38313.1.3.1")] = new TotalAdminsSensor(*admins);
// Services
sensors[OID(".1.3.6.1.4.1.38313.1.4.1")] = new TotalServicesSensor(*services);
// Corporations
sensors[OID(".1.3.6.1.4.1.38313.1.5.1")] = new TotalCorporationsSensor(*corporations);
// Traffcounter
sensors[OID(".1.3.6.1.4.1.38313.1.6.1")] = new TotalRulesSensor(*traffcounter);

// Table data
tables[".1.3.6.1.4.1.38313.1.2.2"] = new TariffUsersTable(".1.3.6.1.4.1.38313.1.2.2", *tariffs, *users);

UpdateTables();
SetNotifiers();

#ifdef SMUX_DEBUG
Sensors::const_iterator it(sensors.begin());
while (it != sensors.end())
    {
    printfd(__FILE__, "%s = %s\n",
            it->first.ToString().c_str(),
            it->second->ToString().c_str());
    ++it;
    }
#endif

if (!running)
    {
    if (pthread_create(&thread, NULL, Runner, this))
        {
        errorStr = "Cannot create thread.";
	logger("Cannot create thread.");
        printfd(__FILE__, "Cannot create thread\n");
        return -1;
        }
    }

return 0;
}

int SMUX::Stop()
{
printfd(__FILE__, "SMUX::Stop() - Before\n");
running = false;

if (!stopped)
    {
    //5 seconds to thread stops itself
    for (int i = 0; i < 25 && !stopped; i++)
        {
        struct timespec ts = {0, 200000000};
        nanosleep(&ts, NULL);
        }
    }

if (stopped)
    pthread_join(thread, NULL);

ResetNotifiers();

    {
    Tables::iterator it;
    for (it = tables.begin(); it != tables.end(); ++it)
        delete it->second;
    }
    {
    Sensors::iterator it;
    for (it = sensors.begin(); it != sensors.end(); ++it)
        delete it->second;
    }

tables.erase(tables.begin(), tables.end());
sensors.erase(sensors.begin(), sensors.end());

close(sock);

if (!stopped)
    {
    running = true;
    return -1;
    }

printfd(__FILE__, "SMUX::Stop() - After\n");
return 0;
}

int SMUX::Reload(const STG::ModuleSettings & /*ms*/)
{
if (Stop())
    return -1;
if (Start())
    return -1;
if (!needReconnect)
    {
    printfd(__FILE__, "SMUX reconnected succesfully.\n");
    logger("Reconnected successfully.");
    }
return 0;
}

void * SMUX::Runner(void * d)
{
sigset_t signalSet;
sigfillset(&signalSet);
pthread_sigmask(SIG_BLOCK, &signalSet, NULL);

SMUX * smux = static_cast<SMUX *>(d);

smux->Run();

return NULL;
}

void SMUX::Run()
{
stopped = true;
if (!SendOpenPDU(sock))
    needReconnect = true;
if (!SendRReqPDU(sock))
    needReconnect = true;
running = true;
stopped = false;

while(running)
    {
    if (WaitPackets(sock) && !needReconnect)
        {
        SMUX_PDUs_t * pdus = RecvSMUXPDUs(sock);
        if (pdus)
            {
            DispatchPDUs(pdus);
            ASN_STRUCT_FREE(asn_DEF_SMUX_PDUs, pdus);
            }
        else if (running)
            Reconnect();
        }
    else if (running && needReconnect)
        Reconnect();
    if (!running)
        break;
    }
SendClosePDU(sock);
stopped = true;
}

bool SMUX::PrepareNet()
{
sock = socket(AF_INET, SOCK_STREAM, 0);

if (sock < 0)
    {
    errorStr = "Cannot create socket.";
    logger("Cannot create a socket: %s", strerror(errno));
    printfd(__FILE__, "Cannot create socket\n");
    return true;
    }

struct sockaddr_in addr;

addr.sin_family = AF_INET;
addr.sin_port = htons(smuxSettings.GetPort());
addr.sin_addr.s_addr = smuxSettings.GetIP();

if (connect(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)))
    {
    errorStr = "Cannot connect.";
    logger("Cannot connect the socket: %s", strerror(errno));
    printfd(__FILE__, "Cannot connect. Message: '%s'\n", strerror(errno));
    return true;
    }

return false;
}

bool SMUX::Reconnect()
{
if (needReconnect && difftime(time(NULL), lastReconnectTry) < reconnectTimeout)
    return true;

time(&lastReconnectTry);
SendClosePDU(sock);
close(sock);
if (!PrepareNet())
    if (SendOpenPDU(sock))
        if (SendRReqPDU(sock))
            {
            reconnectTimeout = 1;
            needReconnect = false;
            logger("Connected successfully");
            printfd(__FILE__, "Connected successfully\n");
            return false;
            }

if (needReconnect)
    if (reconnectTimeout < 60)
        reconnectTimeout *= 2;

needReconnect = true;
return true;
}

bool SMUX::DispatchPDUs(const SMUX_PDUs_t * pdus)
{
SMUXHandlers::iterator it(smuxHandlers.find(pdus->present));
if (it != smuxHandlers.end())
    {
    return (this->*(it->second))(pdus);
    }
#ifdef SMUX_DEBUG
else
    {
    switch (pdus->present)
        {
        case SMUX_PDUs_PR_NOTHING:
            printfd(__FILE__, "PDUs: nothing\n");
            break;
        case SMUX_PDUs_PR_open:
            printfd(__FILE__, "PDUs: open\n");
            break;
        case SMUX_PDUs_PR_registerRequest:
            printfd(__FILE__, "PDUs: registerRequest\n");
            break;
        default:
            printfd(__FILE__, "PDUs: undefined\n");
        }
    asn_fprint(stderr, &asn_DEF_SMUX_PDUs, pdus);
    }
#endif
return false;
}

bool SMUX::UpdateTables()
{
Sensors newSensors;
bool done = true;
Tables::iterator it(tables.begin());
while (it != tables.end())
    {
    try
        {
        it->second->UpdateSensors(newSensors);
        }
    catch (const std::runtime_error & ex)
        {
        printfd(__FILE__,
                "SMUX::UpdateTables - failed to update table '%s': '%s'\n",
                it->first.c_str(), ex.what());
        done = false;
        break;
        }
    ++it;
    }
if (!done)
    {
    Sensors::iterator it(newSensors.begin());
    while (it != newSensors.end())
        {
        delete it->second;
        ++it;
        }
    return false;
    }

it = tables.begin();
while (it != tables.end())
    {
    std::pair<Sensors::iterator, Sensors::iterator> res;
    res = std::equal_range(sensors.begin(),
                           sensors.end(),
                           std::pair<OID, Sensor *>(OID(it->first), NULL),
                           SPrefixLess);
    Sensors::iterator sit(res.first);
    while (sit != res.second)
        {
        delete sit->second;
        ++sit;
        }
    sensors.erase(res.first, res.second);
    ++it;
    }

sensors.insert(newSensors.begin(), newSensors.end());

return true;
}

void SMUX::SetNotifier(UserPtr userPtr)
{
notifiers.push_back(CHG_AFTER_NOTIFIER(*this, userPtr));
userPtr->GetProperties().tariffName.AddAfterNotifier(&notifiers.back());
}

void SMUX::UnsetNotifier(UserPtr userPtr)
{
std::list<CHG_AFTER_NOTIFIER>::iterator it = notifiers.begin();
while (it != notifiers.end())
    {
    if (it->GetUserPtr() == userPtr)
        {
        userPtr->GetProperties().tariffName.DelAfterNotifier(&(*it));
        notifiers.erase(it);
        break;
        }
    ++it;
    }
}

void SMUX::SetNotifiers()
{
int h = users->OpenSearch();
assert(h && "USERS::OpenSearch is always correct");

UserPtr u;
while (!users->SearchNext(h, &u))
    SetNotifier(u);

users->CloseSearch(h);

users->AddNotifierUserAdd(&addUserNotifier);
users->AddNotifierUserDel(&delUserNotifier);

tariffs->AddNotifierAdd(&addDelTariffNotifier);
tariffs->AddNotifierDel(&addDelTariffNotifier);
}

void SMUX::ResetNotifiers()
{
tariffs->DelNotifierDel(&addDelTariffNotifier);
tariffs->DelNotifierAdd(&addDelTariffNotifier);

users->DelNotifierUserDel(&delUserNotifier);
users->DelNotifierUserAdd(&addUserNotifier);

std::list<CHG_AFTER_NOTIFIER>::iterator it(notifiers.begin());
while (it != notifiers.end())
    {
    it->GetUserPtr()->GetProperties().tariffName.DelAfterNotifier(&(*it));
    ++it;
    }
notifiers.clear();
}
