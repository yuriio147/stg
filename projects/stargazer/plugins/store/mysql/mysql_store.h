 /*
 $Revision: 1.4 $
 $Date: 2010/01/19 11:07:57 $
 */


#ifndef FILE_STORE_H
#define FILE_STORE_H

#include <string>

#include "base_settings.h"
#include "base_store.h"
#include "user_traff.h"
#include <mysql/mysql.h>

using namespace std;
//-----------------------------------------------------------------------------
extern "C" BASE_STORE * GetStore();
//-----------------------------------------------------------------------------
class MYSQL_STORE_SETTINGS//: public BASE_SETTINGS
{
public:
    MYSQL_STORE_SETTINGS();
    virtual ~MYSQL_STORE_SETTINGS();
    virtual int ParseSettings(const MODULE_SETTINGS & s);
    virtual const string & GetStrError() const;

    string  GetDBUser() const;
    string  GetDBPassword() const;
    string  GetDBHost() const;
    string  GetDBName() const;

private:
    const MODULE_SETTINGS * settings;

    int     ParseParam(const vector<PARAM_VALUE> & moduleParams, 
			const string & name, string & result);

	string  errorStr;

    string  dbUser;
    string  dbPass;
	string  dbName;
    string  dbHost;
};
//-----------------------------------------------------------------------------
class MYSQL_STORE: public BASE_STORE
{
public:
    MYSQL_STORE();
    virtual ~MYSQL_STORE();
    virtual const string & GetStrError() const;

    //User
    virtual int GetUsersList(vector<string> * usersList) const;
    virtual int AddUser(const string & login) const;
    virtual int DelUser(const string & login) const;
    virtual int SaveUserStat(const USER_STAT & stat, const string & login) const;
    virtual int SaveUserConf(const USER_CONF & conf, const string & login) const;
    virtual int RestoreUserStat(USER_STAT * stat, const string & login) const;
    virtual int RestoreUserConf(USER_CONF * conf, const string & login) const;
    virtual int WriteUserChgLog(const string & login,
                                const string & admLogin,
                                uint32_t       admIP,
                                const string & paramName,
                                const string & oldValue,
                                const string & newValue,
                                const string & message = "") const;
    virtual int WriteUserConnect(const string & login, uint32_t ip) const;
    virtual int WriteUserDisconnect(const string & login,
                                    const DIR_TRAFF & up,
                                    const DIR_TRAFF & down,
                                    const DIR_TRAFF & sessionUp,
                                    const DIR_TRAFF & sessionDown,
                                    double cash,
                                    double freeMb,
                                    const std::string & reason) const;

    virtual int WriteDetailedStat(const map<IP_DIR_PAIR, STAT_NODE> * statTree,
                                  time_t lastStat,
                                  const string & login) const;

    virtual int AddMessage(STG_MSG * msg, const string & login) const;
    virtual int EditMessage(const STG_MSG & msg, const string & login) const;
    virtual int GetMessage(uint64_t id, STG_MSG * msg, const string & login) const;
    virtual int DelMessage(uint64_t id, const string & login) const;
    virtual int GetMessageHdrs(vector<STG_MSG_HDR> * hdrsList, const string & login) const;

    virtual int SaveMonthStat(const USER_STAT & stat, int month, int year, const string & login) const;

    //Admin
    virtual int GetAdminsList(vector<string> * adminsList) const;
    virtual int AddAdmin(const string & login) const;
    virtual int DelAdmin(const string & login) const;
    virtual int RestoreAdmin(ADMIN_CONF * ac, const string & login) const;
    virtual int SaveAdmin(const ADMIN_CONF & ac) const;

    //Tariff
    virtual int GetTariffsList(vector<string> * tariffsList) const;
    virtual int AddTariff(const string & name) const;
    virtual int DelTariff(const string & name) const;
    virtual int SaveTariff(const TARIFF_DATA & td, const string & tariffName) const;
    virtual int RestoreTariff(TARIFF_DATA * td, const string & tariffName) const;

    //Corparation
    virtual int GetCorpsList(vector<string> *) const {return 0;};
    virtual int SaveCorp(const CORP_CONF &) const {return 0;};
    virtual int RestoreCorp(CORP_CONF *, const string &) const {return 0;};
    virtual int AddCorp(const string &) const {return 0;};
    virtual int DelCorp(const string &) const {return 0;};

    // Services
    virtual int GetServicesList(vector<string> *) const {return 0;};
    virtual int SaveService(const SERVICE_CONF &) const {return 0;};
    virtual int RestoreService(SERVICE_CONF *, const string &) const {return 0;};
    virtual int AddService(const string &) const {return 0;};
    virtual int DelService(const string &) const {return 0;};

    //virtual BASE_SETTINGS * GetStoreSettings();
    virtual void            SetSettings(const MODULE_SETTINGS & s);
    virtual int             ParseSettings();
    virtual const string &  GetVersion() const;

private:
    virtual int WriteLogString(const string & str, const string & login) const;
	int GetAllParams(vector<string> * ParamList, const string & table, const string & name) const;
	int CheckAllTables(MYSQL * sock);
	bool IsTablePresent(const string & str,MYSQL * sock);
    mutable string          errorStr;
//    int						Reconnect();
    int						MysqlQuery(const char* sQuery,MYSQL * sock) const;
    int                     MysqlGetQuery(const char * Query,MYSQL * & sock) const;
    int                     MysqlSetQuery(const char * Query) const;
    MYSQL  *                MysqlConnect() const ;
    string                  version;
    MYSQL_STORE_SETTINGS    storeSettings;
    MODULE_SETTINGS         settings;
	//mutable MYSQL					mysql;
	//mutable MYSQL*					sock;
};
//-----------------------------------------------------------------------------

#endif //FILE_STORE_H