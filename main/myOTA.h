// for reference, https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html#secure-ota-updates

#ifndef MY_OTA_H
#define MY_OTA_H

int setupOTA();
void executeOTA();

int getUpdatePercent();

#endif