#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <algorithm>
using std::min;using std::max;
#define PROGMEM
#define F(x) x
class __FlashStringHelper;
class String:public std::string{public:using std::string::string;String():std::string(){}String(const std::string&s):std::string(s){}String(int n):std::string(std::to_string(n)){}String(long n):std::string(std::to_string(n)){}String(double n,int digits){char b[80];snprintf(b,80,"%.*f",digits,n);assign(b);}unsigned length()const{return size();}String substring(int a,int b)const{return substr(a,b-a);}void replace(const char*a,const char*b){size_t p=0;while((p=find(a,p))!=npos){std::string::replace(p,strlen(a),b);p+=strlen(b);}}};
inline String operator+(const String&a,const String&b){return String(static_cast<const std::string&>(a)+static_cast<const std::string&>(b));}
inline String operator+(const char*a,const String&b){return String(a)+b;}
inline String operator+(const String&a,const char*b){return a+String(b);}
inline void yield(){}
