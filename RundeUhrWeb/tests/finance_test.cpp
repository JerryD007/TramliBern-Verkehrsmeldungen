#include <Adafruit_GFX.h>
#include "../RundeUhr/FinanceRender.h"
#include "../RundeUhr/FinanceParse.h"
#include "../RundeUhr/ConfigCodec.h"
#include <cassert>
#include <fstream>
void save(GFXcanvas8&g,const char*path){std::ofstream o(path,std::ios::binary);o<<"P6\n240 240\n255\n";for(int i=0;i<57600;i++){auto p=palette[g.getBuffer()[i]&15];char rgb[]={char(((p>>11)&31)*255/31),char(((p>>5)&63)*255/63),char((p&31)*255/31)};o.write(rgb,3);}}
int main(){setenv("TZ",ZURICH_TZ,1);tzset();time_t now;assert(parseDate("2026-11-15",now));now+=12*3600;Config c;FinanceData f;JsonDocument d;
 deserializeJson(d,R"({"data":{"base":"BTC","currency":"CHF","amount":"80000.12"}})");assert(parseFinance(d,f.btc,"BTC","CHF",true,now));assert(!parseFinance(d,f.btc,"BTC","EUR",true,now));
 for(auto value:{"NaN","-1","0","1.2oops","inf",""}){d["data"]["amount"]=value;assert(!parseFinance(d,f.btc,"BTC","CHF",true,now));}
 deserializeJson(d,R"({"date":"2026-11-13","base":"CHF","quote":"CZK","rate":26.50})");assert(parseFinance(d,f.fx[0],"CHF","CZK",false,now));d["date"]="2026-02-30";assert(!parseFinance(d,f.fx[1],"CHF","CZK",false,now));d["date"]="2099-01-01";assert(!parseFinance(d,f.fx[1],"CHF","CZK",false,now));
 f.fx[1]=f.fx[0];f.fx[1].value=25;f.fx[2]=f.fx[0];f.fx[2].value=.94;
 assert(!quoteStale(f.btc,now+599,true));assert(quoteStale(f.btc,now+601,true));strcpy(f.btc.error,"HTTP 429");assert(quoteStale(f.btc,now,true));f.btc.error[0]=0;
 assert(VIEW_COUNT==13&&c.mask==1011&&nextView(9,4095)==10&&nextView(10,4095)==11&&nextView(11,4095)==0&&nextView(9,1023)==0);
 GFXcanvas8 g(240,240);
 for(int view=10;view<12;view++){c.view=view;renderFinance(g,c,f,now,true);save(g,view==10?"preview/bitcoin.ppm":"preview/currency.ppm");for(int i=0;i<57600;i++)if((i%240-120)*(i%240-120)+(i/240-120)*(i/240-120)>119*119)assert(g.getBuffer()[i]==BG);}
 for(double price:{.00001,1.,80000.,1234567.,999999999999.}){f.btc.value=price;c.view=10;renderFinance(g,c,f,now,false);}
 // Successful observations, equal prices, errors and per-pair independence.
 Quote t;JsonDocument td;deserializeJson(td,R"({"data":{"base":"BTC","currency":"CHF","amount":"100"}})");
 assert(parseFinance(td,t,"BTC","CHF",true,now)&&t.trend==0);
 td["data"]["amount"]="101";assert(parseFinance(td,t,"BTC","CHF",true,now)&&t.trend==1);
 td["data"]["amount"]="bad";assert(!parseFinance(td,t,"BTC","CHF",true,now)&&t.value==101&&t.trend==1);
 td["data"]["amount"]="100";assert(parseFinance(td,t,"BTC","CHF",true,now)&&t.trend==-1);
 assert(parseFinance(td,t,"BTC","CHF",true,now)&&t.trend==0);
 f.btc=t;setQuoteValue(f.btc,80000);f.fx[0].trend=1;f.fx[1].trend=-1;f.fx[2].trend=0;
 auto arrowCount=[&](){int n=0;for(int k=0;k<57600;k++)n+=(g.getBuffer()[k]==RED||g.getBuffer()[k]==GREEN);return n;};
 for(int view=10;view<=11;view++){
  c.view=view;renderFinance(g,c,f,now,true);assert(arrowCount()>0);save(g,view==10?"preview/rc13_bitcoin.ppm":"preview/rc13_currency.ppm");
  for(int k=0;k<57600;k++)if((k%240-120)*(k%240-120)+(k/240-120)*(k/240-120)>119*119)assert(g.getBuffer()[k]==BG);
  renderFinance(g,c,f,now,false);assert(arrowCount()==0);
  renderFinance(g,c,f,now+86401,true);assert(arrowCount()==0);
 }
 for(int direction:{-1,0,1}){t.trend=direction;g.fillScreen(BG);financeArrow(g,t,now,true,true,211,120);assert((arrowCount()>0)==(direction!=0));if(direction)assert(g.getBuffer()[(120+(direction>0?-11:11))*240+211]==(direction>0?GREEN:RED));}
 puts("PASS finance trends: first/equal/up/down, invalid data retention, independent pairs, green up/red down directions, stale/offline hidden and circular bounds.");
 Config original;strcpy(original.ssid,"saved");strcpy(original.pass,"secret");
 deserializeJson(d,R"({"schema":4,"city":"Bern","lat":46.948,"lon":7.4474,"event":"Ferien","date":"","view":4,"mask":508,"interval":5,"automatic":true,"countdownType":0,"animations":true,"customEvent":"Mein Ereignis","countdownDates":["","2027-03-25","","2027-05-31",""]})");assert(decodeConfig(d,original));assert(original.mask==490&&!strcmp(original.pass,"secret")&&!strcmp(original.btcCurrency,"CHF"));
 d["view"]=11;d["mask"]=4095;d["btcCurrency"]="USD";assert(decodeConfig(d,original));assert(original.view==11&&original.mask==4075&&!strcmp(original.pass,"secret"));
 Config before=original;d["btcCurrency"]="XXX";assert(!decodeConfig(d,original));assert(!memcmp(&before,&original,sizeof(before)));
 puts("PASS finance: parsers, malformed/nonfinite/negative/mismatched/future data, stale handling, 12-view cycles, original schema4 migration and credentials, invalid settings atomicity, circular renderer.");
}
