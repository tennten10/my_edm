/* Modified from WiFi station Example
   The example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
    modifications not public domain
*/
#include "globals.h"
#include "mySPIFFS.h"
#include "myFunctions.h"
#include "debug.h"

#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"



/* The examples use WiFi configuration that you can set via project configuration menu
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

static bool initFlag = false;
wifi_init_config_t cfg;
wifi_config_t wifi_config = {};

/* FreeRTOS event group to signal when we are connected */
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

//static const char *TAG = "wifi station";

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    debugPrintln("in event_handler");
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 10) { // replaced ESP_EXAMPLE_MAX_NUM_RETRY
            esp_wifi_connect();
            s_retry_num++;
            debugPrintln("retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        debugPrintln("connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        char temp[64];
        sprintf( temp ,"got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        debugPrintln(temp);
        s_retry_num = 0;
        
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}



int initWiFi(){
    //Initialize NVS
    if(!initFlag){
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
            // partition table. This size mismatch may cause NVS initialization to fail.
            // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
            // If this happens, we erase NVS partition and initialize NVS again.
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);

        

        ESP_ERROR_CHECK(esp_netif_init());

        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_sta();

        cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        wifi_config.sta.pmf_cfg.capable = true;
        wifi_config.sta.pmf_cfg.required = false;

        initFlag = true;
    }
    
    return 1;
}

int isInit(){
    return initFlag;
}

int stopWiFi(){
    esp_wifi_stop();
    esp_wifi_deinit();
    initFlag = false;
    return 1;
}
int disconnectWiFi(){
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    return 1;
}

int connectWiFi(WiFiStruct ws){
    int ret = 0;
    //if(!initFlag){
        esp_err_t ret0 = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
            // partition table. This size mismatch may cause NVS initialization to fail.
            // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
            // If this happens, we erase NVS partition and initialize NVS again.
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret0);

        

        ESP_ERROR_CHECK(esp_netif_init());

        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_sta();

        cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        wifi_config.sta.pmf_cfg.capable = true;
        wifi_config.sta.pmf_cfg.required = false;

    //    initFlag = true;
    //}

    s_wifi_event_group = xEventGroupCreate();
    debugPrintln("beginning of connectWiFi");
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    debugPrintln("event handler register");
    memcpy(wifi_config.sta.ssid, ws.ssid,
                               sizeof(ws.ssid));
    memcpy(wifi_config.sta.password, ws.pswd,
                               sizeof(ws.pswd));
    debugPrintln("set ssid and password");
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    debugPrintln("wifi_start :)");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
    debugPrintln("after eventbits");

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        debugPrint("connected to ap SSID:");
        debugPrint(ws.ssid);
        debugPrint(" password:");
        debugPrintln(ws.pswd);
        ret = 1;
    } else if (bits & WIFI_FAIL_BIT) {
        debugPrint("Failed to connect to SSID:");
        debugPrint(ws.ssid);
        debugPrint(" password:");
        debugPrintln(ws.pswd);
        ret = 2;
    } else {
        debugPrintln("UNEXPECTED EVENT");
        ret = 5;
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);

    return ret;
}


bool verifyWiFiInfo(WiFiStruct w){ // char& s, char& p){ //WiFiStruct wfi
    //char _s[32];
    //char _p[64];
    //strcpy(_s, &s);
    //strcpy(_p, &p);
    char s_temp[32];
    char p_temp[64];
    int ret = 0;


    if(!isInit()){

        s_wifi_event_group = xEventGroupCreate();
        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;
        
    }
    
    // if already connected, disconnect 

    disconnectWiFi();
    
    // save previous connection info
    if(esp_wifi_get_config(WIFI_IF_STA, &wifi_config) == ESP_OK){
        strcpy(s_temp, (char*)wifi_config.sta.ssid);
        strcpy(p_temp, (char*)wifi_config.sta.password);
    }
    
    // set new ssid, password
    ret = connectWiFi(w);


    // if successful, disconnect and return true
    if(ret == 1){
        disconnectWiFi();
        return true;
    }

    // if unsuccessful, reset old ssid, password and return false

    if(ret != 1){
        // successfully disconnected
        // reset to old ssid, password
        memcpy(wifi_config.sta.ssid, s_temp,
                               sizeof(s_temp));
        memcpy(wifi_config.sta.password, p_temp,
                               sizeof(p_temp));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        disconnectWiFi();
    }
    return false;

}



void scanNetworks(uint16_t& num, std::string * ap) {
    uint16_t networkNum = num; 
    wifi_ap_record_t apRecords[networkNum];
    // nvs init... ? not sure if I need to re-disable this when I'm done or not
    if(!initFlag)
    {   
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
            // partition table. This size mismatch may cause NVS initialization to fail.
            // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
            // If this happens, we erase NVS partition and initialize NVS again.
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);
    }
    // wifi init
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    // wifi scan, but without setting ssid, etc???
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    //ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, ) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = false
    };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    esp_err_t e = esp_wifi_scan_get_ap_records(&networkNum, apRecords);
    if( e != ESP_OK){
        debugPrintln("error when retrieving ap records");
        debugPrintln(e);
        if(e == ESP_ERR_WIFI_NOT_INIT){
            // WiFi is not initialized by esp_wifi_init
            debugPrintln("esp_wifi_not_init");
        }else if(e == ESP_ERR_WIFI_NOT_STARTED){ 
            // WiFi is not started by esp_wifi_start
            debugPrintln("esp_wifi_not_started");
        }else if(e == ESP_ERR_INVALID_ARG){
            // invalid argument
            debugPrintln("esp_invalid_arg");
        }else if(e == ESP_ERR_NO_MEM){
            //: out of memory
            debugPrintln("out of memory");
        }else{
            debugPrintln("unknown error");
        }
    }
    debugPrintln("scan complete, filling return values");
    num = networkNum;
    debugPrintln(num);

    for(int i =0; i < networkNum; i++){
       ap[i] = std::string((char*)apRecords[i].ssid);
       debugPrintln(ap[i]);
    }
    debugPrintln("end");
    disconnectWiFi();
    stopWiFi();

}