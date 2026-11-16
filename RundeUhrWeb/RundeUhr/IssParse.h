#pragma once
#include <ArduinoJson.h>
#include "IssModel.h"
inline bool parseIss(JsonDocument&d,IssData&out,time_t now){
 if(d["id"]!=25544||!d["units"].is<const char*>()||strcmp(d["units"],"kilometers"))return false;
 for(auto k:{"latitude","longitude","altitude","velocity","timestamp"})if(!d[k].is<double>()||!isfinite(d[k].as<double>()))return false;
 IssData n=out;n.lat=d["latitude"];n.lon=d["longitude"];n.altitude=d["altitude"];n.speed=d["velocity"];double stamp=d["timestamp"];
 if(n.lat < -90||n.lat>90||n.lon < -180||n.lon>180||n.altitude<100||n.altitude>1000||n.speed<0||n.speed>40000||stamp<1704067200||stamp>double(now)+10||double(now)-stamp>90)return false;
 n.positionAt=time_t(stamp);n.received=now;n.valid=true;n.error[0]=0;out=n;return true;
}
