#pragma once
#include "QuakeParse.h"
struct QuakeResult {QuakeData data;};
QueueHandle_t quakeResultQueue;
inline void fetchQuake(QuakeData&q){
 WiFiClientSecure client;client.setCACert(FINANCE_TRUST);client.setHandshakeTimeout(8);
 HTTPClient http;http.setConnectTimeout(5000);http.setTimeout(6000);http.useHTTP10(true);
 if(!http.begin(client,"https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/4.5_day.geojson")){strcpy(q.error,"HTTPS Start fehlgeschlagen");return;}
 http.setUserAgent("RundeUhr/3.2.0-rc19");http.addHeader("Accept-Encoding","identity");q.http=http.GET();
 if(q.http!=200){snprintf(q.error,sizeof(q.error),"HTTP %d",q.http);http.end();return;}
 int length=http.getSize();if(length>24576){strcpy(q.error,"USGS Antwort zu gross");http.end();return;}
 BoundedBody body(length>0?size_t(length):24576);if(!body.ready()){strcpy(q.error,"RAM fehlt");http.end();return;}
 int count=http.writeToStream(&body);http.end();client.stop();
 if(count<0||body.overflowed()||body.allocationFailed()||(length>=0&&count!=length)){strcpy(q.error,"USGS Antwort unvollstaendig");return;}
 JsonDocument filter;filter["type"]=true;filter["metadata"]["generated"]=true;filter["metadata"]["count"]=true;
 for(auto key:{"mag","place","time","type"})filter["features"][0]["properties"][key]=true;
 filter["features"][0]["geometry"]["coordinates"]=true;
 JsonDocument d;if(deserializeJson(d,body.bytes(),body.size(),DeserializationOption::Filter(filter),DeserializationOption::NestingLimit(7))||!parseQuake(d,q,time(nullptr)))strcpy(q.error,"Ungueltige oder alte USGS Daten");
}
inline void quakeTick(const Config&c){
 static QuakeData data;static uint32_t last=0;static bool attempted=false;
 if(c.view!=14&&!(c.mask&(1<<14)))return;
 if(attempted&&uint32_t(millis()-last)<300000UL)return;
 networkBusy=true;if(otaActive){networkBusy=false;return;}
 Quote probe;data.http=0;if(!financeMemoryReady(probe))strcpy(data.error,"RAM: Abruf verschoben");else fetchQuake(data);
 networkBusy=false;last=millis();attempted=true;QuakeResult result{data};xQueueOverwrite(quakeResultQueue,&result);
}
