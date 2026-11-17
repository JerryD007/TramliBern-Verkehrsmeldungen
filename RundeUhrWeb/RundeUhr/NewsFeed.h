#pragma once
#include <Stream.h>
#include "NewsModel.h"
#include "AircraftRoute.h"
// RSS may contain old but valid articles; launch dates have a stricter lower bound.
inline bool newsUTC(const char*s,time_t&out){
 if(!s||strlen(s)!=20||s[4]!='-'||s[7]!='-'||s[10]!='T'||s[13]!=':'||s[16]!=':'||s[19]!='Z')return false;
 for(int i=0;i<19;i++)if(i!=4&&i!=7&&i!=10&&i!=13&&i!=16&&(s[i]<'0'||s[i]>'9'))return false;
 int y,m,d,h,n,sec;if(sscanf(s,"%4d-%2d-%2dT%2d:%2d:%2dZ",&y,&m,&d,&h,&n,&sec)!=6||y<1970||y>2099||m<1||m>12||h>23||n>59||sec>59)return false;
 int days[]={31,28+int(launchLeap(y)),31,30,31,30,31,31,30,31,30,31};if(d<1||d>days[m-1])return false;
 int64_t total=0;for(int z=1970;z<y;z++)total+=365+int(launchLeap(z));for(int z=0;z<m-1;z++)total+=days[z];
 out=time_t(0LL+(total+d-1)*86400+h*3600+n*60+sec);return true;
}
// Bounded streaming RSS reader: three headlines only; descriptions/images discarded.
// XML is parsed through the final </rss>; incomplete responses never replace cache.
class NewsFeed:public Stream {
 NewsData result;NewsItem item;char tag[192]={},text[768]={},stack[20][32]={};
 size_t tn=0,used=0,total=0;int depth=0,field=0,mode=0;bool inItem=false,bad=false,closed=false,cnn=false,cnnSource=false;char quote=0,lastTagChar=0;
 void append(char c){if(field&&used+1<sizeof(text)){text[used++]=c;text[used]=0;}}
 void finishField(){
  if(!field)return;
  // Decode the five XML entities and numeric Unicode entities, no HTML execution.
  char decoded[768]={};size_t j=0;
  for(size_t i=0;i<used&&j+5<sizeof(decoded);i++){
   if(text[i]=='&'){const char*e=strchr(text+i,';');if(!e||e-(text+i)>12){bad=true;break;}char entity[16]={};memcpy(entity,text+i+1,e-(text+i)-1);unsigned cp=0;
    if(!strcmp(entity,"amp"))cp='&';else if(!strcmp(entity,"lt"))cp='<';else if(!strcmp(entity,"gt"))cp='>';else if(!strcmp(entity,"quot"))cp='"';else if(!strcmp(entity,"apos"))cp='\'';else if(entity[0]=='#'){char*end;cp=strtoul(entity+(entity[1]=='x'?2:1),&end,entity[1]=='x'?16:10);if(*end)cp=0;}
    if(!cp||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff)){bad=true;break;}i=e-text;
    if(cp<128)decoded[j++]=char(cp);else if(cp<2048){decoded[j++]=0xc0|(cp>>6);decoded[j++]=0x80|(cp&63);}else if(cp<65536){decoded[j++]=0xe0|(cp>>12);decoded[j++]=0x80|((cp>>6)&63);decoded[j++]=0x80|(cp&63);}else {decoded[j++]=0xf0|(cp>>18);decoded[j++]=0x80|((cp>>12)&63);decoded[j++]=0x80|((cp>>6)&63);decoded[j++]=0x80|(cp&63);}
   }else decoded[j++]=text[i];
  }
  if(field==1)routeText(item.title,sizeof(item.title),decoded);
  if(field==2){
   const char* fallback=nullptr;
   if(cnn){if(!strncmp(decoded,"https://news.google.com/rss/articles/",37)||!strncmp(decoded,"https://news.google.com/articles/",33))fallback="https://www.cnn.com/";else bad=true;}
   else if(!strncmp(decoded,"https://ct24.ceskatelevize.cz/",30))fallback="https://ct24.ceskatelevize.cz/";
   else if(!strncmp(decoded,"https://www.tagesschau.de/",25))fallback="https://www.tagesschau.de/";
   else if(!strncmp(decoded,"https://www.srf.ch/",18))fallback="https://www.srf.ch/";
   else bad=true;
   // Titles are shown on-device and in Safari; retain a publisher landing URL for
   // aggregator/oversized URLs instead of allocating a multi-kilobyte link buffer.
   if(fallback)strcpy(item.link,cnn||strlen(decoded)>=sizeof(item.link)?fallback:decoded);
  }
  if(field==5)cnnSource=!strcmp(decoded,"CNN");
  if(field==3&&!newsUTC(decoded,item.at))bad=true;
  if(field==4&&!item.at){char day[4],mon[4],zone[8];int dd,yy,h,m,sec;const char*months[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
   if(sscanf(decoded,"%3s, %d %3s %d %d:%d:%d %7s",day,&dd,mon,&yy,&h,&m,&sec,zone)!=8){bad=true;}else {int month=0;for(int k=0;k<12;k++)if(!strcmp(mon,months[k]))month=k+1;char iso[24];snprintf(iso,sizeof(iso),"%04d-%02d-%02dT%02d:%02d:%02dZ",yy,month,dd,h,m,sec);time_t t;if(!newsUTC(iso,t))bad=true;else if(!strcmp(zone,"GMT")||!strcmp(zone,"UTC"))item.at=t;else {int zh,zm;if(strlen(zone)!=5||(zone[0]!='+'&&zone[0]!='-')||sscanf(zone+1,"%2d%2d",&zh,&zm)!=2||zh>23||zm>59)bad=true;else item.at=t-(zone[0]=='+'?1:-1)*(zh*3600+zm*60);}}
  }
  field=0;used=0;text[0]=0;
 }
 void token(){
  if(tag[0]=='?')return;
  if(tag[0]=='!'){bad=true;return;} // no DTD/entity expansion
  bool end=tag[0]=='/';char name[32]={};size_t k=end?1:0,j=0;
  while(tag[k]&&tag[k]!=' '&&tag[k]!='\t'&&tag[k]!='/'&&j<31)name[j++]=tag[k++];
  if(!name[0]||j==31){bad=true;return;}bool self=lastTagChar=='/';
  if(end){if(!depth||strcmp(stack[depth-1],name)){bad=true;return;}
   if(inItem&&field&&(!strcmp(name,"title")||!strcmp(name,"link")||!strcmp(name,"dc:date")||!strcmp(name,"pubDate")||!strcmp(name,"source")))finishField();
   if(!strcmp(name,"item")){if(!item.title[0]||!item.link[0]||!item.at)bad=true;else if(!cnn||cnnSource){int pos=0;while(pos<result.count&&result.items[pos].at>=item.at)pos++;if(pos<3){if(result.count<3)result.count++;for(int k=result.count-1;k>pos;k--)result.items[k]=result.items[k-1];result.items[pos]=item;}}inItem=false;}
   depth--;if(!strcmp(name,"rss"))closed=true;
  }else if(!self){if((depth==0&&strcmp(name,"rss"))||depth>=20||closed){bad=true;return;}strcpy(stack[depth++],name);
   if(!strcmp(name,"item")){if(inItem){bad=true;return;}inItem=true;item=NewsItem();cnnSource=false;}
   else if(inItem&&depth>=2&&!strcmp(stack[depth-2],"item")){field=!strcmp(name,"title")?1:!strcmp(name,"link")?2:!strcmp(name,"dc:date")?3:!strcmp(name,"pubDate")?4:!strcmp(name,"source")?5:0;used=0;text[0]=0;}
  }
 }
 public:
 explicit NewsFeed(bool cnnAggregator=false):cnn(cnnAggregator){}
 size_t write(uint8_t c)override{return write(&c,1);}
 size_t write(const uint8_t*p,size_t n)override{
  if(total+n>262144){bad=true;return 0;}total+=n;
  for(size_t i=0;i<n;i++){char c=p[i];if(bad)continue;
   if(mode==2||mode==3){ // CDATA/comment terminator, retain two pending bytes
    tag[tn++]=c;if(tn>=3){bool done=mode==2?tag[tn-3]==']'&&tag[tn-2]==']'&&c=='>':tag[tn-3]=='-'&&tag[tn-2]=='-'&&c=='>';
     if(done){tn=0;mode=0;continue;}if(mode==2){if(tag[0]=='&'){for(const char*p="&amp;";*p;p++)append(*p);}else append(tag[0]);}tag[0]=tag[1];tag[1]=tag[2];tn=2;}continue;
   }
   if(mode==0){if(c=='<'){mode=1;tn=0;quote=0;lastTagChar=0;}else append(c);continue;}
   if(c=='>'&&!quote){tag[tn]=0;token();tn=0;mode=0;continue;}
   lastTagChar=c;if(tn+1<sizeof(tag)){tag[tn++]=c;tag[tn]=0;}if(!strcmp(tag,"![CDATA[")){mode=2;tn=0;}else if(!strcmp(tag,"!--")){mode=3;tn=0;}else if(c=='\''||c=='"'){if(!quote)quote=c;else if(quote==c)quote=0;}
  }return n;
 }
 bool finish(NewsData&out,time_t now){if(bad||!closed||depth||mode)return false;result.valid=true;result.received=now;result.http=out.http;result.source=out.source;out=result;return true;}
 bool failed()const{return bad;}
 size_t size()const{return total;}
 int available()override{return 0;}int read()override{return -1;}int peek()override{return -1;}void flush()override{}
};
