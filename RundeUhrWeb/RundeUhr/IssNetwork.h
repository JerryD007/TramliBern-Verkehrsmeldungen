#pragma once
#include "IssParse.h"
struct IssResult {IssData data;};
QueueHandle_t issResultQueue;
inline void fetchIss(IssData&i){
 WiFiClientSecure client;client.setCACert(FINANCE_TRUST);client.setHandshakeTimeout(8);
 HTTPClient http;http.setConnectTimeout(5000);http.setTimeout(6000);http.useHTTP10(true);
 if(!http.begin(client,"https://api.wheretheiss.at/v1/satellites/25544")){strcpy(i.error,"HTTPS Start fehlgeschlagen");return;}
 http.setUserAgent("RundeUhr/3.2.0-rc21");http.addHeader("Accept-Encoding","identity");i.http=http.GET();
 if(i.http!=200){char detail[48]={};int tls=client.lastError(detail,sizeof(detail));snprintf(i.error,sizeof(i.error),"HTTP %d / TLS %d",i.http,tls);http.end();return;}
 int length=http.getSize();if(length>2048){strcpy(i.error,"Antwort zu gross");http.end();return;}
 BoundedBody body(length>0?size_t(length):2048);if(!body.ready()){strcpy(i.error,"RAM fehlt");http.end();return;}
 int count=http.writeToStream(&body);http.end();client.stop();
 if(body.allocationFailed()){snprintf(i.error,sizeof(i.error),"RAM beim Empfang: %u B",unsigned(body.size()));return;}
 if(body.overflowed()){strcpy(i.error,"Antwort zu gross");return;}
 if(count<0||(length>=0&&count!=length)){snprintf(i.error,sizeof(i.error),"Lesefehler %d: %u/%d B",count,unsigned(body.size()),length);return;}
 JsonDocument d;if(deserializeJson(d,body,DeserializationOption::NestingLimit(4))||!parseIss(d,i,time(nullptr)))strcpy(i.error,"Ungueltige oder alte ISS Daten");
}
inline void issTick(const Config&c){
 static IssData data;static uint32_t last=0,retry=30000UL;static bool attempted=false;
 if(c.view!=13&&!(c.mask&(1<<13)))return;
 uint32_t ms=millis();if(attempted&&uint32_t(ms-last)<retry)return;
 networkBusy=true;if(otaActive){networkBusy=false;return;}
 Quote probe;data.http=0;if(!financeMemoryReady(probe)){retry=15000UL;strcpy(data.error,"RAM: Abruf verschoben; in 15 s");}else {fetchIss(data);retry=data.error[0]?300000UL:30000UL;}
 networkBusy=false;last=millis();attempted=true;IssResult result{data};xQueueOverwrite(issResultQueue,&result);
}
