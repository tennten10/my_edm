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
#include "status.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_ota_ops.h"


class SystemX 
{
public: 
    SystemX(Device _device) : device{_device}
    {
        debugPrintln("inside system constructor");
        this->buttons = new ButtonsX(true);
        this->weight = new WeightX();
        this->wifiInfo = getActiveWifiInfo();
        
    }
    ~SystemX(){
        
    }
    
    // temp
    void init(){
        //this->display = new DisplayX(); // constructor
        vTaskDelay(50);
        this->getSavedVals();
        this->weight->setLocalUnits(eUnits);
        

    }

    void incrementUnits();
    void decrementUnits();

    Units getUnits(){
        Units ret = err;
        if(xSemaphoreTake(unitsMutex, (TickType_t) 20)==pdTRUE){
            ret = eUnits;
            xSemaphoreGive(unitsMutex);
        }else{
            debugPrintln("could not get unitsMutex in getUnits");
        }
        
        return ret;
    }

    void setUnits(Units u){
        if(xSemaphoreTake(unitsMutex, (TickType_t) 20)==pdTRUE){
            eUnits = u;
            xSemaphoreGive(unitsMutex);
            callbackFlag = true;
        }else{
            debugPrintln("could not get unitsMutex in setUnits");
        }
        
    }

    int getBattery(){
        int ret = 0;
        if(xSemaphoreTake(batteryMutex, (TickType_t) 20)==pdTRUE){
            ret = batteryLevel;
            xSemaphoreGive(batteryMutex);
        }else{
            debugPrintln("could not get batteryMutex in getBattery");
        }
        return ret;
    }

    void setBattery(int b){
        if(b > 100){
            debugPrintln("Battery over 100% ?????? - might be charging");
            b = 100;
        }else if(b < 0){
            debugPrintln("Battery less than 0% ???????");
            b = 0;
        }
        if(xSemaphoreTake(batteryMutex, (TickType_t)10)==pdTRUE){
            batteryLevel = b;
            xSemaphoreGive(batteryMutex);
        }else{
            debugPrintln("could not get batteryMutex in setBattery");
        }
    }

    void goToSleep();
    void reboot();

    void runUpdate();
    
    PAGE getPage(){
        PAGE ret = WEIGHTSTREAM;
        if(xSemaphoreTake(pageMutex, (TickType_t) 20)==pdTRUE){
            ret = ePage;
            xSemaphoreGive(pageMutex);
        }else{
            debugPrintln("could not get pageMutex in getPage");
        }
        return ret;
    }

    void setPage(PAGE p){
        if(xSemaphoreTake(pageMutex, (TickType_t) 20)==pdTRUE){
            ePage = p;
            xSemaphoreGive(pageMutex);
            callbackFlag = true;
        }else{
            debugPrintln("could not get pageMutex in setPage");
        }
        
    }

    MODE getMode(){
        MODE ret= STANDARD;
        if(xSemaphoreTake(modeMutex, (TickType_t) 20)==pdTRUE){        
            ret = eMode;
            xSemaphoreGive(modeMutex);
        }else{
            debugPrintln("could not get modeMutex in getMode");
        }
        return ret;
    }
    void setMode(MODE m){
        if(xSemaphoreTake(modeMutex, (TickType_t) 20)==pdTRUE){
            eMode = m;
            xSemaphoreGive(modeMutex);
            callbackFlag = true;
        }else{
            debugPrintln("could not get modeMutex in setMode");
        }
    }

    std::string getSN(){
        std::string ret = "";
        if(xSemaphoreTake(deviceMutex, (TickType_t) 10)==pdTRUE){
            ret = std::string(device.SN);
            xSemaphoreGive(deviceMutex);
        }else{
            debugPrintln("could not get deviceMutex in getSN");
        }
        return ret;
    }

    std::string getVER(){
        std::string ret = "";
        if(xSemaphoreTake(deviceMutex, (TickType_t) 10)==pdTRUE){
            ret = std::string(this->device.VER);
            xSemaphoreGive(deviceMutex);
        }else{
            debugPrintln("could not get deviceMutex in getVER");
        }
        return ret;
    }

    void setWiFiInfo(WiFiStruct w){
        if(xSemaphoreTake(deviceMutex, (TickType_t) 10)==pdTRUE){
            wifiInfo = w;
            xSemaphoreGive(deviceMutex);
        }else{
            debugPrintln("could not get deviceMutex in setWiFiInfo");
        }
        
    }
    WiFiStruct getWiFiInfo(){
        static WiFiStruct ret = WiFiStruct();
        if(xSemaphoreTake(deviceMutex, (TickType_t) 10)==pdTRUE){
            ret = wifiInfo;
            xSemaphoreGive(deviceMutex);
        }else{
            debugPrintln("could not get deviceMutex in getWiFiInfo");
        }
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

    void getSavedVals(); // initialize values saved across boots from NVS
    void saveVals();
    
    Units eUnits = kg;
    int batteryLevel;

    MODE eMode = STANDARD;
    PAGE ePage = WEIGHTSTREAM;
    
    WiFiStruct wifiInfo;
    
};

#endif