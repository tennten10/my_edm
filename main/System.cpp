#include "a_config.h"
#include "globals.h"
#include "debug.h"
#include "Weight.h"
#include "Buttons.h"
#include "IOTComms.h"
#include "display.h"
#include "System.h"
#include "main.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_ota_ops.h"


void SystemX::goToSleep(){
    // create shutdown function
    // Deep sleep turned off until chip is on board
    //esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0); // 1 = HIGH, 0 = LOW // but1
    //rtc_gpio_pullup_en(GPIO_NUM_13);
    //debugPrintln("sleeping...");
    vTaskDelay(10);
    init_ulp_program();
  
}

void SystemX::incrementUnits(){
    xSemaphoreTake(unitsMutex, (TickType_t)10);
    switch(eUnits){
        case g:
        eUnits = kg;
        break;
        case kg: 
        eUnits = oz;
        break;
        case oz:
        eUnits = lb;
        break;
        case lb:
        eUnits = g;
        break;
    }
    xSemaphoreGive(unitsMutex);
}

void SystemX::decrementUnits(){
    xSemaphoreTake(unitsMutex, (TickType_t)10);
    switch(eUnits){
        case g:
        eUnits = lb;
        break;
        case kg: 
        eUnits = g;
        break;
        case oz:
        eUnits = kg;
        break;
        case lb:
        eUnits = oz;
        break;
    }
    xSemaphoreGive(unitsMutex);
}

