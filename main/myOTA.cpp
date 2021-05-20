#include "a_config.h"
#include "myWiFi.h"
#include "globals.h"
#include "debug.h"




/* Based on Advanced HTTPS OTA example - 
   
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "System.h"
#include "esp-nimble-cpp/src/NimBLEDevice.h"

SemaphoreHandle_t updateMutex;
extern SystemX *_sys;
extern NimBLEServer *pServer;

// // copied from esp_https_ota.c
// // needed to access the http header... there should be an easier way to do it 
// typedef enum {
//     ESP_HTTPS_OTA_INIT,
//     ESP_HTTPS_OTA_BEGIN,
//     ESP_HTTPS_OTA_IN_PROGRESS,
//     ESP_HTTPS_OTA_SUCCESS,
// } esp_https_ota_state;

// struct esp_https_ota_handle {
//     esp_ota_handle_t update_handle;
//     const esp_partition_t *update_partition;
//     esp_http_client_handle_t http_client;
//     char *ota_upgrade_buf;
//     size_t ota_upgrade_buf_size;
//     int binary_file_len;
//     esp_https_ota_state state;
// };
// // end copy

// // my function to handle the weird type conversions to access http headers
// int get_http_header(esp_https_ota_handle_t https_ota_handle){
//     esp_https_ota_handle *handle = (esp_https_ota_handle *)https_ota_handle;
//     if (handle == NULL)  {
//         debugPrintln("esp_https_ota_read_img_desc: Invalid argument");
//         return ESP_ERR_INVALID_ARG;
//     }
//     if (handle->state < ESP_HTTPS_OTA_BEGIN) {
//         debugPrintln("esp_https_ota_read_img_desc: Invalid state");
//         return ESP_FAIL;
//     }
//     debugPrintln("getting http headers...");

//     return esp_http_client_fetch_headers(handle->http_client);

// }
// // end weird function

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

#define OTA_URL_SIZE 256

static esp_err_t validate_image_header(esp_app_desc_t *new_app_info) // This checks current version, new version
{
    if (new_app_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        debugPrint("Running firmware version: ");
        debugPrintln(running_app_info.version);
    }

    // does the version check
    if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
        debugPrintln("Current running version is the same as a new. We will not continue the update.");
        
        return ESP_FAIL;
    }


    return ESP_OK;
}

void ota_task(void *pvParameter)
{
    debugPrintln("Starting OTA Task");

        //Retrieves SSID & Password from SPIFFS filesystem
    //Starts WiFi in STA mode    
    debugPrintln("before initWiFi");    
    // if(!isInit()){                                                               //Run flags here to see what goes wrong
    //     initWiFi();
    // }
    // debugPrintln("after initWiFi");
    connectWiFi(_sys->getWiFiInfo());
    debugPrintln("after connect WiFi");
    if(esp_wifi_get_mode(NULL)==ESP_OK){
      // if it gets in here wifi connection is successful
      debugPrintln("inside wifi flag...");
    }else{
        debugPrintln(" not in wifi thingy if statement");
    }

    /* Ensure to disable any WiFi power save mode, this allows best throughput
     * and hence timings for overall OTA operation.
     */
    // Since WiFi and BT working together needs one not taking all the cpu time, we need some sleep time for it to work. WIFI_PS_MIN_MODEM
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM); //WIFI_PS_NONE);




    esp_err_t ota_finish_err = ESP_OK;
    esp_http_client_config_t config = {
        .url = UPDATE_URL, // CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL,
        //.cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = CONFIG_EXAMPLE_OTA_RECV_TIMEOUT,
    };

    debugPrintln("Set http client config");
#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };
    debugPrintln("Set ota http config");
    esp_https_ota_handle_t https_ota_handle;// = NULL;
    debugPrintln("before beginning ota");
    //debugPrintln(ota_config.http_config);
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);

    int full_size = esp_https_ota_get_image_size(https_ota_handle);
    
    if (err != ESP_OK) {
        debugPrintln("ESP HTTPS OTA Begin failed");
        vTaskDelete(NULL);
    }

    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    long long int t = 0;
    int current = 0;
    int last = 0;
    
    if (err != ESP_OK) {
        debugPrintln("esp_https_ota_read_img_desc failed");
        goto ota_end;
    }
    err = validate_image_header(&app_desc);
    if (err != ESP_OK) {
        debugPrintln("image header verification failed");
        goto ota_end;
    }
    debugPrintln("Made it past header verification so that should be fine");
    if(full_size < 1){
        debugPrintln("error reading header file size, printed from ota_task");
        full_size = 1;
    }else{
        debugPrint("Full Size: ");
        debugPrintln(full_size);
    }
    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        // esp_https_ota_perform returns after every read operation which gives user the ability to
        // monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
        // data read so far.
        
        t += esp_https_ota_get_image_len_read(https_ota_handle);
        debugPrintln(std::to_string(t));
        current = t/full_size;
        if( current > last){
            _sys->display->displayUpdateScreen(current);
            last = current;
            debugPrint("Image progress: ");
            debugPrintln(last);
        }
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        // the OTA image was not completely received and user can customise the response to this situation.
        debugPrintln("Complete data was not received.");
    }

ota_end:
    ota_finish_err = esp_https_ota_finish(https_ota_handle);
    if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
        _sys->display->displayLogo();
        debugPrintln("ESP_HTTPS_OTA upgrade successful. Rebooting ...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        _sys->display->displayOff();
        esp_restart();
    } else {
        if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
            debugPrintln("Image validation failed, image is corrupted");
        }
        debugPrint("ESP_HTTPS_OTA upgrade failed ");
        debugPrintln(ota_finish_err);
        vTaskDelete(NULL);
    }
}

// Make sure this is called before executeOTA()
// This sets up the WiFi and server connection prereqs
int setupOTA() {

    updateMutex = xSemaphoreCreateMutex();
    if(_sys->getBattery() < 30 ){
        debugPrintln(" battery too low for update");
        return -1;
    }
    if(size_t s = NimBLEDevice::getClientListSize() ){

        std::list<NimBLEClient *> b = *NimBLEDevice::getClientList(); 
        std::list<NimBLEClient *>::iterator it = b.begin();

        for(int i = 0; i < s; i++)
        {
            debugPrintln("Bluetooth is connected");
            
            NimBLEDevice::deleteClient(*it);
            std::advance(it,1);
        }
        debugPrintln("All bluetooth connections closed");
        //return -1;

    }
    
    pServer->stopAdvertising();

    return 0;
}

void executeOTA(){
    xTaskCreate(&ota_task, "ota_task", 1024 * 8, NULL, 5, NULL);
}


    
// look here
int getUpdatePercent(){
  int i= -1;
  /*if(Update.isRunning()){
    if(contentLength > 0){
      i = (int) 100* Update.progress()/contentLength;
    }else{
      i = 1;
    }
    debugPrintln(i);
    return i;
  }*/
  debugPrintln("update not running?");
  debugPrintln("-1");
  return i;
  
}

