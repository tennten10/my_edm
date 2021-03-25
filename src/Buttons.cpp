#include "Buttons.h"
#include "PinDefs.h"
#include "debug.h"

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string>

//uncomment next line if buttons don't have RC filter or have debounce issues
//#define SOFTWAREDEBOUNCE


bool debounce = false;
//Button button1;
//Button button2;
//Button button3;
//Button button4;
uint8_t longPressTime = (uint8_t) 1500;
QueueHandle_t buttonQueue;
extern TickType_t xBlockTime;
uint8_t debounceCount = 10;

// defined in header now
// struct Button
// {
//     long buttonTimer = 0;
//     int debounceCounter = 0;
//     bool buttonStatus = false;
//     bool shortPress = false;
//     bool longPress = false;
//     bool executeFlag = false;
//     bool block = false;
//     volatile char cmd = 'N';
// };

void    ButtonsX::readButtons(bool _debounce)
{

    if(_debounce){
    // read all 4 buttons?
    // look for state change
    // record the time
    // do stuff based on that info
        //static bool but1_state = button1.buttonStatus;
        //static bool but2_state = button2.buttonStatus;
        //static bool but3_state = button3.buttonStatus;
        //static bool but4_state = button4.buttonStatus;
        
        
        //NOTE: button logic is flipped. since pulled up, LOW is active, etc.
        static bool read1 = !gpio_get_level(but1); //digitalRead(but1);
        static bool read2 = !gpio_get_level(but2); //digitalRead(but2);
        static bool read3 = !gpio_get_level(but3); //digitalRead(but3);
        static bool read4 = !gpio_get_level(but4); //digitalRead(but4);
        
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
                //debugPrintln("button 1 long press trig");
            }
            else
            {
                // long press = true. do nothing?
                button1.executeFlag = true;
                button1.block = true;
                //debugPrintln("button 1 long press");
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
            //
            button2.cmd = 'L';
            debugPrintln("B2 long");
            button2.longPress = false;
            button2.executeFlag = false;
        }
        else if (button2.shortPress)
        {
            //
            button2.cmd = 'S';
            debugPrintln("B2 short");
            button2.shortPress = false;
            button2.executeFlag = false;
        }
        if (button3.longPress)
        {
            //
            button3.cmd = 'L';
            debugPrintln("B3 long");
            button3.longPress = false;
            button3.executeFlag = false;
        }
        else if (button3.shortPress)
        {
            //
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
        char doo[] = {button1.cmd, button2.cmd, button3.cmd, button4.cmd};
        //queue.push(doo);
        xQueueSend(buttonQueue, &doo, xBlockTime);
        debugPrintln(doo);
        button1.cmd = 'N';
        button2.cmd = 'N';
        button3.cmd = 'N';
        button4.cmd = 'N';
    }
}

std::string ButtonsX::getEvents(){
    std::string t ="";
    if(uxQueueMessagesWaiting(buttonQueue)){
        xQueueReceive(buttonQueue, &t, (TickType_t)20);
        return t;
    }
    return t;
}

void ButtonsX::sleepPreparation() 
{
    //vDeleteTask(buttonHandler_TH){}
}

void ButtonsX::Main()
{
    for (;;)
    {
        readButtons(debounce);
        verifyButtons();
        vTaskDelay(10);
    }
}
void ButtonsX::setup()
{
    //
    //gpio_config(constgpio_config_t *pGPIOConfig);
    //gpio_get_level(pin);

    gpio_pullup_en(but1);
    gpio_set_direction(but1, GPIO_MODE_INPUT);
    gpio_pullup_en(but2);
    gpio_set_direction(but2, GPIO_MODE_INPUT);
    gpio_pullup_en(but3);
    gpio_set_direction(but3, GPIO_MODE_INPUT);
    gpio_pullup_en(but4);
    gpio_set_direction(but4, GPIO_MODE_INPUT);
    
    //pinMode(but1, INPUT_PULLUP);
    //pinMode(but2, INPUT_PULLUP);
    //pinMode(but3, INPUT_PULLUP);
    //pinMode(but4, INPUT_PULLUP);
    // TODO: Find a way for the task function to be compatible with the class 
    // xTaskCreate(
    //     buttonHandler_,   /* Task function. */
    //     "Button Handler", /* String with name of task. */
    //     10000,            /* Stack size in words, not bytes. */
    //     NULL,             /* Parameter passed as input of the task */
    //     0,                /* Priority of the task. */
    //     &buttonHandler_TH /* Task handle. */
    // );
    // debugPrintln("Button thread created...");

    buttonQueue = xQueueCreate(20, sizeof(uint32_t));
#ifdef SOFTWAREDEBOUNCE
    debugPrintln("Using Software Debounce");
#endif
}

