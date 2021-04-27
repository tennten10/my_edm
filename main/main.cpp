#include "a_config.h"
#include "globals.h"
#include "debug.h"
//#include "Weight.h"
#include "Buttons.h"
//#include "IOTComms.h"
#include "display.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

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
#include "esp32/ulp.h"
#include "ulp_main.h"
//#include "Eigen/Dense" note: DO NOT USE in same file/namespace as any ULP library. it has naming conflicts.

extern "C" {
    void app_main();
}
/* Function Prototypes, if necessary */
 
/**/

TaskHandle_t battery_TH;
TickType_t xBlockTime = pdMS_TO_TICKS(200);
SemaphoreHandle_t systemMutex;
SemaphoreHandle_t pageMutex; // this was extern before...

System _sys = {"10011001", "0.1", g, 80};
STATE eState = STANDARD;
volatile PAGE ePage = WEIGHTSTREAM;

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

void ulp_deinit(){
    gpio_num_t gpio_num = but1;
    rtc_gpio_hold_dis(gpio_num);
    rtc_gpio_deinit(gpio_num);    
}

static void init_ulp_program(){
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start,
            (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    /* GPIO used for pulse counting. */
    gpio_num_t gpio_num = but1;
    int rtcio_num = rtc_io_number_get(gpio_num);
    assert(rtc_gpio_is_valid_gpio(gpio_num) && "GPIO used for pulse counting must be an RTC IO");

    /* Initialize some variables used by ULP program.
     * Each 'ulp_xyz' variable corresponds to 'xyz' variable in the ULP program.
     * These variables are declared in an auto generated header file,
     * 'ulp_main.h', name of this file is defined in component.mk as ULP_APP_NAME.
     * These variables are located in RTC_SLOW_MEM and can be accessed both by the
     * ULP and the main CPUs.
     *
     * Note that the ULP reads only the lower 16 bits of these variables.
     */
    ulp_debounce_counter = 3;
    ulp_debounce_max_count = 3;
    ulp_next_edge = 0;
    ulp_io_number = rtcio_num; /* map from GPIO# to RTC_IO# */
    ulp_edge_count_to_wake_up = 10;

    /* Initialize selected GPIO as RTC IO, enable input, disable pullup and pulldown */
    rtc_gpio_init(gpio_num);
    rtc_gpio_set_direction(gpio_num, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_dis(gpio_num);
    rtc_gpio_pullup_dis(gpio_num);
    rtc_gpio_pullup_en(gpio_num);
    rtc_gpio_hold_en(gpio_num);

    /* Disconnect GPIO12 and GPIO15 to remove current drain through
     * pullup/pulldown resistors.
     * GPIO12 may be pulled high to select flash voltage.
     */
    rtc_gpio_isolate(GPIO_NUM_12);
    rtc_gpio_isolate(GPIO_NUM_15);
    esp_deep_sleep_disable_rom_logging(); // suppress boot messages

    /* Set ULP wake up period to T = 20ms. -> changed to 50ms
     * Minimum pulse width has to be T * (ulp_debounce_counter + 1) = 80ms.
     */
    ulp_set_wakeup_period(0, 50000);

    /* Start the program */
    err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);

}

static void printPulseCount(){
    //const char* namespace = "plusecnt";
    //const char* count_key = "count";

    //ESP_ERROR_CHECK( nvs_flash_init() );
    //nvs_handle_t handle;
    //ESP_ERROR_CHECK( nvs_open(namespace, NVS_READWRITE, &handle));
    //uint32_t pulse_count = 0;
    //esp_err_t err = nvs_get_u32(handle, count_key, &pulse_count);
    //assert(err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND);
    //printf("Read pulse count from NVS: %5d\n", pulse_count);

    /* ULP program counts signal edges, convert that to the number of pulses */
    uint32_t pulse_count_from_ulp = (ulp_edge_count & UINT16_MAX) / 2;
    /* In case of an odd number of edges, keep one until next time */
    ulp_edge_count = ulp_edge_count % 2;
    printf("Pulse count from ULP: %5d\n", pulse_count_from_ulp);

    /* Save the new pulse count to NVS */
    //pulse_count += pulse_count_from_ulp;
    //ESP_ERROR_CHECK(nvs_set_u32(handle, count_key, pulse_count));
    //ESP_ERROR_CHECK(nvs_commit(handle));
    //nvs_close(handle);
    //printf("Wrote updated pulse count to NVS: %5d\n", pulse_count);
}


void goToSleep(){
  // create shutdown function
  // Deep sleep turned off until chip is on board
  //esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0); // 1 = HIGH, 0 = LOW // but1
  //rtc_gpio_pullup_en(GPIO_NUM_13);
  //debugPrintln("sleeping...");
  vTaskDelay(10);
  init_ulp_program();
  esp_deep_sleep_start();
  
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
    // Setup
    systemMutex = xSemaphoreCreateMutex();
    pageMutex = xSemaphoreCreateMutex();
    debugSetup();

    // change pin modes if it woke up from ULP vs power up
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        printf("Not ULP wakeup, initializing main prog\n");
        vTaskDelay(250);
        
    } else {
        printf("ULP wakeup, saving pulse count\n");
        printPulseCount();
        ulp_deinit();
    }

    xTaskCreate(    
        batteryHandler_,          /* Task function. */
        "Battery Handler",        /* String with name of task. */
        5000,            /* Stack size in words, not bytes. */
        NULL,             /* Parameter passed as input of the task */
        1,                /* Priority of the task. */
        &battery_TH              /* Task handle. */  
        );  
    
    // strainGaugeSetup();
    //BLEsetup();
    displaySetup();
    ButtonsX buttons{true};
    std::string event;
    vTaskDelay(100);
    debugPrintln("Before main loop...");
    // loop
    for (;;)
    {
        event = buttons.getEvents();//ButX_c_getEvents( *buttons ); //"NSNN" ;//buttons.getEvents();
        if (!(event.compare("") == 0)) 
        {
            //debugPrint("a: ");
            //debugPrintln(event);
            //printf("%s bool: %d", event.c_str(), event.compare(0,4, "LNNN",0,4) );

            if (eState == STANDARD)
            {
                xSemaphoreTake(pageMutex, (TickType_t)10);
                if (ePage == WEIGHTSTREAM)
                {
                    xSemaphoreGive(pageMutex);
                    if (event.compare(0,4,"SNNN",0,4) == 0)
                    {
                        //TODO: TARE FUNCTION
                        //tare();
                    }
                    if (event.compare(0,4,"LNNN",0,4) == 0)
                    {
                        //TODO: SHUTDOWN FUNCTION
                        debugPrintln("sleepy time");
                        goToSleep();
                    }
                    if (event.compare(0,4,"NSNN", 0, 4) == 0)
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
                    if (event.compare(0,4, "SNNN",0,4) == 0)
                    {
                        //TODO:  FUNCTION
                    }
                    if (event.compare(0,4, "LNNN",0,4) == 0)
                    {
                        //TODO:  go to power off function
                        goToSleep();
                    }
                    if (event.compare(0,4,"NSNN",0,4) == 0)
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