// for reference, https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html#secure-ota-updates


#ifndef MY_OTA_H
#define MY_OTA_H

int setupOTA();
void executeOTA();
//void resetOTA();
// void retry();
// void setup2();

//esp_err_t _http_event_handler(esp_http_client_event_t *evt);

int getUpdatePercent();

#endif