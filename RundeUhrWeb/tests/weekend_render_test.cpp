#include <Adafruit_GFX.h>
#include "../RundeUhr/Render.h"
#include <fstream>
#include <cassert>
void save(GFXcanvas8&g,const char*name){std::ofstream o(name,std::ios::binary);o<<"P6\n240 240\n255\n";for(int y=0;y<240;y++)for(int x=0;x<240;x++){uint16_t p=activePalette[g.getBuffer()[y*240+x]&15];char rgb[]={char((p>>11)*255/31),char(((p>>5)&63)*255/63),char((p&31)*255/31)};o.write(rgb,3);if((x-120)*(x-120)+(y-120)*(y-120)>119*119)assert(g.getBuffer()[y*240+x]==0);}}
int main(){setenv("TZ",ZURICH_TZ,1);tzset();Config c;c.view=7;time_t now;assert(parseDate("2026-11-20",now));now+=16*3600;GFXcanvas8 g(240,240);for(int i=0;i<20;i++){render(g,c,Weather(),Moon(),now,true,false,"","",i*180);save(g,"preview/weekend_large.ppm");}c.weekendTransition=true;for(int i=0;i<6;i++){render(g,c,Weather(),Moon(),now,true,false,"","",i*550);char f[100];sprintf(f,"preview/weekend_toast_%d.ppm",i);save(g,f);}render(g,c,Weather(),Moon(),now+56*3600,true,false,"","",0);assert(activePalette==palette);save(g,"preview/weekend_after.ppm");puts("PASS renderer: enlarged mugs, six animation phases, circular clipping, palette restoration");}
