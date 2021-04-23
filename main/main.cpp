/* LVGL Example project
 *
 * Basic project to test LVGL on ESP32 based projects.
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"

#include "display.h"
#include "globals.h"
#include "debug.h"

TickType_t xBlockTime = pdMS_TO_TICKS(200);
SemaphoreHandle_t systemMutex;
SemaphoreHandle_t pageMutex; // this was extern before...

System _sys = {"10011001", "0.1", g, 80};  
STATE eState = STANDARD;
volatile PAGE ePage;

/**********************
 *   APPLICATION MAIN
 **********************/
extern "C"{
    void app_main();
};
void app_main() { 
    systemMutex = xSemaphoreCreateMutex();
    pageMutex = xSemaphoreCreateMutex();
    debugSetup();
    displaySetup();
    
    for(;;){
        printf("do nothing\n");
        vTaskDelay(500);
    }
}

