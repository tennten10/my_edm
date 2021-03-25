#include "globals.h"
#include "a_config.h"
//#include <SPI.h>
#include "debug.h"
#include "PinDefs.h"
//#include "IOTComms.h"
//#include "FreeFonts.h"
//#include <TFT_eSPI.h>
//#include <Adafruit_GFX.h>    // Core graphics library
//#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
//#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789

#include <string>
#include "lvgl.h"

#include "lvgl/src/lvgl.h"
#include "lvgl_esp32_drivers/lvgl_tft/st7735s.h"
//#include "lv_conf_.h"
// <SPI.h>
//#include "AWS_OTA.h"

// // Globals
// #ifdef DISPLAY_320x240_SPI_FULL
//   TFT_eSPI tft = TFT_eSPI();
// #endif
// #ifdef DISPLAY_120x80_I2C
//   //Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
//   GFXcanvas1 canvas(160, 80);
// #endif

// TaskHandle_t displayHandler_TH;
// extern QueueHandle_t weightQueue;
// extern TickType_t xBlockTime;
// extern SemaphoreHandle_t systemMutex;

// extern struct System sys;
// static char currentWeight[32]= "0.0";
// static bool updateStarted = 0;
// //Screens:
// // weight with constant updates
// // Update progress bar
// // battery low
// // Unit change
// // Settings to see undecided things
// //CurrentScreen eCurrentScreen = WeightStream;
// extern volatile PAGE ePage = WEIGHTSTREAM;
// Units currentUnits = kg;

// SemaphoreHandle_t pageMutex;

// void resizeWeight(char* w){
  
//   #ifdef DISPLAY_320x240_SPI_FULL
//     if(strlen(w)>4){
//       tft.setFont(3);
//     }else if (strlen(w) > 3){
//       tft.setFont(4);
//     }else if (strlen(w) > 2){
//       tft.setFont(5);
//     }else{
//       tft.setFont(6);
//     }
//   #endif
  
//   #ifdef DISPLAY_120x80_I2C
//     //tft.textSize();
//     //tft.setCursor();

    
//     if(strlen(w)>4){ // TODO: Figure out proper font sizes for this resizing function
//       //tft.setFont(FMB9);
//       tft.setTextSize(3);
//       debugPrintln("size 3 Font");
//     }else if (strlen(w) > 3){
//       //tft.setFont(FMB9);
//       tft.setTextSize(3);
//       debugPrintln("size 3' Font");
//     }else if (strlen(w) > 2){
//       //tft.setFont(FMB9);
//       tft.setTextSize(4);
//       debugPrintln("size 4 Font");
//     }else{
//       //tft.setFont(FMB9);
//       tft.setTextSize(4);
//       debugPrintln("size 4 default Font");
//     }
//   #endif

  
// }

// void displayWeight(char* weight){
//   char now[32];
//   strcpy(now, weight);
//   resizeWeight(weight);
//   //tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
//   #ifdef DISPLAY_320x240_SPI_FULL
//     tft.setTextDatum(MC_DATUM);
//     tft.setFreeFont(FF22);
//     //tft.setFont(6);
//     //tft.setTextSize(8);
//     //tft.drawFloat(now, 1, 80, 120, 4);
//     tft.fillScreen(TFT_BLACK);
//     tft.drawString(now, 80, 120);
//   #endif
//   #ifdef DISPLAY_120x80_I2C

//     tft.setTextSize(6);
//     tft.fillScreen(ST77XX_BLACK);
//     tft.setCursor(0, 0);
//     tft.setTextColor(ST77XX_WHITE);
//     tft.print(weight);
//     delay(1000);
//     //canvas.println("");
//     //tft.drawBitmap(10, 10, canvas.getBuffer(), 160, 80, ST77XX_WHITE, ST77XX_BLACK); 
//   #endif
// }


// // Units
// void displayUnits(){
//   xSemaphoreTake(systemMutex, (TickType_t)10);
//   switch (sys.eUnits){
//     case kg:
//     xSemaphoreGive(systemMutex);
//     #ifdef DISPLAY_320x240_SPI_FULL
//       tft.fillScreen(TFT_BLACK);
//       tft.drawString("kg",160,180,2);
//     #endif
//     #ifdef DISPLAY_320x240_SPI_PARTIAL
//       tft.fillScreen(TFT_BLACK);
//       tft.drawString("kg",160,180,2);
//     #endif
//     #ifdef DISPLAY_120x80_I2C
//       tft.fillScreen(TFT_BLACK);
//       //tft.drawString("kg",40,60,2);
//     #endif
    
//     break;
//     case g:
//     xSemaphoreGive(systemMutex);
//     #ifdef DISPLAY_320x240_SPI_FULL
//       tft.fillScreen(TFT_BLACK);
//       tft.drawString("g",160,180,2);
//     #endif
//     #ifdef DISPLAY_320x240_SPI_PARTIAL
//       tft.fillScreen(TFT_BLACK);
//       tft.drawString("g",160,180,2);
//     #endif
//     #ifdef DISPLAY_120x80_I2C
//       tft.fillScreen(TFT_BLACK);
//       //tft.drawString("g",40,60,2);
//     #endif
//     break;
//     case lb:
//     xSemaphoreGive(systemMutex);
//     #ifdef DISPLAY_320x240_SPI_FULL
//       tft.fillScreen(TFT_BLACK);
//       tft.drawString("lb",160,180,2);
//     #endif
//     #ifdef DISPLAY_320x240_SPI_PARTIAL
//       tft.fillScreen(TFT_BLACK);
//       tft.drawString("lb",160,180,2);
//     #endif
//     #ifdef DISPLAY_120x80_I2C
//       tft.fillScreen(TFT_BLACK);
//       //tft.drawString("lb",40,60,2);
//     #endif
//     break;
//     case oz:
//     xSemaphoreGive(systemMutex);
//     #ifdef DISPLAY_320x240_SPI_FULL
//       tft.fillScreen(TFT_BLACK);
//       tft.drawString("oz",160,180,2);
//     #endif
//     #ifdef DISPLAY_320x240_SPI_PARTIAL
//       tft.fillScreen(TFT_BLACK);
//       tft.drawString("oz",160,180,2);
//     #endif
//     #ifdef DISPLAY_120x80_I2C
//       tft.fillScreen(TFT_BLACK);
//       tft.setTextSize(3);
//       tft.setCursor(0,0);
//       tft.print("oz");
//     #endif
    
//     break;
//   }
// }

// // Settings
// void displaySettings(){
  
// }


// // Battery & Info
// void displayDeviceInfo(){
  
//   #ifdef DISPLAY_320x240_SPI_FULL
//     tft.fillScreen(TFT_BLACK);
//     tft.drawString("INFO",160,180,2);
//     delay(500);
//     tft.fillScreen(TFT_BLACK);
//     tft.drawString("something",160,180,2);
//   #endif
//   #ifdef DISPLAY_120x80_I2C
//     //tft.setTextDatum(TL_DATUM);
//     //tft.setFreeFont(FMB18);
//     //tft.fillScreen(TFT_BLACK);
//     //tft.drawString("1000", 80, 120);
//   #endif
// }

// // Update
// void displayUpdateScreen(int pct){
//   debugPrintln("inside displayUpdateScreen");
//   if(!updateStarted){
//     //flashing using whole screen as progress bar
//     canvas.setCursor(30,30);
//     canvas.setTextSize(2);
//     canvas.print("Updating...");
//     // outline
//     canvas.drawFastHLine(10,70, 120, ST7735_WHITE);
//     canvas.drawFastHLine(10,30, 120, ST7735_WHITE);
//     canvas.drawFastVLine(10,30, 40, ST7735_WHITE);
//     canvas.drawFastVLine(140,30, 40, ST7735_WHITE);
//     tft.drawBitmap(0, 0, canvas.getBuffer(), 160, 80, ST77XX_WHITE, ST77XX_BLACK);
//     updateStarted = !updateStarted;
//   }
//   pct = (int)pct*126/100;
//   tft.fillRect( 12, 45, pct, 24, ST7735_WHITE);
// }

// void displayLogo(){
//   //
// }

// void displayBattery(){
//   #ifdef DISPLAY_320x240_SPI_FULL
//     tft.fillScreen(TFT_BLACK);
//     tft.drawString("Bat: ",160,180,2);
//     xSemaphoreTake(systemMutex,(TickType_t) 10);
//     tft.drawFloat((float)sys.batteryLevel,0,160,180,2);
//     xSemaphoreGive(systemMutex);
//   #endif
//   #ifdef DISPLAY_320x240_SPI_PARTIAL
//     tft.fillScreen(TFT_BLACK);
//     tft.drawString("Bat: ",160,180,2);
//     xSemaphoreTake(systemMutex,(TickType_t) 10);
//     tft.drawFloat((float)sys.batteryLevel,0,160,180,2);
//     xSemaphoreGive(systemMutex);
//   #endif
//   #ifdef DISPLAY_160x80_I2C
//     canvas.drawBitmap(0, 0, SPIFFS.open("/Battery.bmp"), 160, 80, ST77xx_BLACK);
    
//     canvas.setCursor(30,30);
//     canvas.setTextSize(3);
//     xSemaphoreTake(systemMutex,(TickType_t) 10);
//     canvas.print(sys.batteryLevel);
//     xSemaphoreGive(systemMutex);
//     tft.drawBitmap(0, 0, canvas.getBuffer(), 160, 80, ST77XX_WHITE, ST77XX_BLACK);
//   #endif
// }

// void displayLowBattery(){
//   #ifdef DISPLAY_320x240_SPI_FULL
//     tft.fillScreen(TFT_BLACK);
//     // do something
//   #endif
//   #ifdef DISPLAY_160x80_I2C
//     tft.fillScreen(TFT_BLACK);
//     tft.drawBitmap(0, 0, SPIFFS.open("/Low_Battery.bmp"), 160, 80, ST77XX_WHITE, ST77XX_BLACK); 
    
//   #endif
  
// }

// void displayOff(){
//   tft.fillScreen(TFT_BLACK);
//   ledcWrite(0, 0);
// }

// void displayHandler_(void * pvParameters){
//   displayWeight(currentWeight);
  
//   int battery = 100;
//   long t = millis();
//   int q = 0;
//   int q_last = 0;
  
//   for(;;){
//     // Look at if the screen should change from button interactions
//     xSemaphoreTake(pageMutex, (TickType_t)10);
//     switch(ePage){ //typedef enum{WEIGHTSTREAM, SETTINGS, INFO, UNITS, pUPDATE} PAGE;
//       case WEIGHTSTREAM:
//         xSemaphoreGive(pageMutex);
//         //debugPrintln(String(uxQueueMessagesWaiting(weightQueue)));
//         if(uxQueueMessagesWaiting(weightQueue) > 0){
//           xQueueReceive(weightQueue, &currentWeight, xBlockTime);
          
//           //displayWeight(currentWeight);
//           displayWeight(currentWeight);
//           if(isBtConnected()){
//             updateBTWeight(currentWeight);
//           }
//           debugPrintln("This is where weight would be printed");
            
//         }
//       break;
//       case pUPDATE:
//         xSemaphoreGive(pageMutex);
        
//         q = getUpdatePercent();
//         if(q < 3 && q != q_last){
//           displayUpdateScreen(3);
//           q_last = q;
//         }else if(q > q_last){
//           displayUpdateScreen(q);
//           q_last  = q;
//         }
//         // TODO: dynamically update percentage when downloading and inistalling updated code
//       break;
//       case INFO:
//         xSemaphoreGive(pageMutex);
//         displayDeviceInfo();
//       break;
//       case UNITS:
//         xSemaphoreGive(pageMutex);
//         displayUnits();
//       break;
//       case SETTINGS:
//         xSemaphoreGive(pageMutex);
//         displaySettings();
//       break;
//       default:
//       xSemaphoreGive(pageMutex);
//       break;
//     }
//     // If the current task needs periodic updates do that here


//     // Low battery warning
//     xSemaphoreTake(systemMutex, (TickType_t)10);
//     battery = sys.batteryLevel;
//     xSemaphoreGive(systemMutex);

//     if( battery < 20 && (millis()-t > 10000)){
//       displayLowBattery();
//       vTaskDelay(1000);
//       t = millis();
//     }
          
//     vTaskDelay(500);
//   }
// }

// void displaySetup(){
//   #ifdef DISPLAY_320x240_SPI_FULL
//     tft.init();
//     tft.setRotation(1);
//     tft.fillScreen(TFT_BLACK);
//     tft.fillScreen(TFT_WHITE);
//     tft.setTextColor(TFT_WHITE, TFT_BLACK);
//     //tft.setTextSize(8);
//   #endif
//   #ifdef DISPLAY_120x80_I2C
//     lv.init();

//     //change everything over...
//     tft.initR(INITR_MINI160x80);  // Init ST7735S mini display
//     tft.setRotation(1);
//     tft.fillScreen(ST77XX_BLACK);
//     tft.setCursor(0, 0);
//     tft.setTextColor(ST77XX_WHITE);
//     tft.setTextSize(3);
//     tft.setTextWrap(true);
//     tft.print("Booting please wait...");
//     delay(1000);
//   #endif
//   xTaskCreate(    
//         displayHandler_,          /* Task function. */
//         "Display Handler",        /* String with name of task. */
//         20000,            /* Stack size in words, not bytes. */
//         NULL,             /* Parameter passed as input of the task */
//         0,                /* Priority of the task. */
//         &displayHandler_TH              /* Task handle. */  
//         );  
//   debugPrintln("Display thread created...");

//   ledcSetup(0, 500, 8);
//   ledcAttachPin(ledB, 0);
//   ledcWrite(0, 100);
//   /*#ifdef DISPLAY_320x240_SPI_FULL
//   ledcAttachPin(ledB, 0);
//   ledcWrite(0, 100);
//   #endif
//   #ifdef DISPLAY_320x240_SPI_PARTIAL
//   ledcAttachPin(ledB, 0);
//   ledcWrite(0, 100);
//   #endif
//   #ifdef DISPLAY_120x80_I2C
//   ledcAttachPin(ledB, 0);
//   ledcWrite(0, 100);
//   #endif*/
// }