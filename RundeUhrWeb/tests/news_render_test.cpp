#include <cassert>
#include <Adafruit_GFX.h>
#include "../RundeUhr/NewsRender.h"
int main(){
 char german[32];size_t length=newsDisplayText("ÄÖÜ äöü ß",german,sizeof(german));const unsigned char expected[]={142,153,154,32,132,148,129,32,225};assert(length==sizeof(expected));for(unsigned i=0;i<sizeof(expected);i++)assert(uint8_t(german[i])==expected[i]);
 assert(newsHeadlineDuration("12345678")==3133);
GFXcanvas8 g(240,240);Config c;NewsData n;time_t now=1794890000;n.valid=true;n.count=3;n.received=now;for(int i=0;i<3;i++){memset(n.items[i].title,'W',255);n.items[i].title[255]=0;n.items[i].at=now;}
 for(int phase=0;phase<400000;phase+=977){renderNews(g,c,n,now,true,phase);for(int y=0;y<240;y++)for(int x=0;x<240;x++)assert((x-120)*(x-120)+(y-120)*(y-120)<=119*119||!g.getBuffer()[y*240+x]);}puts("PASS scrolling news circle bounds across long headlines and rotation");}
