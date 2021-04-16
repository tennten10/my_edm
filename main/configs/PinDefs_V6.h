#ifndef PIN_DEFS_H
#define PIN_DEFS_H

#ifdef __cplusplus
extern "C"{
#endif

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
#define SG1 39 // also called sensor_vn
#define SG2 34
#define SG3 35
#define SG4 32

#define but1 13
#define but2 14
#define but3 19
#define but4 21

#define ledB 26 // can also used for lcd backlight pwm
#define ledG 25
#define ledR 23

#define batV 33

#define enable_165 27
#define onLED 22
//#elif defined(CONFIG_LV_TFT_DISPLAY_CONTROLLER_ST7735S)
//  #define ledB "this works"

// #ifdef DISPLAY_120x80_I2C
//   #define LCD_CS  18
//   #define LCD_DC  16
//   #define LCD_RESET 17
//   #define LCD_MOSI  15  // aka SDA/SD0
//   #define LCD_SCL 5

//   //#define TFT_MISO 4
//   //#define TFT_MOSI 15
//   //#define TFT_SCLK 27
//   //#define TFT_CS    18   // Chip select control pin
//   //#define TFT_DC    16   // Data Command control pin
//   //#define TFT_RST   17
// #endif
#ifdef __cplusplus
}
#endif
#endif