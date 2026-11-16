#include <Adafruit_GFX.h>
#include "../RundeUhr/QuakeRender.h"
#include <cassert>
#include <iostream>
int main(){GFXcanvas8 g(240,240);QuakeData q;time_t now=1794850000;for(int k=0;k<5;k++){q.valid=k>0;q.count=k>1?1:0;q.generated=k==3?now-1000:now;q.events[0].magnitude=9.9;q.events[0].depth=1000;q.events[0].at=now;strcpy(q.events[0].place,"Sehr langer Ortsname mit Sonderzeichen Zürich und ausführlichem Zusatz");renderQuake(g,q,now,k!=4);for(int y=0;y<240;y++)for(int x=0;x<240;x++)if(g.getPixel(x,y))assert((x-120)*(x-120)+(y-120)*(y-120)<=118*118);}std::cout<<"PASS quake circle bounds: loading, empty, long place, stale, offline\n";}
