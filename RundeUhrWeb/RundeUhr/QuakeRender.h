#pragma once
#include "Render.h"
#include "QuakeModel.h"
inline void renderQuake(Adafruit_GFX&g,const QuakeData&q,time_t now,bool online,const Config&c=Config()){
 g.fillScreen(BG);g.setTextWrap(false);g.drawCircle(120,120,116,ORANGE);
 txt(g,"ERDBEBEN",25,2,ORANGE);char filter[48];snprintf(filter,sizeof(filter),"M%.1f+ / %u min",c.quakeMagnitude,c.quakeWindow);txt(g,filter,49,0,SILVER);
 if(!q.valid){txt(g,"Warte auf Daten",103,1);txt(g,!online?"WLAN offline":q.error[0]?"Abruf fehlgeschlagen":"USGS wird geladen",143,0,ORANGE);}
 else if(!q.count){txt(g,"Keine passenden",94,1);txt(g,"Erdbeben",119,7);txt(g,c.quakeRadius?"Umkreisfilter aktiv":"Weltweiter Filter",157,0,SILVER);txt(g,"Filter im Web pruefen",178,0,SILVER);}
 else {const auto&e=q.events[(uint64_t(now)/8)%q.count];char b[128];snprintf(b,sizeof(b),"M %.1f",e.magnitude);txt(g,b,72,3,quakeStale(q,now)?GRAY:ORANGE);
 // Fixed-width ASCII transliteration avoids broken UTF-8 on the small font.
 char place[112];snprintf(place,sizeof(place),"%s",latin(String(e.place)).c_str());
 for(int row=0;row<3;row++){char line[29]={};size_t offset=row*28;if(offset>=strlen(place))break;snprintf(line,sizeof(line),"%.28s",place+offset);txt(g,line,116+row*13,0,WHITE);}
 struct tm t;localtime_r(&e.at,&t);strftime(b,sizeof(b),"%d.%m. %H:%M",&t);txt(g,b,162,1);
 snprintf(b,sizeof(b),"Tiefe %.0f km",e.depth);txt(g,b,184,0,SILVER);}
 txt(g,!online?"Offline":quakeStale(q,now)?"Nicht aktuell":q.error[0]?"Abruffehler":"USGS / alle 5 min",207,0,q.error[0]?ORANGE:SILVER);
}
