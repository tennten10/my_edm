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

SemaphoreHandle_t updateMutex;

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
    esp_https_ota_handle_t https_ota_handle = NULL;
    debugPrintln("before beginning ota");
    //debugPrintln(ota_config.http_config);
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        debugPrintln("ESP HTTPS OTA Begin failed");
        vTaskDelete(NULL);
    }

    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
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

    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        // esp_https_ota_perform returns after every read operation which gives user the ability to
        // monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
        // data read so far.
        debugPrint("Image bytes read: ");
        debugPrintln(esp_https_ota_get_image_len_read(https_ota_handle));
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        // the OTA image was not completely received and user can customise the response to this situation.
        debugPrintln("Complete data was not received.");
    }

ota_end:
    ota_finish_err = esp_https_ota_finish(https_ota_handle);
    if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
        debugPrintln("ESP_HTTPS_OTA upgrade successful. Rebooting ...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
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
void setupOTA() {

    updateMutex = xSemaphoreCreateMutex();
    //Retrieves SSID & Password from SPIFFS filesystem
    //Starts WiFi in STA mode
    startWiFi();
    if(esp_wifi_get_mode(NULL)==ESP_OK){
      // if it gets in here wifi connection is successful
      debugPrintln("inside wifi flag...");
    }

    /* Ensure to disable any WiFi power save mode, this allows best throughput
     * and hence timings for overall OTA operation.
     */
    // Since WiFi and BT working together needs one not taking all the cpu time, we need some sleep time for it to work. 
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM); //WIFI_PS_NONE);

}

void executeOTA(){
    xTaskCreate(&ota_task, "ota_task", 1024 * 8, NULL, 5, NULL);
}


//     esp_http_client_config_t config = 
//     {
//         .url = "sudo-test2.s3.us-east-2.amazonaws.com/",        /*!< HTTP URL, the information on the URL is most important, it overrides the other fields below, if any */
//         //.host = ,           /*!< Domain or IP as string */
//         .port = 80,/*!< Port to connect, default depend on esp_http_client_transport_t (80 or 443) */
//         //.username = ,/*!< Using for Http authentication */
//         //.password = ,/*!< Using for Http authentication */
//         //.auth_type = ,/*!< Http authentication type, see `esp_http_client_auth_type_t` */
//         //.path = ,/*!< HTTP Path, if not set, default is `/` */
//         //.query = ,/*!< HTTP query */
//         .cert_pem = (char *)server_cert_pem_start,/*!< SSL server certification, PEM format as string, if the client requires to verify server */
//         //.client_cert_pem = ,/*!< SSL client certification, PEM format as string, if the server requires to verify client */
//         //.client_key_pem = ,/*!< SSL client key, PEM format as string, if the server requires to verify client */
//         //.method = ,/*!< HTTP Method */
//         .timeout_ms = 3000,/*!< Network timeout in milliseconds */
//         //.disable_auto_redirect = ,/*!< Disable HTTP automatic redirects */
//         //.max_redirection_count = ,/*!< Max redirection number, using default value if zero*/
//         //.event_handler = ,/*!< HTTP Event Handle */
//         //.transport_type = ,/*!< HTTP transport type, see `esp_http_client_transport_t` */
//         //.buffer_size = ,/*!< HTTP receive buffer size */
//         //.buffer_size_tx = ,/*!< HTTP transmit buffer size */
//         //.user_data = ,/*!< HTTP user_data context */
//         //.is_async = , /*!< Set asynchronous mode, only supported with HTTPS for now */
//         //.use_global_ca_store = ,/*!< Use a global ca_store for all the connections in which this bool is set. */
//         //.skip_cert_common_name_check = ,/*!< Skip any validation of server certificate CN field */

//     };
//     #ifdef V1_BASE
//       config.host = "sudo-test2.s3.us-east-2.amazonaws.com/";
//       config.path = "/OneBoard_Beta_v1.ino.esp32.bin";
//       config.port = 80;
//       //.query = "esp", // really not sure what this does...
//       //config.event_handler = _http_event_handler;
//       //config.user_data = local_response_buffer;        // Pass address of local buffer to get response
//       config.disable_auto_redirect = true;
//     #endif
//     #ifdef V3_FLIP
//       config.host = "sudo-test2.s3.us-east-2.amazonaws.com/";
//       config.path = "/OneBoard_Beta_v3.ino.esp32.bin";
//       //config.port = 80;
//       //.query = "esp", // really not sure what this does...
//       //config.event_handler = _http_event_handler;
//       //config.user_data = local_response_buffer;        // Pass address of local buffer to get response
//       config.disable_auto_redirect = true;    
    
//     #endif
    
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
