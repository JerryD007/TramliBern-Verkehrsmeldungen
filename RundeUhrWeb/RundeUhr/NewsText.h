#pragma once
#include <stdint.h>
#include <stddef.h>
// UTF-8 to the built-in font's CP437 glyphs. No heap allocation; original RSS stays UTF-8.
inline size_t newsDisplayText(const char* text,char* out,size_t capacity){
 size_t n=0;
 while(*text){
  uint8_t first=uint8_t(*text++);uint32_t cp=first;unsigned need=0;uint32_t minimum=0;
  if(first>=0xc2&&first<=0xdf){cp=first&31;need=1;minimum=0x80;}
  else if(first>=0xe0&&first<=0xef){cp=first&15;need=2;minimum=0x800;}
  else if(first>=0xf0&&first<=0xf4){cp=first&7;need=3;minimum=0x10000;}
  else if(first>=128)cp='?';
  bool valid=true;for(unsigned i=0;i<need;i++){if((uint8_t(*text)&0xc0)!=0x80){valid=false;break;}cp=(cp<<6)|(uint8_t(*text++)&63);}
  if(!valid||cp<minimum||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))cp='?';
  // Also support decomposed German umlauts (base letter followed by U+0308).
  if(uint8_t(text[0])==0xcc&&uint8_t(text[1])==0x88){
   uint32_t composed=cp=='a'?0xe4:cp=='o'?0xf6:cp=='u'?0xfc:cp=='A'?0xc4:cp=='O'?0xd6:cp=='U'?0xdc:0;
   if(composed){cp=composed;text+=2;}
  }
  if(cp>=0x300&&cp<=0x36f)continue;
  uint8_t glyph='?';
  if(cp>=32&&cp<127)glyph=cp;
  else switch(cp){
   case 0xe4:glyph=132;break;case 0xf6:glyph=148;break;case 0xfc:glyph=129;break;
   case 0xc4:glyph=142;break;case 0xd6:glyph=153;break;case 0xdc:glyph=154;break;case 0xdf:case 0x1e9e:glyph=225;break;
   case 0xe9:glyph=130;break;case 0xe8:glyph=138;break;case 0xe0:glyph=133;break;case 0xe2:glyph=131;break;case 0xea:glyph=136;break;case 0xe7:glyph=135;break;
   case 0xa0:case 0x202f:case 9:case 10:case 13:glyph=' ';break;
   case 0x2018:case 0x2019:glyph=39;break;
   case 0x201c:case 0x201d:case 0x201e:case 0xab:case 0xbb:glyph=34;break;
   case 0x2013:case 0x2014:case 0x2212:glyph='-';break;
  }
  if(out&&n+1<capacity)out[n]=char(glyph);n++;
 }
 if(out&&capacity)out[n<capacity?n:capacity-1]=0;
 return n;
}
