#pragma once
#include "Render.h"
#include "LaunchModel.h"
inline void launchIcon(Adafruit_GFX&g){
 g.fillTriangle(120,45,112,59,128,59,CYAN);g.fillRoundRect(112,58,17,27,4,WHITE);g.fillCircle(120,66,4,SEA);g.fillTriangle(112,73,102,88,112,84,CYAN);g.fillTriangle(128,73,138,88,128,84,CYAN);g.fillTriangle(115,86,125,86,120,99,GOLD);
}
inline void launchName(Adafruit_GFX&g,String name){
 name=latin(name);unsigned start=0;
 for(int row=0;row<2&&start<name.length();row++){
  unsigned end=start,lastSpace=start;g.setTextSize(1);g.setFont(&FreeSansBold9pt7b);
  while(end<name.length()){int16_t x,y;uint16_t w,h;String part=name.substring(start,end+1);g.getTextBounds(part.c_str(),0,0,&x,&y,&w,&h);if(w>204)break;if(name[end]==' ')lastSpace=end;end++;}
  if(row==0&&end<name.length()&&lastSpace>start)end=lastSpace;
  if(end==start)end++;
  String line=name.substring(start,end);if(row==1&&end<name.length())line=line+"..";
  txt(g,line,103+row*19,1,WHITE);start=end;while(start<name.length()&&name[start]==' ')start++;
 }
}
inline void renderLaunch(Adafruit_GFX&g,const LaunchData&d,time_t now,bool online){
 g.fillScreen(BG);g.setTextWrap(false);g.drawCircle(120,120,116,CYAN);txt(g,"RAKETEN",25,7,CYAN);launchIcon(g);
 if(!d.valid){txt(g,"Warte auf Daten",114,1);txt(g,d.error[0]?"Abruf fehlgeschlagen":"Daten werden geladen",150,0,SILVER);}
 else if(!d.count){txt(g,"Keine Startdaten",116,1);txt(g,"in der Quelle",145,0,SILVER);}
 else {const auto&e=d.events[(uint64_t(now)/12)%d.count];launchName(g,String(e.name));
  txt(g,strcmp(e.status,"Go")?"Unbestaetigt":strcmp(e.precision,"MIN")?"Zeit ungefaehr":"Geplant",144,1,CYAN);char b[48];bool precise=!strcmp(e.precision,"MIN"),hourly=!strcmp(e.precision,"HR");
  if(!e.net)strcpy(b,"Termin offen");else if(!strcmp(e.precision,"DAY"))snprintf(b,sizeof(b),"%s",fmtTime(e.net,"%d.%m.%Y").c_str());else if(precise)snprintf(b,sizeof(b),"%s",fmtTime(e.net,"%d.%m. %H:%M").c_str());else if(hourly)snprintf(b,sizeof(b),"%s Uhr ca.",fmtTime(e.net,"%d.%m. %H").c_str());else strcpy(b,"Termin ungenau");txt(g,b,166,7,WHITE);
  if(!online||launchStale(d,now)||d.error[0])strcpy(b,"Zeitplan nicht aktuell");else if(e.net&&e.net<=now)strcpy(b,"Termin bitte pruefen");else if(e.net&&precise&&!strcmp(e.status,"Go")){uint64_t minutes=uint64_t(e.net-now)/60;if(minutes>=1440)snprintf(b,sizeof(b),"%llu d %llu h",(unsigned long long)(minutes/1440),(unsigned long long)(minutes/60%24));else snprintf(b,sizeof(b),"%llu h %llu min",(unsigned long long)(minutes/60),(unsigned long long)(minutes%60));}else strcpy(b,"Plan kann sich aendern");txt(g,b,194,1,SILVER);
 }
 txt(g,!online?"Offline":launchStale(d,now)?"Nicht aktuell":d.error[0]?"Abruffehler":"The Space Devs",213,0,SILVER);
}
