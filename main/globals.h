#ifndef GLOBALS_H
#define GLOBALS_H

// #ifdef __cplusplus
// extern "C"{
// #endif
#include <string>
#include <cstring>
#include <iostream>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "status.h"
#include "configs/a_config.h"

// Data Type Definitions


typedef enum
{
    BOOT,
    SHUTDOWN,
    CALIBRATION,
    sUPDATE,
    STANDARD
} MODE;
typedef enum
{
    WEIGHTSTREAM,
    SETTINGS,
    INFO,
    UNITS,
    pUPDATE
} PAGE;


struct Device
{
    char SN[9];   // 8 digits
    char VER[32]; // starting at 0.1
};

struct Data
{
    //Units u;
    int intensity;
    int red;
    int green;
    int blue;
};


// #ifdef __cplusplus
// }
// #endif

#endif