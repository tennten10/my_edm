#include "a_config.h"
#include "globals.h"
#include "PinDefs.h"
#include "debug.h"
#include "Weight.h"
#include "Buttons.h"
//#include "IOTComms.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h" 

extern "C" {
    void app_main(void);
}
/* Function Prototypes, if necessary */
 
/**/

TaskHandle_t battery_TH;
extern QueueHandle_t buttonQueue;
TickType_t xBlockTime = pdMS_TO_TICKS(200);
SemaphoreHandle_t systemMutex;
SemaphoreHandle_t pageMutex; // this was extern before...

System _sys = {"10011001", "0.1", g, 80};
STATE eState = STANDARD;
volatile PAGE ePage;


void goToSleep(){
  // create shutdown function
  // Deep sleep turned off until chip is on board
  //esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0); // 1 = HIGH, 0 = LOW // but1
  //rtc_gpio_pullup_en(GPIO_NUM_13);
  //esp_deep_sleep_start();
  debugPrintln("sleeping...");
}

void batteryHandler_(void * pvParameters){
    int battery=0;
    uint32_t reading=0;
    //uint32_t voltage=0;

    //Characterize ADC at particular atten
    /*esp_adc_cal_characteristics_t *adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    //Check type of calibration value used to characterize ADC
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("eFuse Vref");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Two Point");
    } else {
        printf("Default");
    }*/
    for(;;){
    // Operating voltage range of 3.0-4.2V
        // Gave some weird readings over 200% in the past. Add in a coulomb counting "gas gauge" in the future since Li-ion have a non-linear charge-discharge curve.
        // Voltage divider with 1M & 2M, giving read voltage of 2.8 V = 4.2V, 2.0V = 3.0V
        // Resolution of ~ 0.0008V / division
      

        reading =  adc1_get_raw(batV);
        //voltage = esp_adc_cal_raw_to_voltage(reading, adc_chars);

        battery = (int) 100 *( reading * 3.3 / 4096.0 - 2.0) /(2.8 - 2.0); //(voltage - 2.0) / (2.8 - 2.0) ;
        xSemaphoreTake(systemMutex, (TickType_t)10);
        _sys.batteryLevel = battery;
        xSemaphoreGive(systemMutex);
        if(battery < 2){
        goToSleep();
        }
        vTaskDelay(1500); 
    }
}

void incrementUnits(){
  xSemaphoreTake(systemMutex, (TickType_t)10);
  switch(_sys.eUnits){
    case g:
    _sys.eUnits = kg;
    break;
    case kg: 
    _sys.eUnits = oz;
    break;
    case oz:
    _sys.eUnits = lb;
    break;
    case lb:
    _sys.eUnits = g;
    break;
  }
  xSemaphoreGive(systemMutex);
}

void decrementUnits(){
  xSemaphoreTake(systemMutex, (TickType_t)10);
  switch(_sys.eUnits){
    case g:
    _sys.eUnits = lb;
    break;
    case kg: 
    _sys.eUnits = g;
    break;
    case oz:
    _sys.eUnits = kg;
    break;
    case lb:
    _sys.eUnits = oz;
    break;
  }
  xSemaphoreGive(systemMutex);
}


void app_main() {
  // setup
    systemMutex = xSemaphoreCreateMutex();
    pageMutex = xSemaphoreCreateMutex();

    xTaskCreate(    
        batteryHandler_,          /* Task function. */
        "Battery Handler",        /* String with name of task. */
        5000,            /* Stack size in words, not bytes. */
        NULL,             /* Parameter passed as input of the task */
        1,                /* Priority of the task. */
        &battery_TH              /* Task handle. */  
        );  
    
    
    //BLEsetup();

    // initializes button class
    ButtonsX buttons{true};    
    
    std::string event;
    
    // loop
    for (;;)
    {
        event = buttons.getEvents();
        if (event.compare("") != 0) 
        {
            debugPrint("a: ");
            debugPrintln(event);

            if (eState == STANDARD)
            {
                xSemaphoreTake(pageMutex, (TickType_t)10);
                if (ePage == WEIGHTSTREAM)
                {
                    xSemaphoreGive(pageMutex);
                    if (event.compare("SNNN") == 0)
                    {
                        //TODO: TARE FUNCTION
                        tare();
                    }
                    if (event.compare("LNNN") == 0)
                    {
                        //TODO: SHUTDOWN FUNCTION
                        goToSleep();
                    }
                    if (event.compare("NSNN") == 0)
                    {
                        //TODO: Units
                        xSemaphoreTake(pageMutex, (TickType_t)10);
                        ePage = UNITS;
                        xSemaphoreGive(pageMutex);
                    }
                }
                else if (ePage == UNITS)
                {
                    xSemaphoreGive(pageMutex);
                    if (event.compare("SNNN") == 0)
                    {
                        //TODO:  FUNCTION
                    }
                    if (event.compare("LNNN") == 0)
                    {
                        //TODO:  go to power off function
                        goToSleep();
                    }
                    if (event.compare("NSNN") == 0)
                    {
                        //TODO: Units
                        incrementUnits();
                    }
                }
            }

            if (eState == CALIBRATION)
            {
                /*if(ePage == ){
           * xSemaphoreGive(pageMutex);
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
              //TODO: Units
              incrementUnits();
              }
            }*/
            }

            event = "";
        }
        vTaskDelay(20);
    }
}