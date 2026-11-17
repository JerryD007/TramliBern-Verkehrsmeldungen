#pragma once
#include "LaunchParse.h"
struct LaunchResult {LaunchData data;};
QueueHandle_t launchResultQueue;
inline void fetchLaunch(LaunchData&d,uint32_t&retry){
 WiFiClientSecure client;client.setCACert(FINANCE_TRUST);client.setHandshakeTimeout(20);
 HTTPClient http;http.setConnectTimeout(5000);http.setTimeout(6000);http.useHTTP10(true);
 if(!http.begin(client,"https://ll.thespacedevs.com/2.3.0/launches/upcoming/?limit=3&mode=list")){strcpy(d.error,"HTTPS Start fehlgeschlagen");return;}
 const char*headers[]={"Retry-After"};http.collectHeaders(headers,1);http.setUserAgent("RundeUhr/3.2.0-rc21 personal display");http.addHeader("Accept-Encoding","identity");d.http=http.GET();
 if(d.http!=200){char detail[48]={};int tls=client.lastError(detail,sizeof(detail));snprintf(d.error,sizeof(d.error),"HTTP %d / TLS %d",d.http,tls);if(d.http==429)retry=launchRetry(http.header("Retry-After").c_str(),time(nullptr));http.end();return;}
 int length=http.getSize();if(length>12288){strcpy(d.error,"Antwort zu gross");http.end();return;}
 BoundedBody body(length>0?size_t(length):12288);if(!body.ready()){strcpy(d.error,"RAM fehlt");http.end();return;}
 int count=http.writeToStream(&body);http.end();client.stop();
 if(body.allocationFailed()){snprintf(d.error,sizeof(d.error),"RAM beim Empfang: %u B",unsigned(body.size()));return;}
 if(body.overflowed()){strcpy(d.error,"Antwort zu gross");return;}
 if(count<0||(length>=0&&count!=length)){snprintf(d.error,sizeof(d.error),"Lesefehler %d: %u/%d B",count,unsigned(body.size()),length);return;}
 JsonDocument filter;filter["count"]=true;for(auto k:{"name","net"})filter["results"][0][k]=true;filter["results"][0]["status"]["abbrev"]=true;filter["results"][0]["net_precision"]["abbrev"]=true;
 JsonDocument doc;if(deserializeJson(doc,body,DeserializationOption::Filter(filter),DeserializationOption::NestingLimit(7))||!parseLaunch(doc,d,time(nullptr)))strcpy(d.error,"Ungueltige Startdaten");
}
inline void launchTick(const Config&c){
 static LaunchData data;static uint32_t last=0,retry=1800000UL;static bool attempted=false;
 if((c.view!=15&&!(c.mask&(1<<15)))||otaActive)return;
 if(attempted&&uint32_t(millis()-last)<retry)return;
 networkBusy=true;if(otaActive){networkBusy=false;return;}
 Quote probe;data.http=0;retry=1800000UL;if(!financeMemoryReady(probe)){retry=15000UL;strcpy(data.error,"RAM: Abruf verschoben; in 15 s");}else fetchLaunch(data,retry);
 networkBusy=false;last=millis();attempted=true;data.nextAttempt=time(nullptr)+retry/1000;LaunchResult result{data};xQueueOverwrite(launchResultQueue,&result);
}
