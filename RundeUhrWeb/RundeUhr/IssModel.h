#pragma once
#include "Model.h"
struct IssData {bool valid=false;double lat=0,lon=0,altitude=0,speed=0;time_t positionAt=0,received=0;int http=0;char error[80]="";time_t attempted=0,tlsFailureAt=0;uint32_t heapBefore=0,largestBefore=0;int tlsCode=0;char tlsDetail[192]="",tlsFailure[192]="";};
inline bool issStale(const IssData&i,time_t now){return !i.valid||!timeValid(now)||now<i.positionAt||now-i.positionAt>90;}
struct IssLook {double distance,azimuth,elevation;};
// Spherical Earth approximation; observer altitude omitted. Not a pass predictor.
inline IssLook issLook(const IssData&i,double lat,double lon){
 const double rad=M_PI/180,r=6371,rs=r+i.altitude,p=lat*rad,q=i.lat*rad,d=(i.lon-lon)*rad;
 const double east=rs*cos(q)*sin(d),north=rs*(cos(p)*sin(q)-sin(p)*cos(q)*cos(d)),up=rs*(sin(p)*sin(q)+cos(p)*cos(q)*cos(d))-r;
 const double horizontal=hypot(east,north);return {hypot(horizontal,up),fmod(atan2(east,north)/rad+360,360),atan2(up,horizontal)/rad};
}
