#pragma once
#include "NewsFeed.h"
#include "NewsBody.h"
#include <memory>
#include <new>
struct NewsResult {NewsData data;};QueueHandle_t newsResultQueue;
inline void fetchNews(NewsData&d){
 WiFiClientSecure client;client.setCACert(FINANCE_TRUST);client.setHandshakeTimeout(20);
 HTTPClient http;http.setConnectTimeout(5000);http.setTimeout(10000);http.useHTTP10(true);
 if(!http.begin(client,NEWS_SOURCES[d.source].url)){strcpy(d.error,"HTTPS Start fehlgeschlagen");return;}
 const char* headers[]={"Transfer-Encoding"};http.collectHeaders(headers,1);
 http.addHeader("Accept-Encoding","identity");http.setUserAgent("RundeUhr/3.2.0-rc25 private RSS reader");d.http=http.GET();
 if(d.http!=200){char detail[48]={};int tls=client.lastError(detail,sizeof(detail));snprintf(d.error,sizeof(d.error),"HTTP %d / TLS %d",d.http,tls);http.end();return;}
 std::unique_ptr<NewsFeed> feed(new(std::nothrow) NewsFeed);
 if(!feed){strcpy(d.error,"RAM beim RSS Empfang");http.end();client.stop();return;}
 int expected=http.getSize();int n=(expected>=0&&http.header("Transfer-Encoding").length()==0)?readNewsBody(client,*feed,expected):http.writeToStream(feed.get());http.end();client.stop();
 if(n<0||(expected>=0&&n!=expected)){snprintf(d.error,sizeof(d.error),"RSS Lesen %d: %u/%d B",n,unsigned(feed->size()),expected);return;}
 if(!feed->finish(d,time(nullptr)))strcpy(d.error,"RSS unvollstaendig/ungueltig");
}
inline void newsTick(const Config&c){
 static NewsData data;static uint32_t last=0,retry=5400000UL;static bool attempted=false;
 if(data.source!=c.newsFeed){data=NewsData();data.source=c.newsFeed;attempted=false;}
 if((c.view!=16&&!(c.mask&(1UL<<16)))||otaActive)return;
 // Feed advertises ttl=90 minutes. Never treat this as a live emergency service.
 if(attempted&&uint32_t(millis()-last)<retry)return;
 networkBusy=true;if(otaActive){networkBusy=false;return;}Quote probe;data.http=0;
 retry=5400000UL;if(!financeMemoryReady(probe)){retry=15000UL;strcpy(data.error,"RAM: Abruf verschoben; in 15 s");}else {fetchNews(data);if(data.error[0]&&data.http<=200)retry=60000UL;}
 networkBusy=false;last=millis();attempted=true;NewsResult r{data};xQueueOverwrite(newsResultQueue,&r);
}
