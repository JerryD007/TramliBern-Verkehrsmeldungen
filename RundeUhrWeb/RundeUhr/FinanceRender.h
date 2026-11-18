#pragma once
#include "Render.h"
#include "FinanceModel.h"
inline String quoteNumber(double n,bool btc){char b[32];snprintf(b,sizeof(b),btc?(n<1000000?"%.2f":"%.0f"):(n<.1?"%.6f":n<100?"%.4f":"%.2f"),n);if(strlen(b)>12)snprintf(b,sizeof(b),"%.4g",n);return String(b);}
// Native pixels avoid font/encoding dependencies. Green rise, red fall.
inline void financeArrow(Adafruit_GFX&g,const Quote&q,time_t now,bool btc,bool online,int x,int y){
 if(!online||quoteStale(q,now,btc)||!q.trend)return;
 const uint16_t color=q.trend>0?GREEN:RED;
 g.fillRect(x-2,y-6,5,13,color);
 if(q.trend>0)g.fillTriangle(x,y-11,x-7,y-3,x+7,y-3,color);
 else g.fillTriangle(x,y+11,x-7,y+3,x+7,y+3,color);
}
inline void financeValue(Adafruit_GFX&g,String value,int y,bool btc){
 const GFXfont* fonts[]={&FreeSansBold24pt7b,&FreeSansBold18pt7b,&FreeSansBold12pt7b,&FreeSansBold9pt7b};
 g.setTextSize(1);int16_t x1,y1;uint16_t w,h;
 for(int i=btc?0:2;i<4;i++){g.setFont(fonts[i]);g.getTextBounds(value.c_str(),0,0,&x1,&y1,&w,&h);if(w<=174)break;}
 // Values formatted by quoteNumber fit this column even at the accepted extremes.
 if(w>174){g.setFont(nullptr);g.getTextBounds(value.c_str(),0,0,&x1,&y1,&w,&h);}
 g.setTextColor(WHITE);g.setCursor(108-int(w)/2-x1,y-y1);g.print(value);g.setFont(nullptr);
}
// Reuse a palette slot unused on this view; keep the 4-bit framebuffer.
static const uint16_t bitcoinPalette[]={0x0000,0xffff,0x07ff,0xfc10,0x4208,0xffe0,0xb35f,0xf800,0x47e9,0x9bd3,0xc618,0x8410,0xfd20,0xfdb8,0x04b5,0x9800};
inline void bitcoinCurve(Adafruit_GFX&g,const BitcoinHistory&h){
 if(h.count<2)return;
 float lo=h.prices[0],hi=lo;for(unsigned i=1;i<h.count;i++){lo=std::min(lo,h.prices[i]);hi=std::max(hi,h.prices[i]);}
 // Minimum vertical range avoids exaggerating sub-cent changes. X follows real observation time.
 double range=std::max(double(hi-lo),std::max(double(hi)*0.001,0.01));double mid=(double(hi)+lo)/2;
 time_t span=h.times[h.count-1]-h.times[0];if(span<=0)return;
 int px=0,py=0;for(unsigned i=0;i<h.count;i++){int x=20+int(double(h.times[i]-h.times[0])*200/double(span));int y=132-int((h.prices[i]-mid)*32/range);if(i)g.drawLine(px,py,x,y,SHADOW);px=x;py=y;}
}
inline void renderFinance(Adafruit_GFX&g,const Config&c,const FinanceData&f,time_t now,bool online){
 activePalette=c.view==10?bitcoinPalette:palette;g.fillScreen(BG);g.setTextWrap(false);int accent=c.view==10?GOLD:CYAN;g.drawCircle(120,120,116,accent);g.drawCircle(120,120,115,accent);
 if(c.view==10){
  bitcoinCurve(g,f.history);
  // Bitcoin symbol drawn using the existing font plus two vertical strokes.
  txt(g,"B",36,5,GOLD);g.fillRect(111,28,3,9,GOLD);g.fillRect(121,28,3,9,GOLD);g.fillRect(111,72,3,9,GOLD);g.fillRect(121,72,3,9,GOLD);
  String value=f.btc.valid?quoteNumber(f.btc.value,true):"--";financeValue(g,value,104,true);financeArrow(g,f.btc,now,true,online,211,120);
  txt(g,c.btcCurrency,155,3,WHITE);txt(g,f.btc.valid?"Abruf "+fmtTime(f.btc.received):"Warte auf Daten",195,0,SILVER);
  txt(g,!online?"Offline":quoteStale(f.btc,now,true)?"Nicht aktuell":"Coinbase",213,0,quoteStale(f.btc,now,true)?ORANGE:DIM);
 }else{
  txt(g,"Currency",28,2,CYAN);bool stale=!online;
  for(int i=0;i<3;i++){int y=57+i*48;txt(g,String(c.fxBase[i])+" > "+c.fxQuote[i],y,2,CYAN);financeValue(g,f.fx[i].valid?"1 = "+quoteNumber(f.fx[i].value,false):"--",y+21,false);financeArrow(g,f.fx[i],now,false,online,205,y+31);stale|=quoteStale(f.fx[i],now,false);}
  txt(g,"Frankfurter - Referenz",201,0,DIM);txt(g,stale?"Nicht aktuell":"Datum: Web-App",214,0,stale?ORANGE:SILVER);
 }
}
