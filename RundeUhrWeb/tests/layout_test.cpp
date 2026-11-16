#include <Adafruit_GFX.h>
#include "../RundeUhr/AircraftRender.h"
#include "fixtures/aircraft_render_rc9.h"
#include <cassert>
#include <cstring>
#include <fstream>
void save(GFXcanvas8&g,const char*path){std::ofstream o(path,std::ios::binary);o<<"P6\n240 240\n255\n";for(int i=0;i<57600;i++){auto p=palette[g.getBuffer()[i]&15];char rgb[]={char(((p>>11)&31)*255/31),char(((p>>5)&63)*255/63),char((p&31)*255/31)};o.write(rgb,3);}}
int main(){AircraftData a;a.found=a.checked=a.altValid=a.speedValid=true;a.error[0]=0;a.received=a.positionAt=1000;strcpy(a.callsign,"LOT416");a.distance=7.3;a.altitude=7978;a.speed=909;GFXcanvas8 g(240,240),old(240,240);
for(int route=0;route<4;route++)for(auto kind:{AircraftKind::Plane,AircraftKind::Helicopter,AircraftKind::Unknown})for(bool online:{true,false})for(int fields=0;fields<4;fields++){
 a.kind=kind;a.altValid=fields&1;a.speedValid=fields&2;strcpy(a.origin,route&1?"Geneva":"");strcpy(a.destination,route&2?"Warsaw":"");renderAircraft(g,a,1000,online);oldRenderAircraft(old,a,1000,online);
 // Lower route section preserved pixel for pixel, including one endpoint.
 if(route)assert(!memcmp(g.getBuffer()+136*240,old.getBuffer()+136*240,(240-136)*240));
 for(int i=0;i<57600;i++)if((i%240-120)*(i%240-120)+(i/240-120)*(i/240-120)>119*119)assert(g.getBuffer()[i]==BG);
 if(kind!=AircraftKind::Unknown){int wanted=0,wrong=0;for(int y=50;y<121;y++)for(int x=76;x<165;x++){auto p=g.getBuffer()[y*240+x];wanted+=p==(kind==AircraftKind::Plane?YELLOW:RED);wrong+=p==(kind==AircraftKind::Plane?RED:YELLOW);}assert(wanted>50&&!wrong);}
 if(online&&fields==3&&(route==0||route==3)&&kind!=AircraftKind::Unknown){const char*path=kind==AircraftKind::Plane?(route?"preview/rc15_plane_route.ppm":"preview/rc15_plane_empty.ppm"):(route?"preview/rc15_heli_route.ppm":"preview/rc15_heli_empty.ppm");save(g,path);}
}
// Extreme accepted telemetry and long identifier still remain inside the circle.
strcpy(a.callsign,"ABCDEFGH");a.altitude=18000;a.speed=2000;a.distance=999.9;a.altValid=a.speedValid=true;for(int route=0;route<2;route++){strcpy(a.origin,route?"Lanzarote Island":"");a.destination[0]=0;renderAircraft(g,a,1000,true);for(int i=0;i<57600;i++)if((i%240-120)*(i%240-120)+(i/240-120)*(i/240-120)>119*119)assert(g.getBuffer()[i]==BG);}
puts("PASS RC15 layout: yellow plane/red helicopter, route area pixel-identical, missing endpoints, no route expanded, missing telemetry, offline, unknown kind, extreme values and circle bounds.");}
