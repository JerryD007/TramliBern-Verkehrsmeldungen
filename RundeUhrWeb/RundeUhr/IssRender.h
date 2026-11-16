#pragma once
#include "Render.h"
#include "IssModel.h"
inline void renderIss(Adafruit_GFX&g,const Config&c,const IssData&i,time_t now,bool online){
 g.fillScreen(BG);g.setTextWrap(false);g.drawCircle(120,120,116,CYAN);txt(g,"ISS",17,3,CYAN);
 if(!i.valid){txt(g,"Warte auf ISS",104,2);txt(g,!online?"WLAN offline":i.error[0]?"Abruf fehlgeschlagen":"Daten werden geladen",145,0,ORANGE);txt(g,"wheretheiss.at",207,0,SILVER);return;}
 const auto look=issLook(i,c.lat,c.lon);const bool fresh=online&&!issStale(i,now);int color=fresh?CYAN:GRAY;
 const int cx=120,cy=104,r=43;g.drawCircle(cx,cy,r,DIM);g.drawCircle(cx,cy,r/2,SHADOW);g.drawFastHLine(cx-r,cy,r*2+1,SHADOW);g.drawFastVLine(cx,cy-r,r*2+1,SHADOW);
 txt(g,"N",51,0,SILVER);g.fillCircle(cx,cy,2,WHITE);
 double radius=r*(1-fmax(0,look.elevation)/90),angle=look.azimuth*M_PI/180;int x=cx+lround(radius*sin(angle)),y=cy-lround(radius*cos(angle));
 // Satellite body, two solar arrays. Below-horizon positions sit at the rim.
 g.fillRect(x-3,y-4,7,9,WHITE);g.fillRect(x-12,y-6,6,13,color);g.fillRect(x+7,y-6,6,13,color);g.drawFastHLine(x-6,y,19,WHITE);
 char b[48];snprintf(b,sizeof(b),"%.0f km",look.distance);txt(g,b,153,3,color);
 snprintf(b,sizeof(b),"Az %.0f  El %+.0f",look.azimuth,look.elevation);txt(g,b,184,0,SILVER);
 txt(g,!online?"Offline":issStale(i,now)?"Nicht aktuell":look.elevation>0?"Ueber Horizont":"Unter Horizont",198,0,fresh?WHITE:ORANGE);
 txt(g,"wheretheiss.at",211,0,DIM);
}
