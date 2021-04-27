#ifndef WEIGHT_H
#define WEIGHT_H

/* #ifdef __cplusplus
extern "C"{
#endif */

#include "a_config.h"
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

/* #ifdef __cplusplus
}
#endif */

#endif