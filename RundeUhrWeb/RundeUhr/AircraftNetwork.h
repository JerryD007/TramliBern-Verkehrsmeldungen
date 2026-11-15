#pragma once
#include "AircraftParse.h"
#include "AircraftRouteNetwork.h"
struct AircraftResult {Config config;AircraftData data;};
QueueHandle_t aircraftResultQueue;
inline void fetchAircraft(AircraftData&a,const Config&c,uint32_t&retry){
 WiFiClientSecure client;client.setCACert(FINANCE_TRUST);client.setHandshakeTimeout(8);
 HTTPClient http;http.setConnectTimeout(5000);http.setTimeout(6000);http.useHTTP10(true);
 String url="https://opendata.adsb.fi/api/v3/lat/"+String(c.aircraftLat,5)+"/lon/"+String(c.aircraftLon,5)+"/dist/"+String(int(ceil(c.aircraftRadius/1.852)));
 if(!http.begin(client,url)){strcpy(a.error,"HTTPS start failed");return;}
 const char*headers[]={"Retry-After"};http.collectHeaders(headers,1);http.addHeader("Accept-Encoding","identity");http.setUserAgent("RundeUhr/3.2.0-rc3 personal display");a.http=http.GET();
 if(a.http!=200){char detail[80]={};a.tls=client.lastError(detail,sizeof(detail));snprintf(a.error,sizeof(a.error),"HTTP %d / TLS %d",a.http,a.tls);if(a.http==429){long seconds=http.header("Retry-After").toInt();retry=uint32_t(seconds>300?(seconds>86400?86400:seconds):300)*1000UL;}http.end();return;}
 int length=http.getSize();constexpr int limit=16384;if(length>limit){strcpy(a.error,"Odpoved nad 16 KiB: zmensi okruh");http.end();return;}
 BoundedBody body(length>0?size_t(length):limit);if(!body.ready()){strcpy(a.error,"RAM chybi pri prijmu");http.end();return;}
 int count=http.writeToStream(&body);http.end();client.stop();
 if(count<0||body.overflowed()||body.allocationFailed()||(length>=0&&count!=length)){strcpy(a.error,"Neuplna odpoved / limit 16 KiB");return;}
 JsonDocument filter;filter["now"]=true;filter["msg"]=true;for(auto k:{"hex","flight","r","t","lat","lon","seen_pos","alt_baro","gs","track"})filter["ac"][0][k]=true;
 JsonDocument d;if(deserializeJson(d,body.bytes(),body.size(),DeserializationOption::Filter(filter),DeserializationOption::NestingLimit(8))||!parseAircraft(d,a,c,time(nullptr)))strcpy(a.error,"Neplatna nebo zastarala data");
}
inline void aircraftTick(const Config&c){
 static AircraftData data;static Config previous;static bool started=false,attempted=false;static uint32_t last=0,retry=30000;
 // Settings changes clear old positions, but never bypass the request rate limit.
 if(!started||!aircraftConfigEqual(previous,c)){data=AircraftData();previous=c;started=true;}
 if(!(c.view==12||(c.mask&(1<<12))||(c.automatic&&c.aircraftPriority))||otaActive)return;
 if(attempted&&uint32_t(millis()-last)<retry)return;
 networkBusy=true;if(otaActive){networkBusy=false;return;}
 Quote memory;data.memoryDeferred=false;data.http=data.tls=0;retry=300000;
 if(!financeMemoryReady(memory)){data.memoryDeferred=true;strcpy(data.error,"RAM: dalsi pokus za 15 s");retry=15000;}
 else {data.heapBefore=memory.heapBefore;data.largestBefore=memory.largestBefore;fetchAircraft(data,c,retry);if(!data.error[0])retry=30000;}
 data.heapBefore=memory.heapBefore;data.largestBefore=memory.largestBefore;last=millis();attempted=true;
 AircraftResult result;result.config=c;result.data=data;xQueueOverwrite(aircraftResultQueue,&result);
 if(!data.error[0]){enrichAircraftRoute(data);result.data=data;xQueueOverwrite(aircraftResultQueue,&result);}
 networkBusy=false;
}
