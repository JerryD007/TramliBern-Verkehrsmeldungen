#pragma once
#include "Render.h"
#include "NewsModel.h"
// Clip individual pixels so letters enter and leave continuously, including partial glyphs.
class NewsBand:public Adafruit_GFX {
 Adafruit_GFX& target;
 public:explicit NewsBand(Adafruit_GFX&g):Adafruit_GFX(240,240),target(g){}
 void glyph(int16_t x,int16_t y,uint8_t code){
  if(code>=1&&code<=30){for(int row=0;row<8;row++)for(int col=0;col<5;col++)if(NEWS_CZECH_ROWS[code-1][row]&(1<<(4-col)))fillRect(x+col*4,y+row*4,4,4,WHITE);}
  else {setCursor(x,y);write(code);}
 }
 void drawPixel(int16_t x,int16_t y,uint16_t color)override{if(x>=28&&x<212&&y>=100&&y<132)target.drawPixel(x,y,color);}
};
inline void renderNews(Adafruit_GFX&g,const Config&c,const NewsData&n,time_t now,bool online,uint32_t ms,bool partial=false){
 if(partial)g.fillRect(28,100,184,32,BG);else {g.fillScreen(BG);g.setTextWrap(false);g.drawCircle(120,120,116,CYAN);txt(g,"NACHRICHTEN",37,2,CYAN);}
 if(!n.valid||!n.count){txt(g,"Keine Nachrichten",110,1);txt(g,n.error[0]?"Abruf fehlgeschlagen":"Daten werden geladen",140,0,ORANGE);return;}
 // A fixed horizontal text band stays inside the circular display.
 auto frame=newsFrame(c,n,now,ms);if(frame.index<0){txt(g,"Keine neuen Meldungen",120,0);return;}
 uint32_t phase=frame.phase;auto&e=n.items[frame.index];
 char headline[256];size_t length=newsDisplayText(e.title,headline,sizeof(headline));
 g.setFont(nullptr);g.setTextSize(4);g.cp437(true);g.setTextColor(WHITE);int width=length*NEWS_ADVANCE,travel=width>184?width-184:0;int offset=phase<1500?0:int((phase-1500)*NEWS_SPEED/1000);if(offset>travel)offset=travel;
 NewsBand band(g);band.setTextWrap(false);band.setTextSize(4);band.cp437(true);band.setTextColor(WHITE);
 for(size_t j=0;j<length;j++){int x=28+int(j)*NEWS_ADVANCE-offset;if(x<212&&x+24>28){band.glyph(x,100,uint8_t(headline[j]));}}
 g.cp437(false);
 if(partial)return;
 tm t;localtime_r(&e.at,&t);char b[32];strftime(b,sizeof(b),"%d.%m. %H:%M",&t);txt(g,b,164,1,SILVER);txt(g,NEWS_SOURCES[n.source].credit,190,1,CYAN);if(!online||newsStale(n,now)||n.error[0])txt(g,"Nicht aktuell",211,0,ORANGE);
}
