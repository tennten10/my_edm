#include "a_config.h"
#include "globals.h"
#include "debug.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_NIMBLE_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif


void BLEsetup(std::string SN, std::string Version, int battery, Units units, WiFiStruct w);
void updateBTWeight(std::string w);
bool isBtConnected();
void BLEstop();
void BLESleepPrep();