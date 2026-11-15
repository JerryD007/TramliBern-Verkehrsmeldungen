#include <Adafruit_GFX.h>
#include "FeuerwehrRender.h"
#include <cassert>
#include <fstream>
void save(GFXcanvas8&g,int frame){char p[128];snprintf(p,sizeof(p),"build/fire_frames/%03d.ppm",frame);std::ofstream o(p,std::ios::binary);o<<"P6\n240 240\n255\n";for(int i=0;i<57600;i++){auto c=palette[g.getBuffer()[i]&15];char rgb[]={char(((c>>11)&31)*255/31),char(((c>>5)&63)*255/63),char((c&31)*255/31)};o.write(rgb,3);}}
int main(){
 assert(fireScrollOffset(9999,100)==0);assert(fireScrollOffset(1999,400)==0);
 assert(fireScrollOffset(2050,400)==1);assert(fireScrollOffset(11800,400)==196);
 assert(fireScrollOffset(13799,400)==196);assert(fireScrollOffset(13800,400)==0);
 FireEntry e{FireStatus::Demo,"Ostermundigen","Automatischer Alarm","15.11. 18:42","ca. 1 h verzoegert"};GFXcanvas8 g(240,240),first(240,240);
 renderFeuerwehr(first,e,0);
 for(int i=0;i<140;i++){renderFeuerwehr(g,e,i*100);for(int k=0;k<57600;k++){
  int x=k%240,y=k/240;if((x-120)*(x-120)+(y-120)*(y-120)>119*119)assert(g.getBuffer()[k]==BG);
  if(y<130||y>=162)assert(g.getBuffer()[k]==first.getBuffer()[k]);
 }save(g,i);}
 for(auto s:{FireStatus::Unconfigured,FireStatus::Empty,FireStatus::Unavailable}){e.status=s;renderFeuerwehr(g,e,0);assert(memcmp(g.getBuffer(),first.getBuffer(),57600));}
 puts("PASS: 20 px/s scroll, start/end pauses, stationary short text, clipped glyphs, no outside-circle pixels, static fields unchanged, explicit demo/unconfigured/unavailable/empty states. Host test only.");
}
