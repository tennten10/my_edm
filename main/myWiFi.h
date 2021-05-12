#ifndef MY_WIFI_H
#define MY_WIFI_H
#include "esp_wifi.h"

esp_netif_t* startWiFi();
bool verifyNetwork(char* s, char* p);
//wifi_ap_record_t* scanNetworks();
// esp_netif_t* getNetIF();

#endif