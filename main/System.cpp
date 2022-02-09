
#include "globals.h"
#include "a_config.h"
#include "_adc.h"
#include "Buttons.h"
#include "display.h"
#include "System.h"
#include "main.h"
#include "mySPIFFS.h"

#include "nvs.h"
#include "nvs_flash.h"



void SystemX::reboot(){

    display->displayLogo();
    saveVals();
    vTaskDelay(100);
    display->displaySleepPrep();
    adc->sleepPreparation();

    
    esp_restart();
}


void SystemX::getSavedVals(){
    
    Data d = getSaveData();

    //this->setUnits(d.u);
    // note: make sure display is created before this
    this->display->setIntensity(d.intensity);
    this->display->setColor(d.red,d.green,d.blue);

    //print("Units set to: ");
    //println(unitsToString(d.u));
   
}

void SystemX::saveVals(){
    Data d = {display->getIntensity(), display->getRed(), display->getGreen(), display->getBlue()};
    setSaveData(d);

}

void SystemX::validateDataAcrossObjects(){
    // Since I don't know how to properly implement callbacks, this is my version in order to verify 
    // objects such as Weight, and Display have the proper local versions of systme variables they need to use
    if(!callbackFlag){
        return;
    }
    //println("verify function");
    /******************************************/
    // on units change
    //Units t = this->weight->getLocalUnits();
    //Units v = getUnits();
    //print("local units: ");
    //println(unitsToString(t));
    //bool ummidk = !(t == v); // janky but it doesn't work in if statement otherwise
    //print("Validate compare: ");
    //print((int)v);
    //print(" (v), (t) ");
    //print((int)t);
    //print(" compare:  ");
    //println((int)ummidk);
    //if (ummidk){
        //printf("setting units in validate: %s, %s\n", unitsToString(getUnits()).c_str(), unitsToString( weight->getLocalUnits()).c_str());
        //printf("setting units in validate: %d, %d\n", getUnits(), weight->getLocalUnits());
        //weight->setLocalUnits(getUnits());
        // if(isBtConnected()){
        //     updateBTUnits(getUnits());
        // }
        // if(UNITS == this->getPage()){
        //     this->display->updateUnits(getUnits());
        // }
        
    //}
    static std::string current;
    if( WEIGHTSTREAM == this->getPage() ){
        //println("weight update");
        current = this->adc->getWeightStr();
        // checking if value changes and truncation happen in weight main loop. 
        // If not, it passes -1 which doesn't change the display at all
        display->updateWeight(current);
        // bluetooth value is also updated from here
        
    }
    // if(getUpdateFlag()){
    //     this->runUpdate();
    //     updateFlag = false;
    // }
    
    callbackFlag = false;
}


