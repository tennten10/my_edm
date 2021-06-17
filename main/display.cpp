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
#ifdef CONFIG_SB_V3_ST7735S
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
#endif

esp_timer_handle_t periodic_timer;

/* Initialize image files for display
* These are stored as .c files and converted from https://lvgl.io/tools/imageconverter
*/
LV_IMG_DECLARE(img_battery);
LV_IMG_DECLARE(img_low_battery);
LV_IMG_DECLARE(img_logo_black);
LV_IMG_DECLARE(img_logo_white);

LV_FONT_DECLARE(vt323_70);
LV_FONT_DECLARE(vt323_90);
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
    debugPrint("starting display loop: ");
    debugPrintln((int)esp_timer_get_time()/1000 );
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
    

#if defined(CONFIG_SB_V1_HALF_ILI9341) || defined(CONFIG_SB_V6_FULL_ILI9341)
  ledc_channel_config(&b_ledc_c_config);
  ledc_fade_func_install(0);
  
  ledc_set_duty_and_update(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, intensity, 0);
#endif
#ifdef CONFIG_SB_V3_ST7735S
  ledc_channel_config(&r_ledc_c_config);
  ledc_channel_config(&g_ledc_c_config);
  ledc_channel_config(&b_ledc_c_config);
  ledc_set_duty_and_update(r_ledc_c_config.speed_mode, r_ledc_c_config.channel, 100, 0);
  ledc_set_duty_and_update(g_ledc_c_config.speed_mode, g_ledc_c_config.channel, 100, 0);
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
    //esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    //init my style types
    styleInit();
    disp_flag = true;
    debugPrintln("drawing starting screen");

    // draw my starting screen
    long t = esp_timer_get_time() / 1000;
    
    displayLogo();
    while (!ready)  //esp_timer_get_time() / 1000 - t < 500)
    {
        lv_task_handler();
        vTaskDelay(10);
    }
    vTaskDelay(100);
    

    // pageTestRoutine((long)(esp_timer_get_time() / 1000));
    debugPrint("end of display init: ");
    debugPrintln((int)esp_timer_get_time()/1000);
    while (1)
    {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        if (disp_flag && pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
          lv_task_handler();
          xSemaphoreGive(xGuiSemaphore);
        }else{
          debugPrintln("Could not get xGuiSemaphore in main display loop");
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

#ifdef CONFIG_SB_V1_HALF_ILI9341
    lv_style_
        lv_obj_align(label1, NULL, LV_ALIGN_CENTER, 0, 0);

    lv_style_set_text_font(&style, LV_STATE_DEFAULT, &lv_font_montserrat_14);
    lv_style_set_text_color(&style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_bg_color(&style, LV_STATE_DEFAULT, LV_COLOR_BLACK);

#endif
#ifdef CONFIG_SB_V3_ST7735S
    
    // lv_style_set_text_font(&weightStyle_s, LV_STATE_DEFAULT, &lv_font_montserrat_40);
    // lv_style_set_text_color(&weightStyle_s, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    
    // lv_style_set_text_font(&weightStyle_m, LV_STATE_DEFAULT, &lv_font_montserrat_40);
    // lv_style_set_text_color(&weightStylem, LV_STATE_DEFAULT, LV_COLOR_WHITE);

    // lv_style_set_text_font(&weightStyle_l, LV_STATE_DEFAULT, &lv_font_montserrat_40);
    // lv_style_set_text_color(&weightStylel, LV_STATE_DEFAULT, LV_COLOR_WHITE);

    lv_style_set_text_font(&unitStyle, LV_STATE_DEFAULT, &lv_font_montserrat_40);
    lv_style_set_text_color(&unitStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    
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
    
    lv_style_set_text_font(&weightStyle, LV_STATE_DEFAULT, &vt323_140);
    lv_style_set_text_color(&weightStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    
    // lv_style_set_text_font(&weightStyle_m, LV_STATE_DEFAULT, &lv_font_montserrat_90);
    // lv_style_set_text_color(&weightStylem, LV_STATE_DEFAULT, LV_COLOR_WHITE);

    // lv_style_set_text_font(&weightStyle_l, LV_STATE_DEFAULT, &lv_font_montserrat_120);
    // lv_style_set_text_color(&weightStylel, LV_STATE_DEFAULT, LV_COLOR_WHITE);

    lv_style_set_text_font(&unitStyle, LV_STATE_DEFAULT, &vt323_140);
    lv_style_set_text_color(&unitStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    

    
    lv_style_set_bg_color(&logoStyle, LV_STATE_DEFAULT, LV_COLOR_BLACK);

    lv_style_set_bg_color(&backgroundStyle, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_border_opa(&backgroundStyle, LV_STATE_DEFAULT, 0);

    lv_style_set_text_color(&infoStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_text_font(&infoStyle, LV_STATE_DEFAULT, &montserrat_90);

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
  static int lastLen = 0;
  //printf("resizeWeight: %d, %d\n", strlen(w), lastLen);

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
  

  if (strlen(w) > 4)
  { // TODO: Figure out proper font sizes for this resizing function
    //style
    debugPrintln("size 3 Font");
  }
  else if (strlen(w) > 3 && lastLen != 3)
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
  if (strlen(w) > 7)
  {
    if(lastLen > 7){
      //lv_style_reset(&weightStyle);
      lv_style_remove_prop(&weightStyle, LV_STYLE_TEXT_FONT);
      lv_style_set_text_font(&weightStyle, LV_STATE_DEFAULT, &vt323_120); 
      //lv_style_set_text_color(&weightStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    }
  }
  else if (strlen(w) > 4)
  {
    if(lastLen > 4){
      //lv_style_reset(&weightStyle);
      lv_style_remove_prop(&weightStyle, LV_STYLE_TEXT_FONT);
      lv_style_set_text_font(&weightStyle, LV_STATE_DEFAULT, &vt323_130); 
      //lv_style_set_text_color(&weightStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    }  
  }
  else if (strlen(w) > 0 )
  {
    if(lastLen > 0){
      //lv_style_reset(&weightStyle);
      lv_style_remove_prop(&weightStyle, LV_STYLE_TEXT_FONT);
      lv_style_set_text_font(&weightStyle, LV_STATE_DEFAULT, &vt323_140);
      //lv_style_set_text_color(&weightStyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    } 
  }
  else
  {
    
  }
#endif
  lastLen = strlen(w);
  //debugPrintln("end of resizeWeight");
}

// displayWeight should be called first. This only updates the number value
void DisplayX::updateWeight(std::string weight){
  if(weight.compare("-1") == 0){
    // TODO: have a way to check if the screen already has that value so it doesn't waste time doing it again.
    debugPrintln("breaking out of updateWeight");
    return;
  }
  if(xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)==pdTRUE){
    static char now[16];
    if(weightLabel != NULL){
      strcpy(now, weight.c_str());
      debugPrintln(now);
      resizeWeight(now);
      lv_label_set_text_fmt(weightLabel, "%s", now);
      //lv_obj_remove_style(weightLabel, LV_OBJ_PART_MAIN, &weightStyle);
      //lv_obj_add_style(weightLabel, LV_OBJ_PART_MAIN, &weightStyle);
      // lv_label_set_text(weightLabel, weight.c_str());
    }
    xSemaphoreGive(xGuiSemaphore);
  }else{
    debugPrintln("could not get xGuiSemaphore in updateWeight");
  }
}

// This one is for the first time
void DisplayX::displayWeight(std::string weight)
{
  if(weight.compare("-1") == 0){
    // TODO: have a way to check if the screen already has that value so it doesn't waste time doing it again.
    debugPrintln("breaking out of displayWeight");
    return;
  }
  debugPrintln(weight);
  debugPrint(" gets past catching -1 value in displayWeight: ");
  if(xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)==pdTRUE){
    debugPrintln("Got xGuiSemaphore in displayWeight");
    
    static char now[32];
    strcpy(now, weight.c_str());
    resizeWeight(now); // lv_obj_refresh_style(obj, part, property);?
    //debugPrintln(now);
    //debugPrintln(weight.c_str());
    
    debugPrintln("umm here 1");
    lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
    debugPrintln("umm here 1.1");
    lv_obj_t *cont = lv_cont_create(bkgrnd, NULL);
    debugPrintln("umm here 1.2");
    weightLabel = lv_label_create(cont, NULL);
    debugPrintln("umm here 2");
    lv_cont_set_fit(cont, LV_FIT_PARENT);
    lv_cont_set_layout(cont, LV_LAYOUT_CENTER);
    debugPrintln("umm here 3");

  #ifdef CONFIG_SB_V1_HALF_ILI9341
    lv_obj_set_width(bkgrnd, SB_HORIZ);
    lv_obj_set_height(bkgrnd, SB_VERT);
    lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

    lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
    lv_obj_add_style(cont, LV_OBJ_PART_MAIN, &transpCont);
    lv_obj_add_style(weightLabel, LV_OBJ_PART_MAIN, &weightStyle);
  #endif
  #ifdef CONFIG_SB_V3_ST7735S
    lv_obj_set_width(bkgrnd, SB_HORIZ);
    lv_obj_set_height(bkgrnd, SB_VERT);
    lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

    lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
    lv_obj_add_style(cont, LV_OBJ_PART_MAIN, &transpCont);
    lv_obj_add_style(weightLabel, LV_OBJ_PART_MAIN, &weightStyle);

  #endif

  #ifdef CONFIG_SB_V6_FULL_ILI9341
    lv_obj_set_width(bkgrnd, SB_HORIZ);
    lv_obj_set_height(bkgrnd, SB_VERT);
    lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
    lv_label_set_text_fmt(weightLabel, "%s", now);
  debugPrintln("umm here 4");

    lv_obj_align(cont, NULL, LV_ALIGN_IN_RIGHT_MID, 0, 0);
    lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
    lv_obj_add_style(cont, LV_OBJ_PART_MAIN, &transpCont);
    lv_obj_add_style(weightLabel, LV_OBJ_PART_MAIN, &weightStyle);
    debugPrintln("umm here 5");
  #endif
    xSemaphoreGive(xGuiSemaphore);
    debugPrintln("gave back xGuiSemaphore in displayWeight");
  }else{
    debugPrintln("could not get xGuiSemaphore in displayWeight");
  }
}

  // Units
void DisplayX::displayUnits(Units u)
{
  if(xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)==pdTRUE){
    debugPrintln("got xGuiSemaphore in displayUnits");
    
    lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_t *cont = lv_cont_create(bkgrnd, NULL);
    unitsLabel = lv_label_create(cont, NULL);

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
      
    lv_label_set_text(unitsLabel, unitsToString(u).c_str());

    lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
    lv_obj_add_style(cont, LV_OBJ_PART_MAIN, &transpCont);
    lv_obj_add_style(unitsLabel, LV_OBJ_PART_MAIN, &unitStyle);
    xSemaphoreGive(xGuiSemaphore);
    debugPrintln("gave back xGuiSemaphore in displayUnits");
  }else{
    debugPrintln("could not get xGuiSemaphore in displayUnits");
  }
}

void DisplayX::updateUnits(Units u){
  if(xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)==pdTRUE){
    debugPrintln("got xGuiSemaphore in updateUnits");
    if(unitsLabel != NULL){
      lv_label_set_text(unitsLabel, unitsToString(u).c_str());
    }else{
      debugPrintln("unitsLabel not initialized. run displayUnits before this.");
    }
    

  #ifdef CONFIG_SB_V1_HALF_ILI9341
  #endif
  #ifdef CONFIG_SB_V3_ST7735S
    
    
  #endif
  #ifdef CONFIG_SB_V6_FULL_ILI9341
    
  #endif
      
    xSemaphoreGive(xGuiSemaphore);
    debugPrintln("gave back xGuiSemaphore in updateUnits");
  }else{
    debugPrintln("could not get xGuiSemaphore in updateUnits");
  }
}

// Settings
void DisplayX::displaySettings()
{
  if(xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)==pdTRUE){
    debugPrintln("got xGuiSemaphore in displaySettings");
  // TODO: For settings menu, whenever that is implemented
#ifdef CONFIG_SB_V1_HALF_ILI9341
#endif
#ifdef CONFIG_SB_V3_ST7735S
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
#endif
    xSemaphoreGive(xGuiSemaphore);
    debugPrintln("gave back xGuiSemaphore in displaySettings");
  }else{
    debugPrintln("could not get xGuiSemaphore in displaySettings");
  }
}

// Battery & Info
void DisplayX::displayDeviceInfo(std::string SN, std::string VER)
{
  if(xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)==pdTRUE){
    debugPrintln("got xGuiSemaphore in displayDeviceInfo");
  
    // create objects to display info
    lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_t *label3 = lv_label_create(bkgrnd, NULL);
    debugPrintln("after setting style values");
  #ifdef CONFIG_SB_V1_HALF_ILI9341

  #endif
  #ifdef CONFIG_SB_V3_ST7735S

    lv_obj_set_width(bkgrnd, SB_HORIZ);
    lv_obj_set_height(bkgrnd, SB_VERT);

    
    lv_label_set_text_fmt(label3, "SUDOBOARD INFO\nSN: %s\n VER: %s", SN, VER); // , 5, 10); //"sn", "ver");
    xSemaphoreGive(xGuiSemaphore);
    debugPrintln("after setting label text values");
    lv_label_set_align(label3, LV_ALIGN_CENTER);
    lv_obj_align(label3, NULL, LV_ALIGN_CENTER, 0,0);
    debugPrintln("After setting alignment");

  #endif
  #ifdef CONFIG_SB_V6_FULL_ILI9341
    lv_obj_set_width(bkgrnd, SB_HORIZ);
    lv_obj_set_height(bkgrnd, SB_VERT);


    lv_label_set_text_fmt(label3, "SUDOBOARD INFO\nSN: %s\n VER: %s", SN, VER); // , 5, 10); //"sn", "ver");
    debugPrintln("after setting label text values");
    lv_label_set_align(label3, LV_ALIGN_CENTER);
    lv_obj_align(label3, NULL, LV_ALIGN_CENTER, 0,0);
    debugPrintln("After setting alignment");


  #endif
    lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
    lv_obj_add_style(label3, LV_OBJ_PART_MAIN, &infoStyle);

    xSemaphoreGive(xGuiSemaphore);
    debugPrintln("gave back xGuiSemaphore in displayDeviceInfo");
  }else{
    debugPrintln("can't get xGuiSemaphore in displayDeviceInfo");
  }
}

// Update
void DisplayX::displayUpdateScreen(int pct)
{
  if(xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)==pdTRUE){
    debugPrintln("got xGuiSemaphore in displayUpdateScreen");
  
// create objects to display info
    lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_t *label3 = lv_label_create(bkgrnd, NULL);
    debugPrintln("after setting style values");
  #ifdef CONFIG_SB_V1_HALF_ILI9341

  #endif
  #ifdef CONFIG_SB_V3_ST7735S

  #endif
  #ifdef CONFIG_SB_V6_FULL_ILI9341
    lv_obj_set_width(bkgrnd, SB_HORIZ);
    lv_obj_set_height(bkgrnd, SB_VERT);


    lv_label_set_text(label3, "Updating..."); // , 5, 10); //"sn", "ver");
    debugPrintln("after setting label text values");
    lv_label_set_align(label3, LV_ALIGN_CENTER);
    lv_obj_align(label3, NULL, LV_ALIGN_CENTER, 0,0);
    debugPrintln("After setting alignment");


  #endif
    lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
    lv_obj_add_style(label3, LV_OBJ_PART_MAIN, &infoStyle);
  
    xSemaphoreGive(xGuiSemaphore);
    debugPrintln("gave back xGuiSemaphore in displayUpdateScreen");
  }else{
    debugPrintln("can't get xGuiSemaphore in displayUpdateScreen");
  }
}

void DisplayX::displayLogo()
{
  if(xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)==pdTRUE){
    debugPrintln("got xGuiSemaphore in displayLogo");
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
    lv_img_set_zoom(img, 200);
    lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, 0);
  #endif
    lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);

    xSemaphoreGive(xGuiSemaphore);
    debugPrintln("gave back xGuiSemaphore in displayLogo");
  }else{
    debugPrintln("can't get xGuiSemaphore in displayLogo");
  }
}

void DisplayX::displayBattery(int bat)
{
  if(xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)==pdTRUE){
    debugPrintln("got xGuiSemaphore in displayBattery");
    
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

    
    lv_label_set_text_fmt(label, "%d%%", bat);
    
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

    
    lv_label_set_text_fmt(label, "%d%%", bat);
    
    lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0,0);
  #endif
  
    lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
    lv_obj_add_style(cont, LV_CONT_PART_MAIN, &transpCont);
    lv_obj_add_style(label, LV_CONT_PART_MAIN, &infoStyle);
    xSemaphoreGive(xGuiSemaphore);
    debugPrintln("gave back xGuiSemaphore in displayBattery");
  }else{
    debugPrintln("Can't get xGuiSemaphore in displayBattery");
  }
}

void DisplayX::displayLowBattery()
{
  if(xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)==pdTRUE){
      debugPrintln("got xGuiSemaphore in displayLowBattery");

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
    debugPrintln("gave back xGuiSemaphore in displayLowBattery");
  }else{
    debugPrintln("Can't get xGuiSemaphore in displayLowBattery");
  }
}

void DisplayX::displayOff()
{
  if(xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)==pdTRUE){
    debugPrintln("got xGuiSemaphore in displayoff");
    lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_set_width(bkgrnd, SB_HORIZ);
    lv_obj_set_height(bkgrnd, SB_VERT);
    lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
    lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
    //lv_obj_clean(lv_scr_act());
    xSemaphoreGive(xGuiSemaphore);
    debugPrintln("gave back xGuiSemaphore in displayOff");
    disp_flag = false;
    //setIntensity(0);
  #ifdef CONFIG_SB_V1_HALF_ILI9341
    ledc_set_duty_and_update(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, 0, 0);
  #endif
  #ifdef CONFIG_SB_V3_ST7735S
    green = green*intensity/255;
    red = red*intensity/255;
    
    ledc_set_duty_and_update(r_ledc_c_config.speed_mode, r_ledc_c_config.channel, 0, 0);
    ledc_set_duty_and_update(g_ledc_c_config.speed_mode, g_ledc_c_config.channel, 0, 0);
    ledc_set_duty_and_update(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, 0, 0);
  #endif
  #ifdef CONFIG_SB_V6_FULL_ILI9341
    ledc_set_duty_and_update(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, 0, 0);
  #endif
  }else{
    debugPrintln("Can't get xGuiSemaphore in displayOff");
  }
}

void DisplayX::displayOn()
{
  disp_flag = true;
  setIntensity(intensity);
}

void DisplayX::displaySleepPrep(){
  
  displayOff();
  // save color and intensity to nvs
  
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
  #ifdef CONFIG_SB_V3_ST7735S
  ledc_set_duty_and_update(r_ledc_c_config.speed_mode, r_ledc_c_config.channel, red, 0);
  ledc_set_duty_and_update(g_ledc_c_config.speed_mode, g_ledc_c_config.channel, green, 0);
  #endif
  ledc_set_duty_and_update(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, blue, 0);
}

void DisplayX::setIntensity(int i){
  if(i>8192){
    intensity = 8192;
  }else if(i < 0){
    intensity = 0;
  }else{
    intensity = i;
  }


  
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

void DisplayX::onDelete(){
  #if defined(CONFIG_SB_V1_HALF_ILI9341) || defined(CONFIG_SB_V6_FULL_ILI9341)
  ledc_stop(b_ledc_c_config.speed_mode, b_ledc_c_config.channel, 0); 
  #endif
  #ifdef CONFIG_SB_V3_ST7735S
  ledc_stop(r_ledc_c_config.speed_mode, r_ledc_c_config.channel, 0);
  ledc_stop(g_ledc_c_config.speed_mode, g_ledc_c_config.channel, 0);
  
  #endif
  ledc_timer_pause(LEDC_LOW_SPEED_MODE , LEDC_TIMER_1); //ledc_timer
  esp_timer_delete(periodic_timer);
  debugPrintln("finished display onDelete");
  
}
