#pragma once
#include <stdint.h>
#include <stddef.h>
// Czech glyph slots 1..30 are rendered by NewsBand, never sent to the built-in font.
static constexpr uint16_t NEWS_CZECH_CODEPOINTS[]={0xe1,0x10d,0x10f,0xe9,0x11b,0xed,0x148,0xf3,0x159,0x161,0x165,0xfa,0x16f,0xfd,0x17e,0xc1,0x10c,0x10e,0xc9,0x11a,0xcd,0x147,0xd3,0x158,0x160,0x164,0xda,0x16e,0xdd,0x17d};
static constexpr uint8_t NEWS_CZECH_ROWS[][8]={
 {2,4,0,14,1,15,17,15}, // á
 {10,4,0,14,16,16,17,14}, // č
 {10,4,1,1,15,17,17,15}, // ď
 {2,4,0,14,17,31,16,14}, // é
 {10,4,0,14,17,31,16,14}, // ě
 {2,4,0,0,12,4,4,14}, // í
 {10,4,0,30,17,17,17,17}, // ň
 {2,4,0,14,17,17,17,14}, // ó
 {10,4,0,22,25,16,16,16}, // ř
 {10,4,0,15,16,14,1,30}, // š
 {10,4,8,28,8,8,9,6}, // ť
 {2,4,0,17,17,17,19,13}, // ú
 {4,10,4,17,17,17,19,13}, // ů
 {2,4,0,17,17,15,1,14}, // ý
 {10,4,0,31,2,4,8,31}, // ž
 {2,4,14,17,17,31,17,17}, // Á
 {10,4,14,17,16,16,17,14}, // Č
 {10,4,30,17,17,17,17,30}, // Ď
 {2,4,31,16,30,16,16,31}, // É
 {10,4,31,16,30,16,16,31}, // Ě
 {2,4,14,4,4,4,4,14}, // Í
 {10,4,17,25,21,19,17,17}, // Ň
 {2,4,14,17,17,17,17,14}, // Ó
 {10,4,30,17,30,20,18,17}, // Ř
 {10,4,15,16,14,1,17,14}, // Š
 {10,4,31,4,4,4,4,4}, // Ť
 {2,4,17,17,17,17,17,14}, // Ú
 {4,10,4,17,17,17,17,14}, // Ů
 {2,4,17,17,10,4,4,4}, // Ý
 {10,4,31,1,2,4,8,31}, // Ž
};
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
  for(unsigned k=0;k<30;k++)if(cp==NEWS_CZECH_CODEPOINTS[k]){glyph=k+1;break;}
  if(glyph!='?'){if(out&&n+1<capacity)out[n]=char(glyph);n++;continue;}
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
