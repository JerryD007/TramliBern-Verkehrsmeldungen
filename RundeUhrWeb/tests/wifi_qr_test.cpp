#include <Adafruit_GFX.h>
#include "../RundeUhr/Render.h"
#include <cassert>
#include <fstream>
#include <string>
void save(GFXcanvas8& g,const char* path){
 std::ofstream f(path,std::ios::binary);f<<"P6\n240 240\n255\n";
 for(int y=0;y<240;y++)for(int x=0;x<240;x++){
  int i=g.getPixel(x,y);assert(i>=0&&i<16);
  if((x-120)*(x-120)+(y-120)*(y-120)>119*119)assert(i==BG);
  uint16_t c=palette[i];char rgb[]={char(((c>>11)&31)*255/31),char(((c>>5)&63)*255/63),char((c&31)*255/31)};f.write(rgb,3);
 }
}
int main(){
 char out[256];assert(WifiSetupQR::payload("RundeUhr-Test","AB23456789CD",out,sizeof(out)));
 assert(std::string(out)=="WIFI:T:WPA;S:RundeUhr-Test;P:AB23456789CD;;");
 assert(WifiSetupQR::payload("A;B:C,D\\E\"","pass;word",out,sizeof(out)));
 assert(std::string(out)=="WIFI:T:WPA;S:A\\;B\\:C\\,D\\\\E\\\";P:pass\\;word;;");
 char tiny[4];assert(!WifiSetupQR::payload("abc","def",tiny,sizeof(tiny)));assert(tiny[3]==0);
 assert(!WifiSetupQR::payload("abc","def",nullptr,0));
 WifiSetupQR q;assert(q.prepare("RundeUhr-Test","AB23456789CD"));auto original=q.code[1];assert(q.prepare("RundeUhr-Test","AB23456789CD"));assert(q.code[1]==original);
 assert(q.prepare("RundeUhr-Another","AB23456789CD"));
 std::string huge(300,'x');assert(!q.prepare(huge.c_str(),"AB23456789CD"));assert(q.prepare("RundeUhr-Another","AB23456789CD"));
 assert(showWifiSetup(true,false,0,0,0,0));
 assert(!showWifiSetup(true,true,1,90000,0,0));
 assert(!showWifiSetup(true,false,1,44999,0,0));
 assert(showWifiSetup(true,false,1,45000,0,0));
 assert(showWifiSetup(true,false,1,999999,0,0));
 assert(!showWifiSetup(false,false,0,999999,0,0));
 assert(showWifiSetup(true,true,1,1000,0,2000));
 assert(!showWifiSetup(true,true,1,2000,0,2000));
 assert(showWifiSetup(true,false,1,20000,UINT32_MAX-30000,0));
 Config c;Weather w;Moon m;GFXcanvas8 g(240,240);
 render(g,c,w,m,0,false,true,"RundeUhr-Test","AB23456789CD");save(g,"build/wifi_setup.ppm");
 std::string worst(32,';');render(g,c,w,m,0,false,true,worst.c_str(),"AB23456789CD");save(g,"build/wifi_setup_escaped.ppm");
 puts("PASS: Wi-Fi QR escaping, overflow, cache, reconnect grace, offline visibility, timer wrap and actual round renderer bounds");
}
