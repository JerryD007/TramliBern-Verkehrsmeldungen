#pragma once
#include "AircraftParse.h"
inline bool routeCallsign(const char*s){size_t n=strlen(s);if(n<3||n>16)return false;for(;*s;s++)if(!(*s>='A'&&*s<='Z')&&!(*s>='0'&&*s<='9'))return false;return true;}
// Retain complete UTF-8 characters for Safari; the TFT uses its existing transliterator.
inline void routeText(char*out,size_t cap,const char*s){
 size_t n=0;while(*s){uint8_t c=*s;size_t k=c<128?1:(c&0xe0)==0xc0?2:(c&0xf0)==0xe0?3:(c&0xf8)==0xf0?4:0;
 if(!k){s++;continue;}bool ok=true;for(size_t j=1;j<k;j++)if(!s[j]||(uint8_t(s[j])&0xc0)!=0x80){ok=false;break;}if(!ok)break;
 if(c>=32&&c!=127){if(n+k>=cap)break;memcpy(out+n,s,k);n+=k;}s+=k;
 }while(n&&out[n-1]==' ')n--;out[n]=0;size_t lead=0;while(out[lead]==' ')lead++;if(lead)memmove(out,out+lead,n-lead+1);
}
inline void routePlace(JsonVariantConst place,char*out,size_t cap){
 out[0]=0;if(!place.is<JsonObjectConst>())return;
 for(auto key:{"municipality","name"}){const char*s=place[key].is<const char*>()?place[key].as<const char*>():"";routeText(out,cap,s);if(out[0]&&strcmp(out,"Unknown")&&strcmp(out,"unknown")&&strcmp(out,"N/A")&&strcmp(out,"-"))return;}
 out[0]=0;
}
inline bool parseAircraftRoute(JsonDocument&d,const char*call,char*origin,char*dest,size_t cap){
 origin[0]=dest[0]=0;auto r=d["response"]["flightroute"];if(!r.is<JsonObject>())return false;
 bool match=false;for(auto key:{"callsign","callsign_icao","callsign_iata"})if(r[key].is<const char*>()&&!strcmp(call,r[key].as<const char*>()))match=true;
 if(!match)return false;routePlace(r["origin"],origin,cap);routePlace(r["destination"],dest,cap);
 return origin[0]||dest[0];
}
inline bool parseAircraftRoute(JsonDocument&d,const char*call,char*dest,size_t cap){char origin[97];return parseAircraftRoute(d,call,origin,dest,cap<=97?cap:97);}
struct RouteEntry {char callsign[17]="",origin[97]="",destination[97]="";uint32_t at=0,ttl=0;int http=0;};
struct AircraftRouteCache {
 RouteEntry entries[4];uint8_t next=0;bool attempted=false;uint32_t last=0,wait=60000;
 const RouteEntry*find(const char*call,uint32_t now)const{for(auto&e:entries)if(e.ttl&&!strcmp(call,e.callsign)&&uint32_t(now-e.at)<e.ttl)return &e;return nullptr;}
 bool ready(uint32_t now)const{return !attempted||uint32_t(now-last)>=wait;}
 void put(const char*call,const char*origin,const char*destination,int http,uint32_t now,uint32_t ttl,uint32_t delay=60000){auto&e=entries[next++%4];strcpy(e.callsign,call);strcpy(e.origin,origin);strcpy(e.destination,destination);e.http=http;e.at=now;e.ttl=ttl;last=now;wait=delay;attempted=true;}
 void put(const char*call,const char*destination,int http,uint32_t now,uint32_t ttl,uint32_t delay=60000){put(call,"",destination,http,now,ttl,delay);}
};
