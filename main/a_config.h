
// uncomment one of the below. small = 11x16, large = 18x24
//#define SMALL
//#define LARGE

//uncomment one of the below 
//#define V1_BASE
//#define V3_FLIP
//#define V6_SIMPLE

// Display style options:
//  DISPLAY_320x240_SPI_FULL
//  DISPLAY_320x240_SPI_PARTIAL
//  DISPLAY_120x80_I2C
/*#ifdef V1_Base
SB_V1_HALF_ILI9341
  #define DISPLAY_320x240_SPI_PARTIAL
#endif
#ifdef V3_FLIP
SB_V3_ST7735S
  #define DISPLAY_120x80_I2C
#endif
#ifdef V6_SIMPLE
SB_V6_FULL_ILI9341
  #define DISPLAY_320x240_SPI_FULL
#endif*/

//Uncommenting one of these will initialize the program to one of these states. 
// TODO: Make functions to change the operations while the program is running
//#define SERIAL_DEBUG
// #define BT_DEBUG
// #define TELNET_DEBUG

#ifndef A_CONFIG_H
#define A_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sdkconfig.h"


#if defined CONFIG_SB_V1_HALF_ILI9341
#include "configs/PinDefs_V1.h"
// declare width and height of the display for use later
#define SB_HORIZ 320
#define SB_VERT 120
#elif defined CONFIG_SB_V3_ST7735S
#include "configs/PinDefs_V3.h"
// declare width and height of the display for use later
#define SB_HORIZ 160
#define SB_VERT 80
#elif defined CONFIG_SB_V6_FULL_ILI9341
#include "configs/PinDefs_V6.h"
// declare width and height of the display for use later
#define SB_HORIZ 320
#define SB_VERT 240
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*DISP_DRIVER_H*/