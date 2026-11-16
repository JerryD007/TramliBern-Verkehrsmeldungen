#pragma once
// Shared renderer: firmware and host previews use exactly these drawing routines.
#include "Model.h"
#include "WeekendFrames.h"
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
static const uint16_t palette[]={0x0000,0xffff,0x07ff,0xfc10,0x4208,0xffe0,0xb35f,0xf800,0x47e9,0x9bd3,0xc618,0x8410,0xfd20,0xfdb8,0x04b5,0x2104};
static const uint16_t* activePalette=palette;
enum {BG,WHITE,CYAN,ORANGE,DIM,YELLOW,PURPLE,RED,GREEN,BROWN,SILVER,GRAY,GOLD,SKIN,SEA,SHADOW};
#include "RenderText.h"
inline String shorten(String s,int n){s=latin(s);return s.length()>(unsigned)n?s.substring(0,n-2)+"..":s;}
inline String fmtTime(time_t t,const char* format="%H:%M"){if(!t)return "--:--";tm z;localtime_r(&t,&z);char b[40];strftime(b,sizeof(b),format,&z);return String(b);}
inline void txt(Adafruit_GFX &g,String s,int y,int size=2,int color=WHITE,int maxchars=0,int cx=120){
 s=latin(s);if(maxchars>0)s=shorten(s,maxchars);g.setTextSize(size==6?2:1);g.setFont(size==0?nullptr:size==7?&FreeSansBold12pt7b:size>=5?&FreeSansBold24pt7b:size>=3?&FreeSansBold18pt7b:&FreeSansBold9pt7b);
 int16_t x1,y1;uint16_t w,h;g.getTextBounds(s.c_str(),0,0,&x1,&y1,&w,&h);
 int dy=max(abs(y-120),abs(y+int(h)-120));int half=sqrt(fmax(0,112*112-dy*dy));int available=2*min(cx-(120-half),(120+half)-cx);
 if(w>available){String original=s;int n=s.length();do{s=original.substring(0,--n)+"..";g.getTextBounds(s.c_str(),0,0,&x1,&y1,&w,&h);}while(n>0&&w>available);}
 g.setTextColor(color);g.setCursor(cx-int(w)/2-x1,y-y1);g.print(s);g.setFont(nullptr);
}
inline void weatherIcon(Adafruit_GFX &g,int code,int x,int y,int scale=1){
 if(code<=2){g.fillCircle(x,y,10*scale,YELLOW);for(int i=0;i<8;i++){double a=i*M_PI/4;g.drawLine(x+cos(a)*14*scale,y+sin(a)*14*scale,x+cos(a)*19*scale,y+sin(a)*19*scale,YELLOW);}}
 if(code>0){g.fillCircle(x-8*scale,y+4*scale,9*scale,WHITE);g.fillCircle(x+4*scale,y,12*scale,WHITE);g.fillRoundRect(x-17*scale,y+4*scale,36*scale,12*scale,5,WHITE);}
 if((code>=51&&code<=67)||(code>=80&&code<=82)||code>=95){for(int i=-1;i<=1;i++)g.drawLine(x+i*10*scale,y+20*scale,x+i*10*scale-3,y+27*scale,CYAN);}
 if((code>=71&&code<=77)||code==85||code==86){for(int i=-1;i<=1;i++){int sx=x+i*11*scale;g.drawLine(sx-3,y+24*scale,sx+3,y+24*scale,WHITE);g.drawLine(sx,y+21*scale,sx,y+27*scale,WHITE);}}
 if(code==45||code==48){for(int i=0;i<3;i++)g.drawLine(x-17*scale,y+(19+i*5)*scale,x+17*scale,y+(19+i*5)*scale,DIM);}
 if(code>=95)g.drawLine(x+3,y+12,x-5,y+31,ORANGE);
}
inline void hand(Adafruit_GFX&g,double a,int length,int width,int color){int x=120+sin(a)*length,y=120-cos(a)*length;for(int i=-width;i<=width;i++)g.drawLine(120+i,120,x+i,y,color);g.fillCircle(x,y,width,color);}
// Native vector icons: no downloaded emoji font or large bitmap buffers.
inline void mug(Adafruit_GFX&g,int x,int y,bool right){
 g.drawRoundRect(x+(right?-18:43),y+16,27,40,9,GOLD);g.drawRoundRect(x+(right?-16:45),y+18,23,36,7,GOLD);
 g.fillRoundRect(x,y+7,52,65,7,GOLD);g.fillRect(x+7,y+22,5,40,YELLOW);g.fillRect(x+38,y+22,5,40,ORANGE);
 g.fillRoundRect(x-4,y,60,22,9,WHITE);g.fillCircle(x+11,y,11,WHITE);g.fillCircle(x+34,y-4,13,WHITE);
}
inline void countdownIcon(Adafruit_GFX&g,int type,uint32_t ms,bool animate){
 double phase=animate?sin(ms*0.004):0;int dy=int(phase*3);
 if(type==0){ // Island, waves, and gently moving palm crown.
  for(int y=-7;y<=7;y++){int w=48*sqrt(1-y*y/49.0);g.drawFastHLine(120-w,139+y,2*w,SEA);}for(int y=-8;y<=8;y++){int w=34*sqrt(1-y*y/64.0);g.drawFastHLine(120-w,135+y,2*w,GOLD);}
  for(int k=0;k<3;k++)g.drawLine(76+k*31,147,94+k*31,147,CYAN);
  g.fillTriangle(116,132,125,132,127,83,BROWN);g.drawLine(121,126,125,91,GOLD);
  int x=126+int(phase*3),y=83;
  g.fillTriangle(x,y,x-36,y+8,x-24,y-10,GREEN);g.fillTriangle(x,y,x+36,y+5,x+21,y-12,GREEN);
  g.fillTriangle(x,y,x-19,y-25,x-3,y-22,GREEN);g.fillTriangle(x,y,x+25,y-23,x+7,y-25,GREEN);
  g.fillTriangle(x,y,x-28,y+22,x-19,y+3,GREEN);g.fillTriangle(x,y,x+26,y+23,x+20,y+3,GREEN);
  g.fillCircle(x-4,y+4,4,BROWN);g.fillCircle(x+4,y+4,4,GOLD);
 }else if(type==1){
  int y=100+dy;
  g.fillRoundRect(91,y,58,42,3,PURPLE);g.fillRect(116,y,8,42,CYAN);
  g.fillTriangle(87,y-7,152,y-12,154,y+1,PURPLE);g.fillTriangle(87,y-7,154,y+1,89,y+6,PURPLE);
  g.drawLine(89,y-6,151,y-11,WHITE);
  g.fillTriangle(116,y-8,102,y+14,113,y+10,CYAN);
  g.fillTriangle(123,y-8,137,y+12,129,y+9,CYAN);
  g.fillCircle(108,y-19,10,CYAN);g.fillCircle(133,y-20,10,CYAN);
  g.fillTriangle(102,y-13,120,y-7,114,y-24,CYAN);
  g.fillTriangle(139,y-14,120,y-7,128,y-25,CYAN);
  g.fillRoundRect(115,y-13,11,10,2,CYAN);
  for(int k=0;k<3;k++){int x=79+k*40,yy=68+((k*11+int(animate?ms/100:0))%17);g.fillTriangle(x,yy-3,x-3,yy,x+3,yy,k%2?CYAN:PURPLE);g.fillTriangle(x-3,yy,x+3,yy,x,yy+3,k%2?CYAN:PURPLE);}

 }else if(type==2){
  int spread=animate?int((phase+1)*5):2;mug(g,68-spread,80+dy,false);mug(g,120+spread,80-dy,true);
  if(!animate||phase<-.4){g.drawLine(120,54,120,62,WHITE);g.drawLine(107,58,112,64,YELLOW);g.drawLine(133,58,128,64,YELLOW);}
 }else if(type==3){ // Stylised hand with one raised middle finger.
  int y=dy;g.fillRoundRect(98,103+y,47,35,10,SKIN);g.fillRoundRect(116,63+y,12,60,6,SKIN);
  g.fillRoundRect(101,91+y,13,29,6,SKIN);g.fillRoundRect(130,94+y,12,27,6,SKIN);g.fillRoundRect(141,101+y,10,25,5,SKIN);
  g.drawRoundRect(101,94+y,13,24,5,BROWN);g.drawRoundRect(130,97+y,12,23,5,BROWN);g.drawLine(119,77+y,125,77+y,BROWN);
  g.fillRoundRect(91,112+y,28,13,6,SKIN);g.drawLine(98,117+y,115,123+y,BROWN);g.fillRect(108,133+y,28,12,SKIN);
 }
}

// Fit complete hourly values in separate columns; fall back for rare wide values.
inline void weatherRowText(Adafruit_GFX&g,String text,int cx,int y,int width,int color){
 g.setTextSize(1);g.setFont(&FreeSansBold9pt7b);int16_t x1,y1;uint16_t w,h;
 g.getTextBounds(text.c_str(),0,0,&x1,&y1,&w,&h);
 if(w>width){g.setFont(nullptr);g.getTextBounds(text.c_str(),0,0,&x1,&y1,&w,&h);}
 g.setTextColor(color);g.setCursor(cx-int(w)/2-x1,y-y1);g.print(text);g.setFont(nullptr);
}
inline void render(Adafruit_GFX &g,const Config&c,const Weather&w,const Moon&moon,time_t now,bool online,bool setup,String apName,String apKey,uint32_t ms=0){
 activePalette=palette;g.fillScreen(BG);g.setTextWrap(false);int v=c.view;bool valid=timeValid(now);tm t={};if(valid)localtime_r(&now,&t);
 const int accents[]={CYAN,YELLOW,PURPLE,YELLOW,PURPLE,ORANGE};g.drawCircle(120,120,116,(v==6?PURPLE:accents[v<5?v:5]));g.drawCircle(120,120,115,(v==6?PURPLE:accents[v<5?v:5]));
 if(setup){txt(g,"WLAN einrichten",35,2,CYAN);txt(g,apName,69,2);txt(g,"WLAN-Passwort",98,1);txt(g,apKey,122,2);txt(g,"Safari oeffnen",155,1);txt(g,"http://192.168.4.1",177,1);txt(g,"Setup ohne Internet",203,0,ORANGE);return;}
 if(!valid){txt(g,"Zeit fehlt",77,3,ORANGE);txt(g,"NTP ueber WLAN",126,2);txt(g,"Verbindung pruefen",153,1);return;}
 switch(v){
 case 0:{for(int i=0;i<60;i++){double a=i*M_PI/30;int r=i%5?105:100;g.drawLine(120+sin(a)*r,120-cos(a)*r,120+sin(a)*109,120-cos(a)*109,i%5?DIM:WHITE);}for(int i=1;i<=12;i++){double a=i*M_PI/6;txt(g,String(i),int(113-cos(a)*87),2,WHITE,0,int(120+sin(a)*87));}txt(g,fmtTime(now,"%d.%m.%Y"),158,2);hand(g,((t.tm_hour%12)+t.tm_min/60.0)*M_PI/6,51,2,WHITE);hand(g,(t.tm_min+t.tm_sec/60.0)*M_PI/30,77,2,WHITE);hand(g,t.tm_sec*M_PI/30,87,0,ORANGE);g.fillCircle(120,120,5,ORANGE);break;}
 case 1:case 2:{
 txt(g,c.city,20,0,CYAN,20);txt(g,"Wetter & Regen",38,2);g.drawLine(40,58,200,58,DIM);
 int j=nextHour(w,now);
 if(w.valid&&j>=0){
  if(w.fields[j]&2)weatherIcon(g,w.codes[j],73,98,w.codes[j]<=3?2:1);else txt(g,"?",82,3,DIM,0,73);
  String temp=(w.fields[j]&1)?String(w.temps[j],0):String("--");
  txt(g,temp,72,temp.length()>2?3:5,WHITE,0,165);g.drawCircle(199,75,2,YELLOW);txt(g,"C",81,0,YELLOW,0,207);txt(g,fmtTime(w.hours[j]),112,2,WHITE,0,165);
  txt(g,((w.fields[j]&4)?String(w.probability[j]):String("--"))+"%",138,2,CYAN,0,68);
  txt(g,((w.fields[j]&8)?String(w.amount[j],1):String("--"))+" mm",138,2,CYAN,0,167);g.drawLine(120,137,120,153,DIM);
  g.drawLine(37,162,203,162,DIM);
  for(int k=1;k<=2;k++){int i=j+k,y=168+(k-1)*22;
   if(i>=w.count){txt(g,"Keine weiteren Daten",y,0,DIM);break;}
   weatherRowText(g,fmtTime(w.hours[i]),68,y,46,WHITE);
   if(w.fields[i]&2){int code=w.codes[i];if(code<=2)g.fillCircle(99,y+6,4,YELLOW);if(code>0){g.fillCircle(101,y+7,4,WHITE);g.fillRect(96,y+8,11,4,WHITE);}}
   weatherRowText(g,((w.fields[i]&1)?String(w.temps[i],0):String("--"))+" C",130,y,40,WHITE);
   weatherRowText(g,((w.fields[i]&4)?String(w.probability[i]):String("--"))+"%",177,y,42,CYAN);
  }

 }else{txt(g,"Keine Vorhersage",105,2);txt(g,"Internet pruefen",140,1,ORANGE);}
 break;}
 case 3:case 4:{
 txt(g,"SONNE &",23,2,WHITE);txt(g,"MOND",44,2,WHITE);
 g.drawLine(120,65,120,177,DIM);g.drawLine(55,187,185,187,DIM);
 // Left: existing weather-provided solar times. Missing values never hide Moon.
 const int sx=66,sy=91;g.fillCircle(sx,sy,14,YELLOW);for(int a=0;a<8;a++){double angle=a*M_PI/4;g.drawLine(sx+cos(angle)*19,sy+sin(angle)*19,sx+cos(angle)*24,sy+sin(angle)*24,YELLOW);}
 int i=sunDay(w,now);bool times=w.valid&&i>=0&&w.sunrise[i]&&w.sunset[i];
 txt(g,times?fmtTime(w.sunrise[i]):"--:--",127,2,YELLOW,0,69);txt(g,"Aufgang",148,0,SILVER,0,69);
 txt(g,times?fmtTime(w.sunset[i]):"--:--",160,2,YELLOW,0,69);
 // Arrows distinguish sunrise and sunset without crowding the small display.
 g.drawLine(25,128,25,140,YELLOW);g.drawLine(25,128,21,132,YELLOW);g.drawLine(25,128,29,132,YELLOW);
 g.drawLine(25,162,25,174,YELLOW);g.drawLine(25,174,21,170,YELLOW);g.drawLine(25,174,29,170,YELLOW);
 if(moon.valid){int r=23;double co=1-2*moon.lit/100.0,si=sqrt(fmax(0,1-co*co))*(moon.phase<.5?1:-1);
 for(int y=-r;y<=r;y++)for(int x=-r;x<=r;x++){double z2=r*r-x*x-y*y;if(z2>=0)g.drawPixel(174+x,91+y,si*x-co*sqrt(z2)>0?WHITE:SHADOW);}
 g.drawCircle(174,91,r,GRAY);txt(g,String(moon.lit,0)+" %",126,2,CYAN,0,174);
 const char*phases[]={"Neumond","zunehmend","1. Viertel","zunehmend","Vollmond","abnehmend","3. Viertel","abnehmend"};txt(g,phases[int(moon.phase*8+.5)%8],153,0,CYAN,0,174);
 txt(g,"Vollmond",195,0,SILVER);txt(g,fmtTime(moon.full,"%d.%m.%Y"),208,0,CYAN);
 }else{g.drawCircle(174,91,23,GRAY);txt(g,"-- %",126,2,CYAN,0,174);txt(g,"Keine Daten",153,0,SILVER,0,174);txt(g,"Vollmond --",201,0,SILVER);}
 txt(g,"Untergang",177,0,SILVER,0,69);
 break;}
 case 5:case 6:case 7:case 8:case 9:{int type=v-5;txt(g,countdownName(c,type),35,2,WHITE,24);time_t target;
 if(type==2&&weekendTransitionActive(now,c)){activePalette=weekendPalette;g.fillScreen(0);g.drawCircle(120,120,116,2);g.drawCircle(120,120,115,2);txt(g,"Wochenende",35,2,1);drawWeekendToast(g,ms);g.drawCircle(120,120,116,2);g.drawCircle(120,120,115,2);return;}
 if(type<4)countdownIcon(g,type,ms,c.animations);
 if(type==2&&weekendPictureOnly(now,c))return;
 if(type==4){
  if(!countdownTarget(c,type,now,target)){txt(g,"Datum waehlen",108,2);txt(g,"In der Web-App",149,2,ORANGE);}
  else {long days=calendarDays(target,now);String digits=String(labs(days));txt(g,digits,digits.length()<=3?84:100,digits.length()<=3?6:5,ORANGE);txt(g,days==0?"Heute!":days<0?"Tage seit Ereignis":"Tage verbleiben",170,2);txt(g,fmtTime(target,"%d.%m.%Y"),199,2);}
  break;
 }
 if(!countdownTarget(c,type,now,target)){txt(g,"Datum waehlen",168,2);txt(g,"In der Web-App",197,0,ORANGE);}else{long days=calendarDays(target,now);
 String remaining=days==0?String("Heute!"):String(labs(days))+(labs(days)==1?" Tag":" Tage");txt(g,remaining,161,remaining.length()>8?2:3,type==1?PURPLE:ORANGE);if(days<0)txt(g,"seit Ereignis",192,0);txt(g,fmtTime(target,"%d.%m.%Y"),200,2);}
 break;}

 }
 String status=!online?"WLAN offline":"";if(v>=1&&v<=3&&weatherStale(w,now))status="Daten veraltet";txt(g,status,216,0,ORANGE);
}
