#include <Adafruit_GFX.h>
#include "../RundeUhr/IssRender.h"
#include <cassert>
#include <iostream>
int main(){Config c;IssData i;time_t now=1794850218;GFXcanvas8 g(240,240);for(int mode=0;mode<5;mode++){i.valid=mode>0;i.positionAt=mode==3?now-100:now;i.altitude=420;i.lon=mode==2?c.lon+180:c.lon;i.lat=mode==2?-c.lat:c.lat;renderIss(g,c,i,now,mode!=4);for(int p=0;p<57600;p++)assert((p%240-120)*(p%240-120)+(p/240-120)*(p/240-120)<=119*119||g.getBuffer()[p]==BG);}std::cout<<"PASS ISS render bounds: loading, overhead, below horizon, stale, offline\n";}
