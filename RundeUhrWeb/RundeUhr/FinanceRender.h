#pragma once
#include "Render.h"
#include "FinanceModel.h"
inline String quoteNumber(double n,bool btc){char b[32];snprintf(b,sizeof(b),btc?(n<1000000?"%.2f":"%.0f"):(n<.1?"%.6f":n<100?"%.4f":"%.2f"),n);if(strlen(b)>12)snprintf(b,sizeof(b),"%.4g",n);return String(b);}
inline void renderFinance(Adafruit_GFX&g,const Config&c,const FinanceData&f,time_t now,bool online){
 g.fillScreen(BG);g.setTextWrap(false);int accent=c.view==10?GOLD:CYAN;g.drawCircle(120,120,116,accent);g.drawCircle(120,120,115,accent);
 if(c.view==10){
  // Bitcoin symbol drawn using the existing font plus two vertical strokes.
  txt(g,"B",36,5,GOLD);g.fillRect(111,28,3,9,GOLD);g.fillRect(121,28,3,9,GOLD);g.fillRect(111,72,3,9,GOLD);g.fillRect(121,72,3,9,GOLD);
  String value=f.btc.valid?quoteNumber(f.btc.value,true):"--";txt(g,value,104,value.length()>9?2:value.length()>7?3:5,WHITE);
  txt(g,c.btcCurrency,155,3,WHITE);txt(g,f.btc.valid?"Abruf "+fmtTime(f.btc.received):"Warte auf Daten",195,0,SILVER);
  txt(g,!online?"Offline":quoteStale(f.btc,now,true)?"Nicht aktuell":"Coinbase",213,0,quoteStale(f.btc,now,true)?ORANGE:DIM);
 }else{
  txt(g,"Currency",28,2,CYAN);bool stale=!online;
  for(int i=0;i<3;i++){int y=62+i*44;txt(g,String(c.fxBase[i])+" > "+c.fxQuote[i],y,2,CYAN);txt(g,f.fx[i].valid?"1 = "+quoteNumber(f.fx[i].value,false):"--",y+22,2,WHITE);stale|=quoteStale(f.fx[i],now,false);}
  txt(g,"Frankfurter - Referenz",201,0,DIM);txt(g,stale?"Nicht aktuell":"Datum: Web-App",214,0,stale?ORANGE:SILVER);
 }
}
