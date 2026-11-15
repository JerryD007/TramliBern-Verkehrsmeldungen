#pragma once
#include "Model.h"
struct AircraftData {
 char destination[97]="";int routeHTTP=0;
 bool approaching=false;
 bool checked=false,found=false,altValid=false,speedValid=false,memoryDeferred=false;
 char callsign[17]="",hex[9]="",registration[17]="",type[17]="",error[96]="Zatim bez dat";
 double distance=0,altitude=0,speed=0;time_t received=0,positionAt=0;
 uint32_t heapBefore=0,largestBefore=0;int http=0,tls=0;
};
inline bool aircraftConfigEqual(const Config&a,const Config&b){return a.aircraftLat==b.aircraftLat&&a.aircraftLon==b.aircraftLon&&a.aircraftRadius==b.aircraftRadius&&!strcmp(a.aircraftPlace,b.aircraftPlace);}
inline bool aircraftStale(const AircraftData&a,time_t now){return !a.checked||a.error[0]||now<a.received||now-a.received>90||(a.found&&(now<a.positionAt||now-a.positionAt>90));}
inline double aircraftDistance(double lat1,double lon1,double lat2,double lon2){const double rad=3.141592653589793/180;double x=sin((lat2-lat1)*rad/2),y=sin((lon2-lon1)*rad/2);double h=x*x+cos(lat1*rad)*cos(lat2*rad)*y*y;return 6371.0088*2*asin(sqrt(fmin(1.,fmax(0.,h))));}

// Heading is true track over ground. Positive projection toward the observer means closing.
inline bool aircraftClosing(double lat,double lon,double observerLat,double observerLon,double track,double knots){
 if(!isfinite(track)||track<0||track>=360||!isfinite(knots)||knots<10)return false;
 const double rad=3.141592653589793/180;double dl=(observerLon-lon)*rad;
 double bearing=atan2(sin(dl)*cos(observerLat*rad),cos(lat*rad)*sin(observerLat*rad)-sin(lat*rad)*cos(observerLat*rad)*cos(dl));
 return cos(track*rad-bearing)>.15;
}
inline bool aircraftPriorityActive(const Config&c,const AircraftData&a,time_t now,bool online){return c.automatic&&c.aircraftPriority&&online&&a.found&&(c.aircraftInRange||a.approaching)&&!aircraftStale(a,now)&&now-a.positionAt<=60;}
inline uint16_t aircraftCycleMask(const Config&c){return c.aircraftPriority?uint16_t(mergedScreenMask(c.mask)&~4096):mergedScreenMask(c.mask);}
inline int aircraftDisplayView(const Config&c,const AircraftData&a,time_t now,bool online){
 if(aircraftPriorityActive(c,a,now,online))return 12;
 if(c.automatic&&c.aircraftPriority&&c.view==12){auto mask=aircraftCycleMask(c);return mask?nextView(12,mask):0;}
 return c.view;
}
