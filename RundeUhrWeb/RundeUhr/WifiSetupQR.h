#pragma once
#include "qrcodegen.h"
#include <cstring>
// Nayuki QR encoder, MIT: https://github.com/nayuki/QR-Code-generator
// Vendored revision 9a217d443937f60cc56c1458c39a211d7f2d3af5.
// Fixed storage: no heap allocation while drawing or during a TLS request.
struct WifiSetupQR {
 static constexpr int MAX_VERSION=6;
 uint8_t code[qrcodegen_BUFFER_LEN_FOR_VERSION(MAX_VERSION)]={};
 uint8_t scratch[qrcodegen_BUFFER_LEN_FOR_VERSION(MAX_VERSION)]={};
 char previous[256]={};
 bool valid=false;
 static bool payload(const char* ssid,const char* key,char* out,size_t capacity){
  size_t n=0;bool ok=true;
  auto put=[&](char c){if(n+1<capacity)out[n++]=c;else ok=false;};
  auto raw=[&](const char* s){while(*s)put(*s++);};
  auto escaped=[&](const char* s){while(*s){if(strchr("\\;,:\"",*s))put('\\');put(*s++);}};
  raw("WIFI:T:WPA;S:");escaped(ssid);raw(";P:");escaped(key);raw(";;");
  if(capacity)out[n]=0;return ok;
 }
 bool prepare(const char* ssid,const char* key){
  char text[256];if(!payload(ssid,key,text,sizeof(text))){valid=false;previous[0]=0;return false;}
  if(!strcmp(text,previous))return valid;
  strcpy(previous,text);
  valid=qrcodegen_encodeText(text,scratch,code,qrcodegen_Ecc_MEDIUM,1,MAX_VERSION,qrcodegen_Mask_AUTO,true);
  return valid;
 }
};
inline bool showWifiSetup(bool ap,bool online,unsigned saved,uint32_t ms,uint32_t offlineSince,uint32_t manualUntil){
 return ap&&((!online&&(!saved||uint32_t(ms-offlineSince)>=45000U))||(manualUntil&&int32_t(manualUntil-ms)>0));
}
