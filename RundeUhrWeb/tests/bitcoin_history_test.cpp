#include <Adafruit_GFX.h>
#include "../RundeUhr/FinanceRender.h"
#include <cassert>
int main(){BitcoinHistory h;Quote q;q.valid=true;q.error[0]=0;q.received=100;q.value=50000;h.add(q);h.add(q);assert(h.count==1);strcpy(q.error,"failed");q.received++;h.add(q);assert(h.count==1);q.error[0]=0;q.value=NAN;h.add(q);assert(h.count==1);
 for(int i=1;i<=40;i++){q.received=100+i*120;q.value=50000+i%5*30;h.add(q);}assert(h.count==32&&h.times[0]==1180&&h.times[31]==4900);
 GFXcanvas8 g(240,240);g.fillScreen(BG);bitcoinCurve(g,h);int pixels=0;for(int y=0;y<240;y++)for(int x=0;x<240;x++)if(g.getPixel(x,y)){assert(x>=20&&x<=220&&y>=116&&y<=148&&g.getPixel(x,y)==SHADOW);pixels++;}assert(pixels>100);
 for(unsigned i=0;i<h.count;i++)h.prices[i]=50000;g.fillScreen(BG);bitcoinCurve(g,h);assert(g.getPixel(20,132)==SHADOW&&g.getPixel(220,132)==SHADOW);
 Config c;c.view=10;FinanceData f;f.btc=q;f.history=h;renderFinance(g,c,f,5000,true);assert(activePalette[SHADOW]==0x9800);c.view=11;renderFinance(g,c,f,5000,true);assert(activePalette==palette);
 puts("PASS BTC history: failures/duplicates/nonfinite rejected, bounded ring, flat/rising/falling curve, graph bounds and palette restore");}
