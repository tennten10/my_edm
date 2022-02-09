#ifndef DISPLAY_H
#define DISPLAY_H
//#ifdef __cplusplus
//extern "C" {
//#endif
#include "lvgl.h"
#include "globals.h"
#include <string>
#include "a_config.h"

// All functions should be thread safe and have semaphore handling
// inside each public function since pretty much everything will be
// called from other threads. Right now only the lvhandler is called
// from inside the thread
/*********************************************/
// reference this website for why we're doing it this way https://fjrg76.wordpress.com/2018/05/23/objectifying-task-creation-in-freertos-ii/

template <typename T>
class ThreadXa
{
public:
    ThreadXa(uint16_t _stackDepth, UBaseType_t _priority, const char *_name = "")
    {
        xTaskCreatePinnedToCore(task, _name, _stackDepth, this, _priority, &this->taskHandle, 1);
        //(displayTask, "display", 4096 * 2, NULL, 0, &displayHandler_TH, 1)
    }
    virtual ~ThreadXa()
    {
        printf("ThreadXa destructor");
        vTaskDelete(taskHandle);
    }

    TaskHandle_t GetHandle()
    {
        return this->taskHandle;
    }
    void Main()
    {
        static_cast<T &>(*this).Main();
    }

private:
    static void task(void *_params)
    {
        ThreadXa *p = static_cast<ThreadXa *>(_params);
        p->Main();
    }

    TaskHandle_t taskHandle;
};

class DisplayX : public ThreadXa<DisplayX>
{
public:
    DisplayX() : ThreadXa{5000, 1, "DisplayHandler"}
    {
    }
    virtual ~DisplayX()
    {
        printf("display destructor");
        onDelete();
        printf("display destructor end");
    }

    void displayWeight(std::string weight);
    void updateWeight(std::string weight);
    // void displayUnits(Units u);
    // void updateUnits(Units u);
    void displaySettings();
    void displayDeviceInfo(std::string SN, std::string VER);
    void displayUpdateScreen();
    void setUpdateScreen(int pct);
    void displayLogo();
    void displayBattery(int bat);
    void displayLowBattery();
    void displayOff();
    void displayOn();
    void displaySleepPrep();
    void Main();
    std::string getCurrentScreen();

    // light functions
    void setColor(int r, int g, int b);
    void setIntensity(int i);
    int getRed()
    {
        return red;
    }
    int getGreen()
    {
        return green;
    }
    int getBlue()
    {
        return blue;
    }
    int getIntensity()
    {
        return intensity;
    }

    bool ready = false;

private:
    void styleInit();
    void displayLoopConditions(long &t, int &q, int &q_last);
    void pageTestRoutine(long t);

    void onDelete();

    // style variables
    lv_style_t weightStyle;
    // lv_style_t weightStyle_m;
    // lv_style_t weightStyle_l;
    lv_style_t unitStyle;
    lv_style_t logoStyle;
    lv_style_t backgroundStyle;
    lv_style_t infoStyle;
    lv_style_t transpCont; // transparent container?

    lv_disp_buf_t disp_buf;
    SemaphoreHandle_t xGuiSemaphore; // use this when ...?
    static void lv_tick_task(void *arg);
    void resizeWeight(char *w); // Not currently use? Already truncated as weight, but text size?
    bool disp_flag = false;     // this signals whether the display is on or off

    char currentWeight[32] = "0.0";

    int red = 255;
    int green = 255;
    int blue = 255;
    int intensity = 8192; // max 8192
    lv_obj_t *weightLabel = NULL;
    lv_obj_t *unitsLabel = NULL;
    lv_obj_t *updateLabel = NULL;
};

#endif