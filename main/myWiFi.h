#ifndef MY_WIFI_H
#define MY_WIFI_H
#include "esp_wifi.h"
#include "globals.h"
#include <string>

int initWiFi();
int isInit();
int stopWiFi();
int connectWiFi(WiFiStruct ws);
int disconnectWiFi();

bool verifyWiFiInfo(WiFiStruct w); //char& s, char& p); //WiFiStruct wfi

void scanNetworks(uint16_t& num, std::string* ap);


#endif