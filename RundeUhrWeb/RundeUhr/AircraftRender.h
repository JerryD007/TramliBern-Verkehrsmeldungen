#pragma once
#include "FinanceRender.h"
#include "AircraftModel.h"
inline void aircraftKindIcon(Adafruit_GFX&g,AircraftKind kind,int y=83,int scale=1){
 const int x=120;
 if(kind==AircraftKind::Helicopter){
  g.drawFastHLine(x-19*scale,y-12*scale,38*scale,RED);g.drawFastHLine(x-19*scale,y-11*scale,38*scale,RED);
  g.fillRect(x-1*scale,y-10*scale,3*scale,6*scale,RED);g.fillRoundRect(x-9*scale,y-4*scale,21*scale,12*scale,5*scale,RED);
  g.fillTriangle(x-8*scale,y-2*scale,x-22*scale,y-5*scale,x-22*scale,y+1*scale,RED);g.drawFastVLine(x-22*scale,y-8*scale,10*scale,RED);
  g.drawFastVLine(x-5*scale,y+7*scale,5*scale,RED);g.drawFastVLine(x+6*scale,y+7*scale,5*scale,RED);g.drawFastHLine(x-10*scale,y+12*scale,26*scale,RED);
  g.fillRoundRect(x+3*scale,y-2*scale,6*scale,5*scale,2*scale,BG);
 }else if(kind==AircraftKind::Plane){
  // Compact jet, identical in both route modes: cockpit, engines, swept wings and tail.
  g.fillTriangle(x,y-7*scale,x-23*scale,y+8*scale,x-23*scale,y+12*scale,YELLOW);
  g.fillTriangle(x,y-7*scale,x+23*scale,y+8*scale,x+23*scale,y+12*scale,YELLOW);
  g.fillTriangle(x,y-7*scale,x-23*scale,y+12*scale,x,y+5*scale,YELLOW);
  g.fillTriangle(x,y-7*scale,x+23*scale,y+12*scale,x,y+5*scale,YELLOW);
  for(int side:{-1,1}){g.fillRoundRect(x+side*10*scale-2*scale,y+2*scale,5*scale,11*scale,2*scale,YELLOW);g.fillRect(x+side*10*scale-1*scale,y+3*scale,3*scale,2*scale,BG);}
  g.fillTriangle(x,y+12*scale,x-10*scale,y+21*scale,x,y+18*scale,YELLOW);
  g.fillTriangle(x,y+12*scale,x+10*scale,y+21*scale,x,y+18*scale,YELLOW);
  g.fillRoundRect(x-3*scale,y-22*scale,7*scale,40*scale,3*scale,YELLOW);
  g.fillTriangle(x-3*scale,y+14*scale,x+3*scale,y+14*scale,x,y+24*scale,YELLOW);
  g.fillRoundRect(x-2*scale,y-17*scale,5*scale,3*scale,1*scale,BG);
  g.drawFastVLine(x,y-17*scale,3*scale,YELLOW);
 }else{g.drawCircle(x,y,12*scale,DIM);g.setTextSize(2*scale);g.setTextColor(SILVER);g.setCursor(x-5*scale,y-7*scale);g.print("?");}
}
inline void aircraftMetric(Adafruit_GFX&g,const char*label,double value,bool valid,const char*unit,int cx,int y){
 if(!valid)return;
 txt(g,label,y,0,SILVER,0,cx);
 char number[16];snprintf(number,sizeof(number),"%.0f",value);
 g.setTextSize(1);g.setFont(&FreeSansBold12pt7b);int16_t x1,y1;uint16_t w,h;
 g.getTextBounds(number,0,0,&x1,&y1,&w,&h);
 if(w>66){g.setFont(&FreeSansBold9pt7b);g.getTextBounds(number,0,0,&x1,&y1,&w,&h);}
 g.setTextColor(WHITE);g.setCursor(cx-int(w)/2-x1,y+13-y1);g.print(number);g.setFont(nullptr);
 txt(g,unit,y+35,0,SILVER,0,cx);
}
inline void renderAircraft(Adafruit_GFX&g,const AircraftData&a,time_t now,bool online){
 g.fillScreen(BG);g.setTextWrap(false);g.drawCircle(120,120,116,CYAN);
 bool stale=aircraftStale(a,now);char b[40];
 if(a.found){const char*name=a.callsign[0]?a.callsign:a.registration[0]?a.registration:a.hex[0]?a.hex:"Unbekannt";
 bool expanded=!a.origin[0]&&!a.destination[0];
 // Fit the complete flight identifier before reducing it to an ellipsis.
 int nameSize=3;g.setTextSize(1);g.setFont(&FreeSansBold18pt7b);int16_t nx,ny;uint16_t nw,nh;g.getTextBounds(name,0,0,&nx,&ny,&nw,&nh);
 if(nw>128)nameSize=7;
 txt(g,name,29,nameSize,WHITE);
 aircraftKindIcon(g,a.kind,expanded?91:80,1);
 aircraftMetric(g,"HOEHE",a.altitude,a.altValid,"m",54,expanded?69:61);
 aircraftMetric(g,"TEMPO",a.speed,a.speedValid,"km/h",186,expanded?69:61);
 snprintf(b,sizeof(b),"%.1f km",a.distance);txt(g,b,expanded?148:110,expanded?5:7,CYAN);
 if(a.origin[0]&&a.destination[0]){
  txt(g,"VON",136,0,SILVER);txt(g,a.origin,146,7,CYAN);
  txt(g,"NACH",174,0,SILVER);txt(g,a.destination,184,7,CYAN);
 }else if(a.origin[0]||a.destination[0]){txt(g,a.origin[0]?"VON":"NACH",153,0,SILVER);txt(g,a.origin[0]?a.origin:a.destination,166,7,CYAN);}
 }else {aircraftKindIcon(g,AircraftKind::Unknown,65);txt(g,a.checked&&!a.error[0]?"Kein aktuelles":"Warte auf",116,2,WHITE);txt(g,a.checked&&!a.error[0]?"Flugzeug":"Daten",144,2,WHITE);}
 if(!online||stale)txt(g,!online?"Offline":"Nicht aktuell",220,0,ORANGE);
}
