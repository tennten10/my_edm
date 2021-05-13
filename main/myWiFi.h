#ifndef MY_WIFI_H
#define MY_WIFI_H
#include "esp_wifi.h"
#include "globals.h"

esp_netif_t* startWiFi();
bool verifyWiFiInfo(char& s, char& p); //WiFiStruct wfi
void scanNetworks(uint16_t & num, wifi_ap_record_t & ap);
// esp_netif_t* getNetIF();

#endif