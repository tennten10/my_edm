#ifndef GLOBALS_H
#define GLOBALS_H

#ifdef __cplusplus
extern "C"{
#endif

// Data Type Definitions
 
typedef enum {g, kg, oz, lb} Units;
typedef enum{BOOT, SHUTDOWN, CALIBRATION, sUPDATE, STANDARD } STATE; 
typedef enum{WEIGHTSTREAM, SETTINGS, INFO, UNITS, pUPDATE} PAGE;
typedef enum{ debugBT, debugSERIAL, debugTELNET, PRODUCTION} DEBUG;

struct WiFiStruct{
  int active;
  char ssid[32];
  char pswd[64];
};

struct System{
  const char SN[9];    // 8 digits
  const char VER[5];  // starting at 0.1  
  Units eUnits;
  volatile int batteryLevel; // should only be between 0 and 100
};


// Global Variable and Structures

//System _sys = {"10011001", "0.1", g, 80};
extern System _sys; //defined in main

//Main
extern STATE eState;

//Debug
// probably don't need this?
//extern DEBUG eDebugState;

//Main? used to be Display 
extern volatile PAGE ePage;
#ifdef __cplusplus
}
#endif

#endif  