#ifndef BUTTONS_H
#define BUTTONS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "a_config.h"
#include "debug.h"
#include "globals.h"
#include <string>

#include "driver/gpio.h"


struct Button
{
    long buttonTimer = 0;
    int debounceCounter = 0;
    bool buttonStatus = false;
    bool shortPress = false;
    bool longPress = false;
    bool executeFlag = false;
    bool block = false;
    volatile char cmd = 'N';
};


/*********************************************/
// reference this website for why we're doing it this way https://fjrg76.wordpress.com/2018/05/23/objectifying-task-creation-in-freertos-ii/

template <typename T>
class ThreadX
{
public:
  ThreadX( uint16_t _stackDepth, UBaseType_t _priority, const char *_name = "")
  {
    xTaskCreate(task, _name, _stackDepth, this, _priority, &this->taskHandle);
  }

  TaskHandle_t GetHandle()
  {
    return this->taskHandle;
  }
  void Main()
  {
    static_cast<T&>(*this).Main();
  }

private:
  static void task(void *_params)
  {
    ThreadX *p = static_cast<ThreadX*>(_params);
    p->Main();
  }

  TaskHandle_t taskHandle;
};

class ButtonsX : public ThreadX<ButtonsX>
{
public:
  ButtonsX(bool _debounce) : ThreadX{5000, 1, "ButtonHandler"},
                                                debounce{_debounce}
  {
    //Main();
  }
  ~ButtonsX(){} 
  
  void sleepPreparation();
  std::string getEvents();
  void Main();


private:
  bool debounce = false;
  int debounceCount = 1;
  Button button1;
  Button button2;
  Button button3;
  Button button4;
  const int longPressTime = 2000;
  void readButtons();
  void verifyButtons();
  QueueHandle_t buttonQueue;
};

//extern "C" std::string ButX_c_getEvents(ButtonsX * b);
#endif