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

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_tick_task(void *arg);
static void displayTask(void *pvParameter);

void displayLoopConditions(long &t, int &q, int &q_last);

/**********************
 *  GLOBAL VARIABLES
 **********************/
TaskHandle_t displayHandler_TH;
//extern QueueHandle_t weightQueue;
extern TickType_t xBlockTime;

SemaphoreHandle_t displayMutex;
extern SemaphoreHandle_t pageMutex;
extern SemaphoreHandle_t systemMutex;
extern struct System _sys;
char currentWeight[32] = "0.0";
bool updateStarted = 0;
extern volatile PAGE ePage; // = WEIGHTSTREAM;
Units currentUnits = kg;
ledc_channel_config_t ledc_c_config{
    .gpio_num = ledB,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_1,
    .duty = (uint32_t)10,
    .hpoint = 0,
};
static bool disp_flag = false;

/* Initialize image files for display
* These are stored as .c files and converted from https://lvgl.io/tools/imageconverter
*/
LV_IMG_DECLARE(img_battery);
LV_IMG_DECLARE(img_low_battery);
LV_IMG_DECLARE(img_logo_black);
LV_IMG_DECLARE(img_logo_white);

// Global style variables
lv_style_t weightStyle;
lv_style_t unitStyle;
lv_style_t logoStyle;
lv_style_t backgroundStyle;
lv_style_t infoStyle;
lv_style_t transpCont;

lv_color_t cbuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(SB_HORIZ, SB_VERT)];

/**********************
 *   
 **********************/

void displaySetup()
{
  /* If you want to use a task to create the graphic, you NEED to create a Pinned task
     * Otherwise there can be problem such as memory corruption and so on.
     * NOTE: When not using Wi-Fi nor Bluetooth you can pin the guiTask to core 0 */
  xTaskCreatePinnedToCore(displayTask, "display", 4096 * 2, NULL, 0, &displayHandler_TH, 1);
  debugPrintln("Display thread created...");
}

/* Creates a semaphore to handle concurrent call to lvgl stuff
 * If you wish to call *any* lvgl function from other threads/tasks
 * you should lock on the very same semaphore! */
SemaphoreHandle_t xGuiSemaphore;

static void displayTask(void *pvParameter)
{
  xGuiSemaphore = xSemaphoreCreateMutex();

  lv_init();

  /* Initialize SPI or I2C bus used by the drivers */
  lvgl_driver_init();

  lv_color_t *buf1 = static_cast<lv_color_t *>(heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA));
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
  ledc_channel_config(&ledc_c_config);

  //ledc_set_duty(ledc_c_config.speed_mode, ledc_c_config.channel, 50);
  //ledc_update_duty(ledc_c_config.speed_mode, ledc_c_config.channel);
  ledc_set_duty_and_update(ledc_c_config.speed_mode, ledc_c_config.channel, 100, 0);

  /* Use double buffered when not working with monochrome displays */
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
  lv_color_t *buf2 = static_cast<lv_color_t *>(heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA));
  assert(buf2 != NULL);
#else
  static lv_color_t *buf2 = NULL;
#endif

  static lv_disp_buf_t disp_buf;

  uint32_t size_in_px = DISP_BUF_SIZE;

#if defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_IL3820 || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_JD79653A || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_UC8151D || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_SSD1306

  /* Actual size in pixels, not bytes. */
  size_in_px *= 8;
#endif

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
  int q = 0;
  int q_last = 0;
  displayLogo();
  while (esp_timer_get_time() / 1000 - t < 1000)
  {
    lv_task_handler();
    //xSemaphoreGive(xGuiSemaphore);
    vTaskDelay(10);
  }

  //displayWeight(currentWeight);

  pageTestRoutine((long)(esp_timer_get_time() / 1000));

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

void displayLoopConditions(long &t, int &q, int &q_last)
{

  // Low battery warning

  if ((esp_timer_get_time() / 1000) - t > 10000)
  {
    xSemaphoreTake(systemMutex, (TickType_t)10);
    if (_sys.batteryLevel < 20)
    {
      xSemaphoreGive(systemMutex);
      displayLowBattery();
      vTaskDelay(1000);
      t = esp_timer_get_time() / 1000;
    }
    else
    {
      xSemaphoreGive(systemMutex);
    }
  }

  // Look at if the screen should change from button interactions
  pageEventCheck(t, q, q_last);
}

void styleInit()
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
#endif
}

void pageTestRoutine(long t)
{
  int show = 2000; // 5 seconds
  bool flag1 = 1, flag2 = 1, flag3 = 1, flag4 = 1;
  bool flag5 = 1, flag6 = 1, flag7 = 1, flag8 = 1;
  bool flag9 = 1, flag10 = 1, flag11 = 1, flag12 = 1;
  bool flag13 = 1;//, flag14 = 1, flag15 = 1, flag16 = 1;
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
    //lv_task_handler();

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
      xSemaphoreTake(systemMutex, (TickType_t)10);
      _sys.eUnits = g;
      xSemaphoreGive(systemMutex);
      displayUnits();
      flag5 = false;
    }
    else if (esp_timer_get_time() / 1000 - t > 6 * show && flag6)
    {
      debugPrintln("Units test kg");
      xSemaphoreTake(systemMutex, (TickType_t)10);
      _sys.eUnits = kg;
      xSemaphoreGive(systemMutex);
      displayUnits();
      flag6 = false;
    }
    else if (esp_timer_get_time() / 1000 - t > 7 * show && flag7)
    {
      debugPrintln("Units test oz");
      xSemaphoreTake(systemMutex, (TickType_t)10);
      _sys.eUnits = oz;
      xSemaphoreGive(systemMutex);
      displayUnits();
      flag7 = false;
    }
    else if (esp_timer_get_time() / 1000 - t > 8 * show && flag8)
    {
      debugPrintln("Units test lb");
      xSemaphoreTake(systemMutex, (TickType_t)10);
      _sys.eUnits = lb;
      xSemaphoreGive(systemMutex);
      displayUnits();
      flag8 = false;
    }
    else if (esp_timer_get_time() / 1000 - t > 9 * show && flag9)
    {
      debugPrintln("Info test\n");
      displayDeviceInfo(); //
      flag9 = false;
    }
    else if (esp_timer_get_time() / 1000 - t > 10 * show && flag10)
    {
      debugPrintln("Battery Image Test");
      displayBattery(); //
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
void pageEventCheck(long &t, int &q, int &q_last)
{
  // do do dooo do do do do.
  xSemaphoreTake(pageMutex, (TickType_t)10);
  switch (ePage)
  { // options are {WEIGHTSTREAM, SETTINGS, INFO, UNITS, pUPDATE};
  case WEIGHTSTREAM:
    xSemaphoreGive(pageMutex);
#ifdef CONFIG_SB_V1_HALF_ILI9341
#endif
#ifdef CONFIG_SB_V3_ST7735S
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
#endif

    //debugPrintln(String(uxQueueMessagesWaiting(weightQueue)));
    // if (uxQueueMessagesWaiting(weightQueue) > 0)
    // {
    //   xQueueReceive(weightQueue, &currentWeight, xBlockTime);

    //   //displayWeight(currentWeight);
    //   displayWeight(*currentWeight);
    //   //if (isBtConnected())
    //   //{
    //   //updateBTWeight(currentWeight);
    //   //}
    //   debugPrintln("This is where weight would be printed");
    // }
    break;
  case pUPDATE:
    xSemaphoreGive(pageMutex);

#ifdef CONFIG_SB_V1_HALF_ILI9341
#endif
#ifdef CONFIG_SB_V3_ST7735S
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
#endif
    q = 50; //getUpdatePercent(); TODO: include update file
    if (q < 3 && q != q_last)
    {
      displayUpdateScreen(3);
      q_last = q;
    }
    else if (q > q_last)
    {
      displayUpdateScreen(q);
      q_last = q;
    }
    // TODO: dynamically update percentage when downloading and inistalling updated code
    break;
  case INFO:
    xSemaphoreGive(pageMutex);
#ifdef CONFIG_SB_V1_HALF_ILI9341
#endif
#ifdef CONFIG_SB_V3_ST7735S
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
#endif

    displayDeviceInfo();
    break;
  case UNITS:
    xSemaphoreGive(pageMutex);
#ifdef CONFIG_SB_V1_HALF_ILI9341
#endif
#ifdef CONFIG_SB_V3_ST7735S
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
#endif
    displayUnits();
    break;
  case SETTINGS:
    xSemaphoreGive(pageMutex);
#ifdef CONFIG_SB_V1_HALF_ILI9341
#endif
#ifdef CONFIG_SB_V3_ST7735S
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
#endif
    displaySettings();
    break;
  default:
    xSemaphoreGive(pageMutex);
    debugPrintln("invalid page type");
    break;
  }
}

static void lv_tick_task(void *arg)
{
  (void)arg;

  lv_tick_inc(LV_TICK_PERIOD_MS);
}

/***   My functions below this line   ***/

void resizeWeight(char *w)
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
    //tft.setFont(FMB9);
    //tft.setTextSize(3);
    debugPrintln("size 3 Font");
  }
  else if (strlen(w) > 3)
  {
    //tft.setFont(FMB9);
    //tft.setTextSize(3);
    debugPrintln("size 3' Font");
  }
  else if (strlen(w) > 2)
  {
    //tft.setFont(FMB9);
    //tft.setTextSize(4);
    debugPrintln("size 4 Font");
  }
  else
  {
    //tft.setFont(FMB9);
    //tft.setTextSize(4);
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

void displayWeight(char *weight)
{
  char now[32];
  strcpy(now, weight);
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

#endif
#ifdef CONFIG_SB_V3_ST7735S
  lv_obj_set_width(bkgrnd, 160);
  lv_obj_set_height(bkgrnd, 80);
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

  // tft.setTextSize(6);
  // tft.fillScreen(ST77XX_BLACK);
  // tft.setCursor(0, 0);
  // tft.setTextColor(ST77XX_WHITE);
  // tft.print(weight);
  delay(1000);
  //canvas.println("");
  //tft.drawBitmap(10, 10, canvas.getBuffer(), 160, 80, ST77XX_WHITE, ST77XX_BLACK);
#endif
}

// Units
void displayUnits()
{
  //lv_obj_t *scr = lv_disp_get_scr_act(NULL);
  lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
  lv_obj_t *cont = lv_cont_create(bkgrnd, NULL);
  lv_obj_t *label1 = lv_label_create(cont, NULL);

  lv_cont_set_fit(cont, LV_FIT_PARENT);
  lv_cont_set_layout(cont, LV_LAYOUT_CENTER);

#ifdef CONFIG_SB_V1_HALF_ILI9341
#endif
#ifdef CONFIG_SB_V3_ST7735S
  lv_obj_set_width(bkgrnd, 160);
  lv_obj_set_height(bkgrnd, 80);
  lv_obj_align(bkgrnd, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
  
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
#endif
  xSemaphoreTake(systemMutex, (TickType_t)10);
  switch (_sys.eUnits)
  {
  case kg:
    xSemaphoreGive(systemMutex);
    lv_label_set_text(label1, "KG");

    break;
  case g:
    xSemaphoreGive(systemMutex);
    lv_label_set_text(label1, "G");

    break;
  case lb:
    xSemaphoreGive(systemMutex);
    lv_label_set_text(label1, "LB");

    break;
  case oz:
    xSemaphoreGive(systemMutex);
    lv_label_set_text(label1, "OZ");

    break;
  default:
    break;
  }
  
  lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
  lv_obj_add_style(cont, LV_OBJ_PART_MAIN, &transpCont);
  lv_obj_add_style(label1, LV_OBJ_PART_MAIN, &weightStyle);
}

// Settings
void displaySettings()
{
#ifdef CONFIG_SB_V1_HALF_ILI9341
#endif
#ifdef CONFIG_SB_V3_ST7735S
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
#endif
}

// Battery & Info
void displayDeviceInfo()
{
  // get device info from struct defined in main.cpp
  xSemaphoreTake(systemMutex, (TickType_t)10);
  char sn[9]; // = _sys.SN;
  strcpy(sn, _sys.SN);
  char ver[4]; //= _sys.VER;
  strcpy(ver, _sys.VER);
  xSemaphoreGive(systemMutex);

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
  lv_label_set_text_fmt(label3, "SUDOBOARD INFO\nSN: %s\n VER: %s", sn, ver); // , 5, 10); //"sn", "ver");
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
}

// Update
void displayUpdateScreen(int pct)
{
  debugPrintln("inside displayUpdateScreen");
  if (!updateStarted)
  {
    //flashing using whole screen as progress bar
    // canvas.setCursor(30, 30);
    // canvas.setTextSize(2);
    // canvas.print("Updating...");
    // // outline
    // canvas.drawFastHLine(10, 70, 120, ST7735_WHITE);
    // canvas.drawFastHLine(10, 30, 120, ST7735_WHITE);
    // canvas.drawFastVLine(10, 30, 40, ST7735_WHITE);
    // canvas.drawFastVLine(140, 30, 40, ST7735_WHITE);
    //tft.drawBitmap(0, 0, canvas.getBuffer(), 160, 80, ST77XX_WHITE, ST77XX_BLACK);
    updateStarted = !updateStarted;
  }
  pct = (int)pct * 126 / 100;
  //tft.fillRect(12, 45, pct, 24, ST7735_WHITE);
}

void displayLogo()
{
  lv_obj_t *bkgrnd = lv_obj_create(lv_scr_act(), NULL);
  lv_obj_t *img = lv_img_create(bkgrnd, NULL);
  
  lv_img_set_src(img, &img_logo_white);
#ifdef CONFIG_SB_V1_HALF_ILI9341
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
}

void displayBattery()
{
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

  xSemaphoreTake(systemMutex, (TickType_t)10);
  lv_label_set_text_fmt(label, "%d%%", _sys.batteryLevel);
  xSemaphoreGive(systemMutex);
  lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0,0);
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
#endif
 
  lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
  lv_obj_add_style(cont, LV_CONT_PART_MAIN, &transpCont);
  lv_obj_add_style(label, LV_CONT_PART_MAIN, &infoStyle);
}

void displayLowBattery()
{
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
#endif
 
  lv_obj_add_style(bkgrnd, LV_OBJ_PART_MAIN, &backgroundStyle);
  lv_obj_add_style(cont, LV_CONT_PART_MAIN, &transpCont);
  lv_obj_add_style(label, LV_CONT_PART_MAIN, &infoStyle);
}

void displayOff()
{
#ifdef CONFIG_SB_V1_HALF_ILI9341
#endif
#ifdef CONFIG_SB_V3_ST7735S
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
#endif
  lv_obj_clean(lv_scr_act());
  disp_flag = false;
  ledc_set_duty_and_update(ledc_c_config.speed_mode, ledc_c_config.channel, 0, 0);
}
void displayOn()
{
#ifdef CONFIG_SB_V1_HALF_ILI9341
#endif
#ifdef CONFIG_SB_V3_ST7735S
#endif
#ifdef CONFIG_SB_V6_FULL_ILI9341
#endif
  disp_flag = true;
  ledc_set_duty_and_update(ledc_c_config.speed_mode, ledc_c_config.channel, 100, 0);
}
