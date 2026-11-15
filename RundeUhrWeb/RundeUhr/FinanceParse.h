#pragma once
#include <ArduinoJson.h>
#include "FinanceModel.h"
inline bool parseFinance(JsonDocument&d,Quote&q,const char*base,const char*currency,bool btc,time_t now){
 double value=0;const char*date="";
 if(btc){auto data=d["data"];if(strcmp(data["base"]|"",base)||strcmp(data["currency"]|"",currency)||!data["amount"].is<const char*>())return false;const char*s=data["amount"];char*end;value=strtod(s,&end);if(end==s||*end)return false;}
 else {if(strcmp(d["base"]|"",base)||strcmp(d["quote"]|"",currency)||!d["rate"].is<double>())return false;value=d["rate"];date=d["date"]|"";time_t dt;if(!parseDate(date,dt)||calendarDays(dt,now)>1)return false;}
 if(!isfinite(value)||value<=0||value>1e12)return false;
 q.value=value;q.received=now;strcpy(q.date,date);q.error[0]=0;q.valid=true;return true;
}
