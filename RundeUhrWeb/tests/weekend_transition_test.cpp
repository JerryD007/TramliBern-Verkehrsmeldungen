#include "../RundeUhr/Model.h"
#include <cassert>
#include <iostream>
time_t at(int day,int h,int m,int s=0){tm t={};t.tm_year=126;t.tm_mon=10;t.tm_mday=day;t.tm_hour=h;t.tm_min=m;t.tm_sec=s;t.tm_isdst=-1;return mktime(&t);}
int main(){setenv("TZ",ZURICH_TZ,1);tzset();Config c;assert(!weekendTransitionActive(at(20,16,0),c));c.weekendTransition=true;
assert(!weekendTransitionActive(at(20,15,59,59),c));assert(weekendTransitionActive(at(20,16,0),c));assert(weekendTransitionActive(at(20,16,29,59),c));assert(!weekendTransitionActive(at(20,16,30),c));assert(!weekendTransitionActive(at(21,16,0),c));assert(!weekendTransitionActive(0,c));
c.animations=false;assert(!weekendTransitionActive(at(20,16,0),c));c.animations=true;c.weekendStartDay=0;c.weekendStartMinute=1430;c.weekendEndDay=1;c.weekendEndMinute=60;
assert(weekendTransitionActive(at(23,0,0),c));assert(weekendTransitionActive(at(23,0,19,59),c));assert(!weekendTransitionActive(at(23,0,20),c));c.weekendEndMinute=5;assert(!weekendTransitionActive(at(23,0,5),c));std::cout<<"PASS defaults, start/end boundaries, invalid time, disabled animation, week rollover, early weekend end\n";}
