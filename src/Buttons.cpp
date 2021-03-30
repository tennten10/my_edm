#include "Buttons.h"
#include "PinDefs.h"
#include "debug.h"

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string>

#define GPIO_INPUT_PIN_SEL ((1ULL<<but1) | (1ULL<<but2) | (1ULL<<but3) | (1ULL<<but4))

//bool ButtonsX::debounce = false;
//uint8_t ButtonsX::longPressTime = (uint8_t) 1500;
//QueueHandle_t ButtonsX::buttonQueue;
extern TickType_t xBlockTime;
uint8_t debounceCount = 10;


#define QUEUE_LENGTH    20
#define ITEM_SIZE       sizeof( char )

/* The variable used to hold the queue's data structure. */
static StaticQueue_t xStaticQueue;

/* The array to use as the queue's storage area.  This must be at least
uxQueueLength * uxItemSize bytes. */
uint8_t ucQueueStorageArea[ QUEUE_LENGTH * ITEM_SIZE ];

void    ButtonsX::readButtons(bool _debounce)
{

    
    // read all 4 buttons?
    // look for state change
    // record the time
    // do stuff based on that info
        //static bool but1_state = button1.buttonStatus;
        //static bool but2_state = button2.buttonStatus;
        //static bool but3_state = button3.buttonStatus;
        //static bool but4_state = button4.buttonStatus;
        
        
        //NOTE: button logic is flipped. since pulled up, LOW is active, etc.
        bool read1 = gpio_get_level(but1); 
        bool read2 = gpio_get_level(but2); 
        bool read3 = gpio_get_level(but3); 
        bool read4 = gpio_get_level(but4); 
        read1 = !read1;
        read2 = !read2;
        read3 = !read3;
        read4 = !read4;
        

    if(_debounce){
    /*
    * Debounce the switches and record change time
    */


        static long nTime = esp_timer_get_time()/1000;

        if ((read1 == button1.buttonStatus) && (button1.debounceCounter > 0))
        {
            button1.debounceCounter--;
        }
        if (read1 != button1.buttonStatus)
        {
            button1.debounceCounter++;
        }
        if (button1.debounceCounter >= debounceCount)
        {
            button1.debounceCounter = 0;
            button1.buttonStatus = !button1.buttonStatus;
            button1.buttonTimer = nTime;
        }

        if ((read2 == button2.buttonStatus) && (button2.debounceCounter > 0))
        {
            button2.debounceCounter--;
        }
        if (read2 != button2.buttonStatus)
        {
            button2.debounceCounter++;
        }
        if (button2.debounceCounter >= debounceCount)
        {
            button2.debounceCounter = 0;
            button2.buttonStatus = !button2.buttonStatus;
            button2.buttonTimer = nTime;
        }

        if ((read3 == button3.buttonStatus) && (button3.debounceCounter > 0))
        {
            button3.debounceCounter--;
        }
        if (read3 != button3.buttonStatus)
        {
            button3.debounceCounter++;
        }
        if (button3.debounceCounter >= debounceCount)
        {
            button3.debounceCounter = 0;
            button3.buttonStatus = !button3.buttonStatus;
            button3.buttonTimer = nTime;
        }

        if ((read4 == button4.buttonStatus) && (button4.debounceCounter > 0))
        {
            button4.debounceCounter--;
        }
        if (read4 != button4.buttonStatus)
        {
            button4.debounceCounter++;
        }
        if (button4.debounceCounter >= debounceCount)
        {
            button4.debounceCounter = 0;
            button4.buttonStatus = !button4.buttonStatus;
            button4.buttonTimer = nTime;
        }
    } else{
        button1.buttonStatus = read1;
        button2.buttonStatus = read2;
        button3.buttonStatus = read3;
        button4.buttonStatus = read4;
    }

  /* 
   * Determine long/short press
   */
    static long longTime = esp_timer_get_time()/1000;
    if (button1.buttonStatus && !button1.block)
    {
        if (longTime - button1.buttonTimer > longPressTime)
        {
            if (button1.longPress == false)
            {
                button1.longPress = true;
                button1.shortPress = false;
                debugPrintln("button 1 long press trig");
            }
            else
            {
                // long press = true. do nothing?
                button1.executeFlag = true;
                button1.block = true;
                debugPrintln("button 1 long press");
            }
        }
        else
        {
            button1.shortPress = true;
            //debugPrintln("button 1 short");
        }
    }
    else if (button1.buttonStatus == false)
    { // When button not pressed
        if (button1.shortPress)
        {
            button1.executeFlag = true;
            //debugPrintln("button 1 shorty execute");
        }
        if (button1.block)
        {
            button1.block = false;
        }
    }

    if (button2.buttonStatus && !button2.block)
    {
        if (longTime - button2.buttonTimer > longPressTime)
        {
            if (button2.longPress == false)
            {
                button2.longPress = true;
                button2.shortPress = false;
                //debugPrintln("button 2 long press trig");
            }
            else
            {
                // long press = true. do nothing?
                button2.executeFlag = true;
                button2.block = true;
                //debugPrintln("button 2 long press");
            }
        }
        else
        {
            button2.shortPress = true;
            //debugPrintln("button 2 short");
        }
    }
    else if (button2.buttonStatus == false)
    { // When button not pressed
        if (button2.shortPress)
        {
            button2.executeFlag = true;
            //debugPrintln("button 2 shorty execute");
        }
        if (button2.block)
        {
            button2.block = false;
        }
    }

    if (button3.buttonStatus && !button3.block)
    {
        if (longTime - button3.buttonTimer > longPressTime)
        {
            if (button3.longPress == false)
            {
                button3.longPress = true;
                button3.shortPress = false;
            }
            else
            {
                // long press = true. do nothing?
                button3.executeFlag = true;
                button3.block = true;
            }
        }
        else
        {
            button3.shortPress = true;
            //debugPrintln("button 3 short");
        }
    }
    else if (button3.buttonStatus == false)
    { // When button not pressed
        if (button3.shortPress)
        {
            button3.executeFlag = true;
            //debugPrintln("button 3 shorty execute");
        }
        if (button3.block)
        {
            button3.block = false;
        }
    }

    if (button4.buttonStatus && !button4.block)
    {
        if (longTime - button4.buttonTimer > longPressTime)
        {
            if (button4.longPress == false)
            {
                button4.longPress = true;
                button4.shortPress = false;
                //debugPrintln("button 4 long press trig");
            }
            else
            {
                // long press = true. do nothing?
                button4.executeFlag = true;
                button4.block = true;
                //debugPrintln("button 4 long press");
            }
        }
        else
        {
            button4.shortPress = true;
            //debugPrintln("button 4 short");
        }
    }
    else if (button4.buttonStatus == false)
    { // When button not pressed
        if (button4.shortPress)
        {
            button4.executeFlag = true;
            //debugPrintln("button 4 shorty execute");
        }
        if (button4.block)
        {
            button4.block = false;
        }
    }

    /* 
   *  
   */
}

void ButtonsX::verifyButtons()
{
    if (button1.executeFlag || button2.executeFlag || button3.executeFlag || button4.executeFlag)
    {
        debugPrintln("Building execute strings");
        if (button1.longPress)
        {
            button1.cmd = 'L';
            debugPrintln("B1 long");
            button1.longPress = false;
            button1.executeFlag = false;
        }
        else if (button1.shortPress)
        {
            button1.cmd = 'S';
            debugPrintln("B1 short");
            button1.shortPress = false;
            button1.executeFlag = false;
        }
        if (button2.longPress)
        {
            button2.cmd = 'L';
            debugPrintln("B2 long");
            button2.longPress = false;
            button2.executeFlag = false;
        }
        else if (button2.shortPress)
        {
            button2.cmd = 'S';
            debugPrintln("B2 short");
            button2.shortPress = false;
            button2.executeFlag = false;
        }
        if (button3.longPress)
        {
            button3.cmd = 'L';
            debugPrintln("B3 long");
            button3.longPress = false;
            button3.executeFlag = false;
        }
        else if (button3.shortPress)
        {
            button3.cmd = 'S';
            debugPrintln("B3 short");
            button3.shortPress = false;
            button3.executeFlag = false;
        }
        if (button4.longPress)
        {
            //
            button4.cmd = 'L';
            debugPrintln("B4 long");
            button4.longPress = false;
            button4.executeFlag = false;
        }
        else if (button4.shortPress)
        {
            //
            button4.cmd = 'S';
            debugPrintln("B4 short");
            button4.shortPress = false;
            button4.executeFlag = false;
        }
        char doo[] = {button1.cmd, button2.cmd, button3.cmd, button4.cmd, '\0'};
        
        xQueueSend(buttonQueue, &doo, xBlockTime);
        debugPrintln(doo);
        debugPrintln("Buttons added to queue");
        button1.cmd = 'N';
        button2.cmd = 'N';
        button3.cmd = 'N';
        button4.cmd = 'N';
    }
}

std::string ButtonsX::getEvents(){
    char temp[5]={};
    std::string t ="";
    if(uxQueueMessagesWaiting(buttonQueue)){
        vTaskDelay(5);
        xQueueReceive(buttonQueue, &temp, (TickType_t)20);
        debugPrintln("Getting buttons from queue");
        debugPrintln(temp);
        t = std::string(temp);
        debugPrintln(t);
        return t;
    }
    return t;
}

void ButtonsX::sleepPreparation() 
{
    //vDeleteTask(buttonHandler_TH){}
    debugPrintln("button inside sleep prep");
}

void ButtonsX::Main() // loop
{
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = (gpio_pullup_t) 1;
    gpio_config(&io_conf);
    buttonQueue = xQueueCreateStatic(QUEUE_LENGTH,  // note: had to change FreeRTOSConfig.h to use this. See website for documentation
                                 ITEM_SIZE,
                                 ucQueueStorageArea,
                                 &xStaticQueue);
    configASSERT(buttonQueue);
    debugPrintln("Button thread created...");
    if(debounce){
        debugPrintln("Using Software Debounce");
    }
    for (;;)
    {
        readButtons(debounce);
        verifyButtons();
        vTaskDelay(10);
    }
}

// Wrapper Functions to use in C...
extern "C" int ButX_c_init(){
    // do something
    return 1;
}

extern "C" std::string ButX_c_getEvents(ButtonsX * b){
    return b->getEvents();
}


