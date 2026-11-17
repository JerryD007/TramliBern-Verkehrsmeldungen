#include <Adafruit_GFX.h>
#include "../RundeUhr/LaunchRender.h"
#include <cassert>
#include <iostream>
int main(){GFXcanvas8 g(240,240);LaunchData d;time_t now=1794850000;for(int k=0;k<10;k++){d.valid=k>0;d.count=k>1?1:0;d.received=k==3?now-7201:now;auto&e=d.events[0];e.net=k==4?0:k==5?now-60:now+36000;strcpy(e.name,"Sehr langer Raketenname | Testmission mit Sonderzeichen Zürich und Erweiterung");strcpy(e.status,k==6?"Hold":"Go");strcpy(e.precision,k==7?"HR":k==8?"DAY":k==9?"MTH":"MIN");renderLaunch(g,d,now,k!=2);for(int y=0;y<240;y++)for(int x=0;x<240;x++)if(g.getPixel(x,y))assert((x-120)*(x-120)+(y-120)*(y-120)<=118*118);}std::cout<<"PASS launch circle bounds: loading, empty, long name, offline/stale, unknown/past time, hold, hour/day/month precision\n";}
