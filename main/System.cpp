
#include "globals.h"
#include "a_config.h"
#include "debug.h"
#include "Weight.h"
#include "Buttons.h"
#include "BLE.h"
#include "display.h"
#include "System.h"
#include "main.h"
#include "myOTA.h"
#include "mySPIFFS.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_sleep.h"

void SystemX::goToSleep(){

    display->displayLogo();
    saveVals();
    vTaskDelay(100);
    display->displaySleepPrep();
    weight->sleepPreparation();
    BLEstop();
    
    // These dont' reinitialize on reboot for some reason...
    // delete weight;
    // delete display;
    // delete buttons;

    init_ulp_program();
}

void SystemX::reboot(){

    display->displayLogo();
    saveVals();
    vTaskDelay(100);
    display->displaySleepPrep();
    weight->sleepPreparation();
    BLEstop();

    // These dont' reinitialize on reboot for some reason...
    // delete display;
    // delete buttons;
    // delete weight;
    // vTaskDelay(1000);
    // delete this;
    esp_restart();
}

void SystemX::incrementUnits(){
    debugPrintln("increment units");
    if(xSemaphoreTake(unitsMutex, (TickType_t)10)==pdTRUE){
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
            default:
            debugPrintln("error in increment units");
            break;
        }
        xSemaphoreGive(unitsMutex);
        callbackFlag = true;
    }else{
        debugPrintln("could not get unitsMutex in incrementUnits");
    }
    
    
}

void SystemX::decrementUnits(){
    debugPrintln("Decrement units");
    if(xSemaphoreTake(unitsMutex, (TickType_t)10)==pdTRUE){
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
            default:
            debugPrintln("error in decrement units");
            break;
        }
        xSemaphoreGive(unitsMutex);
        callbackFlag = true;
    }else{
        debugPrintln("could not get unitsMutex in decrementUnits");
    }
}

void SystemX::getSavedVals(){
    
    Data d = getSaveData();

    this->setUnits(d.u);
    // note: make sure display is created before this
    this->display->setIntensity(d.intensity);
    this->display->setColor(d.red,d.green,d.blue);

    debugPrint("Units set to: ");
    debugPrintln(unitsToString(d.u));
   
}

void SystemX::saveVals(){
    Data d = {getUnits(), display->getIntensity(), display->getRed(), display->getGreen(), display->getBlue()};
    setSaveData(d);

}

void SystemX::validateDataAcrossObjects(){
    // Since I don't know how to properly implement callbacks, this is my version in order to verify 
    // objects such as Weight, and Display have the proper local versions of systme variables they need to use
    if(!callbackFlag){
        return;
    }
    debugPrintln("verify function");
    /******************************************/
    // on units change
    static Units t = this->weight->getLocalUnits();
    //debugPrint("local units: ");
    //debugPrintln(unitsToString(t));
    if (getUnits() != t){
        weight->setLocalUnits(getUnits());
        if(isBtConnected()){
            updateBTUnits(getUnits());
        }
            
    }
    static std::string current;
    if( WEIGHTSTREAM == this->getPage() ){
        debugPrintln("weight update");
        current = this->weight->getWeightStr();
        // checking if value changes and truncation happen in weight main loop. 
        // If not, it passes -1 which doesn't change the display at all
        debugPrint("current: ");
        debugPrintln(current);
        display->updateWeight(current);
        // bluetooth value is also updated from here
        
    }
    
    callbackFlag = false;
}

void SystemX::runUpdate(){
    setPage(pUPDATE);
    this->display->displayUpdateScreen(0);
    saveVals();
    if(setupOTA() == 0){
        executeOTA();
    }
}
