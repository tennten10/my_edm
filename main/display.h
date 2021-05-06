#ifndef DISPLAY_H
#define DISPLAY_H
//#ifdef __cplusplus
//extern "C" {
//#endif
#include "lvgl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string>
#include "a_config.h"
#include "debug.h"



//void displayWeight(float weight);

//void displayWeight(const char)


//void pageEventCheck(long &t, int &q, int &q_last);
//void pageTestRoutine(long t);
//void displayTask(void * pvParameters);
//void menu();



//#ifdef __cplusplus
//}
//#endif





/*********************************************/
// reference this website for why we're doing it this way https://fjrg76.wordpress.com/2018/05/23/objectifying-task-creation-in-freertos-ii/

template <typename T>
class ThreadXa
{
public:
  ThreadXa( uint16_t _stackDepth, UBaseType_t _priority, const char *_name = "")
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
    ThreadXa *p = static_cast<ThreadXa*>(_params);
    p->Main();
  }

  TaskHandle_t taskHandle;
};

class DisplayX : public ThreadXa<DisplayX>
{
public:
  DisplayX() : ThreadXa{5000, 1, "DisplayHandler"}
    {
        //Main();
        //displaySetup();
    }
    ~DisplayX(){} 


    void displayWeight(char* weight);
    void displayUnits( Units u);
    void displaySettings();
    void displayDeviceInfo(char* SN, char* VER);  
    void displayUpdateScreen(int pct);
    void displayLogo();
    void displayBattery();
    void displayLowBattery();
    void displayOff();
    void displayOn();
    void displaySleepPrep();
    void Main();
    std::string getCurrentScreen();


private:
  
    void styleInit();
    void displayLoopConditions(long &t, int &q, int &q_last);
    void pageTestRoutine(long t);
    void pageEventCheck(long &t, int &q, int &q_last);

    // style variables
    lv_style_t weightStyle;
    lv_style_t unitStyle;
    lv_style_t logoStyle;
    lv_style_t backgroundStyle;
    lv_style_t infoStyle;
    lv_style_t transpCont;

    lv_disp_buf_t disp_buf;
    SemaphoreHandle_t xGuiSemaphore;
    static void lv_tick_task(void *arg);
    void resizeWeight(char *w);
    bool disp_flag = false;
    char currentWeight[32] = "0.0";
    bool updateStarted = 0;
};

#endif