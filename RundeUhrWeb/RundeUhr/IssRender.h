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
 g.drawFastHLine(x-19,y,39,WHITE);
 g.fillRect(x-4,y-8,9,17,WHITE);g.fillRect(x-2,y-12,5,5,SILVER);
 g.fillRect(x-21,y-11,13,23,fresh?SEA:GRAY);g.fillRect(x+9,y-11,13,23,fresh?SEA:GRAY);
 for(int side:{-21,9}){g.drawRect(x+side,y-11,13,23,fresh?CYAN:SILVER);for(int k=1;k<4;k++)g.drawFastHLine(x+side,y-11+k*6,13,fresh?YELLOW:SILVER);g.drawFastVLine(x+side+6,y-11,23,fresh?CYAN:SILVER);}
 g.fillRect(x-2,y-3,5,7,fresh?ORANGE:SILVER);
 char b[48];snprintf(b,sizeof(b),"%.0f km",look.distance);txt(g,b,153,3,color);
 snprintf(b,sizeof(b),"Az %.0f  El %+.0f",look.azimuth,look.elevation);txt(g,b,184,0,SILVER);
 txt(g,!online?"Offline":issStale(i,now)?"Nicht aktuell":look.elevation>0?"Ueber Horizont":"Unter Horizont",198,0,fresh?WHITE:ORANGE);
 txt(g,"wheretheiss.at",211,0,DIM);
}
