#include "host/Arduino.h"
#include "../RundeUhr/ParseData.h"
#include <fstream>
#include <cassert>
int main(int argc,char**argv){assert(argc==2);std::ifstream f(argv[1]);JsonDocument d;assert(!deserializeJson(d,f));Weather w;String error;setenv("TZ",ZURICH_TZ,1);tzset();assert(parseWeather(d,w,error,time(nullptr),1));assert(w.valid&&w.count==72&&w.days==3);printf("PASS live weather: firmware parser, %d hours, %d days, TLS verified by host curl (not ESP32)\n",w.count,w.days);}
