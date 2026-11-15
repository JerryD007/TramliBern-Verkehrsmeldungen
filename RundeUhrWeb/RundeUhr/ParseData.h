#pragma once
#include "ConfigCodec.h"
// Keep independent products usable. A missing rain value must not discard the sun.
inline bool parseWeather(JsonDocument &d,Weather&w,String &error,time_t now,uint32_t fetched){
 Weather out;auto h=d["hourly"];size_t n=h["time"].size();bool incomplete=n<3;
 if(n<=72){for(size_t i=0;i<n;i++){
  if(!number(h["time"][i])){incomplete=true;continue;}
  time_t hour=h["time"][i].as<int64_t>();if(!timeValid(hour)||(out.count&&hour<=out.hours[out.count-1])){incomplete=true;continue;}
  int j=out.count++;out.hours[j]=hour;
  auto temp=h["temperature_2m"][i],code=h["weather_code"][i],prob=h["precipitation_probability"][i],amount=h["precipitation"][i];
  if(number(temp)){double v=temp.as<double>();if(isfinite(v)&&v>=-100&&v<=65){out.temps[j]=v;out.fields[j]|=1;}}
  if(code.is<int>()&&code.as<int>()>=0&&code.as<int>()<=99){out.codes[j]=code;out.fields[j]|=2;}
  if(number(prob)){double v=prob.as<double>();if(isfinite(v)&&v>=0&&v<=100){out.probability[j]=lround(v);out.fields[j]|=4;}}
  if(number(amount)){double v=amount.as<double>();if(isfinite(v)&&v>=0){out.amount[j]=v;out.fields[j]|=8;}}
  if(out.fields[j]!=15)incomplete=true;
 }}else incomplete=true;
 auto day=d["daily"];n=day["time"].size();if(n>3)n=3;
 for(size_t i=0;i<n;i++){
  if(!number(day["time"][i]))continue;time_t date=day["time"][i].as<int64_t>();if(!timeValid(date))continue;
  int j=out.days++;out.day[j]=date+12*3600;
  if(number(day["sunrise"][i])&&number(day["sunset"][i])){
   time_t rise=day["sunrise"][i].as<int64_t>(),set=day["sunset"][i].as<int64_t>();
   if(timeValid(rise)&&timeValid(set)&&set>rise){out.sunrise[j]=rise;out.sunset[j]=set;}
  }
 }
 bool usable=false;for(int i=0;i<out.count;i++)if(out.hours[i]>now&&out.fields[i])usable=true;
 int today=sunDay(out,now);if(today>=0&&out.sunrise[today]&&out.sunset[today])usable=true;
 if(!usable){error="Keine nutzbaren Wetter-/Sonnendaten";return false;}
 out.valid=true;out.received=now;out.fetched=fetched;w=out;
 error=incomplete?"Einzelne Stundenwerte fehlen (Anzeige --)":"";return true;
}
