#ifndef GLOBALS_H
#define GLOBALS_H

// #ifdef __cplusplus
// extern "C"{
// #endif
#include <string>
#include <cstring>
// Data Type Definitions
 
typedef enum {g, kg, oz, lb} Units;
typedef enum{BOOT, SHUTDOWN, CALIBRATION, sUPDATE, STANDARD } MODE; 
typedef enum{WEIGHTSTREAM, SETTINGS, INFO, UNITS, pUPDATE} PAGE;
typedef enum{ debugBT, debugSERIAL, debugTELNET, PRODUCTION} DEBUG;

struct WiFiStruct{
  int active; // = 0;
  char ssid[32]; // = {""};
  char pswd[64]; // = {""};

  WiFiStruct() : active(0), ssid(""), pswd(""){}
  WiFiStruct(int act, std::string ss, std::string ps ){
    active = act;
    strcpy(ssid, ss.c_str());
    strcpy(pswd, ps.c_str());
  }
  
};

struct Device{
  char SN[9];    // 8 digits
  char VER[32];  // starting at 0.1  
};


// Global Variable and Structures

//System _sys = {"10011001", "0.1", g, 80};
//extern System _sys; //defined in main

//Main
//extern STATE eState;

//Debug
// probably don't need this?
//extern DEBUG eDebugState;

//Main? used to be Display 
//extern volatile PAGE ePage;
// #ifdef __cplusplus
// }
// #endif

#endif  