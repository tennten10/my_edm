#ifndef DISPLAY_H
#define DISPLAY_H
//#ifdef __cplusplus
//extern "C" {
//#endif


void displaySetup(); // yep
//void displayWeight(float weight);
void displayWeight(char* weight);
//void displayWeight(const char)
void displayUnits();
void displaySettings();
void displayDeviceInfo(); // 
void displayUpdateScreen(int pct);
void displayLogo();
void displayBattery();
void displayLowBattery();
void displayOff();
void displayOn();
void displaySleepPrep();
void pageEventCheck(long &t, int &q, int &q_last);
void pageTestRoutine(long t);
//void displayTask(void * pvParameters);
void menu();
void styleInit();


//#ifdef __cplusplus
//}
//#endif
#endif