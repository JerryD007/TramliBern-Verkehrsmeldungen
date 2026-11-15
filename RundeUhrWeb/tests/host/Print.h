#pragma once
#include "Arduino.h"
class Print{public:virtual size_t write(uint8_t)=0;size_t write(const char*s){size_t n=0;while(*s)n+=write(uint8_t(*s++));return n;}size_t print(const String&s){return write(s.c_str());}size_t print(const char*s){return write(s);}};
