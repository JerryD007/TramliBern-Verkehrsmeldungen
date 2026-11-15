#include <Adafruit_GFX.h>
#include "../RundeUhr/AircraftRender.h"
#include <fstream>
int main(){for(int k=1;k<=1;k++){AircraftData a;a.kind=k==1?AircraftKind::Plane:AircraftKind::Helicopter;a.found=a.checked=a.altValid=a.speedValid=true;a.error[0]=0;a.received=a.positionAt=1000;strcpy(a.callsign,"TAP55E");a.distance=8.8;a.altitude=11278;a.speed=724;GFXcanvas8 g(240,240);renderAircraft(g,a,1000,true);std::ofstream o("preview/rc10_expanded.ppm",std::ios::binary);o<<"P6\n240 240\n255\n";for(int i=0;i<57600;i++){auto p=palette[g.getBuffer()[i]&15];char b[]={char(((p>>11)&31)*255/31),char(((p>>5)&63)*255/63),char((p&31)*255/31)};o.write(b,3);}}}
