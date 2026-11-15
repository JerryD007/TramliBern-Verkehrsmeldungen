#pragma once
static const char* const CURRENCIES[]={"AUD","BRL","CAD","CHF","CNY","CZK","DKK","EUR","GBP","HKD","HUF","IDR","ILS","INR","ISK","JPY","KRW","MXN","MYR","NOK","NZD","PHP","PLN","RON","SEK","SGD","THB","TRY","USD","ZAR"};
inline bool validCurrency(const char*s){if(!s||strlen(s)!=3)return false;for(auto c:CURRENCIES)if(!strcmp(c,s))return true;return false;}
