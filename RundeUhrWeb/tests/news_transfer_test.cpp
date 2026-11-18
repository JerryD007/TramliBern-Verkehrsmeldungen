#include <Adafruit_GFX.h>
#include "../RundeUhr/NewsRender.h"
#include "../RundeUhr/DisplayTransfer.h"
#include <cassert>
#include <vector>
struct Display {int start=0,end=0,windows=0,x=0,y=0,w=0,h=0;std::vector<uint16_t> pixels;void startWrite(){start++;}void endWrite(){end++;}void setAddrWindow(int a,int b,int c,int d){windows++;x=a;y=b;w=c;h=d;}void writePixels(uint16_t*p,int n){pixels.insert(pixels.end(),p,p+n);}};
int main(){GFXcanvas8 full(240,240),part(240,240);Config c;c.automatic=true;NewsData n;n.valid=true;n.count=1;n.received=1794890000;n.items[0].at=n.received;strcpy(n.items[0].title,"Dlouhá zpráva: německé ÄÖÜ a české čřž na displeji");time_t now=n.received;
 renderNews(part,c,n,now,true,0);
 for(unsigned ms:{20U,1000U,1500U,2000U,3500U,6000U,20000U}){renderNews(full,c,n,now,true,ms);renderNews(part,c,n,now,true,ms,true);assert(!memcmp(full.getBuffer(),part.getBuffer(),57600));}
 for(bool band:{false,true}){Display d;transferCanvas(d,part,palette,band);assert(d.start==1&&d.end==1&&d.windows==1);assert(d.pixels.size()==size_t(band?184*32:240*240));int k=0;for(int y=d.y;y<d.y+d.h;y++)for(int x=d.x;x<d.x+d.w;x++)assert(d.pixels[k++]==palette[part.getPixel(x,y)]);}
 auto frame=newsFrame(c,n,now,2000);auto key=newsFrameKey(n,frame,now,true);assert(key==newsFrameKey(n,newsFrame(c,n,now,3000),now,true));n.items[0].at--;assert(key!=newsFrameKey(n,frame,now,true));
 NewsScrollClock clock;clock.reset(100);clock.frame(120);assert(clock.elapsed==20);clock.frame(5120);assert(clock.elapsed==53);clock.reset(0xfffffff0U);clock.frame(4);assert(clock.elapsed==20);
 clock.reset(0);unsigned duration=newsHeadlineDuration(n.items[0].title);clock.frame(duration+5000);assert(clock.elapsed==33&&clock.elapsed<duration);
 puts("PASS partial/full pixels identical; one SPI window; 5888/57600 pixels; metadata invalidation; frame clock stall/wrap");}
