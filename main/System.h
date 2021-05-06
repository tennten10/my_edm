#ifndef SYSTEM_H
#define SYSTEM_H

#include "a_config.h"
#include "globals.h"
#include "debug.h"
#include "Weight.h"
#include "Buttons.h"
#include "IOTComms.h"
#include "display.h"
#include "main.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_ota_ops.h"


class SystemX 
{
public: 
    SystemX(Device _device) : device{_device}
    {
        this->display = new DisplayX(); // constructor
        this->buttons = new ButtonsX(true);
    }
    ~SystemX(){}

    void incrementUnits();
    void decrementUnits();
    Units getUnits(){
        Units ret;
        xSemaphoreTake(unitsMutex, (TickType_t) 20);
        ret = eUnits;
        xSemaphoreGive(unitsMutex);
        return ret;
    }

    void setUnits(Units u){
        xSemaphoreTake(unitsMutex, (TickType_t) 20);
        eUnits = u;
        xSemaphoreGive(unitsMutex);
    }

    int getBattery(){
        int ret;
        xSemaphoreTake(batteryMutex, (TickType_t) 20);
        ret = batteryLevel;
        xSemaphoreGive(batteryMutex);
        return ret;
    }

    void goToSleep();

    void runUpdate();
    
    PAGE getPage(){
        PAGE ret;
        xSemaphoreTake(pageMutex, (TickType_t) 20);
        ret = ePage;
        xSemaphoreGive(pageMutex);
        return ret;
    }

    void setPage(PAGE p){
        xSemaphoreTake(pageMutex, (TickType_t) 20);
        ePage = p;
        xSemaphoreGive(pageMutex);
    }

    MODE getMode(){
        MODE ret;
        xSemaphoreTake(modeMutex, (TickType_t) 10);
        ret = eMode;
        xSemaphoreGive(modeMutex);
        return ret;
    }
    void setMode(MODE m){
        xSemaphoreTake(modeMutex, (TickType_t) 10);
        eMode = m;
        xSemaphoreGive(modeMutex);
    }


    DisplayX *display; // {};
    ButtonsX *buttons; //{_debounce = true};
    

private:
    const Device device;
    SemaphoreHandle_t systemMutex = xSemaphoreCreateMutex();
    SemaphoreHandle_t pageMutex = xSemaphoreCreateMutex();
    SemaphoreHandle_t unitsMutex = xSemaphoreCreateMutex();
    SemaphoreHandle_t batteryMutex = xSemaphoreCreateMutex();
    SemaphoreHandle_t modeMutex = xSemaphoreCreateMutex();

    void setBattery(int b){
        xSemaphoreTake(batteryMutex, (TickType_t)10);
        batteryLevel = b;
        xSemaphoreGive(batteryMutex);
    }
    
    //BLEsetup();
 
    Units eUnits;
    volatile int batteryLevel;

    MODE eMode = STANDARD;
    volatile PAGE ePage = WEIGHTSTREAM;

};

#endif