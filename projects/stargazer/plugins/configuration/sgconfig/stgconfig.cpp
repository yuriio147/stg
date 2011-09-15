#include <unistd.h>

#include <csignal>
#include <algorithm>

#include "stg/tariffs.h"
#include "stg/admins.h"
#include "stg/users.h"
#include "stg/plugin_creator.h"
#include "stgconfig.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
PLUGIN_CREATOR<STG_CONFIG> stgc;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int STG_CONFIG_SETTINGS::ParseSettings(const MODULE_SETTINGS & s)
{
int p;
PARAM_VALUE pv;
vector<PARAM_VALUE>::const_iterator pvi;
///////////////////////////
pv.param = "Port";
pvi = std::find(s.moduleParams.begin(), s.moduleParams.end(), pv);
if (pvi == s.moduleParams.end())
    {
    errorStr = "Parameter \'Port\' not found.";
    printfd(__FILE__, "Parameter 'Port' not found\n");
    return -1;
    }
if (ParseIntInRange(pvi->value[0], 2, 65535, &p))
    {
    errorStr = "Cannot parse parameter \'Port\': " + errorStr;
    printfd(__FILE__, "%s\n", errorStr.c_str());
    return -1;
    }
port = p;

return 0;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
PLUGIN * GetPlugin()
{
return stgc.GetPlugin();
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const std::string STG_CONFIG::GetVersion() const
{
return "Stg configurator v.0.08";
}
//-----------------------------------------------------------------------------
STG_CONFIG::STG_CONFIG()
    : errorStr(),
      stgConfigSettings(),
      thread(),
      nonstop(false),
      isRunning(false),
      config(),
      users(NULL),
      admins(NULL),
      tariffs(NULL),
      store(NULL),
      settings(),
      stgSettings(NULL)
{
}
//-----------------------------------------------------------------------------
int STG_CONFIG::ParseSettings()
{
int ret = stgConfigSettings.ParseSettings(settings);
if (ret)
    errorStr = stgConfigSettings.GetStrError();
return ret;
}
//-----------------------------------------------------------------------------
int STG_CONFIG::Start()
{
if (isRunning)
    return 0;

nonstop = true;

config.SetPort(stgConfigSettings.GetPort());
config.SetAdmins(admins);
config.SetUsers(users);
config.SetTariffs(tariffs);
config.SetStgSettings(stgSettings);
config.SetStore(store);

if (config.Prepare())
    {
    errorStr = config.GetStrError();
    return -1;
    }

if (pthread_create(&thread, NULL, Run, this))
    {
    errorStr = "Cannot create thread.";
    printfd(__FILE__, "Cannot create thread\n");
    return -1;
    }
errorStr = "";
return 0;
}
//-----------------------------------------------------------------------------
int STG_CONFIG::Stop()
{
if (!isRunning)
    return 0;

config.Stop();

//5 seconds to thread stops itself
int i;
for (i = 0; i < 25; i++)
    {
    if (!isRunning)
        break;

    usleep(200000);
    }

//after 5 seconds waiting thread still running. now killing it
if (isRunning)
    {
    //TODO pthread_cancel()
    if (pthread_kill(thread, SIGINT))
        {
        errorStr = "Cannot kill thread.";
        printfd(__FILE__, "Cannot kill thread\n");
        return -1;
        }
    printfd(__FILE__, "STG_CONFIG killed\n");
    }

return 0;
}
//-----------------------------------------------------------------------------
void * STG_CONFIG::Run(void * d)
{
STG_CONFIG * stgConf = (STG_CONFIG *)d;
stgConf->isRunning = true;

stgConf->config.Run();

stgConf->isRunning = false;
return NULL;
}
//-----------------------------------------------------------------------------
