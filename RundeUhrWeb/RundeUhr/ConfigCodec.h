#pragma once
#include <ArduinoJson.h>
#include "Model.h"
inline bool number(JsonVariantConst v){return !v.isNull()&&(v.is<float>()||v.is<int>()||v.is<double>()||v.is<long>());}
inline bool textValue(JsonDocument &d,const char* key,char* dest,size_t cap,bool empty=true){
 if(!d[key].is<const char*>())return false;const char*s=d[key];size_t n=strlen(s);if(n>=cap||(!empty&&!n))return false;
 for(size_t i=0;i<n;i++)if(uint8_t(s[i])<32)return false;memcpy(dest,s,n+1);return true;
}
inline bool decodeConfig(JsonDocument &d,Config &c){
 Config t=c;
 if(!d["deviceName"].isNull()&&!textValue(d,"deviceName",t.deviceName,sizeof(t.deviceName)))return false;
 if(!textValue(d,"city",t.city,sizeof(t.city),false)||!textValue(d,"event",t.event,sizeof(t.event),false)||!textValue(d,"date",t.date,sizeof(t.date)))return false;
 if(!number(d["lat"])||!number(d["lon"]))return false;t.lat=d["lat"];t.lon=d["lon"];
 if(!isfinite(t.lat)||!isfinite(t.lon)||t.lat < -90||t.lat>90||t.lon < -180||t.lon>180)return false;
 for(auto k:{"view","mask","interval"})if(!d[k].is<int>())return false;
 int v=d["view"],m=d["mask"],i=d["interval"];
 if(v<0||v>=VIEW_COUNT||m<1||m>1023||i<5||i>3600||!d["automatic"].is<bool>())return false;
 time_t target;if(strlen(t.date)&&!parseDate(t.date,target))return false;
 if(!d["countdownType"].is<int>()||!d["animations"].is<bool>()||!d["countdownDates"].is<JsonArray>()||d["countdownDates"].size()!=5)return false;
 int type=d["countdownType"];if(type<0||type>4)return false;
 if(!textValue(d,"customEvent",t.customEvent,sizeof(t.customEvent),false))return false;
 for(int k=0;k<5;k++){auto value=d["countdownDates"][k];if(!value.is<const char*>())return false;const char*s=value;if(strlen(s)>10||(strlen(s)&&!parseDate(s,target)))return false;strcpy(t.countdownDates[k],s);}
 for(auto k:{"weekendStartDay","weekendEndDay","weekendStartMinute","weekendEndMinute"}){
  if(d[k].isNull())continue;
  if(!d[k].is<int>())return false;
  int value=d[k];bool day=strstr(k,"Day")!=nullptr;if(value<0||value>(day?6:1439))return false;
 }
 if(!d["weekendStartDay"].isNull())t.weekendStartDay=d["weekendStartDay"];
 if(!d["weekendEndDay"].isNull())t.weekendEndDay=d["weekendEndDay"];
 if(!d["weekendStartMinute"].isNull())t.weekendStartMinute=d["weekendStartMinute"];
 if(!d["weekendEndMinute"].isNull())t.weekendEndMinute=d["weekendEndMinute"];
 if(t.weekendStartDay==t.weekendEndDay && t.weekendStartMinute==t.weekendEndMinute)return false;
 t.countdownType=type;t.animations=d["animations"];syncCountdown(t);
 t.view=v;t.mask=m;t.interval=i;t.automatic=d["automatic"];c=t;return true;
}
inline bool loadConfig(JsonDocument &d,Config&c,bool&migrated){
 migrated=false;Config t;
 // Credentials are recovered independently, even if legacy display settings are invalid.
 textValue(d,"ssid",t.ssid,sizeof(t.ssid));textValue(d,"pass",t.pass,sizeof(t.pass));
 int schema=d["schema"]|1;if(schema==1){int v=d["view"]|0,m=d["mask"]|1023;migrateViews(v,m,t);d["view"]=t.view;d["mask"]=t.mask;migrated=true;}
 if(schema==1||schema==2){
 d["countdownType"]=4;d["customEvent"]=d["event"].as<const char*>();d["animations"]=true;
 JsonArray dates=d["countdownDates"].to<JsonArray>();for(int k=0;k<5;k++)dates.add(k==4?(d["date"]|""):"");migrated=true;
 }
 if(schema>=1&&schema<=3){
 // A former single Countdown maps only to the previously selected motif.
 int type=d["countdownType"]|4;if(type<0||type>4)type=4;
 int v=d["view"]|0,m=d["mask"]|63;
 if(v==5)v=5+type;
 m=(m&31)|((m&32)?(1<<(5+type)):0);
 d["view"]=v;d["mask"]=m?m:1;d["schema"]=4;migrated=true;
 }
 if(schema!=1&&schema!=2&&schema!=3&&schema!=4){c=t;return false;}
 bool ok=decodeConfig(d,t);c=t;return ok;
}
