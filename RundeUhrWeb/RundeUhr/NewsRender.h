#pragma once
#include "Render.h"
#include "NewsModel.h"
inline void renderNews(Adafruit_GFX&g,const Config&c,const NewsData&n,time_t now,bool online,uint32_t ms){
 g.fillScreen(BG);g.setTextWrap(false);g.drawCircle(120,120,116,CYAN);txt(g,"NACHRICHTEN",37,2,CYAN);
 if(!n.valid||!n.count){txt(g,"Keine Nachrichten",110,1);txt(g,n.error[0]?"Abruf fehlgeschlagen":"Daten werden geladen",140,0,ORANGE);return;}
 // A fixed horizontal text band stays inside the circular display.
 int eligible[3],count=0;for(int i=0;i<n.count;i++)if(!c.automatic||(n.items[i].at<=now&&now-n.items[i].at<=c.newsWindow*60))eligible[count++]=i;
 if(!count){txt(g,"Keine neuen Meldungen",120,0);return;}
 // Each headline gets enough time to scroll completely at 30 px/s, with end pauses.
 uint32_t total=0,durations[3];for(int k=0;k<count;k++){durations[k]=5000+uint32_t(strlen(n.items[eligible[k]].title))*400;total+=durations[k];}
 uint32_t phase=ms%total;int idx=0;while(idx+1<count&&phase>=durations[idx])phase-=durations[idx++];auto&e=n.items[eligible[idx]];
 g.setFont(nullptr);g.setTextSize(2);g.setTextColor(WHITE);int width=strlen(e.title)*12,travel=width>184?width-184:0;int offset=phase<2500?0:int((phase-2500)*30/1000);if(offset>travel)offset=travel;
 // Draw only full glyphs in the band, never outside the circle.
 for(size_t j=0;j<strlen(e.title);j++){int x=28+int(j)*12-offset;if(x>=28&&x+12<=212){g.setCursor(x,108);g.write(uint8_t(e.title[j]));}}
 tm t;localtime_r(&e.at,&t);char b[32];strftime(b,sizeof(b),"%d.%m. %H:%M",&t);txt(g,b,164,1,SILVER);txt(g,NEWS_SOURCES[n.source].credit,190,1,CYAN);if(!online||newsStale(n,now)||n.error[0])txt(g,"Nicht aktuell",211,0,ORANGE);
}
