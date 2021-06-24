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
#include "BLE.h"
SemaphoreHandle_t updateMutex;
int updateFlag = 0;
QueueHandle_t updateErrorQueue;
TaskHandle_t otaTask_h;
extern SystemX *_sys;


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
    // TODO: uncomment this when releasing
    // if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
    //     debugPrintln("Current running version is the same as a new. We will not continue the update.");
        
    //     return ESP_FAIL;
    // }


    return ESP_OK;
}

void ota_task(void *pvParameter)
{
    debugPrintln("Starting OTA Task");
    //updateMutex = xSemaphoreCreateMutex();
    int queueReturn = 0;
    
    
    // check server information and confirm
   
    debugPrintln("before connect WiFi");    
    WiFiStruct w = _sys->getWiFiInfo();
    if(strcmp(w.ssid, "") == 0){
        debugPrintln("WiFi info not set");
        queueReturn = SB_UPDATE_FAILED_NO_WIFI_INFO;
        xQueueSend(updateErrorQueue, &queueReturn, (TickType_t)10);
    }
    if(connectWiFi(w) == SB_WIFI_CONNECT_SUCCESS){
        debugPrintln("wifi connection error handling - success");
    }else{
        queueReturn =  SB_UPDATE_FAILED_NO_WIFI_INFO;
        xQueueSend(updateErrorQueue, &queueReturn, (TickType_t)10);
    }
    
          
    debugPrintln("after connect WiFi");
    wifi_mode_t temp;
    int p = esp_wifi_get_mode(&temp); // getting invalid argument return value with NULL. Trying to pass it a real variable now
    debugPrint("wifi get mode: ");
    debugPrintln(p);
    if(p == ESP_OK){
      // if it gets in here wifi connection is successful
      debugPrintln("inside wifi flag...");
    }else{
        debugPrintln(" not in wifi thingy if statement");
    }


    

    esp_http_client_config_t config = {
        .url = UPDATE_URL_META, // CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL,
        //.cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = CONFIG_EXAMPLE_OTA_RECV_TIMEOUT,
    };
    debugPrintln("Set http client config");


    // check server status manually so I can sent it to bluetooth first
    if(queueReturn == 0){
        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_err_t err = esp_http_client_perform(client);
        int status = 404;
        if (err == ESP_OK) {
            status = esp_http_client_get_status_code(client);
            printf("Status = %d, content_length = %d\n",
            status,
            esp_http_client_get_content_length(client));
            
        }
        
        esp_http_client_cleanup(client);
        debugPrint("http server status: ");
        debugPrintln(status);
        if(status != 200){
            queueReturn = SB_UPDATE_FAILED_SERVER;
            xQueueSend(updateErrorQueue, &queueReturn, (TickType_t)10);
            //vTaskDelete(NULL);
            vTaskDelay(100);
        }else{
            queueReturn = SB_UPDATE_STARTED;
            xQueueSend(updateErrorQueue, &queueReturn, (TickType_t)10);
        }
         
    }
         

    vTaskDelay(200);
    BLEstop(); // This was later on, but testing with this earlier
    debugPrintln("after BLEstop");

    config.url = UPDATE_URL;
    /* Ensure to disable any WiFi power save mode, this allows best throughput
     * and hence timings for overall OTA operation.
     */
    // Since WiFi and BT working together needs one not taking all the cpu time, we need some sleep time for it to work. WIFI_PS_MIN_MODEM
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM); //WIFI_PS_NONE);

    
#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif

    esp_https_ota_config_t ota_config = {
        .http_config = &config,         // const esp_http_client_config_t *http_config;   /*!< ESP HTTP client configuration */
        //.http_client_init_cb = {},           // http_client_init_cb_t http_client_init_cb;     /*!< Callback after ESP HTTP client is initialised */
        .bulk_flash_erase = false,           // bool bulk_flash_erase;                         /*!< Erase entire flash partition during initialization. By default flash partition is erased during write operation and in chunk of 4K sector size */
        //.partial_http_download = true,         // bool partial_http_download;                    /*!< Enable Firmware image to be downloaded over multiple HTTP requests */
        //.max_http_request_size = 8190,//,         // int max_http_request_size; 
    };
    debugPrintln("Set ota http config");
    esp_https_ota_handle_t https_ota_handle;// = NULL;
    debugPrintln("before beginning ota");

    vTaskDelay(100);
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);

    // This function is in most recent esp-idf. For now commented out until progress bar is implemented.
    // int full_size = esp_https_ota_get_image_size(https_ota_handle);
    debugPrint("ota begin err value: ");
    debugPrintln((int)err);
    if (err != ESP_OK) {
        debugPrintln("ESP HTTPS OTA Begin failed");
        vTaskDelete(NULL);  
    }

    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    esp_err_t ota_finish_err = ESP_OK;
    
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
    // debugPrintln("Made it past header verification so that should be fine");

    


    // if(full_size < 1){
    //     debugPrintln("error reading header file size, printed from ota_task");
    //     full_size = 1;
    // }else{
    //     debugPrint("Full Size: ");
    //     debugPrintln(full_size);
    // }

    // Note: I'm using a linear correction to hopefully fix the load percentage... It is from collecting a few data points and plotting a graph.
    // I wasn't able to figure out the relationship between the variable t (return value of esp_https_ota_get_image_len_read) and the total size of the downlaod
    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        // esp_https_ota_perform returns after every read operation which gives user the ability to
        // monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
        // data read so far.
        
        //t = esp_https_ota_get_image_len_read(https_ota_handle); //(((esp_https_ota_get_image_len_read(https_ota_handle)-773858) / 5374) +144)/3;
         
        //debugPrintln(std::to_string(t));
        //current = t / full_size;
        // if( current > last){
        // //     _sys->display->displayUpdateScreen(current);
        //     last = current;
        //     debugPrint("Image progress: ");
        //     debugPrintln(last);
        // }
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        // the OTA image was not completely received and user can customise the response to this situation.
        debugPrintln("Complete data was not received.");
    }

ota_end:
    ota_finish_err = esp_https_ota_finish(https_ota_handle);
    if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
        //_sys->display->displayLogo();
        debugPrintln("ESP_HTTPS_OTA upgrade successful. Rebooting ...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        //_sys->display->displayOff();
        esp_restart();
    } else {
        if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
            debugPrintln("Image validation failed, image is corrupted");
        }
        debugPrint("ESP_HTTPS_OTA upgrade failed ");
        debugPrintln(ota_finish_err);
        disconnectWiFi();
        stopWiFi();
        vTaskDelete(NULL);
    }
}


// This sets up the WiFi and server connection prereqs
int startOTA() {
    if(_sys->getBattery() < 10 ){
        debugPrintln(" battery too low for update");
        return SB_UPDATE_FAILED_BATTERY;
    }
    updateErrorQueue = xQueueCreate(3, sizeof(int));
    int buff = 0;
    xTaskCreate(ota_task, "ota_task", 1024 * 8, NULL, 5, &otaTask_h);
    while(1){
        // listen for other errors to return
        if(uxQueueMessagesWaiting(updateErrorQueue) > 0){
            xQueueReceive(updateErrorQueue, &buff, (TickType_t)10);
            break;
        }
        vTaskDelay(10);
    }
    vQueueDelete(updateErrorQueue);
    if(buff == SB_UPDATE_STARTED){ // case where update is running and needs to run to finish
        return buff;
    }else {     // case where update terminates before disconnecting
        vTaskDelete(otaTask_h);
        return buff;
    }
    
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