
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
    BLESleepPrep();

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
        default:
        debugPrintln("error in increment units");
        break;
    }
    xSemaphoreGive(unitsMutex);
    callbackFlag = true;
    debugPrintln("increment units");
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
        default:
        debugPrintln("error in decrement units");
        break;
    }
    xSemaphoreGive(unitsMutex);
    callbackFlag = true;
    debugPrintln("Decrement units");
}

void SystemX::getSavedVals(){
    
    Data d = getSaveData();

    this->setUnits(d.u);

    debugPrint("Units set to: ");
    debugPrintln(unitsToString(d.u));
   
}

void SystemX::saveVals(){
    setSaveData(getUnits());

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
    debugPrint("local units: ");
    debugPrintln(unitsToString(t));
    if (eUnits != t){
        weight->setLocalUnits(eUnits);
    }
    static std::string current;
    if( WEIGHTSTREAM == this->getPage() ){
        debugPrintln("weight update");
        current = this->weight->getWeightStr();
        // checking if value changes and truncation happen in weight main loop. 
        // If not, it passes -1 which doesn't change the display at all
        debugPrint("current: ");
        debugPrintln(current);
        display->displayWeight(current);
        // bluetooth value is also updated from weight loop
        
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

