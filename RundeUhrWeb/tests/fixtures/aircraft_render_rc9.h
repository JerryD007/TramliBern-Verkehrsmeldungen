#pragma once
#include "FinanceRender.h"
#include "AircraftModel.h"
inline void oldAircraftKindIcon(Adafruit_GFX&g,AircraftKind kind,int y=83){
 const int x=120;
 if(kind==AircraftKind::Helicopter){
  g.drawFastHLine(x-19,y-12,38,RED);g.drawFastHLine(x-19,y-11,38,RED);
  g.fillRect(x-1,y-10,3,6,RED);g.fillRoundRect(x-9,y-4,21,12,5,RED);
  g.fillTriangle(x-8,y-2,x-22,y-5,x-22,y+1,RED);g.drawFastVLine(x-22,y-8,10,RED);
  g.drawFastVLine(x-5,y+7,5,RED);g.drawFastVLine(x+6,y+7,5,RED);g.drawFastHLine(x-10,y+12,26,RED);
  g.fillRoundRect(x+3,y-2,6,5,2,BG);
 }else if(kind==AircraftKind::Plane){
  g.fillRoundRect(x-2,y-16,5,30,2,CYAN);g.fillTriangle(x,y-9,x-19,y+5,x+19,y+5,CYAN);g.fillTriangle(x,y+7,x-8,y+15,x+8,y+15,CYAN);
 }else{g.drawCircle(x,y,12,DIM);g.setTextSize(2);g.setTextColor(SILVER);g.setCursor(x-5,y-7);g.print("?");}
}
inline void oldRenderAircraft(Adafruit_GFX&g,const AircraftData&a,time_t now,bool online){
 g.fillScreen(BG);g.setTextWrap(false);g.drawCircle(120,120,116,CYAN);
 bool heli=a.found&&a.kind==AircraftKind::Helicopter;
 txt(g,heli?"NAECHSTER":"NAECHSTES",16,0,CYAN);txt(g,heli?"HELIKOPTER":"FLUGZEUG",29,2,CYAN);
 oldAircraftKindIcon(g,a.found?a.kind:AircraftKind::Unknown,63);
 bool stale=aircraftStale(a,now);char b[40];
 if(a.found){const char*name=a.callsign[0]?a.callsign:a.registration[0]?a.registration:a.hex[0]?a.hex:"Unbekannt";
 txt(g,name,85,2,WHITE);snprintf(b,sizeof(b),"%.1f km",a.distance);txt(g,b,106,2,CYAN);
 char alt[14]="",speed[14]="";if(a.altValid)snprintf(alt,sizeof(alt),"%.0f m",a.altitude);if(a.speedValid)snprintf(speed,sizeof(speed),"%.0f km/h",a.speed);snprintf(b,sizeof(b),"%s  %s",alt,speed);txt(g,b,128,0,WHITE);
 if(a.origin[0]&&a.destination[0]){
  txt(g,"VON",136,0,SILVER);txt(g,a.origin,146,7,CYAN);
  txt(g,"NACH",174,0,SILVER);txt(g,a.destination,184,7,CYAN);
 }else if(a.origin[0]||a.destination[0]){txt(g,a.origin[0]?"VON":"NACH",153,0,SILVER);txt(g,a.origin[0]?a.origin:a.destination,166,7,CYAN);}
 }else {txt(g,a.checked&&!a.error[0]?"Kein aktuelles":"Warte auf",116,2,WHITE);txt(g,a.checked&&!a.error[0]?"Flugzeug":"Daten",144,2,WHITE);}
 if(!online||stale)txt(g,!online?"Offline":"Nicht aktuell",220,0,ORANGE);
}
