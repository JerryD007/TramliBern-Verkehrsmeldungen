#pragma once
#include "IssParse.h"
struct IssResult {IssData data;};
QueueHandle_t issResultQueue;
inline void fetchIss(IssData&i){
 WiFiClientSecure client;client.setCACert(FINANCE_TRUST);client.setHandshakeTimeout(8);
 HTTPClient http;http.setConnectTimeout(5000);http.setTimeout(6000);http.useHTTP10(true);
 if(!http.begin(client,"https://api.wheretheiss.at/v1/satellites/25544")){strcpy(i.error,"HTTPS Start fehlgeschlagen");return;}
 http.setUserAgent("RundeUhr/3.2.0-rc17");http.addHeader("Accept-Encoding","identity");i.http=http.GET();
 if(i.http!=200){snprintf(i.error,sizeof(i.error),"HTTP %d",i.http);http.end();return;}
 int length=http.getSize();if(length>2048){strcpy(i.error,"Antwort zu gross");http.end();return;}
 BoundedBody body(length>0?size_t(length):2048);if(!body.ready()){strcpy(i.error,"RAM fehlt");http.end();return;}
 int count=http.writeToStream(&body);http.end();client.stop();
 if(count<0||body.overflowed()||body.allocationFailed()||(length>=0&&count!=length)){strcpy(i.error,"Antwort unvollstaendig");return;}
 JsonDocument d;if(deserializeJson(d,body.bytes(),body.size(),DeserializationOption::NestingLimit(4))||!parseIss(d,i,time(nullptr)))strcpy(i.error,"Ungueltige oder alte ISS Daten");
}
inline void issTick(const Config&c){
 static IssData data;static uint32_t last=0;static bool attempted=false;
 if(c.view!=13&&!(c.mask&(1<<13)))return;
 uint32_t ms=millis();if(attempted&&uint32_t(ms-last)<(data.error[0]?300000UL:30000UL))return;
 networkBusy=true;if(otaActive){networkBusy=false;return;}
 Quote probe;data.http=0;if(!financeMemoryReady(probe))strcpy(data.error,"RAM: Abruf verschoben");else fetchIss(data);
 networkBusy=false;last=millis();attempted=true;IssResult result{data};xQueueOverwrite(issResultQueue,&result);
}
