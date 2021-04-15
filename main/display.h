#ifndef DISPLAY_H
#define DISPLAY_H
#ifdef __cplusplus

extern "C" {
#endif


void displaySetup();
void displayWeight(float weight);
void displayUnits();
void displaySettings();
void displayDeviceInfo();
void displayUpdateScreen();
void displayLogo();
void displayBattery();
void displayOff();

void displayHandler(void * pvParameters);
void menu();


#ifdef __cplusplus
}
#endif
#endif