#ifndef MY_WIFI_H
#define MY_WIFI_H
#include "esp_wifi.h"
#include "globals.h"
#include <string>


// int isInit();   // for bools, checks flag I set
int stopWiFi(); // disconnects and deinits
int connectWiFi(WiFiStruct ws); // inits
int disconnectWiFi(); // just disconnects

bool verifyWiFiInfo(WiFiStruct w); // inits, connects, deints, returns

void scanNetworks(uint16_t& num, std::string* ap); // 


#endif