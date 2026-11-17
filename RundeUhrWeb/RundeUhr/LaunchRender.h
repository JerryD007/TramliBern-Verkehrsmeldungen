#pragma once
#include "Render.h"
#include "LaunchModel.h"
inline void launchIcon(Adafruit_GFX&g){
 g.fillTriangle(120,48,114,60,126,60,CYAN);g.fillRoundRect(114,59,13,24,3,WHITE);g.fillCircle(120,65,3,SEA);g.fillTriangle(114,73,106,86,114,82,CYAN);g.fillTriangle(126,73,134,86,126,82,CYAN);g.fillTriangle(116,84,124,84,120,94,GOLD);
}
inline void renderLaunch(Adafruit_GFX&g,const LaunchData&d,time_t now,bool online){
 g.fillScreen(BG);g.setTextWrap(false);g.drawCircle(120,120,116,CYAN);txt(g,"RAKETEN",25,1,CYAN);launchIcon(g);
 if(!d.valid){txt(g,"Warte auf Daten",114,1);txt(g,d.error[0]?"Abruf fehlgeschlagen":"Daten werden geladen",150,0,SILVER);}
 else if(!d.count){txt(g,"Keine Startdaten",116,1);txt(g,"in der Quelle",145,0,SILVER);}
 else {const auto&e=d.events[(uint64_t(now)/12)%d.count];String name=latin(String(e.name));for(int row=0;row<2;row++){if(unsigned(row*28)>=name.length())break;txt(g,name.substring(row*28,(row+1)*28),104+row*13,0,WHITE);}
  txt(g,launchStatus(e),137,0,CYAN);char b[48];bool precise=!strcmp(e.precision,"MIN"),hourly=!strcmp(e.precision,"HR");
  if(!e.net)strcpy(b,"Termin offen");else if(!strcmp(e.precision,"DAY"))snprintf(b,sizeof(b),"%s",fmtTime(e.net,"%d.%m.%Y").c_str());else if(precise)snprintf(b,sizeof(b),"%s",fmtTime(e.net,"%d.%m. %H:%M").c_str());else if(hourly)snprintf(b,sizeof(b),"%s Uhr ca.",fmtTime(e.net,"%d.%m. %H").c_str());else strcpy(b,"Termin ungenau");txt(g,b,155,1,WHITE);
  if(!online||launchStale(d,now)||d.error[0])strcpy(b,"Zeitplan nicht aktuell");else if(e.net&&e.net<=now)strcpy(b,"Termin bitte pruefen");else if(e.net&&precise&&!strcmp(e.status,"Go")){uint64_t minutes=uint64_t(e.net-now)/60;if(minutes>=1440)snprintf(b,sizeof(b),"Plan: %llu d %llu h",(unsigned long long)(minutes/1440),(unsigned long long)(minutes/60%24));else snprintf(b,sizeof(b),"Plan: %llu h %llu min",(unsigned long long)(minutes/60),(unsigned long long)(minutes%60));}else strcpy(b,"Plan kann sich aendern");txt(g,b,184,0,SILVER);
 }
 txt(g,!online?"Offline":launchStale(d,now)?"Nicht aktuell":d.error[0]?"Abruffehler":"The Space Devs",208,0,SILVER);
}
