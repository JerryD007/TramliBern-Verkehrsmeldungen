#pragma once
#include "FinanceRender.h"
#include "AircraftModel.h"
inline void renderAircraft(Adafruit_GFX&g,const AircraftData&a,time_t now,bool online){
 g.fillScreen(BG);g.setTextWrap(false);g.drawCircle(120,120,116,CYAN);
 txt(g,"NAECHSTES",22,2,CYAN);txt(g,"FLUGZEUG",41,2,CYAN);
 g.fillRoundRect(117,64,6,35,3,CYAN);g.fillTriangle(120,72,91,89,149,89,CYAN);g.fillTriangle(120,90,109,101,131,101,CYAN);
 bool stale=aircraftStale(a,now);char b[40];
 if(a.found){const char*name=a.callsign[0]?a.callsign:a.registration[0]?a.registration:a.hex[0]?a.hex:"Unbekannt";txt(g,name,110,strlen(name)>8?2:3,WHITE);snprintf(b,sizeof(b),"%.1f km",a.distance);txt(g,b,142,3,CYAN);
 char alt[14]="-- m",speed[14]="-- km/h";if(a.altValid)snprintf(alt,sizeof(alt),"%.0f m",a.altitude);if(a.speedValid)snprintf(speed,sizeof(speed),"%.0f km/h",a.speed);snprintf(b,sizeof(b),"%s  %s",alt,speed);txt(g,b,177,1,WHITE);
 }else {txt(g,a.checked&&!a.error[0]?"Kein aktuelles":"Warte auf",116,2,WHITE);txt(g,a.checked&&!a.error[0]?"Flugzeug":"Daten",144,2,WHITE);}
 txt(g,!online?"Offline":stale?"Nicht aktuell":"adsb.fi",200,1,stale?ORANGE:SILVER);
 if(a.found){snprintf(b,sizeof(b),"Position vor %ld s",long(now>=a.positionAt?now-a.positionAt:0));txt(g,b,216,0,DIM);}
}
