#include <cassert>
#include <Adafruit_GFX.h>
#include "../RundeUhr/NewsRender.h"
int main(){
 char german[32];size_t length=newsDisplayText("ÄÖÜ äöü ß",german,sizeof(german));const unsigned char expected[]={142,153,154,32,132,148,129,32,225};assert(length==sizeof(expected));for(unsigned i=0;i<sizeof(expected);i++)assert(uint8_t(german[i])==expected[i]);
 char cz[64];assert(newsDisplayText("áčďéěíňóřšťúůýžÁČĎÉĚÍŇÓŘŠŤÚŮÝŽ",cz,sizeof(cz))==30);for(int k=0;k<30;k++)assert(uint8_t(cz[k])==k+1);
 GFXcanvas8 a(240,240),b(240,240);NewsBand left(a),shifted(b);for(auto* band:{&left,&shifted}){band->setTextSize(4);band->setTextColor(WHITE);band->setTextWrap(false);}
 left.setCursor(28,100);left.write('W');shifted.setCursor(27,100);shifted.write('W');
 for(int y=100;y<132;y++)for(int x=28;x<211;x++)assert(b.getPixel(x,y)==a.getPixel(x+1,y));
 for(int y=100;y<132;y++)assert(!b.getPixel(27,y)&&!b.getPixel(212,y));
 for(int code=1;code<=30;code++){a.fillScreen(0);b.fillScreen(0);left.glyph(28,100,code);shifted.glyph(27,100,code);for(int y=100;y<132;y++)for(int x=28;x<211;x++)assert(b.getPixel(x,y)==a.getPixel(x+1,y));}
 assert(newsHeadlineDuration("12345678")==3088);
GFXcanvas8 g(240,240);Config c;NewsData n;time_t now=1794890000;n.valid=true;n.count=3;n.received=now;for(int i=0;i<3;i++){memset(n.items[i].title,'W',255);n.items[i].title[255]=0;n.items[i].at=now;}
 renderNews(a,c,n,now,true,1600);renderNews(b,c,n,now,true,1620);
 for(int y=0;y<240;y++)if(y<100||y>=132)for(int x=0;x<240;x++)assert(a.getPixel(x,y)==b.getPixel(x,y));
 for(int phase=0;phase<400000;phase+=977){renderNews(g,c,n,now,true,phase);for(int y=0;y<240;y++)for(int x=0;x<240;x++)assert((x-120)*(x-120)+(y-120)*(y-120)<=119*119||!g.getBuffer()[y*240+x]);}puts("PASS news: German glyphs, 90px/s timing, one-pixel clipped movement, unchanged pixels outside band, long headline bounds");}
