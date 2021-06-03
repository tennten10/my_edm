#ifndef SYSTEM_H
#define SYSTEM_H

#include "a_config.h"
#include "globals.h"
#include "debug.h"
#include "Weight.h"
#include "Buttons.h"
#include "BLE.h"
#include "mySPIFFS.h"
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
        this->weight = new WeightX();

        // this->wifiInfo = availableWiFiInfo();
        this->wifiInfo = getActiveWifiInfo();

        
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
        callbackFlag = true;
    }

    int getBattery(){
        int ret;
        xSemaphoreTake(batteryMutex, (TickType_t) 20);
        ret = batteryLevel;
        xSemaphoreGive(batteryMutex);
        return ret;
    }
    void setBattery(int b){
        if(b > 100){
            debugPrintln("Battery over 100% ??????");
            b = 100;
        }else if(b < 0){
            debugPrintln("Battery less than 0% ???????");
            b = 0;
        }
        xSemaphoreTake(batteryMutex, (TickType_t)10);
        batteryLevel = b;
        xSemaphoreGive(batteryMutex);
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
        callbackFlag = true;
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
        callbackFlag = true;
    }
    std::string getSN(){
        xSemaphoreTake(deviceMutex, (TickType_t) 10);
        std::string ret = std::string(device.SN);
        xSemaphoreGive(deviceMutex);
        return ret;
    }

    std::string getVER(){
        xSemaphoreTake(deviceMutex, (TickType_t) 10);
        static std::string ret = std::string(this->device.VER);
        xSemaphoreGive(deviceMutex);
        return ret;
    }

    void setWiFiInfo(WiFiStruct w){
        xSemaphoreTake(deviceMutex, (TickType_t) 10);
        wifiInfo = w;
        xSemaphoreGive(deviceMutex);
    }
    WiFiStruct getWiFiInfo(){
        xSemaphoreTake(deviceMutex, (TickType_t) 10);
        static WiFiStruct ret = wifiInfo;
        xSemaphoreGive(deviceMutex);
        return ret;
    }

    void validateDataAcrossObjects();

    bool callbackFlag = true;


    DisplayX *display; 
    ButtonsX *buttons; 
    WeightX *weight;
    

private:
    const Device device;
    SemaphoreHandle_t deviceMutex = xSemaphoreCreateMutex();
    SemaphoreHandle_t pageMutex = xSemaphoreCreateMutex();
    SemaphoreHandle_t unitsMutex = xSemaphoreCreateMutex();
    SemaphoreHandle_t batteryMutex = xSemaphoreCreateMutex();
    SemaphoreHandle_t modeMutex = xSemaphoreCreateMutex();
    SemaphoreHandle_t weightMutex = xSemaphoreCreateMutex();
    
 
    Units eUnits;
    int batteryLevel;

    MODE eMode = STANDARD;
    PAGE ePage = WEIGHTSTREAM;
    
    WiFiStruct wifiInfo;
    

};

#endif