#pragma once
#include <Adafruit_GFX.h>
#include <stdlib.h>
#include <string.h>
// Two palette indices per byte, high nibble first (also BMP 4-bit order).
class PaletteCanvas : public Adafruit_GFX {
  uint8_t *buffer;
public:
  PaletteCanvas(int16_t w,int16_t h):Adafruit_GFX(w,h),buffer((uint8_t*)calloc((size_t(w)*h+1)/2,1)){}
  ~PaletteCanvas(){free(buffer);}
  PaletteCanvas(const PaletteCanvas&)=delete;
  PaletteCanvas& operator=(const PaletteCanvas&)=delete;
  uint8_t* getBuffer() const{return buffer;}
  uint8_t getPixel(int16_t x,int16_t y) const {
    if(!buffer||x<0||y<0||x>=WIDTH||y>=HEIGHT)return 0;
    size_t p=size_t(y)*WIDTH+x;return (buffer[p/2]>>(p%2?0:4))&15;
  }
  void drawPixel(int16_t x,int16_t y,uint16_t color) override {
    if(!buffer||x<0||y<0||x>=_width||y>=_height)return;
    int16_t t;
    switch(rotation){case 1:t=x;x=WIDTH-1-y;y=t;break;case 2:x=WIDTH-1-x;y=HEIGHT-1-y;break;case 3:t=x;x=y;y=HEIGHT-1-t;break;}
    size_t p=size_t(y)*WIDTH+x;uint8_t &b=buffer[p/2];color&=15;
    b=p%2?(b&0xf0)|color:(b&0x0f)|(color<<4);
  }
  void fillScreen(uint16_t color) override {if(buffer)memset(buffer,(color&15)*17,(size_t(WIDTH)*HEIGHT+1)/2);}
  void drawFastHLine(int16_t x,int16_t y,int16_t w,uint16_t color) override {
    if(w<0){x+=w+1;w=-w;}
    if(y<0||y>=_height)return;
    int end=min(int(x)+w,int(_width));x=max(int(x),0);
    for(;x<end;x++)drawPixel(x,y,color);
  }
  void drawFastVLine(int16_t x,int16_t y,int16_t h,uint16_t color) override {
    if(h<0){y+=h+1;h=-h;}
    if(x<0||x>=_width)return;
    int end=min(int(y)+h,int(_height));y=max(int(y),0);
    for(;y<end;y++)drawPixel(x,y,color);
  }
};
