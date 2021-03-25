#include "PinDefs.h"
#include "globals.h"

void strainGaugeSetup();
// read all 4 pins
void doStrainGaugeStuff();
//
//float convert(String from, String to);
//void readSensors();
void setUnits(Units m);
double getWeight();
double readVoltage(int pin);
void weightHandler_(void * pvParameters);
void tare();