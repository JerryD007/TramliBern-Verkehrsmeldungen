#pragma once
#include "Model.h"
inline String deviceSlug(String s){
 const char* from[]={"ä","ö","ü","Ä","Ö","Ü","ß","á","č","ď","é","ě","í","ň","ó","ř","š","ť","ú","ů","ý","ž","Á","Č","Ď","É","Ě","Í","Ň","Ó","Ř","Š","Ť","Ú","Ů","Ý","Ž"};
 const char* to[]={"ae","oe","ue","Ae","Oe","Ue","ss","a","c","d","e","e","i","n","o","r","s","t","u","u","y","z","A","C","D","E","E","I","N","O","R","S","T","U","U","Y","Z"};
 for(unsigned i=0;i<sizeof(from)/sizeof(from[0]);i++)s.replace(from[i],to[i]);
 String slug;bool separator=false;
 for(unsigned i=0;i<s.length()&&slug.length()<23;i++){unsigned char c=s[i];if(c>='A'&&c<='Z')c+=32;
  if((c>='a'&&c<='z')||(c>='0'&&c<='9')){if(separator&&slug.length()&&slug.length()<22)slug+='-';if(slug.length()<23)slug+=char(c);separator=false;}
  else separator=true;
 }return slug;
}
inline String deviceHost(const Config&c,String suffix){String slug=deviceSlug(c.deviceName);return "rundeuhr-"+(slug.length()?slug:deviceSlug(suffix));}
inline String deviceSSID(const Config&c,String suffix){String slug=deviceSlug(c.deviceName);return "RundeUhr-"+(slug.length()?slug:suffix);}
inline String deviceLabel(const Config&c,String suffix){return strlen(c.deviceName)?String(c.deviceName):"RundeUhr-"+suffix;}
