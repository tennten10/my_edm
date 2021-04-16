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
//include "PinDefs.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
#if defined CONFIG_LV_USE_DEMO_WIDGETS
#include "lv_examples/src/lv_demo_widgets/lv_demo_widgets.h"
#elif defined CONFIG_LV_USE_DEMO_KEYPAD_AND_ENCODER
#include "lv_examples/src/lv_demo_keypad_and_encoder/lv_demo_keypad_and_encoder.h"
#elif defined CONFIG_LV_USE_DEMO_BENCHMARK
#include "lv_examples/src/lv_demo_benchmark/lv_demo_benchmark.h"
#elif defined CONFIG_LV_USE_DEMO_STRESS
#include "lv_examples/src/lv_demo_stress/lv_demo_stress.h"
#else
#error "No demo application selected."
#endif
#endif

/*********************
 *      DEFINES
 *********************/
//#define TAG "demo"
#define LV_TICK_PERIOD_MS 1

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_tick_task(void *arg);
static void guiTask(void *pvParameter);
static void create_demo_application(void);

/**********************
 *  GLOBAL VARIABLES
 **********************/
TaskHandle_t displayHandler_TH;
extern QueueHandle_t weightQueue;
extern TickType_t xBlockTime;

SemaphoreHandle_t displayMutex;
extern SemaphoreHandle_t pageMutex;
extern SemaphoreHandle_t systemMutex;
extern struct System _sys;
static char currentWeight[32] = "0.0";
static bool updateStarted = 0;
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
  

/**********************
 *   
 **********************/

void displaySetup()
{

  /* If you want to use a task to create the graphic, you NEED to create a Pinned task
     * Otherwise there can be problem such as memory corruption and so on.
     * NOTE: When not using Wi-Fi nor Bluetooth you can pin the guiTask to core 0 */
  xTaskCreatePinnedToCore(guiTask, "gui", 4096 * 2, NULL, 0, NULL, 1);

  debugPrintln("Display thread created...");
  

  /*#ifdef DISPLAY_320x240_SPI_FULL
    ledcAttachPin(ledB, 0);
    ledcWrite(0, 100);
    #endif
    #ifdef DISPLAY_320x240_SPI_PARTIAL
    ledcAttachPin(ledB, 0);
    ledcWrite(0, 100);
    #endif
    #ifdef DISPLAY_120x80_I2C
    ledcAttachPin(ledB, 0);
    ledcWrite(0, 100);
    #endif*/
}

/* Creates a semaphore to handle concurrent call to lvgl stuff
 * If you wish to call *any* lvgl function from other threads/tasks
 * you should lock on the very same semaphore! */
SemaphoreHandle_t xGuiSemaphore;

static void guiTask(void *pvParameter)
{

  (void)pvParameter;
  xGuiSemaphore = xSemaphoreCreateMutex();

  lv_init();

  /* Initialize SPI or I2C bus used by the drivers */
  lvgl_driver_init();

  lv_color_t *buf1 = static_cast<lv_color_t*>(heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA));
  assert(buf1 != NULL);

 // This is my init stuff... might use it or not use it. Not sure yet.
  /*tft.initR(INITR_MINI160x80);  // Init ST7735S mini display
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(3);
    tft.setTextWrap(true);
    tft.print("Booting please wait...");
    delay(1000);*/

    ledc_timer_config_t ledc_timer{
      .speed_mode = LEDC_LOW_SPEED_MODE,    // timer mode
      .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
      .timer_num = LEDC_TIMER_1,            // timer index
      .freq_hz = 5000,                      // frequency of PWM signal
      .clk_cfg = LEDC_AUTO_CLK,             // Auto select the source clock
  };
  //ledc_timer_config_t test {}
  // Set configuration of timer0 for high speed channels
  ledc_timer_config(&ledc_timer);

  ledc_channel_config(&ledc_c_config);

  //ledc_set_duty(ledc_c_config.speed_mode, ledc_c_config.channel, 50);
  //ledc_update_duty(ledc_c_config.speed_mode, ledc_c_config.channel);
  ledc_set_duty_and_update(ledc_c_config.speed_mode, ledc_c_config.channel, 100, 0);

  /* Use double buffered when not working with monochrome displays */
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
  lv_color_t *buf2 = static_cast<lv_color_t*>(heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA));
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

  /* Create the demo application */
  create_demo_application();

  while (1)
  {
    /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Try to take the semaphore, call lvgl related function on success */
    if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
    {
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

static void create_demo_application(void)
{
  /* When using a monochrome display we only show "Hello World" centered on the
     * screen */
#if defined CONFIG_LV_TFT_DISPLAY_MONOCHROME || \
    defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_ST7735S

  /* use a pretty small demo for monochrome displays */
  /* Get the current screen  */
  lv_obj_t *scr = lv_disp_get_scr_act(NULL);

  /*Create a Label on the currently active screen*/
  lv_obj_t *label1 = lv_label_create(scr, NULL);

  /*Modify the Label's text*/
  lv_label_set_text(label1, "Hello\nworld");

  /* Align the Label to the center
     * NULL means align on parent (which is the screen now)
     * 0, 0 at the end means an x, y offset after alignment*/
  lv_obj_align(label1, NULL, LV_ALIGN_CENTER, 0, 0);
#else
  /* Otherwise we show the selected demo */

#if defined CONFIG_LV_USE_DEMO_WIDGETS
  lv_demo_widgets();
#elif defined CONFIG_LV_USE_DEMO_KEYPAD_AND_ENCODER
  lv_demo_keypad_encoder();
#elif defined CONFIG_LV_USE_DEMO_BENCHMARK
  lv_demo_benchmark();
#elif defined CONFIG_LV_USE_DEMO_STRESS
  lv_demo_stress();
#else
#error "No demo application selected."
#endif
#endif
}

static void lv_tick_task(void *arg)
{
  (void)arg;

  lv_tick_inc(LV_TICK_PERIOD_MS);
}

/***   My functions below this line   ***/

void resizeWeight(char *w)
{

#ifdef DISPLAY_320x240_SPI_FULL
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

#ifdef DISPLAY_120x80_I2C
  //tft.textSize();
  //tft.setCursor();

  if (strlen(w) > 4)
  { // TODO: Figure out proper font sizes for this resizing function
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
}

void displayWeight(char *weight)
{
  char now[32];
  strcpy(now, weight);
  resizeWeight(weight);
  //tft.setTextColor(TFT_WHITE, TFT_BLACK);

#ifdef DISPLAY_320x240_SPI_FULL
  //tft.setTextDatum(MC_DATUM);
  //tft.setFreeFont(FF22);
  //tft.setFont(6);
  //tft.setTextSize(8);
  //tft.drawFloat(now, 1, 80, 120, 4);
  //tft.fillScreen(TFT_BLACK);
  //tft.drawString(now, 80, 120);
#endif
#ifdef DISPLAY_120x80_I2C

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
  xSemaphoreTake(systemMutex, (TickType_t)10);
  switch (_sys.eUnits)
  {
  case kg:
    xSemaphoreGive(systemMutex);
#ifdef DISPLAY_320x240_SPI_FULL
    //tft.fillScreen(TFT_BLACK);
    //tft.drawString("kg", 160, 180, 2);
#endif
#ifdef DISPLAY_320x240_SPI_PARTIAL
    //tft.fillScreen(TFT_BLACK);
    //tft.drawString("kg", 160, 180, 2);
#endif
#ifdef DISPLAY_120x80_I2C
    //tft.fillScreen(TFT_BLACK);
    //tft.drawString("kg",40,60,2);
#endif

    break;
  case g:
    xSemaphoreGive(systemMutex);
#ifdef DISPLAY_320x240_SPI_FULL
    //tft.fillScreen(TFT_BLACK);
    //tft.drawString("g", 160, 180, 2);
#endif
#ifdef DISPLAY_320x240_SPI_PARTIAL
    //tft.fillScreen(TFT_BLACK);
    //tft.drawString("g", 160, 180, 2);
#endif
#ifdef DISPLAY_120x80_I2C
    //tft.fillScreen(TFT_BLACK);
    //tft.drawString("g",40,60,2);
#endif
    break;
  case lb:
    xSemaphoreGive(systemMutex);
#ifdef DISPLAY_320x240_SPI_FULL
    //tft.fillScreen(TFT_BLACK);
    //tft.drawString("lb", 160, 180, 2);
#endif
#ifdef DISPLAY_320x240_SPI_PARTIAL
    //tft.fillScreen(TFT_BLACK);
    //tft.drawString("lb", 160, 180, 2);
#endif
#ifdef DISPLAY_120x80_I2C
    //tft.fillScreen(TFT_BLACK);
    //tft.drawString("lb",40,60,2);
#endif
    break;
  case oz:
    xSemaphoreGive(systemMutex);
#ifdef DISPLAY_320x240_SPI_FULL
    //tft.fillScreen(TFT_BLACK);
    //tft.drawString("oz", 160, 180, 2);
#endif
#ifdef DISPLAY_320x240_SPI_PARTIAL
    //tft.fillScreen(TFT_BLACK);
    //tft.drawString("oz", 160, 180, 2);
#endif
#ifdef DISPLAY_120x80_I2C
    //tft.fillScreen(TFT_BLACK);
    //tft.setTextSize(3);
    //tft.setCursor(0, 0);
    //tft.print("oz");
#endif

    break;
  }
}

// Settings
void displaySettings()
{
}

// Battery & Info
void displayDeviceInfo()
{

#ifdef DISPLAY_320x240_SPI_FULL
  // tft.fillScreen(TFT_BLACK);
  // tft.drawString("INFO", 160, 180, 2);
  // delay(500);
  // tft.fillScreen(TFT_BLACK);
  // tft.drawString("something", 160, 180, 2);
#endif
#ifdef DISPLAY_120x80_I2C
  //tft.setTextDatum(TL_DATUM);
  //tft.setFreeFont(FMB18);
  //tft.fillScreen(TFT_BLACK);
  //tft.drawString("1000", 80, 120);
#endif
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
  //
}

void displayBattery()
{
#ifdef DISPLAY_320x240_SPI_FULL
  // tft.fillScreen(TFT_BLACK);
  // tft.drawString("Bat: ", 160, 180, 2);
  // xSemaphoreTake(systemMutex, (TickType_t)10);
  // tft.drawFloat((float)sys.batteryLevel, 0, 160, 180, 2);
  // xSemaphoreGive(systemMutex);
#endif
#ifdef DISPLAY_320x240_SPI_PARTIAL
  // tft.fillScreen(TFT_BLACK);
  // tft.drawString("Bat: ", 160, 180, 2);
  // xSemaphoreTake(systemMutex, (TickType_t)10);
  // tft.drawFloat((float)sys.batteryLevel, 0, 160, 180, 2);
  // xSemaphoreGive(systemMutex);
#endif
#ifdef DISPLAY_160x80_I2C
  // canvas.drawBitmap(0, 0, SPIFFS.open("/Battery.bmp"), 160, 80, ST77xx_BLACK);

  // canvas.setCursor(30, 30);
  // canvas.setTextSize(3);
  // xSemaphoreTake(systemMutex, (TickType_t)10);
  // canvas.print(sys.batteryLevel);
  // xSemaphoreGive(systemMutex);
  //tft.drawBitmap(0, 0, canvas.getBuffer(), 160, 80, ST77XX_WHITE, ST77XX_BLACK);
#endif
}

void displayLowBattery()
{
#ifdef DISPLAY_320x240_SPI_FULL
  //tft.fillScreen(TFT_BLACK);
  // do something
#endif
#ifdef DISPLAY_160x80_I2C
  //tft.fillScreen(TFT_BLACK);
  //tft.drawBitmap(0, 0, SPIFFS.open("/Low_Battery.bmp"), 160, 80, ST77XX_WHITE, ST77XX_BLACK);

#endif
}

void displayOff()
{
  //tft.fillScreen(TFT_BLACK);

  ledc_set_duty_and_update(ledc_c_config.speed_mode, ledc_c_config.channel, 0, 0);
}

void displayHandler_(void *pvParameters)
{
  displayWeight(currentWeight);

  int battery = 100;
  long t = esp_timer_get_time() / 1000;
  int q = 0;
  int q_last = 0;

  for (;;)
  {
    // Look at if the screen should change from button interactions
    xSemaphoreTake(pageMutex, (TickType_t)10);
    switch (ePage)
    { //typedef enum{WEIGHTSTREAM, SETTINGS, INFO, UNITS, pUPDATE} PAGE;
    case WEIGHTSTREAM:
      xSemaphoreGive(pageMutex);
      //debugPrintln(String(uxQueueMessagesWaiting(weightQueue)));
      if (uxQueueMessagesWaiting(weightQueue) > 0)
      {
        xQueueReceive(weightQueue, &currentWeight, xBlockTime);

        //displayWeight(currentWeight);
        displayWeight(currentWeight);
        //if (isBtConnected())
        //{
          //updateBTWeight(currentWeight);
        //}
        debugPrintln("This is where weight would be printed");
      }
      break;
    case pUPDATE:
      xSemaphoreGive(pageMutex);

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
      displayDeviceInfo();
      break;
    case UNITS:
      xSemaphoreGive(pageMutex);
      displayUnits();
      break;
    case SETTINGS:
      xSemaphoreGive(pageMutex);
      displaySettings();
      break;
    default:
      xSemaphoreGive(pageMutex);
      break;
    }
    // If the current task needs periodic updates do that here

    // Low battery warning
    xSemaphoreTake(systemMutex, (TickType_t)10);
    battery = _sys.batteryLevel;
    xSemaphoreGive(systemMutex);

    if (battery < 20 && ((esp_timer_get_time() / 1000) - t > 10000))
    {
      displayLowBattery();
      vTaskDelay(1000);
      t = esp_timer_get_time() / 1000;
    }

    vTaskDelay(500);
  }
}
