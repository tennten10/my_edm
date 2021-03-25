
// /* OTA example
//    This example code is in the Public Domain (or CC0 licensed, at your option.)
//    Unless required by applicable law or agreed to in writing, this
//    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
//    CONDITIONS OF ANY KIND, either express or implied.
// */
// #include "myWiFi.h"

// /* OTA example

//    This example code is in the Public Domain (or CC0 licensed, at your option.)

//    Unless required by applicable law or agreed to in writing, this
//    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
//    CONDITIONS OF ANY KIND, either express or implied.
// */
// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_system.h"
// #include "esp_event.h"
// #include "esp_log.h"
// #include "esp_ota_ops.h"
// #include "esp_http_client.h"
// #include "esp_flash_partitions.h"
// #include "esp_partition.h"
// #include "nvs.h"
// #include "nvs_flash.h"
// #include "driver/gpio.h"
// #include "protocol_examples_common.h"
// #include "errno.h"

// #if CONFIG_EXAMPLE_CONNECT_WIFI
// #include "esp_wifi.h"
// #endif

// #define BUFFSIZE 1024
// #define HASH_LEN 32 /* SHA-256 digest length */

// static const char *TAG = "native_ota_example";
// /*an ota data write buffer ready to write to the flash*/
// static char ota_write_data[BUFFSIZE + 1] = { 0 };
// extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
// extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

// #define OTA_URL_SIZE 256

// static void http_cleanup(esp_http_client_handle_t client)
// {
//     esp_http_client_close(client);
//     esp_http_client_cleanup(client);
// }

// static void __attribute__((noreturn)) task_fatal_error(void)
// {
//     ESP_LOGE(TAG, "Exiting task due to fatal error...");
//     (void)vTaskDelete(NULL);

//     while (1) {
//         ;
//     }
// }

// static void print_sha256 (const uint8_t *image_hash, const char *label)
// {
//     char hash_print[HASH_LEN * 2 + 1];
//     hash_print[HASH_LEN * 2] = 0;
//     for (int i = 0; i < HASH_LEN; ++i) {
//         sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
//     }
//     ESP_LOGI(TAG, "%s: %s", label, hash_print);
// }

// static void infinite_loop(void)
// {
//     int i = 0;
//     ESP_LOGI(TAG, "When a new firmware is available on the server, press the reset button to download it");
//     while(1) {
//         ESP_LOGI(TAG, "Waiting for a new firmware ... %d", ++i);
//         vTaskDelay(2000 / portTICK_PERIOD_MS);
//     }
// }

// static void ota_example_task(void *pvParameter)
// {
//     esp_err_t err;
//     /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
//     esp_ota_handle_t update_handle = 0 ;
//     const esp_partition_t *update_partition = NULL;

//     ESP_LOGI(TAG, "Starting OTA example");

//     const esp_partition_t *configured = esp_ota_get_boot_partition();
//     const esp_partition_t *running = esp_ota_get_running_partition();

//     if (configured != running) {
//         ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
//                  configured->address, running->address);
//         ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
//     }
//     ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
//              running->type, running->subtype, running->address);

//     esp_http_client_config_t config = {
//         .url = "sudo-test2.s3.us-east-2.amazonaws.com/",
//         .cert_pem = (char *)server_cert_pem_start,
//         .timeout_ms = 1000,
//     };

// #ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL_FROM_STDIN
//     char url_buf[OTA_URL_SIZE];
//     if (strcmp(config.url, "FROM_STDIN") == 0) {
//         example_configure_stdin_stdout();
//         fgets(url_buf, OTA_URL_SIZE, stdin);
//         int len = strlen(url_buf);
//         url_buf[len - 1] = '\0';
//         config.url = url_buf;
//     } else {
//         ESP_LOGE(TAG, "Configuration mismatch: wrong firmware upgrade image url");
//         abort();
//     }
// #endif

// #ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
//     config.skip_cert_common_name_check = true;
// #endif

//     esp_http_client_handle_t client = esp_http_client_init(&config);
//     if (client == NULL) {
//         ESP_LOGE(TAG, "Failed to initialise HTTP connection");
//         task_fatal_error();
//     }
//     err = esp_http_client_open(client, 0);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
//         esp_http_client_cleanup(client);
//         task_fatal_error();
//     }
//     esp_http_client_fetch_headers(client);

//     update_partition = esp_ota_get_next_update_partition(NULL);
//     ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
//              update_partition->subtype, update_partition->address);
//     assert(update_partition != NULL);

//     int binary_file_length = 0;
//     /*deal with all receive packet*/
//     bool image_header_was_checked = false;
//     while (1) {
//         int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
//         if (data_read < 0) {
//             ESP_LOGE(TAG, "Error: SSL data read error");
//             http_cleanup(client);
//             task_fatal_error();
//         } else if (data_read > 0) {
//             if (image_header_was_checked == false) {
//                 esp_app_desc_t new_app_info;
//                 if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
//                     // check current version with downloading
//                     memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
//                     ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

//                     esp_app_desc_t running_app_info;
//                     if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
//                         ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
//                     }

//                     const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
//                     esp_app_desc_t invalid_app_info;
//                     if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
//                         ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
//                     }

//                     // check current version with last invalid partition
//                     if (last_invalid_app != NULL) {
//                         if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
//                             ESP_LOGW(TAG, "New version is the same as invalid version.");
//                             ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
//                             ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
//                             http_cleanup(client);
//                             infinite_loop();
//                         }
//                     }
// #ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
//                     if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
//                         ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
//                         http_cleanup(client);
//                         infinite_loop();
//                     }
// #endif

//                     image_header_was_checked = true;

//                     err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
//                     if (err != ESP_OK) {
//                         ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
//                         http_cleanup(client);
//                         task_fatal_error();
//                     }
//                     ESP_LOGI(TAG, "esp_ota_begin succeeded");
//                 } else {
//                     ESP_LOGE(TAG, "received package is not fit len");
//                     http_cleanup(client);
//                     task_fatal_error();
//                 }
//             }
//             err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
//             if (err != ESP_OK) {
//                 http_cleanup(client);
//                 task_fatal_error();
//             }
//             binary_file_length += data_read;
//             ESP_LOGD(TAG, "Written image length %d", binary_file_length);
//         } else if (data_read == 0) {
//            /*
//             * As esp_http_client_read never returns negative error code, we rely on
//             * `errno` to check for underlying transport connectivity closure if any
//             */
//             if (errno == ECONNRESET || errno == ENOTCONN) {
//                 ESP_LOGE(TAG, "Connection closed, errno = %d", errno);
//                 break;
//             }
//             if (esp_http_client_is_complete_data_received(client) == true) {
//                 ESP_LOGI(TAG, "Connection closed");
//                 break;
//             }
//         }
//     }
//     ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);
//     if (esp_http_client_is_complete_data_received(client) != true) {
//         ESP_LOGE(TAG, "Error in receiving complete file");
//         http_cleanup(client);
//         task_fatal_error();
//     }

//     err = esp_ota_end(update_handle);
//     if (err != ESP_OK) {
//         if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
//             ESP_LOGE(TAG, "Image validation failed, image is corrupted");
//         }
//         ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
//         http_cleanup(client);
//         task_fatal_error();
//     }

//     err = esp_ota_set_boot_partition(update_partition);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
//         http_cleanup(client);
//         task_fatal_error();
//     }
//     ESP_LOGI(TAG, "Prepare to restart system!");
//     esp_restart();
//     return ;
// }

// // From Kconfig.projbuild in example folder...
// // config EXAMPLE_GPIO_DIAGNOSTIC
// //         int "Number of the GPIO input for diagnostic"
// //         range 0 39
// //         default 4
// //         help
// //             Used to demonstrate how a rollback works.
// //             The selected GPIO will be configured as an input with internal pull-up enabled.
// //             To trigger a rollback, this GPIO must be pulled low while the message
// //             `Diagnostics (5 sec)...` which will be on first boot.
// //             If GPIO is not pulled low then the operable of the app will be confirmed.



// // static bool diagnostic(void)
// // {
// //     gpio_config_t io_conf;
// //     io_conf.intr_type    = GPIO_PIN_INTR_DISABLE;
// //     io_conf.mode         = GPIO_MODE_INPUT;
// //     io_conf.pin_bit_mask = (1ULL << CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);
// //     io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
// //     io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
// //     gpio_config(&io_conf);

// //     ESP_LOGI(TAG, "Diagnostics (5 sec)...");
// //     vTaskDelay(5000 / portTICK_PERIOD_MS);

// //     bool diagnostic_is_ok = gpio_get_level(CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);

// //     gpio_reset_pin(CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);
// //     return diagnostic_is_ok;
// // }

// void app_main(void)
// {
//     uint8_t sha_256[HASH_LEN] = { 0 };
//     esp_partition_t partition;

//     // get sha256 digest for the partition table
//     partition.address   = ESP_PARTITION_TABLE_OFFSET;
//     partition.size      = ESP_PARTITION_TABLE_MAX_LEN;
//     partition.type      = ESP_PARTITION_TYPE_DATA;
//     esp_partition_get_sha256(&partition, sha_256);
//     print_sha256(sha_256, "SHA-256 for the partition table: ");

//     // get sha256 digest for bootloader
//     partition.address   = ESP_BOOTLOADER_OFFSET;
//     partition.size      = ESP_PARTITION_TABLE_OFFSET;
//     partition.type      = ESP_PARTITION_TYPE_APP;
//     esp_partition_get_sha256(&partition, sha_256);
//     print_sha256(sha_256, "SHA-256 for bootloader: ");

//     // get sha256 digest for running partition
//     esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
//     print_sha256(sha_256, "SHA-256 for current firmware: ");

//     const esp_partition_t *running = esp_ota_get_running_partition();
//     esp_ota_img_states_t ota_state;
//     if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
//         if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
//             // run diagnostic function ...
//             //bool diagnostic_is_ok = diagnostic();
//             //if (diagnostic_is_ok) {
//                 //ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
//                 //esp_ota_mark_app_valid_cancel_rollback();
//             //} else {
//                 //ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
//                 //esp_ota_mark_app_invalid_rollback_and_reboot();
//             //}
//         }
//     }

//     // Initialize NVS.
//     esp_err_t err = nvs_flash_init();
//     if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         // OTA app partition table has a smaller NVS partition size than the non-OTA
//         // partition table. This size mismatch may cause NVS initialization to fail.
//         // If this happens, we erase NVS partition and initialize NVS again.
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         err = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK( err );

//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());

//     /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
//      * Read "Establishing Wi-Fi or Ethernet Connection" section in
//      * examples/protocols/README.md for more information about this function.
//      */
//     //ESP_ERROR_CHECK(example_connect());
//     esp_netif_t* nf = startWiFi();

// #if CONFIG_EXAMPLE_CONNECT_WIFI
//     /* Ensure to disable any WiFi power save mode, this allows best throughput
//      * and hence timings for overall OTA operation.
//      */
//     esp_wifi_set_ps(WIFI_PS_NONE);
// #endif // CONFIG_EXAMPLE_CONNECT_WIFI

//     xTaskCreate(&ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);
// }


// ////////////////////////////////////////////////////////////////////////
// // #include <string.h>
// // #include "freertos/FreeRTOS.h"
// // #include "freertos/task.h"
// // #include "freertos/event_groups.h"
// // #include "esp_system.h"
// // #include "esp_event.h"
// // #include "esp_log.h"
// // #include "esp_ota_ops.h"
// // #include "esp_http_client.h"
// // #include "esp_https_ota.h"
// // #include "nvs.h"
// // #include "nvs_flash.h"
// // #include "protocol_examples_common.h"
// // #include "myWiFi.h"

// // #define CONFIG_EXAMPLE_CONNECT_WIFI 
 
// // #ifdef CONFIG_EXAMPLE_CONNECT_WIFI 
// // #include "esp_wifi.h"
// // #endif

// // static const char *TAG = "advanced_https_ota_example";
// // extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
// // extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

// // #define OTA_URL_SIZE 256

// // static esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
// // {
// //     if (new_app_info == NULL) {
// //         return ESP_ERR_INVALID_ARG;
// //     }

// //     const esp_partition_t *running = esp_ota_get_running_partition();
// //     esp_app_desc_t running_app_info;
// //     if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
// //         ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
// //     }

// // #ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
// //     if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
// //         ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
// //         return ESP_FAIL;
// //     }
// // #endif

// // #ifdef CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
// //     /**
// //      * Secure version check from firmware image header prevents subsequent download and flash write of
// //      * entire firmware image. However this is optional because it is also taken care in API
// //      * esp_https_ota_finish at the end of OTA update procedure.
// //      */
// //     const uint32_t hw_sec_version = esp_efuse_read_secure_version();
// //     if (new_app_info->secure_version < hw_sec_version) {
// //         ESP_LOGW(TAG, "New firmware security version is less than eFuse programmed, %d < %d", new_app_info->secure_version, hw_sec_version);
// //         return ESP_FAIL;
// //     }
// // #endif

// //     return ESP_OK;
// // }

// // static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client)
// // {
// //     esp_err_t err = ESP_OK;
// //     /* Uncomment to add custom headers to HTTP request */
// //     // err = esp_http_client_set_header(http_client, "Custom-Header", "Value");
// //     return err;
// // }

// // void execOTA(void *pvParameter)
// // {
// //     ESP_LOGI(TAG, "Starting Advanced OTA example");

// //     esp_err_t ota_finish_err = ESP_OK;
// //     esp_http_client_config_t config = {
// //         .url = CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL,
// //         .cert_pem = (char *)server_cert_pem_start,
// //         .timeout_ms = CONFIG_EXAMPLE_OTA_RECV_TIMEOUT,
// //         .keep_alive_enable = true,
// //     };

// // #ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL_FROM_STDIN
// //     char url_buf[OTA_URL_SIZE];
// //     if (strcmp(config.url, "FROM_STDIN") == 0) {
// //         example_configure_stdin_stdout();
// //         fgets(url_buf, OTA_URL_SIZE, stdin);
// //         int len = strlen(url_buf);
// //         url_buf[len - 1] = '\0';
// //         config.url = url_buf;
// //     } else {
// //         ESP_LOGE(TAG, "Configuration mismatch: wrong firmware upgrade image url");
// //         abort();
// //     }
// // #endif

// // #ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
// //     config.skip_cert_common_name_check = true;
// // #endif

// //     esp_https_ota_config_t ota_config = {
// //         .http_config = &config,
// //         .http_client_init_cb = _http_client_init_cb, // Register a callback to be invoked after esp_http_client is initialized
// // #ifdef CONFIG_EXAMPLE_ENABLE_PARTIAL_HTTP_DOWNLOAD
// //         .partial_http_download = true,
// //         .max_http_request_size = CONFIG_EXAMPLE_HTTP_REQUEST_SIZE,
// // #endif
// //     };

// //     esp_https_ota_handle_t https_ota_handle = NULL;
// //     esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
// //     if (err != ESP_OK) {
// //         ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
// //         vTaskDelete(NULL);
// //     }

// //     esp_app_desc_t app_desc;
// //     err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
// //     if (err != ESP_OK) {
// //         ESP_LOGE(TAG, "esp_https_ota_read_img_desc failed");
// //         goto ota_end;
// //     }
// //     err = validate_image_header(&app_desc);
// //     if (err != ESP_OK) {
// //         ESP_LOGE(TAG, "image header verification failed");
// //         goto ota_end;
// //     }

// //     while (1) {
// //         err = esp_https_ota_perform(https_ota_handle);
// //         if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
// //             break;
// //         }
// //         // esp_https_ota_perform returns after every read operation which gives user the ability to
// //         // monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
// //         // data read so far.
// //         ESP_LOGD(TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
// //     }

// //     if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
// //         // the OTA image was not completely received and user can customise the response to this situation.
// //         ESP_LOGE(TAG, "Complete data was not received.");
// //     } else {
// //         ota_finish_err = esp_https_ota_finish(https_ota_handle);
// //         if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
// //             ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
// //             vTaskDelay(1000 / portTICK_PERIOD_MS);
// //             esp_restart();
// //         } else {
// //             if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
// //                 ESP_LOGE(TAG, "Image validation failed, image is corrupted");
// //             }
// //             ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
// //             vTaskDelete(NULL);
// //         }
// //     }

// // ota_end:
// //     esp_https_ota_abort(https_ota_handle);
// //     ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed");
// //     vTaskDelete(NULL);
// // }

// // void setupOTA()
// // {
// //     // Initialize NVS.
// //     esp_err_t err = nvs_flash_init();
// //     if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
// //         // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
// //         // partition table. This size mismatch may cause NVS initialization to fail.
// //         // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
// //         // If this happens, we erase NVS partition and initialize NVS again.
// //         ESP_ERROR_CHECK(nvs_flash_erase());
// //         err = nvs_flash_init();
// //     }
// //     ESP_ERROR_CHECK( err );

// //     ESP_ERROR_CHECK(esp_netif_init());
// //     ESP_ERROR_CHECK(esp_event_loop_create_default());

// //     /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
// //      * Read "Establishing Wi-Fi or Ethernet Connection" section in
// //      * examples/protocols/README.md for more information about this function.
// //     */
// //     ESP_ERROR_CHECK(example_connect());

// // #if defined(CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE) && defined(CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK)
// //     /**
// //      * We are treating successful WiFi connection as a checkpoint to cancel rollback
// //      * process and mark newly updated firmware image as active. For production cases,
// //      * please tune the checkpoint behavior per end application requirement.
// //      */
// //     const esp_partition_t *running = esp_ota_get_running_partition();
// //     esp_ota_img_states_t ota_state;
// //     if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
// //         if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
// //             if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
// //                 ESP_LOGI(TAG, "App is valid, rollback cancelled successfully");
// //             } else {
// //                 ESP_LOGE(TAG, "Failed to cancel rollback");
// //             }
// //         }
// //     }

