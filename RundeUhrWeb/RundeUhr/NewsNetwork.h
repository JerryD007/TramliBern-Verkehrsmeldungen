#pragma once
#include "NewsFeed.h"
struct NewsResult {NewsData data;};QueueHandle_t newsResultQueue;
inline void fetchNews(NewsData&d){
 WiFiClientSecure client;client.setCACert(FINANCE_TRUST);client.setHandshakeTimeout(8);
 HTTPClient http;http.setConnectTimeout(5000);http.setTimeout(10000);http.useHTTP10(true);
 if(!http.begin(client,NEWS_SOURCES[d.source].url)){strcpy(d.error,"HTTPS Start fehlgeschlagen");return;}
 http.addHeader("Accept-Encoding","identity");http.setUserAgent("RundeUhr/3.2.0-rc20 private RSS reader");d.http=http.GET();
 if(d.http!=200){snprintf(d.error,sizeof(d.error),"HTTP %d",d.http);http.end();return;}
 static NewsFeed feed;feed=NewsFeed();int n=http.writeToStream(&feed);int expected=http.getSize();http.end();client.stop();
 if(n<0||(expected>=0&&n!=expected)){snprintf(d.error,sizeof(d.error),"RSS Lesen %d: %u/%d B",n,unsigned(feed.size()),expected);return;}
 if(!feed.finish(d,time(nullptr)))strcpy(d.error,"RSS unvollstaendig/ungueltig");
}
inline void newsTick(const Config&c){
 static NewsData data;static uint32_t last=0;static bool attempted=false;
 if(data.source!=c.newsFeed){data=NewsData();data.source=c.newsFeed;attempted=false;}
 if((c.view!=16&&!(c.mask&(1UL<<16)))||otaActive)return;
 // Feed advertises ttl=90 minutes. Never treat this as a live emergency service.
 if(attempted&&uint32_t(millis()-last)<5400000UL)return;
 networkBusy=true;if(otaActive){networkBusy=false;return;}Quote probe;data.http=0;
 if(!financeMemoryReady(probe))strcpy(data.error,"RAM: Abruf verschoben");else fetchNews(data);
 networkBusy=false;last=millis();attempted=true;static NewsResult r;r.data=data;xQueueOverwrite(newsResultQueue,&r);
}
