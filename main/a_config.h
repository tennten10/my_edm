
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

//defines for all configurations
#define WEIGHT_UPDATE_RATE 1

// defines for individual ones
#if defined CONFIG_SB_V1_HALF_ILI9341
#include "configs/PinDefs_V1.h"
// declare width and height of the display for use later
#define SB_HORIZ 320
#define SB_VERT 240
#define UPDATE_URL "https://storage.googleapis.com/sb-firmware-00/firmware_v1.bin"
#define UPDATE_URL_META "https://storage.googleapis.com/storage/v1/b/sb-firmware-00/o/firmware_v1.bin"

#elif defined CONFIG_SB_V3_ST7735S
#include "configs/PinDefs_V3.h"
// declare width and height of the display for use later
#define SB_HORIZ 160
#define SB_VERT 80

// OTA update server
#define UPDATE_URL "https://storage.googleapis.com/sb-firmware-00/firmware_v3.bin"
#define UPDATE_URL_META "https://storage.googleapis.com/storage/v1/b/sb-firmware-00/o/firmware_v3.bin"



#elif defined CONFIG_SB_V6_FULL_ILI9341
#include "configs/PinDefs_V6.h"
// declare width and height of the display for use later
#define SB_HORIZ 320
#define SB_VERT 240
#define UPDATE_URL "https://storage.googleapis.com/sb-firmware-00/firmware_v6.bin"
#define UPDATE_URL_META "https://storage.googleapis.com/storage/v1/b/sb-firmware-00/o/firmware_v6.bin"
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /**/