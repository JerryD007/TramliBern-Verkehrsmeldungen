#pragma once
#include "AircraftRoute.h"
// One short sequential lookup after position fetch. Errors never invalidate position data.
inline int fetchAircraftRoute(const char*call,char*origin,char*dest,uint32_t&delay){
 origin[0]=dest[0]=0;WiFiClientSecure client;client.setCACert(FINANCE_TRUST);client.setHandshakeTimeout(5);
 HTTPClient http;http.setConnectTimeout(3000);http.setTimeout(4000);http.useHTTP10(true);
 if(!http.begin(client,String("https://api.adsbdb.com/v0/callsign/")+call))return -1;
 const char*headers[]={"Retry-After"};http.collectHeaders(headers,1);http.addHeader("Accept-Encoding","identity");http.setUserAgent("RundeUhr/3.2.0-rc3 personal display");int code=http.GET();
 if(code!=200){if(code==429){long s=http.header("Retry-After").toInt();delay=uint32_t(s>300?(s>86400?86400:s):300)*1000UL;}http.end();return code;}
 int length=http.getSize();constexpr int limit=6144;if(length>limit){http.end();return -2;}
 BoundedBody body(length>0?size_t(length):limit);if(!body.ready()){http.end();return -3;}
 int count=http.writeToStream(&body);http.end();client.stop();
 if(count<0||body.overflowed()||body.allocationFailed()||(length>=0&&count!=length))return -4;
 JsonDocument filter;for(auto k:{"callsign","callsign_icao","callsign_iata"})filter["response"]["flightroute"][k]=true;
 for(auto place:{"origin","destination"})for(auto k:{"municipality","name"})filter["response"]["flightroute"][place][k]=true;
 JsonDocument d;if(deserializeJson(d,body,DeserializationOption::Filter(filter),DeserializationOption::NestingLimit(8))||!parseAircraftRoute(d,call,origin,dest,97))return -5;
 return 200;
}
inline void enrichAircraftRoute(AircraftData&a){
 static AircraftRouteCache cache;a.origin[0]=a.destination[0]=0;a.routeHTTP=0;
 if(!a.found||aircraftStale(a,time(nullptr))||!routeCallsign(a.callsign)||otaActive)return;
 auto cached=cache.find(a.callsign,millis());if(cached){strcpy(a.origin,cached->origin);strcpy(a.destination,cached->destination);a.routeHTTP=cached->http;return;}
 if(!cache.ready(millis()))return;
 Quote memory;if(!financeMemoryReady(memory))return;
 if(otaActive)return;char origin[97]="",dest[97]="";uint32_t delay=60000;int code=fetchAircraftRoute(a.callsign,origin,dest,delay);
 uint32_t ttl=code==200?3600000:code==404?900000:300000;
 cache.put(a.callsign,origin,dest,code,millis(),ttl,delay);strcpy(a.origin,origin);strcpy(a.destination,dest);a.routeHTTP=code;
}
