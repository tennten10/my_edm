#include "a_config.h"
#include "globals.h"
#include "System.h"
#include "debug.h"
#include "Weight.h"
#include "Buttons.h"
#include "IOTComms.h"
#include "display.h"
#include "main.h"

//temporarily use this to test ota methods
#include "myOTA.h"

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

#include "driver/rtc_io.h"

#include "esp32/ulp.h"
#include "ulp_main.h"
//#include "bootloader_support/include/esp_app_format.h"
#include "esp_app_format.h"
//#include "Eigen/Sparse" note: DO NOT USE in same file/namespace as any ULP library. it has naming conflicts.

extern "C" {
    void app_main();
}

// struct custom_app_desc_t{
//     char app_size[16];  
// };
// const __attribute__((section(".rodata_custom_desc"))) custom_app_desc_t custom_app_desc = { "1" };

// TaskHandle_t battery_TH;
TickType_t xBlockTime = pdMS_TO_TICKS(200);


SystemX* _sys;


extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

void ulp_deinit(){
    gpio_num_t gpio_num = but1;
    rtc_gpio_hold_dis(gpio_num);
    rtc_gpio_deinit(gpio_num);    
}

//static 
void init_ulp_program(){
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
    ulp_debounce_counter = 45;
    ulp_debounce_max_count = 45;
    ulp_next_edge = 0; // Constant value, but leaving the variable here 
    ulp_io_number = rtcio_num; /* map from GPIO# to RTC_IO# */
    //ulp_edge_count_to_wake_up = 1;

    /* Initialize selected GPIO as RTC IO, enable input, disable pullup and pulldown */
    rtc_gpio_init(gpio_num);
    rtc_gpio_set_direction(gpio_num, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_dis(gpio_num);
    rtc_gpio_pullup_dis(gpio_num);
    rtc_gpio_pullup_en(gpio_num); // I added this line, and this makes it work!
    rtc_gpio_hold_en(gpio_num); // might try disabling this? Not sure why I'd want it to stay held. I guess this only works while it's running and not during deep sleep?

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

    ESP_ERROR_CHECK( esp_sleep_enable_ulp_wakeup() );

    //go to sleep
    esp_deep_sleep_start();

}


Device populateStartData(){
    Device ret;
    //char* buf;
    size_t buflen = 9;
    // read NVS flash for SN and stuff...
    
    nvs_handle fctry_handle;
    nvs_flash_init_partition("fctry");
    nvs_open_from_partition("fctry", "fctryNamespace",  
                NVS_READWRITE, &fctry_handle);
    nvs_get_str(fctry_handle, "serial_no", ret.SN, &buflen);
    //strcpy(ret.SN, buf);


    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        debugPrint("Running firmware version: ");
        debugPrintln(running_app_info.version);
        strcpy(ret.VER, running_app_info.version);
    }
    return ret;
}

void app_main() {
    // Setup
    

    // change pin modes if it woke up from ULP vs power up
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        printf("Not ULP wakeup, initializing main prog\n");
        vTaskDelay(150);
        
    } else {
        printf("ULP wakeup, setting everything up...\n");
        //printPulseCount();
        ulp_deinit();
    }


    debugSetup();
    _sys = new SystemX(populateStartData());
    BLEsetup();

    
    // temporary for testing ota
    //setupOTA();

    
    std::string event;
    
    debugPrintln("Before main loop...");

    // To print the threads/packages that are taking a lot of memory
    //heap_caps_print_heap_info( MALLOC_CAP_DEFAULT );

    // battery variables
    int battery=0;
    uint32_t reading=0;
    long batTime = esp_timer_get_time()/1000;

    // loop
    for (;;)
    {
        //battery update moved into this loop to free up some memory
        if(esp_timer_get_time()/1000-batTime > 5000){
        // Operating voltage range of 3.0-4.2V
            // Gave some weird readings over 200% in the past. Add in a coulomb counting "gas gauge" in the future since Li-ion have a non-linear charge-discharge curve.
            // Voltage divider with 1M & 2M, giving read voltage of 2.8 V = 4.2V, 2.0V = 3.0V
            // Resolution of ~ 0.0008V / division
        

            reading =  adc1_get_raw(batV);
            //voltage = esp_adc_cal_raw_to_voltage(reading, adc_chars);

            battery = (int) 100 *( reading * 3.3 / 4096.0 - 2.0) /(2.8 - 2.0); //(voltage - 2.0) / (2.8 - 2.0) ;
            _sys->setBattery(battery);
            
            if(battery < 2){
                _sys->goToSleep();
            }
            batTime = esp_timer_get_time()/1000;
        }
        
        _sys->validateDataAcrossObjects();
        


        event = _sys->buttons->getEvents();
        if (!(event.compare("") == 0)) 
        {

            if (_sys->getMode() == STANDARD)
            {
                //xSemaphoreTake(pageMutex, (TickType_t)10);
                if (_sys->getPage() == WEIGHTSTREAM)
                {
                    //xSemaphoreGive(pageMutex);
                    if (event.compare(0,4,"SNNN",0,4) == 0)
                    {
                        //TODO: TARE FUNCTION
                        //tare();
                    }
                    if (event.compare(0,4,"LNNN",0,4) == 0)
                    {
                        //TODO: SHUTDOWN FUNCTION
                        debugPrintln("sleepy time");
                        _sys->goToSleep(); 
                    }
                    if (event.compare(0,4,"NSNN", 0, 4) == 0)
                    {
                        //TODO: Units
                        //xSemaphoreTake(pageMutex, (TickType_t)10);
                        _sys->setPage(UNITS);
                        //xSemaphoreGive(pageMutex);
                    }
                    if (event.compare(0,4,"NLNN", 0, 4) == 0)
                    {
                        _sys->display->displayUpdateScreen(0);
                        if(setupOTA() == 0){
                        // temporary for testing ota
                            executeOTA();
                        }
                        
                    }
                }
                else if (_sys->getPage() == UNITS)
                {
                    //xSemaphoreGive(pageMutex);
                    if (event.compare(0,4, "SNNN",0,4) == 0)
                    {
                        //TODO:  FUNCTION
                    }
                    if (event.compare(0,4, "LNNN",0,4) == 0)
                    {
                        //TODO:  go to power off function
                        _sys->goToSleep();
                    }
                    if (event.compare(0,4,"NSNN",0,4) == 0)
                    {
                        //TODO: Units
                        _sys->incrementUnits();
                    }
                }
            }

            if (_sys->getMode() == CALIBRATION)
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
        vTaskDelay(20); // try reducing this to 10 if bluetooth issues come up again
    }
}
