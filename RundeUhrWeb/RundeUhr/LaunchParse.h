#pragma once
#include <ArduinoJson.h>
#include "LaunchModel.h"
#include "AircraftRoute.h"
inline bool parseLaunch(JsonDocument&d,LaunchData&out,time_t now){
 if(!d["count"].is<int>()||d["count"].as<int>()<0||!d["results"].is<JsonArray>()||d["results"].size()>3)return false;
 if(d["count"].as<int>()<int(d["results"].size())||(d["count"].as<int>()>0&&d["results"].size()==0))return false;
 LaunchData n;n.http=out.http;n.received=now;
 for(JsonObject f:d["results"].as<JsonArray>()){
  if(!f["name"].is<const char*>()||!strlen(f["name"].as<const char*>())||!f["status"].is<JsonObject>())return false;
  LaunchEvent e;routeText(e.name,sizeof(e.name),f["name"].as<const char*>());if(!e.name[0])return false;snprintf(e.status,sizeof(e.status),"%s",f["status"]["abbrev"]|"");snprintf(e.precision,sizeof(e.precision),"%s",f["net_precision"]["abbrev"]|"");
  if(!f["net"].isNull()&&(!f["net"].is<const char*>()||!launchUTC(f["net"],e.net)))return false;
  n.events[n.count++]=e;
 }
 n.valid=true;out=n;return true;
}
