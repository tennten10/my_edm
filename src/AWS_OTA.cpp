#include "myWiFi.h"
#include "globals.h"
#include "debug.h"
#include "a_config.h"

/* OTA example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "protocol_examples_common.h"
#include "errno.h"

#if CONFIG_EXAMPLE_CONNECT_WIFI
#include "esp_wifi.h"
#endif

#define BUFFSIZE 1024
#define HASH_LEN 32 /* SHA-256 digest length */

static const char *TAG = "native_ota_example";
/*an ota data write buffer ready to write to the flash*/
static char ota_write_data[BUFFSIZE + 1] = { 0 };
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

#define OTA_URL_SIZE 256

static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static void __attribute__((noreturn)) task_fatal_error(void)
{
    ESP_LOGE(TAG, "Exiting task due to fatal error...");
    (void)vTaskDelete(NULL);

    while (1) {
        ;
    }
}

static void print_sha256 (const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s: %s", label, hash_print);
}

static void infinite_loop(void)
{
    int i = 0;
    ESP_LOGI(TAG, "When a new firmware is available on the server, press the reset button to download it");
    while(1) {
        ESP_LOGI(TAG, "Waiting for a new firmware ... %d", ++i);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void ota_example_task(void *pvParameter)
{
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;


    ESP_LOGI(TAG, "Starting OTA example");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

    esp_http_client_config_t config = 
    {
        .port = 80,
        //.url = "sudo-test2.s3.us-east-2.amazonaws.com/",
        
        //.cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = 3000,
    };
    #ifdef V1_BASE
      config.host = "sudo-test2.s3.us-east-2.amazonaws.com/";
      config.path = "/OneBoard_Beta_v1.ino.esp32.bin";
      config.port = 80;
      //.query = "esp", // really not sure what this does...
      //config.event_handler = _http_event_handler;
      //config.user_data = local_response_buffer;        // Pass address of local buffer to get response
      config.disable_auto_redirect = true;
    #endif
    #ifdef V3_FLIP
      config.host = "sudo-test2.s3.us-east-2.amazonaws.com/";
      config.path = "/OneBoard_Beta_v3.ino.esp32.bin";
      //config.port = 80;
      //.query = "esp", // really not sure what this does...
      //config.event_handler = _http_event_handler;
      //config.user_data = local_response_buffer;        // Pass address of local buffer to get response
      config.disable_auto_redirect = true;    
    
    #endif
    #ifdef V6_SIMPLE
      config.host = "sudo-test2.s3.us-east-2.amazonaws.com/";
      config.path = "/OneBoard_Beta_v6.ino.esp32.bin";
      //config.port = 80;
      //.query = "esp", // really not sure what this does...
      //config.event_handler = _http_event_handler;
      //config.user_data = local_response_buffer;        // Pass address of local buffer to get response
      config.disable_auto_redirect = true;
    #endif

    

#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL_FROM_STDIN 
    char url_buf[OTA_URL_SIZE];
    if (strcmp(config.url, "FROM_STDIN") == 0) {
        example_configure_stdin_stdout();
        fgets(url_buf, OTA_URL_SIZE, stdin);
        int len = strlen(url_buf);
        url_buf[len - 1] = '\0';
        config.url = url_buf;
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong firmware upgrade image url");
        abort();
    }
#endif

#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialise HTTP connection");
        task_fatal_error();
    }
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        task_fatal_error();
    }
    esp_http_client_fetch_headers(client);

    update_partition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);

    int binary_file_length = 0;
    /*deal with all receive packet*/
    bool image_header_was_checked = false;
    while (1) {
        int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
        if (data_read < 0) {
            ESP_LOGE(TAG, "Error: SSL data read error");
            http_cleanup(client);
            task_fatal_error();
        } else if (data_read > 0) {
            if (image_header_was_checked == false) {
                esp_app_desc_t new_app_info;
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
                    // check current version with downloading
                    memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                    ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

                    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
                    }

                    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
                    esp_app_desc_t invalid_app_info;
                    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
                    }

                    // check current version with last invalid partition
                    if (last_invalid_app != NULL) {
                        if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
                            ESP_LOGW(TAG, "New version is the same as invalid version.");
                            ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
                            ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
                            http_cleanup(client);
                            infinite_loop();
                        }
                    }
#ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
                    if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
                        ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
                        http_cleanup(client);
                        infinite_loop();
                    }
#endif

                    image_header_was_checked = true;

                    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        http_cleanup(client);
                        task_fatal_error();
                    }
                    ESP_LOGI(TAG, "esp_ota_begin succeeded");
                } else {
                    ESP_LOGE(TAG, "received package is not fit len");
                    http_cleanup(client);
                    task_fatal_error();
                }
            }
            err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) {
                http_cleanup(client);
                task_fatal_error();
            }
            binary_file_length += data_read;
            ESP_LOGD(TAG, "Written image length %d", binary_file_length);
        } else if (data_read == 0) {
           /*
            * As esp_http_client_read never returns negative error code, we rely on
            * `errno` to check for underlying transport connectivity closure if any
            */
            if (errno == ECONNRESET || errno == ENOTCONN) {
                ESP_LOGE(TAG, "Connection closed, errno = %d", errno);
                break;
            }
            if (esp_http_client_is_complete_data_received(client) == true) {
                ESP_LOGI(TAG, "Connection closed");
                break;
            }
        }
    }
    ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);
    if (esp_http_client_is_complete_data_received(client) != true) {
        ESP_LOGE(TAG, "Error in receiving complete file");
        http_cleanup(client);
        task_fatal_error();
    }

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        }
        ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
    return ;
}

// From Kconfig.projbuild in example folder...
// config EXAMPLE_GPIO_DIAGNOSTIC
//         int "Number of the GPIO input for diagnostic"
//         range 0 39
//         default 4
//         help
//             Used to demonstrate how a rollback works.
//             The selected GPIO will be configured as an input with internal pull-up enabled.
//             To trigger a rollback, this GPIO must be pulled low while the message
//             `Diagnostics (5 sec)...` which will be on first boot.
//             If GPIO is not pulled low then the operable of the app will be confirmed.



// static bool diagnostic(void)
// {
//     gpio_config_t io_conf;
//     io_conf.intr_type    = GPIO_PIN_INTR_DISABLE;
//     io_conf.mode         = GPIO_MODE_INPUT;
//     io_conf.pin_bit_mask = (1ULL << CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);
//     io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
//     io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
//     gpio_config(&io_conf);

//     ESP_LOGI(TAG, "Diagnostics (5 sec)...");
//     vTaskDelay(5000 / portTICK_PERIOD_MS);

//     bool diagnostic_is_ok = gpio_get_level(CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);

//     gpio_reset_pin(CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);
//     return diagnostic_is_ok;
// }

void setup2()
{
    uint8_t sha_256[HASH_LEN] = { 0 };
    esp_partition_t partition;

    // get sha256 digest for the partition table
    partition.address   = ESP_PARTITION_TABLE_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type      = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for the partition table: ");

    // get sha256 digest for bootloader
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // run diagnostic function ...
            //bool diagnostic_is_ok = diagnostic();
            //if (diagnostic_is_ok) {
                //ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
                //esp_ota_mark_app_valid_cancel_rollback();
            //} else {
                //ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
                //esp_ota_mark_app_invalid_rollback_and_reboot();
            //}
        }
    }

    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    //ESP_ERROR_CHECK(example_connect());
    startWiFi();

#if CONFIG_EXAMPLE_CONNECT_WIFI
    /* Ensure to disable any WiFi power save mode, this allows best throughput
     * and hence timings for overall OTA operation.
     */
    esp_wifi_set_ps(WIFI_PS_NONE);
#endif // CONFIG_EXAMPLE_CONNECT_WIFI

    xTaskCreate(&ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);
}


///////////////////////////////////////////////////////////////////////////
/*#include "esp_wifi.h"
#include "esp_http_client.h"
#include "esp_tls.h"
//#include <Update.h>
#include <mySPIFFS.h>
#include "debug.h"
#include "globals.h"
#include "a_config.h"
#include "myWiFi.h"
#include "myFunctions.h"



// Variables to validate
// response from S3
long contentLength = 0;
bool isValidContentType = false;


// S3 Bucket Config
//const char* host = "sudo-test2.s3.us-east-2.amazonaws.com/"; // Host => bucket-name.s3.region.amazonaws.com
//int port = 80; // Non https. For HTTPS 443. As of today, HTTPS doesn't work.
//std::string bin = ""; // Initialized to a blank value so it compiles. Special cases defined as follows. 
//#ifdef V1_SIMPLE
//std::string bin = "/OneBoard_Beta_v1.ino.esp32.bin"; // bin file name with a slash in front.
//#endif
//#ifdef V6_SIMPLE
//std::string bin = "/OneBoard_Beta_v6.ino.esp32.bin"; // bin file name with a slash in front.
//#endif
//#ifdef V3_FLIP
//std::string bin = "/OneBoard_Beta_v3.ino.esp32.bin"; // bin file name with a slash in front.
//#endif

// Utility to extract header value from headers
std::string getHeaderValue(std::string header, std::string headerName) {
  return header.substr(0, strlen(headerName.c_str()));
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    char* debugOutput;
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            debugPrintln("HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            debugPrintln("HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            debugPrintln("HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            sprintf(debugOutput, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            debugPrintln(debugOutput);
            break;
        case HTTP_EVENT_ON_DATA:
            sprintf(debugOutput, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            debugPrintln(debugOutput);
            *
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             *
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            debugPrintln("Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            debugPrintln("HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            debugPrintln("HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                if (output_buffer != NULL) {
                    free(output_buffer);
                    output_buffer = NULL;
                }
                output_len = 0;
                sprintf(debugOutput, "Last esp error code: 0x%x", err);
                debugPrintln(debugOutput);
                sprintf(debugOutput, "Last mbedtls failure: 0x%x", mbedtls_err);
                debugPrintln(debugOutput);
            }
            break;
    }
    return ESP_OK;
}

static void http_rest_with_url(void)
{
    char local_response_buffer[2048] = {0}; //[MAX_HTTP_OUTPUT_BUFFER]
    char* debugOutput;
    **
     * NOTE: All the configuration parameters for http_client must be spefied either in URL or as host and path parameters.
     * If host and path parameters are not set, query parameter will be ignored. In such cases,
     * query parameter should be specified in URL.
     *
     * If URL as well as host and path parameters are specified, values of host and path will be considered.
     *
     esp_http_client_config_t config={};
    #ifdef V1_BASE
      config.host = "sudo-test2.s3.us-east-2.amazonaws.com/";
      config.path = "/OneBoard_Beta_v1.ino.esp32.bin";
      config.port = 80;
      //.query = "esp", // really not sure what this does...
      config.event_handler = _http_event_handler;
      config.user_data = local_response_buffer;        // Pass address of local buffer to get response
      config.disable_auto_redirect = true;
    #endif
    #ifdef V3_FLIP
      config.host = "sudo-test2.s3.us-east-2.amazonaws.com/";
      config.path = "/OneBoard_Beta_v3.ino.esp32.bin";
      config.port = 80;
      //.query = "esp", // really not sure what this does...
      config.event_handler = _http_event_handler;
      config.user_data = local_response_buffer;        // Pass address of local buffer to get response
      config.disable_auto_redirect = true;    
    
    #endif
    #ifdef V6_SIMPLE
      config.host = "sudo-test2.s3.us-east-2.amazonaws.com/";
      config.path = "/OneBoard_Beta_v6.ino.esp32.bin";
      config.port = 80;
      //.query = "esp", // really not sure what this does...
      config.event_handler = _http_event_handler;
      config.user_data = local_response_buffer;        // Pass address of local buffer to get response
      config.disable_auto_redirect = true;
    #endif

    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    // look into using this... 
    //esp_http_client_set_header(client, "Content-Type", "application/json");

    //sprintf(g, "GET %s HTTP/1.1\r\nHost: %s\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n", bin, host);
    // GET
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        sprintf(debugOutput, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        debugPrintln(debugOutput);
        esp_http_client_read(client, )
    } else { 
        sprintf(debugOutput ,"HTTP GET request failed: %s", esp_err_to_name(err));
        debugPrintln(debugOutput);
    }
    //ESP_LOG_BUFFER_HEX(TAG, local_response_buffer, strlen(local_response_buffer));

    esp_http_client_cleanup(client);
}
/ end of esp_http_client code section */
/*
// OTA Logic 
void execOTA(int attempt = 0) {
  
  if(esp_wifi_get_mode(NULL) != ESP_OK){
    // make sure wifi is connected first
    return;
  }
  if (attempt > 3){
    // to set recursive retry max number before timeout 
    return;
  }

  debugPrint("Connecting to: ");
  debugPrintln(host);
  

  // Connect to S3
  bool b = 1; //client.connect(host.c_str(), port); 
  vTaskDelay(1000);
  if (b) {
    // Connection Succeed.
    // Fetching the bin
    //this one
    //debugPrintln("Fetching Bin: " + std::string(bin));
    vTaskDelay(10);
    // Get the contents of the bin file
    //char g[150];
    //sprintf(g, "GET %s HTTP/1.1\r\nHost: %s\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n", bin, host);
    //debugPrintln(g);
    vTaskDelay(100);

    http_rest_with_url();

    //client.print(g);
    //client.print("GET /OneBoard_Beta_v3.ino.esp32.bin HTTP/1.1\r\nHost: ota-update-oneboard-test.s3.us-east-2.amazonaws.com\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n"); 
    //client.print("GET " + bin + " HTTP/1.1\r\n" +
    //            "Host: " + host + "\r\n" +
    //            "Cache-Control: no-cache\r\n" +
    //           "Connection: close\r\n\r\n");

    // Check what is being sent
    //    Serial.print(std::string("GET ") + bin + " HTTP/1.1\r\n" +
    //                 "Host: " + host + "\r\n" +
    //                 "Cache-Control: no-cache\r\n" +
    //                 "Connection: close\r\n\r\n");
    debugPrintln("bin stop 1");
    //unsigned long timeout = millis();
    //while (client.available() == 0) {
    //  if (millis() - timeout > 5000) {
    //    debugPrintln("Client Timeout !");
    //    client.stop();
    //    return;
    //  }
      //debugPrintln("bin while loop");
    
    debugPrintln("bin stop 2");
  //   // Once the response is available,
  //   // check stuff

  //   /
  //      Response Structure
  //       HTTP/1.1 200 OK
  //       x-amz-id-2: NVKxnU1aIQMmpGKhSwpCBh8y2JPbak18QLIfE+OiUDOos+7UftZKjtCFqrwsGOZRN5Zee0jpTd0=
  //       x-amz-request-id: 2D56B47560B764EC
  //       Date: Wed, 14 Jun 2017 03:33:59 GMT
  //       Last-Modified: Fri, 02 Jun 2017 14:50:11 GMT
  //       ETag: "d2afebbaaebc38cd669ce36727152af9"
  //       Accept-Ranges: bytes
  //       Content-Type: application/octet-stream
  //       Content-Length: 357280
  //       Server: AmazonS3
                                   
  //       {{BIN FILE CONTENTS}}

  //   */
   
  //   while (client.available()) {
  //     // read line till /n
  //     std::string r = client.readStringUntil('\n');
  //     // remove space, to check if the line is end of headers
  //     std::string line = trim(r, " ");

  //     // if the the line is empty,
  //     // this is end of headers
  //     // break the while and feed the
  //     // remaining `client` to the
  //     // Update.writeStream();
  //     if (!line.length()) {
  //       //headers ended
  //       break; // and get the OTA started
  //     }

  //     // Check if the HTTP Response is 200
  //     // else break and Exit Update
  //     //if (line.startsWith("HTTP/1.1")) {
  //     if (startsWith(line,"HTTP/1.1")){  
  //       if (line.find_first_of("200") < 0) {
  //         debugPrintln(line);
  //         debugPrintln("Got a non 200 status code from server. Exiting OTA Update.");
  //         break;
  //       }
  //     }
      

  //     // extract headers here
  //     // Start with content length
  //     if (startsWith(line, "Content-Length:")) {
  //       contentLength = atol((getHeaderValue(line, "Content-Length: ")).c_str());
  //       debugPrintln("Got " + std::to_string(contentLength) + " bytes from server");
  //     }

  //     // Next, the content type
  //     if (startsWith(line, "Content-Type:")) {
  //       std::string contentType = getHeaderValue(line, "Content-Type: ");
  //       debugPrintln("Got " + contentType + " payload.");
  //       if (contentType == "application/octet-stream") {
  //         isValidContentType = true;
  //       } 
  //     }
  //   } 
  // } else {
  //   // Connect to S3 failed
  //   // May be try?
  //   // Probably a choppy network?
  //   debugPrintln("Connection to " + std::string(host) + " failed. Please check your setup");
  //   // retry??
  //   execOTA(++attempt);
  //   //retry();
  // }
  // debugPrintln("bin stop 3");
  // // Check what is the contentLength and if content type is `application/octet-stream`
  // debugPrintln("contentLength : " + std::to_string(contentLength) + ", isValidContentType : " + (char)isValidContentType);
   /* Line is here mike
  // check contentLength and content type
  if (contentLength && isValidContentType) {
    // Check if there is enough to OTA Update
    bool canBegin = Update.begin(contentLength);

    // If yes, begin
    if (canBegin) {
      debugPrintln("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
      // No activity would appear on the Serial monitor
      // So be patient. This may take 2 - 5mins to complete
      size_t written = Update.writeStream(client);

      if (written == contentLength) {
        debugPrintln("Written : " + std::string(written) + " successfully");
      } else {
        debugPrintln("Written only : " + std::string(written) + "/" + std::string(contentLength) + ". Retry?" );
        // retry??
        execOTA(++attempt);
      }

      if (Update.end()) {
        debugPrintln("OTA done!");
        if (Update.isFinished()) {
          debugPrintln("Update successfully completed. Rebooting.");
          //ESP.restart();
          esp_restart();
        } else {
          debugPrintln("Update not finished? Something went wrong!");
        }
      } else {
        debugPrintln("Error Occurred. Error #: " + std::string(Update.getError()));
      }
    } else {
      // not enough space to begin OTA
      // Understand the partitions and
      // space availability
      debugPrintln("Not enough space to begin OTA");
      client.flush();
    }
  } else {
    debugPrintln("There was no content in the response");
    client.flush();
  }*

}
}

*/
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

void setupOTA() {

    //Retrieves SSID & Password from SPIFFS filesystem
    //Starts WiFi in STA mode
    startWiFi();
    if(esp_wifi_get_mode(NULL)==ESP_OK){
      // if it gets in here wifi connection is successful
    }
      // Execute OTA Update
      //execOTA();
      // called from bluetooth callback

}
