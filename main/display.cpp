/* LVGL Example project
 *
 * Basic project to test LVGL on ESP32 based projects.
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */

#include "a_config.h"
#include "globals.h"
#include "display.h"
#include "System.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

/* Littlevgl specific */
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#include "lvgl_helpers.h"

/*********************
 *      DEFINES
 *********************/
#define LV_TICK_PERIOD_MS 1

extern SystemX *_sys;

ledc_channel_config_t b_ledc_c_config{
    .gpio_num = ledB,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_1,
    .duty = (uint32_t)10,
    .hpoint = 0,
};


esp_timer_handle_t periodic_timer;

/* Initialize image files for display
* These are stored as .c files and converted from https://lvgl.io/tools/imageconverter
*/
LV_IMG_DECLARE(img_battery);
LV_IMG_DECLARE(img_low_battery);
LV_IMG_DECLARE(img_logo_black);
LV_IMG_DECLARE(img_logo_white);

LV_FONT_DECLARE(vt323_70);
LV_FONT_DECLARE(vt323_80);
LV_FONT_DECLARE(vt323_90);
LV_FONT_DECLARE(vt323_100);
LV_FONT_DECLARE(vt323_120);
LV_FONT_DECLARE(vt323_130);
LV_FONT_DECLARE(vt323_140);
LV_FONT_DECLARE(montserrat_70);
LV_FONT_DECLARE(montserrat_90);
LV_FONT_DECLARE(montserrat_120);

//     /* If you want to use a task to create the graphic, you NEED to create a Pinned task
//       * Otherwise there can be problem such as memory corruption and so on.
//       * NOTE: When not using Wi-Fi nor Bluetooth you can pin the guiTask to core 0 */
//     xTaskCreatePinnedToCore(displayTask, "display", 4096 * 2, NULL, 0, &displayHandler_TH, 1);

//static
void DisplayX::Main()
{
    /* Creates a semaphore to handle concurrent call to lvgl stuff
    * If you wish to call *any* lvgl function from other threads/tasks
    * you should lock on the very same semaphore! */
    xGuiSemaphore = xSemaphoreCreateMutex();
    //print("starting display loop: ");
    ////println((int)esp_timer_get_time() / 1000);
    lv_init();

    /* Initialize SPI or I2C bus used by the drivers */
    lvgl_driver_init();

    lv_color_t *buf1 = reinterpret_cast<lv_color_t *>(heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA));
    assert(buf1 != NULL);

    /* Initialize backlight LED  */

    ledc_timer_config_t ledc_timer{
        .speed_mode = LEDC_LOW_SPEED_MODE,    // timer mode
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .timer_num = LEDC_TIMER_1,            // timer index
        .freq_hz = 3000,                      // frequency of PWM signal
        .clk_cfg = LEDC_AUTO_CLK,             // Auto select the source clock
    };

    ledc_timer_config(&ledc_timer);

    ledc_channel_config(&b_ledc_c_config);
    ledc_fade_func_install(0);

    //ledc_set_duty_and_update(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, intensity, 0);


    /* Use double buffered when not working with monochrome displays */
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    lv_color_t *buf2 = reinterpret_cast<lv_color_t *>(heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA));
    assert(buf2 != NULL);
#else
    static lv_color_t *buf2 = NULL;
#endif

    //static lv_disp_buf_t disp_buf;

    uint32_t size_in_px = DISP_BUF_SIZE;

    /* Initialize the working buffer depending on the selected display.
      * NOTE: buf2 == NULL when using monochrome displays. */
    lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;

    /* When using a monochrome display we need to register the callbacks:
      * - rounder_cb
      * - set_px_cb */
#ifdef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    disp_drv.rounder_cb = disp_driver_rounder;
    disp_drv.set_px_cb = disp_driver_set_px;
#endif

    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    /* Register an input device when enabled on the menuconfig */
#if CONFIG_LV_TOUCH_CONTROLLER != TOUCH_CONTROLLER_NONE
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.read_cb = touch_driver_read;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register(&indev_drv);
#endif

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    //init my style types
    styleInit();
    disp_flag = true;
    ////println("drawing starting screen");
    // draw my starting screen
    displayLogo();
    //long t = esp_timer_get_time() / 1000;
    while (!ready) //esp_timer_get_time() / 1000 - t < 500)
    {
        lv_task_handler();
        vTaskDelay(10);
    }
    vTaskDelay(100);

    // pageTestRoutine((long)(esp_timer_get_time() / 1000));
    //print("end of display init: ");
    //println((int)esp_timer_get_time() / 1000);
    while (1)
    {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        if (disp_flag && pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
        else
        {
            //println("Could not get xGuiSemaphore in main display loop");
        }
    }

    /* A task should NEVER return */
    free(buf1);
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    free(buf2);
#endif
    vTaskDelete(NULL);
}

void DisplayX::styleInit()
{
    lv_style_init(&weightStyle);
    // lv_style_init(&weightStyle_m);
    // lv_style_init(&weightStyle_l);
    lv_style_init(&unitStyle);
    lv_style_init(&logoStyle);
    lv_style_init(&backgroundStyle);
    lv_style_init(&infoStyle);
    lv_style_init(&transpCont);

    lv_style_set_text_font(&weightStyle, LV_STATE_DEFAULT, &vt323_140);
    lv_style_set_text_color(&weightStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);

    lv_style_set_text_font(&unitStyle, LV_STATE_DEFAULT, &vt323_140);
    lv_style_set_text_color(&unitStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);

    lv_style_set_bg_color(&logoStyle, LV_STATE_DEFAULT, LV_COLOR_BLACK);

    lv_style_set_bg_color(&backgroundStyle, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_border_opa(&backgroundStyle, LV_STATE_DEFAULT, 0);

    lv_style_set_text_color(&infoStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_text_font(&infoStyle, LV_STATE_DEFAULT, &montserrat_90);

    lv_style_set_bg_opa(&transpCont, LV_STATE_DEFAULT, 0);
    lv_style_set_border_opa(&transpCont, LV_STATE_DEFAULT, 0);

}

void DisplayX::pageTestRoutine(long t)
{
    int show = 2000; // 5 seconds
    bool flag1 = 1, flag2 = 1, flag3 = 1, flag4 = 1;
    bool flag5 = 1, flag6 = 1, flag7 = 1, flag8 = 1;
    bool flag9 = 1, flag10 = 1, flag11 = 1, flag12 = 1;
    bool flag13 = 1; //, flag14 = 1, flag15 = 1, flag16 = 1;
    //bool flag17 = 1, flag18 = 1, flag19 = 1, flag20 = 1;
    //bool flag21 = 1, flag22 = 1, flag23 = 1, flag24 = 1;
    //bool flag25 = 1, flag26 = 1, flag27 = 1, flag28 = 1;
    //bool flag29 = 1, flag30 = 1, flag31 = 1, flag32 = 1;

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }

        if (esp_timer_get_time() / 1000 - t < show && flag1)
        {
            //println("weight test 2 char");
            displayWeight((char *)"12"); // and for each character length
            flag1 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 2 * show && flag2)
        {
            //println("weight test 3 char");
            displayWeight((char *)"1.9");
            flag2 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 3 * show && flag3)
        {
            //println("weight test 4 char");
            displayWeight((char *)"300.2");
            flag3 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 4 * show && flag4)
        {
            //println("weight test 4+ char");
            displayWeight((char *)"800.15");
            flag4 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 5 * show && flag5)
        {
            //println("Units test g");
            //displayUnits(Units::g);
            flag5 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 6 * show && flag6)
        {
            //println("Units test kg");
            //displayUnits(Units::kg);
            flag6 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 7 * show && flag7)
        {
            //println("Units test oz");
            //displayUnits(Units::oz);
            flag7 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 8 * show && flag8)
        {
            //println("Units test lb");
            //displayUnits(Units::lb);
            flag8 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 9 * show && flag9)
        {
            //println("Info test\n");
            displayDeviceInfo("01100101", "1.0.2"); //
            flag9 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 10 * show && flag10)
        {
            //println("Battery Image Test");
            displayBattery(40); //
            flag10 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 11 * show && flag11)
        {
            //println("Low Battery Test");
            displayLowBattery(); //
            flag11 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 12 * show && flag12)
        {
            //println("Display Off");
            displayOff(); // ready
            flag12 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 13 * show && flag13)
        {
            //println("Display logo and resetting...");
            displayOn();
            displayLogo(); // ready
            flag13 = false;
        }
        else if (!flag13)
        {
            // displaySettings(); // Need to figure out what I want on this menu first

            // displayUpdateScreen(int pct); // add this later?
            //displayOff(); // ready
            return;
        }
    }
}

// this needs to run for the library to work
void DisplayX::lv_tick_task(void *arg) 
{
    (void)arg;

    lv_tick_inc(LV_TICK_PERIOD_MS);
}

void DisplayX::resizeWeight(char *w)
{
    static int lastLen = 0;

    if (strlen(w) > 7)
    {
        if (lastLen > 7)
        {
            lv_style_remove_prop(&weightStyle, LV_STYLE_TEXT_FONT);
            lv_style_set_text_font(&weightStyle, LV_STATE_DEFAULT, &vt323_120);
        }
    }
    else if (strlen(w) > 4)
    {
        if (lastLen > 4)
        {
            lv_style_remove_prop(&weightStyle, LV_STYLE_TEXT_FONT);
            lv_style_set_text_font(&weightStyle, LV_STATE_DEFAULT, &vt323_130);
        }
    }
    else if (strlen(w) > 0)
    {
        if (lastLen > 0)
        {
            lv_style_remove_prop(&weightStyle, LV_STYLE_TEXT_FONT);
            lv_style_set_text_font(&weightStyle, LV_STATE_DEFAULT, &vt323_140);
        }
    }
    else
    {
    }

    lastLen = strlen(w);
    ////println("end of resizeWeight");
}

// displayWeight should be called first. This only updates the number value
void DisplayX::updateWeight(std::string weight)
{
    if (weight.compare("-1") == 0)
    {
        // Value same as last value. No need to update.
        ////println("breaking out of updateWeight");
        return;
    }
    if (xSemaphoreTake(xGuiSemaphore, portMAX_DELAY) == pdTRUE)
    {
        static char now[16];
        if (weightLabel != NULL)
        {
            strcpy(now, weight.c_str());
            //println(now);
            resizeWeight(now);
            lv_label_set_text_fmt(weightLabel, "%s", now);
            //lv_obj_remove_style(weightLabel, LV_OBJ_PART_MAIN, &weightStyle);
            //lv_obj_add_style(weightLabel, LV_OBJ_PART_MAIN, &weightStyle);
            // lv_label_set_text(weightLabel, weight.c_str());
        }
        xSemaphoreGive(xGuiSemaphore);
    }
    else
    {
        //println("could not get xGuiSemaphore in updateWeight");
    }
}

// This one is for the first time
void DisplayX::displayWeight(std::string weight)
{
    if (weight.compare("-1") == 0)
    {
        // Value same as last value. No need to update. But this is the initializing function, so it shouldn't be here?
        //println("breaking out of displayWeight - probably shouldn't ever do this");
        return;
    }
    //println(weight);
    // print(" gets past catching -1 value in displayWeight: ");
    if (xSemaphoreTake(xGuiSemaphore, portMAX_DELAY) == pdTRUE)
    {
        //println("Got xGuiSemaphore in displayWeight");

        static char now[32];
        strcpy(now, weight.c_str());
        resizeWeight(now); // lv_obj_refresh_style(obj, part, property);?
        ////println(now);
        ////println(weight.c_str());

        //println("umm here 1");
        lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
        //println("umm here 1.1");
        lv_obj_t *cont = lv_cont_create(bkgrnd, NULL);
        //println("umm here 1.2");
        weightLabel = lv_label_create(cont, NULL);
        //println("umm here 2");
        lv_cont_set_fit(cont, LV_FIT_PARENT);
        lv_cont_set_layout(cont, LV_LAYOUT_CENTER);
        //println("umm here 3");


        lv_obj_set_width(bkgrnd, HORIZ);
        lv_obj_set_height(bkgrnd, VERT);
        lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

        lv_obj_align(cont, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
        lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
        lv_obj_add_style(cont, LV_OBJ_PART_MAIN, &transpCont);
        lv_obj_add_style(weightLabel, LV_OBJ_PART_MAIN, &weightStyle);

        xSemaphoreGive(xGuiSemaphore);
        //println("gave back xGuiSemaphore in displayWeight");
    }
    else
    {
        //println("could not get xGuiSemaphore in displayWeight");
    }
}

// Units
// void DisplayX::displayUnits(Units u)
// {
//     if (xSemaphoreTake(xGuiSemaphore, portMAX_DELAY) == pdTRUE)
//     {
//         //println("got xGuiSemaphore in displayUnits");

//         lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
//         lv_obj_t *cont = lv_cont_create(bkgrnd, NULL);
//         unitsLabel = lv_label_create(cont, NULL);

//         lv_cont_set_fit(cont, LV_FIT_PARENT);
//         lv_cont_set_layout(cont, LV_LAYOUT_CENTER);

//         lv_obj_set_width(bkgrnd, HORIZ);
//         lv_obj_set_height(bkgrnd, VERT);
//         lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);



//         lv_label_set_text(unitsLabel, unitsToString(u).c_str());



//         lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0, 0);


//         lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
//         lv_obj_add_style(cont, LV_OBJ_PART_MAIN, &transpCont);
//         lv_obj_add_style(unitsLabel, LV_OBJ_PART_MAIN, &unitStyle);
//         xSemaphoreGive(xGuiSemaphore);
//         //println("gave back xGuiSemaphore in displayUnits");
//     }
//     else
//     {
//         //println("could not get xGuiSemaphore in displayUnits");
//     }
// }

// void DisplayX::updateUnits(Units u)
// {
//     if (xSemaphoreTake(xGuiSemaphore, portMAX_DELAY) == pdTRUE)
//     {
//         //println("got xGuiSemaphore in updateUnits");
//         if (unitsLabel != NULL)
//         {
//             lv_label_set_text(unitsLabel, unitsToString(u).c_str());
//         }
//         else
//         {
//             //println("unitsLabel not initialized. run displayUnits before this.");
//         }

//         xSemaphoreGive(xGuiSemaphore);
//         //println("gave back xGuiSemaphore in updateUnits");
//     }
//     else
//     {
//         //println("could not get xGuiSemaphore in updateUnits");
//     }
// }

// Settings
void DisplayX::displaySettings()
{
    if (xSemaphoreTake(xGuiSemaphore, portMAX_DELAY) == pdTRUE)
    {
        //println("got xGuiSemaphore in displaySettings");
        // TODO: For settings menu, whenever that is implemented

        xSemaphoreGive(xGuiSemaphore);
        //println("gave back xGuiSemaphore in displaySettings");
    }
    else
    {
        //println("could not get xGuiSemaphore in displaySettings");
    }
}

// Battery & Info
void DisplayX::displayDeviceInfo(std::string SN, std::string VER)
{
    if (xSemaphoreTake(xGuiSemaphore, portMAX_DELAY) == pdTRUE)
    {
        //println("got xGuiSemaphore in displayDeviceInfo");

        // create objects to display info
        lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
        lv_obj_t *label3 = lv_label_create(bkgrnd, NULL);
        //println("after setting style values");

        // TODO: Just copy and pasted from FULL version. Adjust in future.
        lv_obj_set_width(bkgrnd, HORIZ);
        lv_obj_set_height(bkgrnd, VERT);

        lv_label_set_text_fmt(label3, "SUDOBOARD INFO\nSN: %s\n VER: %s", SN, VER); // , 5, 10); //"sn", "ver");
        //println("after setting label text values");
        lv_label_set_align(label3, LV_ALIGN_CENTER);
        lv_obj_align(label3, NULL, LV_ALIGN_CENTER, 0, 0);
        //println("After setting alignment");


        lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
        lv_obj_add_style(label3, LV_OBJ_PART_MAIN, &infoStyle);

        xSemaphoreGive(xGuiSemaphore);
        //println("gave back xGuiSemaphore in displayDeviceInfo");
    }
    else
    {
        //println("can't get xGuiSemaphore in displayDeviceInfo");
    }
}

// Update
void DisplayX::displayUpdateScreen()
{
    if (xSemaphoreTake(xGuiSemaphore, portMAX_DELAY) == pdTRUE)
    {
        //println("got xGuiSemaphore in displayUpdateScreen");

        // create objects to display info
        lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
        updateLabel = lv_label_create(bkgrnd, NULL);
        //println("after setting style values");

        lv_obj_set_width(bkgrnd, HORIZ);
        lv_obj_set_height(bkgrnd, VERT);


        lv_label_set_text(updateLabel, "Updating..."); // , 5, 10); //"sn", "ver");
        //println("after setting label text values");
        lv_label_set_align(updateLabel, LV_ALIGN_CENTER);
        lv_obj_align(updateLabel, NULL, LV_ALIGN_CENTER, 0, 0);
        //println("After setting alignment");


        lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
        lv_obj_add_style(updateLabel, LV_OBJ_PART_MAIN, &infoStyle);

        xSemaphoreGive(xGuiSemaphore);
        //println("gave back xGuiSemaphore in displayUpdateScreen");
    }
    else
    {
        //println("can't get xGuiSemaphore in displayUpdateScreen");
    }
}

void DisplayX::setUpdateScreen(int pct){
    lv_label_set_text(updateLabel, "f");
}

void DisplayX::displayLogo()
{
    if (xSemaphoreTake(xGuiSemaphore, portMAX_DELAY) == pdTRUE)
    {
        //println("got xGuiSemaphore in displayLogo");
        lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
        lv_obj_t *img = lv_img_create(bkgrnd, NULL);

        lv_img_set_src(img, &img_logo_white);
        lv_obj_set_width(bkgrnd, HORIZ);
        lv_obj_set_height(bkgrnd, VERT);
        
        lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
        lv_img_set_zoom(img, 70);
        lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, 0);

        lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);

        xSemaphoreGive(xGuiSemaphore);
        //println("gave back xGuiSemaphore in displayLogo");
    }
    else
    {
        //println("can't get xGuiSemaphore in displayLogo");
    }
}

void DisplayX::displayBattery(int bat)
{
    if (xSemaphoreTake(xGuiSemaphore, portMAX_DELAY) == pdTRUE)
    {
        //println("got xGuiSemaphore in displayBattery");

        lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
        lv_obj_t *img = lv_img_create(bkgrnd, NULL);
        lv_obj_t *cont = lv_cont_create(img, NULL);
        lv_obj_t *label = lv_label_create(cont, NULL);

        lv_img_set_src(img, &img_battery);
        lv_obj_set_width(bkgrnd, HORIZ);
        lv_obj_set_height(bkgrnd, VERT);
        lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);



        lv_img_set_zoom(img, 100);
        lv_obj_align(img, NULL, LV_ALIGN_CENTER, 10, 0);

        lv_cont_set_fit(cont, LV_FIT_PARENT);
        lv_cont_set_layout(cont, LV_LAYOUT_CENTER);

        lv_label_set_text_fmt(label, "%d%%", bat);

        lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0, 0);


        lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
        lv_obj_add_style(cont, LV_CONT_PART_MAIN, &transpCont);
        lv_obj_add_style(label, LV_CONT_PART_MAIN, &infoStyle);
        xSemaphoreGive(xGuiSemaphore);
        //println("gave back xGuiSemaphore in displayBattery");
    }
    else
    {
        //println("Can't get xGuiSemaphore in displayBattery");
    }
}

void DisplayX::displayLowBattery()
{
    if (xSemaphoreTake(xGuiSemaphore, portMAX_DELAY) == pdTRUE)
    {
        //println("got xGuiSemaphore in displayLowBattery");

        lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
        lv_obj_t *img = lv_img_create(bkgrnd, NULL);
        lv_obj_t *cont = lv_cont_create(img, NULL);
        lv_obj_t *label = lv_label_create(cont, NULL);

        lv_img_set_src(img, &img_low_battery);
        lv_obj_set_width(bkgrnd, HORIZ);
        lv_obj_set_height(bkgrnd, VERT);
        lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);


        lv_img_set_zoom(img, 100);
        lv_obj_align(img, NULL, LV_ALIGN_CENTER, 10, 0);

        lv_cont_set_fit(cont, LV_FIT_PARENT);
        lv_cont_set_layout(cont, LV_LAYOUT_CENTER);

        lv_label_set_text(label, "LOW");
        lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0, 0);


        lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
        lv_obj_add_style(cont, LV_CONT_PART_MAIN, &transpCont);
        lv_obj_add_style(label, LV_CONT_PART_MAIN, &infoStyle);
        xSemaphoreGive(xGuiSemaphore);
        //println("gave back xGuiSemaphore in displayLowBattery");
    }
    else
    {
        //println("Can't get xGuiSemaphore in displayLowBattery");
    }
}

void DisplayX::displayOff()
{
    if (xSemaphoreTake(xGuiSemaphore, portMAX_DELAY) == pdTRUE)
    {
        //println("got xGuiSemaphore in displayoff");
        lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
        lv_obj_set_width(bkgrnd, HORIZ);
        lv_obj_set_height(bkgrnd, VERT);
        lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
        lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
        //lv_obj_clean(lv_scr_act());
        xSemaphoreGive(xGuiSemaphore);
        //println("gave back xGuiSemaphore in displayOff");
        disp_flag = false;
        //setIntensity(0);

        ledc_set_duty_and_update(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, 0, 0);

    }
    else
    {
        //println("Can't get xGuiSemaphore in displayOff");
    }
}

void DisplayX::displayOn()
{
    disp_flag = true;
    setIntensity(intensity);
}

void DisplayX::displaySleepPrep()
{

    displayOff();
    // save color and intensity to nvs - this is done in separate function
}

void DisplayX::setColor(int r, int g, int b)
{
// This pretty much is only for the projector version of the display


    if (r > 255)
    {
        r = 255;
    }
    else if (r < 0)
    {
        r = 0;
    }
    if (r > 255)
    {
        g = 255;
    }
    else if (r < 0)
    {
        g = 0;
    }
    if (r > 255)
    {
        b = 255;
    }
    else if (r < 0)
    {
        b = 0;
    }

    red = r * intensity / 255;
    green = g * intensity / 255;
    blue = b * intensity / 255;

    ledc_set_duty_and_update(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, blue, 0);
}

void DisplayX::setIntensity(int i)
{
    if (i > 8192)
    {
        intensity = 8192;
    }
    else if (i < 0)
    {
        intensity = 0;
    }
    else
    {
        intensity = i;
    }

    blue = blue * intensity / 255;

    ledc_set_duty_and_update(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, blue, 0);

}

// Not using this since deleting the objects prevents proper initialization when it turns back on
// Maybe I'll be able to figure it out at some point but it works for now
void DisplayX::onDelete()
{

    ledc_stop(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, 0);


    ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1); //ledc_timer
    esp_timer_delete(periodic_timer);
    //println("finished display onDelete");
}
