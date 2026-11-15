#include <Adafruit_GFX.h>
#include "../RundeUhr/AircraftRender.h"
#include <fstream>
int main(){for(int k=1;k<=2;k++){AircraftData a;a.kind=k==1?AircraftKind::Plane:AircraftKind::Helicopter;a.found=a.checked=a.altValid=a.speedValid=true;a.error[0]=0;a.received=a.positionAt=1000;strcpy(a.callsign,k==1?"SWR123":"HELI01");a.distance=k==1?12.4:3.2;a.altitude=k==1?9450:850;a.speed=k==1?780:190;GFXcanvas8 g(240,240);renderAircraft(g,a,1000,true);std::ofstream o(k==1?"preview/icon_plane.ppm":"preview/icon_helicopter.ppm",std::ios::binary);o<<"P6\n240 240\n255\n";for(int i=0;i<57600;i++){auto p=palette[g.getBuffer()[i]&15];char b[]={char(((p>>11)&31)*255/31),char(((p>>5)&63)*255/63),char((p&31)*255/31)};o.write(b,3);}}}
