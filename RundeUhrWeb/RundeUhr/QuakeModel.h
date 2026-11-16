#pragma once
#include "Model.h"
struct QuakeEvent {float magnitude=0,depth=0;time_t at=0;char place[112]="";};
struct QuakeData {bool valid=false;uint8_t count=0;time_t generated=0,received=0;int http=0;char error[80]="";QuakeEvent events[5];};
inline bool quakeStale(const QuakeData&q,time_t now){return !q.valid||now<q.generated||now-q.generated>900;}
