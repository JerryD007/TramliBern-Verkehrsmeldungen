#pragma once
#include "../../RundeUhr/Render.h"
// Source-independent renderer. No network requests and no source authorization assumed.
enum class FireStatus { Unconfigured, Demo, Data, Empty, Unavailable };
struct FireEntry {
 FireStatus status=FireStatus::Unconfigured;
 const char* town="";
 const char* message="";
 const char* occurred="";
 const char* delay=""; // supplied by verified adapter; never infer live freshness
};
inline uint32_t fireScrollOffset(uint32_t elapsed,int width,int viewport=204){
 if(width<=viewport)return 0;
 const uint32_t travel=uint32_t(width-viewport),duration=travel*50; // 20 pixels/s
 const uint32_t phase=elapsed%(duration+4000);
 if(phase<2000)return 0;
 if(phase>=2000+duration)return travel;
 return (phase-2000)/50;
}
class FireClip:public Adafruit_GFX {
 Adafruit_GFX&target;
 public: explicit FireClip(Adafruit_GFX&g):Adafruit_GFX(240,240),target(g){}
 void drawPixel(int16_t x,int16_t y,uint16_t color) override {
  if(x>=18&&x<222&&y>=130&&y<162)target.drawPixel(x,y,color);
 }
};
inline void fireTruck(Adafruit_GFX&g){
 g.fillRect(78,65,62,24,ORANGE);g.fillRoundRect(141,68,23,21,3,ORANGE);
 g.fillRect(146,72,12,9,BG);g.fillRect(83,70,15,13,BG);g.fillRect(103,70,30,13,BG);
 g.drawFastHLine(81,58,68,ORANGE);g.drawFastHLine(81,62,68,ORANGE);
 for(int x=82;x<149;x+=9)g.drawFastVLine(x,58,5,ORANGE);
 g.fillCircle(91,90,7,BG);g.fillCircle(91,90,5,ORANGE);
 g.fillCircle(150,90,7,BG);g.fillCircle(150,90,5,ORANGE);
}
inline void renderFeuerwehr(Adafruit_GFX&g,const FireEntry&e,uint32_t elapsed){
 g.fillScreen(BG);g.setTextWrap(false);g.drawCircle(120,120,116,ORANGE);
 txt(g,"FEUERWEHR",27,2,WHITE);fireTruck(g);
 if(e.status==FireStatus::Unconfigured){txt(g,"Datenquelle",113,7,WHITE);txt(g,"nicht verbunden",145,2,SILVER);return;}
 if(e.status==FireStatus::Unavailable){txt(g,"Daten nicht",113,7,WHITE);txt(g,"verfuegbar",145,7,WHITE);return;}
 if(e.status==FireStatus::Empty){txt(g,"Keine Meldungen",115,2,WHITE);txt(g,"im gewaehlten Gebiet",148,0,SILVER);return;}
 String town=latin(e.town);txt(g,town,101,town.length()>10?2:7,WHITE);
 g.fillRect(18,130,204,32,CYAN);
 FireClip clip(g);clip.setTextWrap(false);clip.setFont(&FreeSansBold12pt7b);clip.setTextSize(1);clip.setTextColor(BG);
 String message=latin(e.message);int16_t x1,y1;uint16_t w,h;clip.getTextBounds(message.c_str(),0,0,&x1,&y1,&w,&h);
 int x=w<=204?120-int(w)/2:18-int(fireScrollOffset(elapsed,w));
 clip.setCursor(x-x1,136-y1);clip.print(message);
 txt(g,e.occurred,177,0,WHITE);
 txt(g,e.delay,194,0,YELLOW);
 if(e.status==FireStatus::Demo)txt(g,"DEMO",215,0,ORANGE);
}
