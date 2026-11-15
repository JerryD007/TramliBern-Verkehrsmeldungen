#pragma once
#include <ArduinoJson.h>
#include "AircraftModel.h"
inline bool aircraftNumber(JsonVariantConst v){return v.is<double>()&&isfinite(v.as<double>());}
inline void aircraftText(char*dest,size_t cap,JsonVariantConst v){const char*s=v.is<const char*>()?v.as<const char*>():"";size_t n=0;for(;*s&&n+1<cap;s++)if(uint8_t(*s)>=32&&uint8_t(*s)<127)dest[n++]=*s;while(n&&dest[n-1]==' ')n--;dest[n]=0;}
// ADS-B emitter category: A7 rotorcraft, A1-A6 aeroplane categories.
// Missing, malformed and other categories remain unknown; never infer from speed/callsign.
inline AircraftKind aircraftKind(JsonVariantConst v){
 const char*s=v.is<const char*>()?v.as<const char*>():"";
 if(strlen(s)!=2||s[0]!='A')return AircraftKind::Unknown;
 if(s[1]=='7')return AircraftKind::Helicopter;
 if(s[1]>='1'&&s[1]<='6')return AircraftKind::Plane;
 return AircraftKind::Unknown;
}
inline bool parseAircraft(JsonDocument&d,AircraftData&out,const Config&c,time_t now){
 if(!d["ac"].is<JsonArray>()||!aircraftNumber(d["now"]))return false;
 double source=d["now"].as<double>();if(source>1e12)source/=1000.;if(source<now-120||source>now+30)return false;
 if(!d["msg"].isNull()&&strcmp(d["msg"]|"","No error"))return false;
 AircraftData next;next.checked=true;next.received=now;next.error[0]=0;double best=c.aircraftRadius+0.001;
 for(JsonObjectConst a:d["ac"].as<JsonArrayConst>()){
  if(!aircraftNumber(a["lat"])||!aircraftNumber(a["lon"])||!aircraftNumber(a["seen_pos"]))continue;
  double lat=a["lat"],lon=a["lon"],age=a["seen_pos"];if(lat < -90||lat>90||lon < -180||lon>180||age<0||age>60||now-(source-age)>60)continue;
  if(a["alt_baro"].is<const char*>()&&!strcmp(a["alt_baro"],"ground"))continue;
  double distance=aircraftDistance(c.aircraftLat,c.aircraftLon,lat,lon);if(distance>c.aircraftRadius||distance>=best)continue;
  next.approaching=aircraftNumber(a["track"])&&aircraftNumber(a["gs"])&&aircraftClosing(lat,lon,c.aircraftLat,c.aircraftLon,a["track"].as<double>(),a["gs"].as<double>());
  next.kind=aircraftKind(a["category"]);next.found=true;best=distance;next.distance=distance;next.positionAt=time_t(source-age);
  aircraftText(next.callsign,sizeof(next.callsign),a["flight"]);aircraftText(next.hex,sizeof(next.hex),a["hex"]);aircraftText(next.registration,sizeof(next.registration),a["r"]);aircraftText(next.type,sizeof(next.type),a["t"]);
  next.altValid=aircraftNumber(a["alt_baro"])&&a["alt_baro"].as<double>()>=-2000&&a["alt_baro"].as<double>()<=100000;next.altitude=next.altValid?a["alt_baro"].as<double>()*.3048:0;
  next.speedValid=aircraftNumber(a["gs"])&&a["gs"].as<double>()>=0&&a["gs"].as<double>()<=2000;next.speed=next.speedValid?a["gs"].as<double>()*1.852:0;
 }
 next.heapBefore=out.heapBefore;next.largestBefore=out.largestBefore;next.http=out.http;next.tls=out.tls;out=next;return true;
}
