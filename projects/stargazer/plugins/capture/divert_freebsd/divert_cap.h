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
 Author : Boris Mikhailenko <stg34@stg.dp.ua>
*/

/*
$Revision: 1.6 $
$Date: 2009/06/23 11:32:27 $
*/

#ifndef DIVERT_CAP_H
#define DIVERT_CAP_H

#include <string>
#include <pthread.h>

#include "base_plugin.h"
#include "base_settings.h"
#include "../../../traffcounter.h"

using namespace std;

extern "C" BASE_PLUGIN * GetPlugin();

//-----------------------------------------------------------------------------
class DIVERT_CAP :public BASE_PLUGIN
{
public:
    DIVERT_CAP();
    virtual ~DIVERT_CAP(){};

    void                SetUsers(USERS *){};
    void                SetTariffs(TARIFFS *){};
    void                SetAdmins(ADMINS *){};
    void                SetTraffcounter(TRAFFCOUNTER * tc);
    void                SetStore(BASE_STORE *){};
    void                SetStgSettings(const SETTINGS *){};

    int                 Start();
    int                 Stop();
    int                 Reload() { return 0; };
    bool                IsRunning();

    void                SetSettings(const MODULE_SETTINGS & s);
    int                 ParseSettings();
    const string      & GetStrError() const;
    const string        GetVersion() const;
    uint16_t            GetStartPosition() const;
    uint16_t            GetStopPosition() const;

private:
    static void *       Run(void *);

    int                 DivertCapOpen();
    int                 DivertCapOpen(int n);
    int                 DivertCapRead(char * buffer, int blen, char ** iface);
    int                 DivertCapRead(char * buffer, int blen, char ** iface, int n);
    int                 DivertCapClose();

    int                 ParseIntInRange(const string & str, int min, int max, int * val);

    MODULE_SETTINGS     settings;

    int                 port;

    mutable string      errorStr;

    pthread_t           thread;

    bool                nonstop;
    bool                isRunning;

    //int                 capSock;

    TRAFFCOUNTER *      traffCnt;
};
//-----------------------------------------------------------------------------


#endif