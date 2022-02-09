#include "globals.h"
#include "System.h"
#include "_adc.h"
#include "Buttons.h"
#include "display.h"
#include "main.h"


#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "esp_sleep.h"
#include "nvs.h"
#include "nvs_flash.h"
 #include "soc/rtc_cntl_reg.h"
 #include "soc/sens_reg.h"
 #include "soc/rtc_periph.h"
#include "driver/rtc_io.h"


//#include "Eigen/Dense" note: DO NOT USE in same file/namespace as any ULP library. it has naming conflicts.

extern "C" {
    void app_main();
}

SystemX* _sys;

Device populateStartData(){
    Device ret;
    size_t buflen = 9;
    // read NVS flash for SN and stuff...
    
    nvs_handle fctry_handle;
    nvs_flash_init_partition("fctry");
    nvs_open_from_partition("fctry", "fctryNamespace",  
                NVS_READWRITE, &fctry_handle);
    nvs_get_str(fctry_handle, "serial_no", ret.SN, &buflen);
    
    return ret;
}

void app_main() {
    // Setup
    
    _sys = new SystemX(populateStartData());
 
    vTaskDelay(5);
    _sys->init(); // this needs to be after BLE setup otherwise throws eFuse/NVS errors
    
    std::string event;
    PAGE page;
    MODE mode;
    
    // To print the threads/packages that are taking a lot of memory
    //heap_caps_print_heap_info( MALLOC_CAP_DEFAULT );

    long batTime = esp_timer_get_time()/1000;



    // page timeout counters
    long timeout = 0;
    //_sys->weight->runTheoreticalWeight(100., 50., 50.);

    _sys->adc->tare();
    _sys->display->ready = true;
    _sys->display->displayWeight(_sys->adc->getWeightStr());
    //println("Before main loop...");
    for (;;)
    {
        //battery update moved into this loop to free up some memory
        if(esp_timer_get_time()/1000-batTime > 10000){
        // Operating voltage range of 3.0-4.2V
            // Gave some weird readings over 200% in the past. Add in a coulomb counting "gas gauge" in the future since Li-ion have a non-linear charge-discharge curve.
            // Voltage divider with 1M & 2M, giving read voltage of 2.8 V = 4.2V, 2.0V = 3.0V
            // Resolution of ~ 0.0008V / division
        

            // bat_reading =  adc1_get_raw(batV);
            // //voltage = esp_adc_cal_raw_to_voltage(reading, adc_chars);

            // battery = (int) 100 *( bat_reading * 3.3 / 4096.0 - 2.0) /(2.8 - 2.0); //(voltage - 2.0) / (2.8 - 2.0) ;
            // if(battery != _sys->getBattery()){
            //     _sys->setBattery(battery);
                
                //print("battery: ");
                //println(battery);
            //}
            
            
            
            batTime = esp_timer_get_time()/1000;
        }

        _sys->validateDataAcrossObjects();


        event = _sys->buttons->getEvents();
        // print("event: ");
        // println(event);
        page = _sys->getPage();
        mode = _sys->getMode();
        

        if (mode == STANDARD)
        {
            if (page == WEIGHTSTREAM)
            {
                if (event.compare("") == 0) 
                {}       
                else if (event.compare(0,4,"SNNN",0,4) == 0)
                {
                    _sys->adc->tare();
                    timeout = esp_timer_get_time()/1000;
                }
                else if (event.compare(0,4,"LNNN",0,4) == 0)
                {
                    //TODO: SHUTDOWN FUNCTION
                    //println("sleepy time");
                    timeout = esp_timer_get_time()/1000; 
                }
                else if (event.compare(0,4,"NSNN", 0, 4) == 0)
                {
                    //TODO: Units
                    //_sys->setPage(UNITS);
                    //_sys->display->displayUnits(_sys->getUnits());
                    
                    timeout = esp_timer_get_time()/1000;
                }
                else if (event.compare(0,4,"NLNN", 0, 4) == 0)
                {
                    //_sys->setUpdateFlag();
                    timeout = esp_timer_get_time()/1000;
                }
                else if (event.compare(0,4,"NNSN", 0, 4) == 0)
                {
                    timeout = esp_timer_get_time()/1000;
                }
                else if (event.compare(0,4,"NNLN", 0, 4) == 0)
                {
                    timeout = esp_timer_get_time()/1000;
                }else{
                    //println("button press type not recognized");
                    timeout = esp_timer_get_time()/1000;
                }
            }
            else if (page == UNITS)
            {
                // if no button presses while on this page for a few seconds, revert back to displaying the weight
                if((esp_timer_get_time()/1000) - timeout > 2000){
                    _sys->setPage(WEIGHTSTREAM);
                    _sys->display->displayWeight(_sys->adc->getWeightStr());
                    //println("units screen timeout");
                }else{
                    if (event.compare("") == 0) 
                    {}
                    else if (event.compare(0,4, "SNNN",0,4) == 0)
                    {
                        //TODO:  FUNCTION
                        _sys->setPage(WEIGHTSTREAM);
                        _sys->display->displayWeight(_sys->adc->getWeightStr());
                        timeout = esp_timer_get_time()/1000;
                    }
                    else if (event.compare(0,4, "LNNN",0,4) == 0)
                    {
                        //println("sleepy time");
                        timeout = esp_timer_get_time()/1000;
                    }
                    else if (event.compare(0,4,"NSNN",0,4) == 0)
                    { 
                        timeout = esp_timer_get_time()/1000;
                    }
                    else if (event.compare(0,4,"NLNN", 0, 4) == 0)
                    {
                        timeout = esp_timer_get_time()/1000;
                    }
                    else if (event.compare(0,4,"NNSN", 0, 4) == 0)
                    {
                        //_sys->incrementUnits();
                        timeout = esp_timer_get_time()/1000;
                    }
                    else if (event.compare(0,4,"NNLN", 0, 4) == 0)
                    {
                        timeout = esp_timer_get_time()/1000;
                    }
                    else if (event.compare(0,4,"NNNS", 0, 4) == 0)
                    {
                        //_sys->decrementUnits();
                        timeout = esp_timer_get_time()/1000;
                    }
                    else if (event.compare(0,4,"NNNL", 0, 4) == 0)
                    {
                        timeout = esp_timer_get_time()/1000;
                    }else{
                        //println("button press type not recongnized");
                        timeout = esp_timer_get_time()/1000;
                    }
                }
            } else if(page == pUPDATE){
            //     // NOT SURE IF I WANT TO DO THIS HERE OR IN A DIFFERENT LOOP. STAY TUNED.
            //     q = 50; //getUpdatePercent(); TODO: include update amount -> delayed for later since giving weird numbers
            //     if (q < 3 && q != q_last)
            //     {
            //         _sys->display->displayUpdateScreen(3);
            //         q_last = q;
            //     }
            //     else if (q > q_last)
            //     {
            //         _sys->display->displayUpdateScreen(q);
            //         q_last = q;
            //     }
            //     // TODO: dynamically update percentage when downloading and inistalling updated code
            //     break;
            }else if(page == SETTINGS){
                
            }else if(page == INFO){
                // device info stuff
                //_sys->display->displayDeviceInfo(_sys->getSN(), _sys->getVER());
            } //else if(page == )

            // if(page != pUPDATE){
            //     if((esp_timer_get_time()/1000) - timeout > 1200000){ // timeout after roughly 20 minutes
            //         println("timeout sleepy time");
            //         _sys->goToSleep();
            //     }
            // }

        } else if (mode == CALIBRATION)
        {
            /*if(ePage == ){
        
            if(strcmp(event,"SNNN")){
        
            }
            if(strcmp(event, "LNNN")){
            }
            if(strcmp(event, "NSNN")){
            
            }
        }
        else if(ePage == ){
            if(strcmp(event,"SNNN")){
            //TODO:  FUNCTION
            }
            if(strcmp(event, "LNNN")){
            //TODO:  FUNCTION
            }
            if(strcmp(event, "NSNN")){
            //TODO: FUNCTION
            }
            }*/
        }

        
        vTaskDelay(20); 
    }
}
