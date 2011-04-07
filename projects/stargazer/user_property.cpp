#include "stg/user_property.h"

//-----------------------------------------------------------------------------
USER_PROPERTIES::USER_PROPERTIES(const std::string & sd)
:
cash            (stat.cash,             "cash",             false, true, GetStgLogger(), sd),
up              (stat.up,               "upload",           false, true, GetStgLogger(), sd),
down            (stat.down,             "download",         false, true, GetStgLogger(), sd),
lastCashAdd     (stat.lastCashAdd,      "lastCashAdd",      false, true, GetStgLogger(), sd),
passiveTime     (stat.passiveTime,      "passiveTime",      false, true, GetStgLogger(), sd),
lastCashAddTime (stat.lastCashAddTime,  "lastCashAddTime",  false, true, GetStgLogger(), sd),
freeMb          (stat.freeMb,           "freeMb",           false, true, GetStgLogger(), sd),
lastActivityTime(stat.lastActivityTime, "lastActivityTime", false, true, GetStgLogger(), sd),


password    (conf.password,     "password",     true,  false, GetStgLogger(), sd),
passive     (conf.passive,      "passive",      false, false, GetStgLogger(), sd),
disabled    (conf.disabled,     "disabled",     false, false, GetStgLogger(), sd),
disabledDetailStat(conf.disabledDetailStat, "DisabledDetailStat", false, false, GetStgLogger(), sd),
alwaysOnline(conf.alwaysOnline, "alwaysOnline", false, false, GetStgLogger(), sd),
tariffName  (conf.tariffName,   "tariff",       false, false, GetStgLogger(), sd),
nextTariff  (conf.nextTariff,   "new tariff",   false, false, GetStgLogger(), sd),
address     (conf.address,      "address",      false, false, GetStgLogger(), sd),
note        (conf.note,         "note",         false, false, GetStgLogger(), sd),
group       (conf.group,        "group",        false, false, GetStgLogger(), sd),
email       (conf.email,        "email",        false, false, GetStgLogger(), sd),
phone       (conf.phone,        "phone",        false, false, GetStgLogger(), sd),
realName    (conf.realName,     "realName",     false, false, GetStgLogger(), sd),
credit      (conf.credit,       "credit",       false, false, GetStgLogger(), sd),
creditExpire(conf.creditExpire, "creditExpire", false, false, GetStgLogger(), sd),
ips         (conf.ips,          "IP",           false, false, GetStgLogger(), sd),
userdata0   (conf.userdata[0],  "userdata0",    false, false, GetStgLogger(), sd),
userdata1   (conf.userdata[1],  "userdata1",    false, false, GetStgLogger(), sd),
userdata2   (conf.userdata[2],  "userdata2",    false, false, GetStgLogger(), sd),
userdata3   (conf.userdata[3],  "userdata3",    false, false, GetStgLogger(), sd),
userdata4   (conf.userdata[4],  "userdata4",    false, false, GetStgLogger(), sd),
userdata5   (conf.userdata[5],  "userdata5",    false, false, GetStgLogger(), sd),
userdata6   (conf.userdata[6],  "userdata6",    false, false, GetStgLogger(), sd),
userdata7   (conf.userdata[7],  "userdata7",    false, false, GetStgLogger(), sd),
userdata8   (conf.userdata[8],  "userdata8",    false, false, GetStgLogger(), sd),
userdata9   (conf.userdata[9],  "userdata9",    false, false, GetStgLogger(), sd)
{
}
//-----------------------------------------------------------------------------
