#include <Adafruit_GFX.h>
#include "../RundeUhr/AircraftRender.h"
#include "fixtures/aircraft_render_rc9.h"
#include <cassert>
#include <cstring>
int main(){AircraftData a;a.found=a.checked=a.altValid=a.speedValid=true;a.error[0]=0;a.received=a.positionAt=1000;strcpy(a.callsign,"TAP55E");a.distance=8.8;a.altitude=11278;a.speed=724;GFXcanvas8 g(240,240),old(240,240);
for(int route=0;route<4;route++)for(auto kind:{AircraftKind::Plane,AircraftKind::Helicopter,AircraftKind::Unknown})for(bool online:{true,false}){a.kind=kind;strcpy(a.origin,route&1?"Bern":"");strcpy(a.destination,route&2?"Hamburg":"");renderAircraft(g,a,1000,online);oldRenderAircraft(old,a,1000,online);if(route)assert(!memcmp(g.getBuffer(),old.getBuffer(),57600));else assert(memcmp(g.getBuffer(),old.getBuffer(),57600));for(int i=0;i<57600;i++)if((i%240-120)*(i%240-120)+(i/240-120)*(i/240-120)>119*119)assert(g.getBuffer()[i]==BG);}
puts("PASS layout: with either/both endpoints pixel-identical to RC9; without endpoints expanded; plane/helicopter/unknown and offline states within circular bounds.");}
