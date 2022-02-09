#include "Motion.h"
//#include "a_config.h"

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string>
#include <iostream>



void ButtonsX::sleepPreparation() 
{
    //vDeleteTask(buttonHandler_TH){}
    //println("button inside sleep prep");
}

void ButtonsX::Main() // loop
{
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = (gpio_pullup_t) 1;
    gpio_config(&io_conf);
    
    buttonQueue = xQueueCreate(50, 10*sizeof(char)); /*xQueueCreateStatic(QUEUE_LENGTH,  // note: had to change FreeRTOSConfig.h to use this. See website for documentation
                                 ITEM_SIZE,
                                 ucQueueStorageArea,
                                 &xStaticQueue); */
    //println("Button assert???");                             
    configASSERT(buttonQueue); // why is this here? don't remember, but is it throwing the assert failed error? No. It still throws the error when this is commented out. 
    //println("Button thread created...");
    if(debounce){
        //println("Using Software Debounce");
        debounceCount = 5;
    }else{
        debounceCount = 1;
    }

    for (;;)
    {
        readButtons();
        verifyButtons();
        vTaskDelay(3);
    }
}

// Wrapper Functions to use in C...
// extern "C" int ButX_c_init(){
//     // do something
//     return 1;
// }

// extern "C" std::string ButX_c_getEvents(ButtonsX * b){
//     return b->getEvents();
// }


