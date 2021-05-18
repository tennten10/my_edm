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
//#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
//#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
//#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

WiFiStruct wifi;

/* FreeRTOS event group to signal when we are connected*/
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

void wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Not sure what this is or if it was part of the example or my attempt at transferring code over... maybe uncomment later?
    // esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    // // Prefix the interface description with the module TAG
    // // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
    // asprintf(&desc, "%s: %s", TAG, esp_netif_config.if_desc);
    // esp_netif_config.if_desc = desc;
    // esp_netif_config.route_prio = 128;
    // esp_netif_t *netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);

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
  

    wifi_config_t wifi_config ={};

    memcpy(wifi_config.sta.ssid, wifi.ssid,
                               sizeof(wifi.ssid));
    memcpy(wifi_config.sta.password, wifi.pswd,
                               sizeof(wifi.pswd));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    debugPrintln("wifi_init_sta finished.");

    // WHAT IS GOING ON HERE??????? NOT GETTING PAST EVENT GROUP...

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
        debugPrint(wifi.ssid);
        debugPrint(" password:");
        debugPrintln(wifi.pswd);
        
        
    } else if (bits & WIFI_FAIL_BIT) {
        debugPrint("Failed to connect to SSID:");
        debugPrint(wifi.ssid);
        debugPrint(" password:");
        debugPrintln(wifi.pswd);

    } else {
        debugPrintln("UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

void startWiFi()
{
    wifi =  _sys->getWiFiInfo(); //getActiveWifiInfo();
    if(strcmp(wifi.ssid,"")==0){
        debugPrintln("WIFI Info NOT LOADED. Exiting WIFI Functions.");
        return; 
    }else{
        //Initialize NVS
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

        debugPrintln("ESP_WIFI_MODE_STA");
        wifi_init_sta();
        
    }
}

bool verifyWiFiInfo(char& s, char& p){ //WiFiStruct wfi
    char _s[32];
    char _p[64];
    strcpy(_s, &s);
    strcpy(_p, &p);
    char s_temp[32];
    char p_temp[64];
    int ret = 0;


    s_wifi_event_group = xEventGroupCreate();
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    

    
    // if already connected, disconnect 
    wifi_config_t wfi;
    esp_err_t h = esp_wifi_disconnect();
    if(h == ESP_FAIL){
        debugPrintln("ESP FAIL IN VERIFY WIFI INFO");
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
        vEventGroupDelete(s_wifi_event_group);
        return false;
    }
    if(h == ESP_OK){
        // successfully disconnected
        // temporarily save old ssid, password
        if(esp_wifi_get_config(WIFI_IF_STA, &wfi) == ESP_OK){
            strcpy(s_temp, (char*)wfi.sta.ssid);
            strcpy(p_temp, (char*)wfi.sta.password);
        }
    }
    
    if(h == ESP_ERR_WIFI_NOT_INIT){
        ESP_ERROR_CHECK(esp_netif_init());

        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        debugPrintln("wifi not init in verify wifi info");
    }
        
    

    // set new ssid, password

    memcpy(wfi.sta.ssid, _s,
                               sizeof(_s));
    memcpy(wfi.sta.password, _p,
                               sizeof(_p));
    //wfi.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    //wfi.sta.pmf_cfg.capable = true;
    //wfi.sta.pmf_cfg.required = false;
    
    // try to connect
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wfi) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    debugPrintln("wifi_init_sta finished.");    

    
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
    

    if (bits & WIFI_CONNECTED_BIT) {
        debugPrint("connected to ap SSID:");
        debugPrint(_s);
        debugPrint(" password:");
        debugPrintln(_p);
        ret = 1;
        
        
    } else if (bits & WIFI_FAIL_BIT) {
        debugPrint("Failed to connect to SSID:");
        debugPrint(_s);
        debugPrint(" password:");
        debugPrintln(_p);
        
    } else {
        debugPrintln("UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);

    // if successful, disconnect and return true
    if(ret == 1){
        ESP_ERROR_CHECK(esp_wifi_deinit());
        return true;
    }

    // if unsuccessful, reset old ssid, password and return false

    if(h == ESP_OK){
        // successfully disconnected
        // reset to old ssid, password
        memcpy(wfi.sta.ssid, s_temp,
                               sizeof(s_temp));
        memcpy(wfi.sta.password, p_temp,
                               sizeof(p_temp));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wfi) );
        ESP_ERROR_CHECK(esp_wifi_deinit());
        
    }
    return false;

}



void scanNetworks(uint16_t& num, std::string * ap) {
    uint16_t networkNum = num; 
    wifi_ap_record_t apRecords[networkNum];
    // nvs init... ? not sure if I need to re-disable this when I'm done or not
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

}