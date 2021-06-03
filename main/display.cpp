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
#include "debug.h"
#include "BLE.h"
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

#include "display.h"

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
/**********************
 *  STATIC PROTOTYPES
 **********************/
//static 
//void lv_tick_task(void *arg);
//static 
//void displayTask(void *pvParameter);

//void displayLoopConditions(long &t, int &q, int &q_last);

/**********************
 *  GLOBAL VARIABLES
 **********************/

extern TickType_t xBlockTime;


ledc_channel_config_t b_ledc_c_config{
    .gpio_num = ledB,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_1,
    .duty = (uint32_t)10,
    .hpoint = 0,
};
ledc_channel_config_t g_ledc_c_config{
    .gpio_num = ledG,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_1,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_1,
    .duty = (uint32_t)10,
    .hpoint = 0,
};
ledc_channel_config_t r_ledc_c_config{
    .gpio_num = ledR,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_2,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_1,
    .duty = (uint32_t)10,
    .hpoint = 0,
};

/* Initialize image files for display
* These are stored as .c files and converted from https://lvgl.io/tools/imageconverter
*/
LV_IMG_DECLARE(img_battery);
LV_IMG_DECLARE(img_low_battery);
LV_IMG_DECLARE(img_logo_black);
LV_IMG_DECLARE(img_logo_white);


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
        .freq_hz = 5000,                      // frequency of PWM signal
        .clk_cfg = LEDC_AUTO_CLK,             // Auto select the source clock
    };

    ledc_timer_config(&ledc_timer);
    

#ifdef CONFIG_SB_V1_HALF_ILI9341
  ledc_channel_config(&b_ledc_c_config);
  ledc_set_duty_and_update(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, 100, 0);
#endif
#ifdef CONFIG_SB_V3_ST7735S
  ledc_channel_config(&r_ledc_c_config);
  ledc_channel_config(&g_ledc_c_config);
  ledc_channel_config(&b_ledc_c_config);
  ledc_set_duty_and_update(r_ledc_c_config.speed_mode, r_ledc_c_config.channel, 100, 0);
  ledc_set_duty_and_update(g_ledc_c_config.speed_mode, g_ledc_c_config.channel, 100, 0);
  ledc_set_duty_and_update(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, 100, 0);
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
  ledc_channel_config(&b_ledc_c_config);
  ledc_set_duty_and_update(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, 100, 0);
#endif


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

    // draw my starting screen
    long t = esp_timer_get_time() / 1000;
    
    displayLogo();
    while (esp_timer_get_time() / 1000 - t < 1000)
    {
        lv_task_handler();
        //xSemaphoreGive(xGuiSemaphore);
        vTaskDelay(10);
    }

    //pageTestRoutine((long)(esp_timer_get_time() / 1000));

    while (1)
    {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        if (disp_flag && pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
          //displayLoopConditions(t, q, q_last);
          lv_task_handler();
          xSemaphoreGive(xGuiSemaphore);
        }
    }

    /* A task should NEVER return */
    free(buf1);
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    free(buf2);
#endif
    vTaskDelete(NULL);
}

// void DisplayX::displayLoopConditions(long &t, int &q, int &q_last)
// {
//     // Look at if the screen should change from button interactions
//     eventCheck(t, q, q_last);
// }

void DisplayX::styleInit()
{
    lv_style_init(&weightStyle);
    lv_style_init(&unitStyle);
    lv_style_init(&logoStyle);
    lv_style_init(&backgroundStyle);
    lv_style_init(&infoStyle);
    lv_style_init(&transpCont);

#ifdef CONFIG_SB_V1_HALF_ILI9341
    lv_style_
        lv_obj_align(label1, NULL, LV_ALIGN_CENTER, 0, 0);

    lv_style_set_text_font(&style, LV_STATE_DEFAULT, &lv_font_montserrat_14);
    lv_style_set_text_color(&style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_bg_color(&style, LV_STATE_DEFAULT, LV_COLOR_BLACK);

#endif
#ifdef CONFIG_SB_V3_ST7735S
    //lv_style_set_bg_color(&weightStyle, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_text_font(&weightStyle, LV_STATE_DEFAULT, &lv_font_montserrat_40);
    lv_style_set_text_color(&weightStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    //lv_style_set_bg_opa(&weightStyle, LV_STATE_DEFAULT, 100);

    lv_style_set_text_font(&unitStyle, LV_STATE_DEFAULT, &lv_font_montserrat_40);
    lv_style_set_text_color(&unitStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    //lv_style_set_bg_color(&unitStyle, LV_STATE_DEFAULT, LV_COLOR_BLACK);

    //lv_style_set_text_font(&logoStyle, LV_STATE_DEFAULT, &lv_font_montserrat_14);
    //lv_style_set_text_color(&logoStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_bg_color(&logoStyle, LV_STATE_DEFAULT, LV_COLOR_BLACK);

    lv_style_set_bg_color(&backgroundStyle, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_border_opa(&backgroundStyle, LV_STATE_DEFAULT, 0);

    lv_style_set_text_color(&infoStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_text_font(&unitStyle, LV_STATE_DEFAULT, &lv_font_montserrat_30);

    lv_style_set_bg_opa(&transpCont, LV_STATE_DEFAULT, 0);
    lv_style_set_border_opa(&transpCont, LV_STATE_DEFAULT, 0);
  
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
// TODO: just copy and pasted for now... need to change things up
    //lv_style_set_bg_color(&weightStyle, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_text_font(&weightStyle, LV_STATE_DEFAULT, &lv_font_montserrat_40);
    lv_style_set_text_color(&weightStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    //lv_style_set_bg_opa(&weightStyle, LV_STATE_DEFAULT, 100);

    lv_style_set_text_font(&unitStyle, LV_STATE_DEFAULT, &lv_font_montserrat_40);
    lv_style_set_text_color(&unitStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    //lv_style_set_bg_color(&unitStyle, LV_STATE_DEFAULT, LV_COLOR_BLACK);

    //lv_style_set_text_font(&logoStyle, LV_STATE_DEFAULT, &lv_font_montserrat_14);
    //lv_style_set_text_color(&logoStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_bg_color(&logoStyle, LV_STATE_DEFAULT, LV_COLOR_BLACK);

    lv_style_set_bg_color(&backgroundStyle, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_border_opa(&backgroundStyle, LV_STATE_DEFAULT, 0);

    lv_style_set_text_color(&infoStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_text_font(&unitStyle, LV_STATE_DEFAULT, &lv_font_montserrat_30);

    lv_style_set_bg_opa(&transpCont, LV_STATE_DEFAULT, 0);
    lv_style_set_border_opa(&transpCont, LV_STATE_DEFAULT, 0);
  
#endif
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
            debugPrintln("weight test 2 char");
            displayWeight((char *)"12"); // and for each character length
            flag1 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 2 * show && flag2)
        {
            debugPrintln("weight test 3 char");
            displayWeight((char *)"1.9");
            flag2 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 3 * show && flag3)
        {
            debugPrintln("weight test 4 char");
            displayWeight((char *)"300.2");
            flag3 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 4 * show && flag4)
        {
            debugPrintln("weight test 4+ char");
            displayWeight((char *)"800.15");
            flag4 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 5 * show && flag5)
        {
            debugPrintln("Units test g");
            displayUnits(Units::g);
            flag5 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 6 * show && flag6)
        {
            debugPrintln("Units test kg");
            displayUnits(Units::kg);
            flag6 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 7 * show && flag7)
        {
            debugPrintln("Units test oz");
            displayUnits(Units::oz);
            flag7 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 8 * show && flag8)
        {
            debugPrintln("Units test lb");
            displayUnits(Units::lb);
            flag8 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 9 * show && flag9)
        {
            debugPrintln("Info test\n");
            displayDeviceInfo("01100101", "1.0.2"); //
            flag9 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 10 * show && flag10)
        {
            debugPrintln("Battery Image Test");
            displayBattery(40); //
            flag10 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 11 * show && flag11)
        {
            debugPrintln("Low Battery Test");
            displayLowBattery(); //
            flag11 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 12 * show && flag12)
        {
            debugPrintln("Display Off");
            displayOff(); // ready
            flag12 = false;
        }
        else if (esp_timer_get_time() / 1000 - t > 13 * show && flag13)
        {
            debugPrintln("Display logo and resetting...");
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


void DisplayX::lv_tick_task(void *arg)
{
    (void)arg;

    lv_tick_inc(LV_TICK_PERIOD_MS);
}

/***   My functions below this line   ***/

void DisplayX::resizeWeight(char *w)
{

#ifdef CONFIG_SB_V1_HALF_ILI9341
  if (strlen(w) > 4)
  {
    //tft.setFont(3);
  }
  else if (strlen(w) > 3)
  {
    //tft.setFont(4);
  }
  else if (strlen(w) > 2)
  {
    //tft.setFont(5);
  }
  else
  {
    //tft.setFont(6);
  }
#endif

#ifdef CONFIG_SB_V3_ST7735S
  //tft.textSize();
  //tft.setCursor();

  // get active screen
  // Mike look here

  // lv_obj_t * obj1 = lv_obj_create(lv_scr_act(),NULL);
  // lv_obj_set_size(obj1, LV_HOR_RES, LV_VER_RES);
  // lv_obj_align(obj1, NULL, LV_ALIGN_CENTER, -60, -30);

  // // create style
  // static lv_style_t st;
  // lv_style_init(&st);
  // lv_style_set_text_font(&st, LV_STATE_DEFAULT, &lv_font_montserrat_8);
  // lv_style_set_text_color(&st, LV_STATE_DEFAULT, LV_COLOR_WHITE);

  // //create label with text
  // lv_obj_t * label1 = lv_label_create(lv_scr_act(), NULL);
  // lv_label_set_text_fmt(label1, "something %d", 1);
  // // apply style to label
  // lv_obj_add_style(label1, LV_LABEL_PART_MAIN , &st);

  // send to screen

  if (strlen(w) > 4)
  { // TODO: Figure out proper font sizes for this resizing function
    //style
    debugPrintln("size 3 Font");
  }
  else if (strlen(w) > 3)
  {
    
    debugPrintln("size 3' Font");
  }
  else if (strlen(w) > 2)
  {
    
    debugPrintln("size 4 Font");
  }
  else
  {
    
    debugPrintln("size 4 default Font");
  }
#endif

#ifdef CONFIG_SB_V6_FULL_ILI9341
  if (strlen(w) > 4)
  {
    //tft.setFont(3);
  }
  else if (strlen(w) > 3)
  {
    //tft.setFont(4);
  }
  else if (strlen(w) > 2)
  {
    //tft.setFont(5);
  }
  else
  {
    //tft.setFont(6);
  }
#endif
}

void DisplayX::displayWeight(std::string weight)
{

  if(weight.compare("-1") == 0){
    // TODO: have a way to check if the screen already has that value so it doesn't waste time doing it again.
    debugPrintln("breaking out of displayWeight");
    return;
  }
  debugPrintln(" gets past catching -1 value in displayWeight");
  xSemaphoreTake(xGuiSemaphore, (TickType_t)10);
  char now[32];
  strcpy(now, weight.c_str());
  //static lv_obj_t * canvas = lv_canvas_create(lv_scr_act(), NULL);
  //lv_canvas_set_buffer(canvas, cbuf, SB_HORIZ, SB_VERT, LV_IMG_CF_TRUE_COLOR);
  //lv_obj_align(canvas, NULL, LV_ALIGN_CENTER, 0, 0);
  //lv_canvas_fill_bg(canvas, LV_COLOR_BLACK, LV_OPA_COVER);

  lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
  lv_obj_t *cont = lv_cont_create(bkgrnd, NULL);
  lv_obj_t *label1 = lv_label_create(cont, NULL);

  lv_cont_set_fit(cont, LV_FIT_PARENT);
  lv_cont_set_layout(cont, LV_LAYOUT_CENTER);

#ifdef CONFIG_SB_V1_HALF_ILI9341
  lv_obj_set_width(bkgrnd, SB_HORIZ);
  lv_obj_set_height(bkgrnd, SB_VERT);
  lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  if (strlen(now) > 4)
  {
    lv_label_set_text_fmt(label1, "%s", now);
    debugPrintln("4+chars");
  }
  else if (strlen(now) > 3)
  {
    lv_label_set_text_fmt(label1, "%s", now);
    debugPrintln("4 chars");
  }
  else if (strlen(now) > 2)
  {
    lv_label_set_text_fmt(label1, "%s", now);
    debugPrintln("3 chars");
  }
  else
  {
    lv_label_set_text_fmt(label1, "%s", now);
    debugPrintln("default font");
  }

  lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
  lv_obj_add_style(cont, LV_OBJ_PART_MAIN, &transpCont);
  lv_obj_add_style(label1, LV_OBJ_PART_MAIN, &weightStyle);
#endif
#ifdef CONFIG_SB_V3_ST7735S
  lv_obj_set_width(bkgrnd, SB_HORIZ);
  lv_obj_set_height(bkgrnd, SB_VERT);
  lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  if (strlen(now) > 4)
  {
    lv_label_set_text_fmt(label1, "%s", now);
    debugPrintln("4+chars");
  }
  else if (strlen(now) > 3)
  {
    lv_label_set_text_fmt(label1, "%s", now);
    debugPrintln("4 chars");
  }
  else if (strlen(now) > 2)
  {
    lv_label_set_text_fmt(label1, "%s", now);
    debugPrintln("3 chars");
  }
  else
  {
    lv_label_set_text_fmt(label1, "%s", now);
    debugPrintln("default font");
  }

  lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
  lv_obj_add_style(cont, LV_OBJ_PART_MAIN, &transpCont);
  lv_obj_add_style(label1, LV_OBJ_PART_MAIN, &weightStyle);

#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
  lv_obj_set_width(bkgrnd, SB_HORIZ);
  lv_obj_set_height(bkgrnd, SB_VERT);
  lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  if (strlen(now) > 4)
  {
    lv_label_set_text_fmt(label1, "%s", now);
    debugPrintln("4+chars");
  }
  else if (strlen(now) > 3)
  {
    lv_label_set_text_fmt(label1, "%s", now);
    debugPrintln("4 chars");
  }
  else if (strlen(now) > 2)
  {
    lv_label_set_text_fmt(label1, "%s", now);
    debugPrintln("3 chars");
  }
  else
  {
    lv_label_set_text_fmt(label1, "%s", now);
    debugPrintln("default font");
  }

  lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
  lv_obj_add_style(cont, LV_OBJ_PART_MAIN, &transpCont);
  lv_obj_add_style(label1, LV_OBJ_PART_MAIN, &weightStyle);
  
#endif
  xSemaphoreGive(xGuiSemaphore);
}

// Units
void DisplayX::displayUnits(Units u)
{
  xSemaphoreTake(xGuiSemaphore, (TickType_t)10);

  //lv_obj_t *scr = lv_disp_get_scr_act(NULL);
  lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
  lv_obj_t *cont = lv_cont_create(bkgrnd, NULL);
  lv_obj_t *label1 = lv_label_create(cont, NULL);

  lv_cont_set_fit(cont, LV_FIT_PARENT);
  lv_cont_set_layout(cont, LV_LAYOUT_CENTER);

#ifdef CONFIG_SB_V1_HALF_ILI9341
#endif
#ifdef CONFIG_SB_V3_ST7735S
  lv_obj_set_width(bkgrnd, SB_HORIZ);
  lv_obj_set_height(bkgrnd, SB_VERT);
  lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
  
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
  lv_obj_set_width(bkgrnd, SB_HORIZ);
  lv_obj_set_height(bkgrnd, SB_VERT);
  lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
#endif
  //xSemaphoreTake(systemMutex, (TickType_t)10);
  switch (u)
  {
  case kg:
    //xSemaphoreGive(systemMutex);
    lv_label_set_text(label1, "KG");

    break;
  case g:
    //xSemaphoreGive(systemMutex);
    lv_label_set_text(label1, "G");

    break;
  case lb:
    //xSemaphoreGive(systemMutex);
    lv_label_set_text(label1, "LB");

    break;
  case oz:
    //xSemaphoreGive(systemMutex);
    lv_label_set_text(label1, "OZ");

    break;
  default:
    break;
  }
  
  lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
  lv_obj_add_style(cont, LV_OBJ_PART_MAIN, &transpCont);
  lv_obj_add_style(label1, LV_OBJ_PART_MAIN, &weightStyle);
  xSemaphoreGive(xGuiSemaphore);
}

// Settings
void DisplayX::displaySettings()
{
  xSemaphoreTake(xGuiSemaphore, (TickType_t)10);
  // TODO: For settings menu, whenever that is implemented
#ifdef CONFIG_SB_V1_HALF_ILI9341
#endif
#ifdef CONFIG_SB_V3_ST7735S
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
#endif
  xSemaphoreGive(xGuiSemaphore);
}

// Battery & Info
void DisplayX::displayDeviceInfo(std::string SN, std::string VER)
{
  xSemaphoreTake(xGuiSemaphore, (TickType_t)10);
  
  // create objects to display info
  lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
  lv_obj_t *label3 = lv_label_create(bkgrnd, NULL);
  debugPrintln("after setting style values");
#ifdef CONFIG_SB_V1_HALF_ILI9341

#endif
#ifdef CONFIG_SB_V3_ST7735S

  lv_obj_set_width(bkgrnd, SB_HORIZ);
  lv_obj_set_height(bkgrnd, SB_VERT);

  while (!(pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)))
  {
    debugPrintln("loop");
  }
  lv_label_set_text_fmt(label3, "SUDOBOARD INFO\nSN: %s\n VER: %s", SN, VER); // , 5, 10); //"sn", "ver");
  xSemaphoreGive(xGuiSemaphore);
  debugPrintln("after setting label text values");
  lv_label_set_align(label3, LV_ALIGN_CENTER);
  lv_obj_align(label3, NULL, LV_ALIGN_CENTER, 0,0);
  debugPrintln("After setting alignment");

#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
#endif
  lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
  lv_obj_add_style(label3, LV_OBJ_PART_MAIN, &infoStyle);

  xSemaphoreGive(xGuiSemaphore);
}

// Update
void DisplayX::displayUpdateScreen(int pct)
{
  xSemaphoreTake(xGuiSemaphore, (TickType_t)10);
  debugPrintln("inside displayUpdateScreen");
  
  
  
  xSemaphoreGive(xGuiSemaphore);
}

void DisplayX::displayLogo()
{
  xSemaphoreTake(xGuiSemaphore, (TickType_t)10);
  lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
  lv_obj_t *img = lv_img_create(bkgrnd, NULL);
  
  lv_img_set_src(img, &img_logo_white);

#ifdef CONFIG_SB_V1_HALF_ILI9341
  lv_obj_set_width(bkgrnd, SB_HORIZ);
  lv_obj_set_height(bkgrnd, SB_VERT);
  lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
  lv_img_set_zoom(img, 70);
  lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, 0);
#endif
#ifdef CONFIG_SB_V3_ST7735S
  lv_obj_set_width(bkgrnd, SB_HORIZ);
  lv_obj_set_height(bkgrnd, SB_VERT);
  lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
  lv_img_set_zoom(img, 70);
  lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, 0);
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
  lv_obj_set_width(bkgrnd, SB_HORIZ);
  lv_obj_set_height(bkgrnd, SB_VERT);
  lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
  lv_img_set_zoom(img, 250);
  lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, 0);
#endif
  lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);

  xSemaphoreGive(xGuiSemaphore);
}

void DisplayX::displayBattery(int bat)
{
  xSemaphoreTake(xGuiSemaphore, (TickType_t)10);
  lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
  lv_obj_t *img = lv_img_create(bkgrnd, NULL);
  lv_obj_t *cont = lv_cont_create(img, NULL);
  lv_obj_t *label = lv_label_create(cont, NULL);
#ifdef CONFIG_SB_V1_HALF_ILI9341
#endif
#ifdef CONFIG_SB_V3_ST7735S
  lv_img_set_src(img, &img_battery);
  lv_obj_set_width(bkgrnd, SB_HORIZ);
  lv_obj_set_height(bkgrnd, SB_VERT);
  lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  lv_img_set_zoom(img, 100);
  lv_obj_align(img, NULL, LV_ALIGN_CENTER, 10, 0);
  
  
  lv_cont_set_fit(cont, LV_FIT_PARENT);
  lv_cont_set_layout(cont, LV_LAYOUT_CENTER);

  //xSemaphoreTake(systemMutex, (TickType_t)10);
  lv_label_set_text_fmt(label, "%d%%", bat);
  //xSemaphoreGive(systemMutex);
  lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0,0);
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
  lv_img_set_src(img, &img_battery);
  lv_obj_set_width(bkgrnd, SB_HORIZ);
  lv_obj_set_height(bkgrnd, SB_VERT);
  lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  lv_img_set_zoom(img, 100);
  lv_obj_align(img, NULL, LV_ALIGN_CENTER, 10, 0);
  
  
  lv_cont_set_fit(cont, LV_FIT_PARENT);
  lv_cont_set_layout(cont, LV_LAYOUT_CENTER);

  //xSemaphoreTake(systemMutex, (TickType_t)10);
  lv_label_set_text_fmt(label, "%d%%", bat);
  //xSemaphoreGive(systemMutex);
  lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0,0);
#endif
 
  lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
  lv_obj_add_style(cont, LV_CONT_PART_MAIN, &transpCont);
  lv_obj_add_style(label, LV_CONT_PART_MAIN, &infoStyle);
  xSemaphoreGive(xGuiSemaphore);
}

void DisplayX::displayLowBattery()
{
  xSemaphoreTake(xGuiSemaphore, (TickType_t)10);
  lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
  lv_obj_t *img = lv_img_create(bkgrnd, NULL);
  lv_obj_t *cont = lv_cont_create(img, NULL);
  lv_obj_t *label = lv_label_create(cont, NULL);
#ifdef CONFIG_SB_V1_HALF_ILI9341
#endif
#ifdef CONFIG_SB_V3_ST7735S
  lv_img_set_src(img, &img_low_battery);
  lv_obj_set_width(bkgrnd, SB_HORIZ);
  lv_obj_set_height(bkgrnd, SB_VERT);
  lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  lv_img_set_zoom(img, 100);
  lv_obj_align(img, NULL, LV_ALIGN_CENTER, 10, 0);
  
  
  lv_cont_set_fit(cont, LV_FIT_PARENT);
  lv_cont_set_layout(cont, LV_LAYOUT_CENTER);

  lv_label_set_text(label, "LOW");
  lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0,0);
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
lv_img_set_src(img, &img_low_battery);
  lv_obj_set_width(bkgrnd, SB_HORIZ);
  lv_obj_set_height(bkgrnd, SB_VERT);
  lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  lv_img_set_zoom(img, 100);
  lv_obj_align(img, NULL, LV_ALIGN_CENTER, 10, 0);
  
  
  lv_cont_set_fit(cont, LV_FIT_PARENT);
  lv_cont_set_layout(cont, LV_LAYOUT_CENTER);

  lv_label_set_text(label, "LOW");
  lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0,0);
#endif
 
  lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
  lv_obj_add_style(cont, LV_CONT_PART_MAIN, &transpCont);
  lv_obj_add_style(label, LV_CONT_PART_MAIN, &infoStyle);
  xSemaphoreGive(xGuiSemaphore);
}

void DisplayX::displayOff()
{
  xSemaphoreTake(xGuiSemaphore, (TickType_t)10);
  lv_obj_clean(lv_scr_act());
  xSemaphoreGive(xGuiSemaphore);
  disp_flag = false;
  setIntensity(0);  

}

void DisplayX::displayOn()
{

  disp_flag = true;
  setIntensity(100);

}

void DisplayX::displaySleepPrep(){
  
  displayOff();
  
}

void DisplayX::setColor(int r, int g, int b){
  // This pretty much is only for the projector version of the display
  #ifndef CONFIG_SB_V3_ST7735S
  debugPrintln("This configuration isn't set up to control color. Only for V3.");
  return;
  #endif

  if(r>255){
    r = 255;
  }else if(r<0){
    r = 0;
  }
  if(r>255){
    g = 255;
  }else if(r<0){
    g = 0;
  }
  if(r>255){
    b = 255;
  }else if(r<0){
    b = 0;
  }

  red = r*intensity/255;
  green = g*intensity/255;
  blue = b*intensity/255;

  ledc_set_duty_and_update(r_ledc_c_config.speed_mode, r_ledc_c_config.channel, red, 0);
  ledc_set_duty_and_update(g_ledc_c_config.speed_mode, g_ledc_c_config.channel, green, 0);
  ledc_set_duty_and_update(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, blue, 0);

}

void DisplayX::setIntensity(int i){
  intensity = i;


  
  blue = blue*intensity/255;
  
#ifdef CONFIG_SB_V1_HALF_ILI9341
  ledc_set_duty_and_update(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, blue, 0);
#endif
#ifdef CONFIG_SB_V3_ST7735S
  green = green*intensity/255;
  red = red*intensity/255;
  
  ledc_set_duty_and_update(r_ledc_c_config.speed_mode, r_ledc_c_config.channel, red, 0);
  ledc_set_duty_and_update(g_ledc_c_config.speed_mode, g_ledc_c_config.channel, green, 0);
  ledc_set_duty_and_update(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, blue, 0);
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
  ledc_set_duty_and_update(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, blue, 0);
#endif

}
