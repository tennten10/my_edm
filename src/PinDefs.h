#include "a_config.h"

// Make sure to modify the TFT_eSPI setup document for each display type
// Pin Definitions
/*
 * IO0    n/c     Strapping pin for booting and flashing so easier to leave disconnected
 * IO1    n/c     TX pin so use for uploading stuff
 * IO2    n/c     Strapping pin for booting and flashing so easier to leave disconnected
 * IO3    n/c     RX pin so use for uploading stuff
 * IO4    LCD_MISO     g
 * IO5    n/c     Extended to extention header
 * IO12   n/c     Strapping pin for booting and flashing so easier to leave disconnected
 * IO13   but1    Generic button pulled up internally ---Add acutal functionality. Recommended to be light adjustment for display
 * IO14   but2     
 * IO15   LCD_SDA  also known as mosi, but bidirectional    
 * IO16   LCD_RS  SPI... TODO: find out what rs, sda, etc do 
 * IO17   LCD_RESET   SPI reset for LCD communication
 * IO18   LCD_CS  SPI chip select for LCD communication
 * IO19   but3    Generic button pulled up internally  --- Add actual funcitonality when decided
 * IO21   but4    Generic button pulled up internally   ---Add actual funcitonality when decided
 * IO22   On_LED  To control green led when on. Also extended to extention header position 5
 * IO23   ledR     
 * IO25   ledG    Monitor battery voltage with ADC through voltage divider
 * IO26   ledB    Also use for LCD led signaling  
 * IO27   enable165  
 * IO32   SG4     Extended to extention header position 8
 * IO33   batteryMonitor    PWM for Blue backlight
 * IO34   SG2    PWM for Green backlight // Change this in updated version since this can't do pwm
 * IO35   SG3    PWM for Red backlight   // Change this in updated version since this can't do pwm
 * IO6     LCD_SCK
 * IO39     SG1
 */


 #ifdef V3_FLIP
  #define SG1     ADC1_CHANNEL_3 // 39  // also called sensor_vn
  #define SG2     ADC1_CHANNEL_6 // 34  
  #define SG3     ADC1_CHANNEL_7 // 35
  #define SG4     ADC1_CHANNEL_4 // 32
  
  #define but1    GPIO_NUM_13    
  #define but2    GPIO_NUM_14
  #define but3    GPIO_NUM_19
  #define but4    GPIO_NUM_21
  
  #define ledB    26  // can also used for lcd backlight pwm
  #define ledG    25
  #define ledR    23
  
  #define batV    ADC1_CHANNEL_5 //GPIO_NUM_33 

  #define enable_165  27
  #define onLED   22
#endif

#ifdef V1_BASE
  #define SG1   0
  #define SG2   2
  #define SG3   4
  #define SG4   15
  
  #define but1  12
  #define but2  13
  #define but3  19
  #define but4  21
  
  #define batV  25
#endif

#ifdef V6_SIMPLE
  #define SG1     39  // also called sensor_vn
  #define SG2     34  
  #define SG3     35
  #define SG4     32
  
  #define but1    13    
  #define but2    14
  #define but3    19
  #define but4    21
  
  #define ledB    26  // can also used for lcd backlight pwm
  #define ledG    25
  #define ledR    23
  
  #define batV    33

  #define enable_165  27
  #define onLED   22
#endif

#ifdef DISPLAY_320x240_SPI_FULL
  //#define TFT_MISO 14
  //#define TFT_MOSI 6
  //#define TFT_SCLK 27
  //#define TFT_CS   18  // Chip select control pin
  //#define TFT_DC    16  // Data Command control pin
  //#define TFT_RST   17
#endif

#ifdef DISPLAY_320x240_SPI_PARTIAL
  //#define TFT_MISO 14
  //#define TFT_MOSI 6
  //#define TFT_SCLK 27
  //#define TFT_CS   18  // Chip select control pin
  //#define TFT_DC    16  // Data Command control pin
  //#define TFT_RST   17
#endif

#ifdef DISPLAY_120x80_I2C
  #define LCD_CS  18
  #define LCD_DC  16
  #define LCD_RESET 17
  #define LCD_MOSI  15  // aka SDA/SD0
  #define LCD_SCL 5

  //#define TFT_MISO 4
  //#define TFT_MOSI 15
  //#define TFT_SCLK 27
  //#define TFT_CS    18   // Chip select control pin
  //#define TFT_DC    16   // Data Command control pin
  //#define TFT_RST   17
#endif


//void pinSetup(){
  
//  pinMode(SG1, INPUT);
//  pinMode(SG2, INPUT);
//  pinMode(SG3, INPUT);
//  pinMode(SG4, INPUT);

/*  
  #ifdef flip
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_RS, OUTPUT);
  pinMode(LCD_RESET, OUTPUT);
  pinMode(LCD_SDA, OUTPUT);
  pinMode(LCD_SCL, OUTPUT);
  #endif
*/
  // Done in Display Backlight Setup
  //ledcAttachPin(ledR, 1);
  //ledcAttachPin(ledG, 2);
  //ledcAttachPin(ledB, 3);
  //ledcWrite(3, 100);

  //Might not need this since analogRead supposedly works fine
  //adc1_config_width(ADC_WIDTH_BIT_12);
  //adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0);
  //adc1_get_raw(ADC1_CHANNEL_0);
//}