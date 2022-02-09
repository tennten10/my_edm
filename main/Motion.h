#ifndef MOTION_H
#define MOTION_H


//#include "a_config.h"
#include "globals.h"
#include <string>

#include "driver/gpio.h"


struct motion_config
{
    long bTimer = 0;
};


/*********************************************/
// reference this website for why we're doing it this way https://fjrg76.wordpress.com/2018/05/23/objectifying-task-creation-in-freertos-ii/

template <typename T>
class ThreadXc
{
public:
  ThreadXc( uint16_t _stackDepth, UBaseType_t _priority, const char *_name = "")
  {
    xTaskCreate(task, _name, _stackDepth, this, _priority, &this->taskHandle);
  }
  virtual ~ThreadXc(){
    vTaskDelete(taskHandle);
    printf("Motion task deleted");
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
    ThreadXc *p = static_cast<ThreadXc*>(_params);
    p->Main();
  }

  TaskHandle_t taskHandle;
};

class MotionX : public ThreadXc<MotionX>
{
public:
  MotionX(bool _debounce) : ThreadXc{5000, 1, "MotionHandler"},
                                                debounce{_debounce}
  {
    //Main();
  }
  virtual ~MotionX(){
    vQueueDelete(motionQueue);
  } 
  
  void sleepPreparation();
  void Main();


private:
  bool debounce = false;
  QueueHandle_t motionQueue;
};

//extern "C" std::string ButX_c_getEvents(ButtonsX * b);
#endif