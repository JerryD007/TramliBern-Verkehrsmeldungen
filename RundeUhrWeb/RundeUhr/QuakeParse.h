#pragma once
#include <ArduinoJson.h>
#include "QuakeModel.h"
inline bool parseQuake(JsonDocument&d,QuakeData&out,time_t now){
 if(d["type"]!="FeatureCollection"||!d["features"].is<JsonArray>()||!d["metadata"]["generated"].is<int64_t>())return false;
 int64_t generated=d["metadata"]["generated"].as<int64_t>()/1000;
 if(generated<1704067200||generated>int64_t(now)+60||int64_t(now)-generated>900)return false;
 if(!d["metadata"]["count"].is<int>()||d["metadata"]["count"].as<int>()!=int(d["features"].size()))return false;
 QuakeData n;n.http=out.http;n.generated=generated;n.received=now;
 for(JsonObject f:d["features"].as<JsonArray>()){
  auto p=f["properties"];auto c=f["geometry"]["coordinates"];
  if(p["type"]!="earthquake")continue;
  if(!p["mag"].is<double>()||!p["time"].is<int64_t>()||!c[2].is<double>())return false;
  double mag=p["mag"],depth=c[2];int64_t at=p["time"].as<int64_t>()/1000;
  if(!isfinite(mag)||mag<0||mag>10||!isfinite(depth)||depth< -20||depth>1000||at<1704067200||at>int64_t(now)+60)return false;
  if(mag<4.5||int64_t(now)-at>86400)continue;
  QuakeEvent e;e.magnitude=mag;e.depth=depth;e.at=at;
  const char* place=p["place"]|"Ort unbekannt";snprintf(e.place,sizeof(e.place),"%s",place);
  // Keep the five newest entries independent of server order.
  int pos=0;while(pos<n.count&&n.events[pos].at>=e.at)pos++;
  if(pos<5){int end=n.count<5?n.count:4;for(int j=end;j>pos;j--)n.events[j]=n.events[j-1];n.events[pos]=e;if(n.count<5)n.count++;}
 }
 n.valid=true;out=n;return true;
}
