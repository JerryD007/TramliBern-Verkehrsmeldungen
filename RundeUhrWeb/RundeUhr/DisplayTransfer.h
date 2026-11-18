#pragma once
// One SPI transaction and address window per frame. Only a scanline buffer is needed.
template<class Display,class Canvas>
inline void transferCanvas(Display&display,Canvas&canvas,const uint16_t*colors,bool newsBand){
 const int x0=newsBand?28:0,y0=newsBand?100:0,width=newsBand?184:240,height=newsBand?32:240;
 static uint16_t row[240];
 display.startWrite();display.setAddrWindow(x0,y0,width,height);
 for(int y=y0;y<y0+height;y++){for(int x=0;x<width;x++)row[x]=colors[canvas.getPixel(x0+x,y)];display.writePixels(row,width);}
 display.endWrite();
}
