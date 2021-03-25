#ifndef AWS_OTA_H
#define AWS_OTA_H
//void setSSID(char &text);
//void setPSWD(char &text);
void setupOTA();
void execOTA(int attempt);
void retry();
void setup2();

//esp_err_t _http_event_handler(esp_http_client_event_t *evt);

int getUpdatePercent();

#endif