#include "../RundeUhr/Model.h"
#include <cassert>
#include <iostream>
time_t at(int month,int day,int h,int m,int s=0){tm t={};t.tm_year=126;t.tm_mon=month-1;t.tm_mday=day;t.tm_hour=h;t.tm_min=m;t.tm_sec=s;t.tm_isdst=-1;return mktime(&t);}
int main(){setenv("TZ",ZURICH_TZ,1);tzset();Config c;
assert(!weekendTransitionActive(at(11,20,16,0),c));c.weekendTransition=true;
assert(!weekendTransitionActive(at(11,20,15,59,59),c));
assert(weekendTransitionActive(at(11,20,16,0),c));
assert(weekendTransitionActive(at(11,20,16,30),c));
assert(weekendTransitionActive(at(11,21,16,0),c));
assert(weekendTransitionActive(at(11,22,23,59,59),c));
assert(!weekendTransitionActive(at(11,23,0,0),c));
assert(weekendTransitionActive(at(11,27,16,0),c));
assert(!weekendTransitionActive(0,c));
c.animations=false;assert(!weekendTransitionActive(at(11,20,16,0),c));c.animations=true;
// An arbitrary short overnight window, crossing the Sunday/Monday boundary.
c.weekendStartDay=0;c.weekendStartMinute=1430;c.weekendEndDay=1;c.weekendEndMinute=5;
assert(!weekendTransitionActive(at(11,22,23,29),c));
assert(weekendTransitionActive(at(11,22,23,50),c));
assert(weekendTransitionActive(at(11,23,0,4,59),c));
assert(!weekendTransitionActive(at(11,23,0,5),c));
// Local calendar boundaries survive the spring and autumn DST changes.
c.weekendStartDay=5;c.weekendStartMinute=960;c.weekendEndDay=1;c.weekendEndMinute=0;
assert(weekendTransitionActive(at(3,29,3,30),c));assert(!weekendTransitionActive(at(3,30,0,0),c));
assert(weekendTransitionActive(at(10,25,3,30),c));assert(!weekendTransitionActive(at(10,26,0,0),c));
std::cout<<"PASS weekly figure: full interval, exact boundaries, next week, disabled flags, invalid time, midnight, DST\n";
}
