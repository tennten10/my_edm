#ifndef SYSTEM_H
#define SYSTEM_H

//#include "a_config.h"
#include "globals.h"
#include "_adc.h"
#include "Buttons.h"
#include "mySPIFFS.h"
#include "display.h"
#include "main.h"
#include "status.h"
#include "Motion.h"

#include "nvs.h"
#include "nvs_flash.h"


class SystemX 
{
public: 
    SystemX(Device _device) : device{_device}
    {
        //println("inside system constructor");
        this->buttons = new ButtonsX(true);
        this->adc = new ADCX();
        
    }
    ~SystemX(){
        
    }
    
    // temp
    void init(){
        this->display = new DisplayX(); 
        vTaskDelay(75);
        this->getSavedVals();
        //this->adc->setLocalUnits(eUnits);
        

    }

    // void incrementUnits();
    // void decrementUnits();

    // Units getUnits(){
    //     Units ret = err;
    //     if(xSemaphoreTake(unitsMutex, (TickType_t) 20)==pdTRUE){
    //         ret = eUnits;
    //         xSemaphoreGive(unitsMutex);
    //     }else{
    //         //println("could not get unitsMutex in getUnits");
    //     }
        
    //     return ret;
    // }

    // void setUnits(Units u){
    //     if(xSemaphoreTake(unitsMutex, (TickType_t) 20)==pdTRUE){
    //         eUnits = u;
    //         xSemaphoreGive(unitsMutex);
    //         callbackFlag = true;
    //     }else{
    //         //println("could not get unitsMutex in setUnits");
    //     }
        
    // }

    

    void reboot();

    //void runUpdate();
    
    PAGE getPage(){
        PAGE ret = WEIGHTSTREAM;
        if(xSemaphoreTake(pageMutex, (TickType_t) 20)==pdTRUE){
            ret = ePage;
            xSemaphoreGive(pageMutex);
        }else{
            //println("could not get pageMutex in getPage");
        }
        return ret;
    }

    void setPage(PAGE p){
        if(xSemaphoreTake(pageMutex, (TickType_t) 20)==pdTRUE){
            ePage = p;
            xSemaphoreGive(pageMutex);
            callbackFlag = true;
        }else{
            //println("could not get pageMutex in setPage");
        }
        
    }

    MODE getMode(){
        MODE ret= STANDARD;
        if(xSemaphoreTake(modeMutex, (TickType_t) 20)==pdTRUE){        
            ret = eMode;
            xSemaphoreGive(modeMutex);
        }else{
            //println("could not get modeMutex in getMode");
        }
        return ret;
    }
    void setMode(MODE m){
        if(xSemaphoreTake(modeMutex, (TickType_t) 20)==pdTRUE){
            eMode = m;
            xSemaphoreGive(modeMutex);
            callbackFlag = true;
        }else{
            //println("could not get modeMutex in setMode");
        }
    }

    std::string getSN(){
        std::string ret = "";
        if(xSemaphoreTake(deviceMutex, (TickType_t) 10)==pdTRUE){
            ret = std::string(device.SN);
            xSemaphoreGive(deviceMutex);
        }else{
            //println("could not get deviceMutex in getSN");
        }
        return ret;
    }

    std::string getVER(){
        std::string ret = "";
        if(xSemaphoreTake(deviceMutex, (TickType_t) 10)==pdTRUE){
            ret = std::string(this->device.VER);
            xSemaphoreGive(deviceMutex);
        }else{
            //println("could not get deviceMutex in getVER");
        }
        return ret;
    }

    

    void setUpdateFlag(){
        if(xSemaphoreTake(updateFlagMutex, (TickType_t) 10)==pdTRUE){
            //updateFlag = true;
            xSemaphoreGive(updateFlagMutex);
        }else{
            //println("could not get updateFlagMutex in setUpdateFlag");
        }
    }
    bool getUpdateFlag(){
        static bool ret = false;
        if(xSemaphoreTake(updateFlagMutex, (TickType_t) 10)==pdTRUE){
            //ret = updateFlag;
            xSemaphoreGive(updateFlagMutex);
        }else{
            //println("could not get deviceMutex in getWiFiInfo");
        }
        return ret;
    }
    
    void validateDataAcrossObjects();

    bool callbackFlag = true;

    
    DisplayX *display; 
    ButtonsX *buttons; 
    ADCX *adc;
    MotionX *motion;
    

private:
    const Device device;
    SemaphoreHandle_t deviceMutex = xSemaphoreCreateMutex();        // for accessing device information variables
    SemaphoreHandle_t pageMutex = xSemaphoreCreateMutex();          // for accessing page variable
    SemaphoreHandle_t unitsMutex = xSemaphoreCreateMutex();         // for accessing units variable
    SemaphoreHandle_t modeMutex = xSemaphoreCreateMutex();          //  for accessing mode variable
    SemaphoreHandle_t adcMutex = xSemaphoreCreateMutex();        //  for accessing local weight variable
    SemaphoreHandle_t upMutex = xSemaphoreCreateMutex();            // so that two update signals don't run at once
    SemaphoreHandle_t updateFlagMutex = xSemaphoreCreateMutex();    // for accessing updateFlag variable

    void getSavedVals(); // initialize values saved across boots from NVS
    void saveVals();
    

    MODE eMode = STANDARD;
    PAGE ePage = WEIGHTSTREAM;
    
};

#endif