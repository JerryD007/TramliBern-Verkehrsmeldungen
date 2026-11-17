#pragma once
#include "Render.h"
#include "QuakeModel.h"
inline void renderQuake(Adafruit_GFX&g,const QuakeData&q,time_t now,bool online,const Config&c=Config()){
 static const uint16_t quakePalette[]={0x0000,0xffff,0x07f5,0xfc10,0x4208,0xffe0,0xb35f,0xf800,0x47e9,0x9bd3,0xc618,0x8410,0xfd20,0xfdb8,0x04b5,0x2104};
 activePalette=quakePalette;g.fillScreen(BG);g.setTextWrap(false);g.drawCircle(120,120,116,RED);
 txt(g,"ERDBEBEN",25,2,CYAN);char filter[48];snprintf(filter,sizeof(filter),"M%.1f+ / %u min",c.quakeMagnitude,c.quakeWindow);txt(g,filter,49,0,SILVER);
 if(!q.valid){txt(g,"Warte auf Daten",103,1);txt(g,!online?"WLAN offline":q.error[0]?"Abruf fehlgeschlagen":"USGS wird geladen",143,0,ORANGE);}
 else if(!q.count){txt(g,"Keine passenden",94,1);txt(g,"Erdbeben",119,7);txt(g,c.quakeRadius?"Umkreisfilter aktiv":"Weltweiter Filter",157,0,SILVER);txt(g,"Filter im Web pruefen",178,0,SILVER);}
 else {const auto&e=q.events[(uint64_t(now)/8)%q.count];char b[128];snprintf(b,sizeof(b),"M %.1f",e.magnitude);txt(g,b,72,3,quakeStale(q,now)?GRAY:RED);
 // Wrap enlarged region text by measured pixels, keeping date and depth clear.
 String place=latin(String(e.place));unsigned start=0;
 for(int row=0;row<2&&start<place.length();row++){
  unsigned end=start,lastSpace=start;g.setTextSize(1);g.setFont(&FreeSansBold9pt7b);
  while(end<place.length()){int16_t x,y;uint16_t w,h;String part=place.substring(start,end+1);g.getTextBounds(part.c_str(),0,0,&x,&y,&w,&h);if(w>204)break;if(place[end]==' ')lastSpace=end;end++;}
  if(row==0&&end<place.length()&&lastSpace>start)end=lastSpace;
  if(end==start)end++;
  String line=place.substring(start,end);if(row==1&&end<place.length())line=line+"..";
  txt(g,line,113+row*20,1,WHITE);start=end;while(start<place.length()&&place[start]==' ')start++;
 }
 struct tm t;localtime_r(&e.at,&t);strftime(b,sizeof(b),"%d.%m. %H:%M",&t);txt(g,b,162,1);
 snprintf(b,sizeof(b),"Tiefe %.0f km",e.depth);txt(g,b,184,1,WHITE);}
 txt(g,!online?"Offline":quakeStale(q,now)?"Nicht aktuell":q.error[0]?"Abruffehler":"USGS / alle 5 min",207,0,q.error[0]?ORANGE:SILVER);
}
