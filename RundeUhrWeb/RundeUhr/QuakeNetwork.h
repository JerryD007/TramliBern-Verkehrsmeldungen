#pragma once
#include "QuakeParse.h"
struct QuakeResult {QuakeData data;};
QueueHandle_t quakeResultQueue;
inline void fetchQuake(QuakeData&q,const Config&c){
 WiFiClientSecure client;client.setCACert(FINANCE_TRUST);client.setHandshakeTimeout(20);
 HTTPClient http;http.setConnectTimeout(5000);http.setTimeout(6000);http.useHTTP10(true);
 if(!http.begin(client,"https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/4.5_day.geojson")){strcpy(q.error,"HTTPS Start fehlgeschlagen");return;}
 http.setUserAgent("RundeUhr/3.2.0-rc25");http.addHeader("Accept-Encoding","identity");q.http=http.GET();
 if(q.http!=200){char detail[48]={};int tls=client.lastError(detail,sizeof(detail));snprintf(q.error,sizeof(q.error),"HTTP %d / TLS %d",q.http,tls);http.end();return;}
 int length=http.getSize();if(length>24576){strcpy(q.error,"USGS Antwort zu gross");http.end();return;}
 BoundedBody body(length>0?size_t(length):24576);if(!body.ready()){strcpy(q.error,"RAM fehlt");http.end();return;}
 int count=http.writeToStream(&body);http.end();client.stop();
 if(body.allocationFailed()){snprintf(q.error,sizeof(q.error),"RAM beim Empfang: %u B",unsigned(body.size()));return;}
 if(body.overflowed()){strcpy(q.error,"Antwort zu gross");return;}
 if(count<0||(length>=0&&count!=length)){snprintf(q.error,sizeof(q.error),"Lesefehler %d: %u/%d B",count,unsigned(body.size()),length);return;}
 JsonDocument filter;filter["type"]=true;filter["metadata"]["generated"]=true;filter["metadata"]["count"]=true;
 for(auto key:{"mag","place","time","type"})filter["features"][0]["properties"][key]=true;
 filter["features"][0]["geometry"]["coordinates"]=true;
 JsonDocument d;if(deserializeJson(d,body,DeserializationOption::Filter(filter),DeserializationOption::NestingLimit(7))||!parseQuake(d,q,time(nullptr),&c))strcpy(q.error,"Ungueltige oder alte USGS Daten");
}
inline void quakeTick(const Config&c){
 static QuakeData data;static uint32_t last=0,retry=300000UL;static bool attempted=false;static Config previous;
 if(previous.quakeWindow!=c.quakeWindow||previous.quakeMagnitude!=c.quakeMagnitude||previous.quakeRadius!=c.quakeRadius||previous.lat!=c.lat||previous.lon!=c.lon){data=QuakeData();attempted=false;previous=c;}
 if(c.view!=14&&!(c.mask&(1<<14)))return;
 if(attempted&&uint32_t(millis()-last)<retry)return;
 networkBusy=true;if(otaActive){networkBusy=false;return;}
 Quote probe;data.http=0;if(!financeMemoryReady(probe)){retry=15000UL;strcpy(data.error,"RAM: Abruf verschoben; in 15 s");}else {fetchQuake(data,c);retry=300000UL;}
 networkBusy=false;last=millis();attempted=true;QuakeResult result{data};xQueueOverwrite(quakeResultQueue,&result);
}
