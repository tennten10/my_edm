#include "a_config.h"
#include "globals.h"
#include "debug.h"
#include "src/NimBLEDevice.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_NIMBLE_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

void preBLEsetup();
void BLEsetup(std::string SN, std::string Version, int battery, Units units, WiFiStruct w);

bool isBtConnected();
void BLEstop();

void updateBTWeight(std::string w);
void updateBTUnits(Units unit);
void updateBTBattery(int bat);
void updateBTStatus(int status);

void sendOnConnect(NimBLEServer *pServer);