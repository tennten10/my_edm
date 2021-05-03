#include "a_config.h"
#include "globals.h"
//#include "PinDefs.h"
//#include "TelnetServer.h"
#include "esp_log.h"
//#include "driver/uart.h"
#include <cstring>
//#include <string.h>
#include <string>
#include <cstdio>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// debugState is defined in a_config for ease of use. 
// Is enum so it can be changed as a program variable rather than needing to be compiled again
// Not that anymore ^ defined from Kconfig files and menuconfig options
#ifdef CONFIG_SB_DEBUG_SERIAL
    DEBUG eDebugState = debugSERIAL;
#else
#ifdef CONFIG_SB_DEBUG_BT
    DEBUG eDebugState = debugBT;
#else 
#ifdef CONFIG_SB_DEBUG_TELNET
    DEBUG eDebugState = debugTELNET;
#else
    DEBUG eDebugState = PRODUCTION;
#endif
#endif
#endif

TaskHandle_t SerialMonitor_TH;
QueueHandle_t serialQueue;
extern TickType_t xBlockTime;
xSemaphoreHandle debugMutex;

// static const char *TAG = "MyModule";

//#ifdef __cplusplus
//extern "C"{
//#endif

/*void DebugSerialHandler_(void *pvParameters)
{
    char event[200];
    for (;;)
    {
        //if (uxQueueMessagesWaiting(serialQueue) > 0)
        //{
        //     xQueueReceive(serialQueue, &event, xBlockTime);

        //     //ESP_LOGV(TAG, "%s", event);
        //     printf("%s", event);
        // }
        vTaskDelay(10);
    }
}*/

void debugSetup()
{
   
    switch (eDebugState)
    {
    case debugSERIAL:

        debugMutex = xSemaphoreCreateMutex();
        printf("debugMutex mutex created \n");
        /*//ESP_LOGD(TAG, "Booting Serial Monitor...");
        serialQueue = xQueueCreate(75, 200);
        printf("creatied serialQueue. starting debugSerial Task\n");
        xTaskCreate(
            DebugSerialHandler_,    / Task function. */
            //"Serial Print Handler", /* String with name of task. */
            //20000,                  /* Stack size in words, not bytes. */
            ///NULL,                   /* Parameter passed as input of the task */
            //1,                      /* Priority of the task. */
            //&SerialMonitor_TH       /* Task handle. */
        //);
        
        break;
    //case debugBT:
    //SerialBT.println("Booting...");
    //break;
    case debugTELNET:
        // TODO: Hold off until this becomes useful again... Make sure WiFi is connected as first step
        //remote_log_init();
        break;
    default:
        break;
    }
}

void debugClose()
{
    switch (eDebugState)
    {
    case debugSERIAL:
        printf("Exiting Serial...");
        
        break;
    //case debugBT:
    //SerialBT.println("Exiting Bluetooth...");
    //SerialBT.flush();
    //SerialBT.end();
    //break;
    case debugTELNET:
        //TelnetStream2.println("bye bye");
        //TelnetStream2.flush();
        //TelnetStream2.stop();
        break;
    default:
        break;
    }
}

void debugPrint(const char* str)
{
    switch (eDebugState)
    {
    case debugSERIAL:
        static char doo[64];
        strcpy(doo, str);
        xSemaphoreTake(debugMutex, (TickType_t)20);
        printf("%s", doo);
        xSemaphoreGive(debugMutex);
        strcpy(doo, "");
        break;
    //case debugBT:
    //SerialBT.println(str);
    //break;
    case debugTELNET:
        //TelnetStream2.println(str);
        break;
    case PRODUCTION:
        break;
    default:
        break;
    }
}
void debugPrint(std::string str)
{
    switch (eDebugState)
    {
    case debugSERIAL:
        //Serial.print(str);
        //static char doo[64];
        //strcpy(doo, str.c_str());
        xSemaphoreTake(debugMutex, (TickType_t)20);
        printf("%s", str.c_str());
        xSemaphoreGive(debugMutex);
        //strcpy(doo, "");
        break;
    //case debugBT:
    //SerialBT.print(str);
    //break;
    case debugTELNET:
        //TelnetStream2.print(str);
        break;
    case PRODUCTION:
        break;
    default:
        break;
    }
}

void debugPrint(std::string * str)
{
    switch (eDebugState)
    {
    case debugSERIAL:
        //Serial.print(str);
        //static char doo[64];
        //strcpy(doo, str->c_str());
        xSemaphoreTake(debugMutex, (TickType_t)20);
        printf("%s", str->c_str());
        xSemaphoreGive(debugMutex);        
        //strcpy(doo, "");
        break;
    //case debugBT:
    //SerialBT.print(str);
    //break;
    case debugTELNET:
        //TelnetStream2.print(str);
        break;
    case PRODUCTION:
        break;
    default:
        break;
    }
}

void debugPrint(double str)
{   
    switch (eDebugState)
    {
    case debugSERIAL:
        
        xSemaphoreTake(debugMutex, (TickType_t)20);
        printf("%f", str);
        xSemaphoreGive(debugMutex);
        break;
    //case debugBT:

    case debugTELNET:
        //TelnetStream2.print(str);
        break;
    case PRODUCTION:
        break;
    default:
        break;
    }
}

void debugPrint(int i)
{

    switch (eDebugState)
    {
    case debugSERIAL:
        xSemaphoreTake(debugMutex, (TickType_t)20);
        printf("%d", i);
        xSemaphoreGive(debugMutex);        
        break;
    //case debugBT:

    case debugTELNET:
        //TelnetStream2.print(str);
        break;
    case PRODUCTION:
        break;
    default:
        break;
    }
}

void debugPrintln(std::string str)
{
    switch (eDebugState)
    {
    case debugSERIAL:
        
        xSemaphoreTake(debugMutex, (TickType_t)20);
        printf("%s\n", str.c_str());
        xSemaphoreGive(debugMutex);
        
        break;
    //case debugBT:
    //SerialBT.println(str);
    //break;
    case debugTELNET:
        //TelnetStream2.println(str);
        break;
    case PRODUCTION:
        break;
    default:
        break;
    }
}
void debugPrintln(std::string * str)
{
    switch (eDebugState)
    {
    case debugSERIAL:
        //Serial.print(str);
        xSemaphoreTake(debugMutex, (TickType_t)20);
        printf("%s\n", str->c_str());
        xSemaphoreGive(debugMutex);
        break;
    //case debugBT:
    //SerialBT.print(str);
    //break;
    case debugTELNET:
        //TelnetStream2.print(str);
        break;
    case PRODUCTION:
        break;
    default:
        break;
    }
}
void debugPrint(char * const str){
    switch (eDebugState)
    {
    case debugSERIAL:
        xSemaphoreTake(debugMutex, (TickType_t)20);
        printf("%s", str );
        xSemaphoreGive(debugMutex);   
        break;
    //case debugBT:
    //SerialBT.println(str);
    //break;
    case debugTELNET:
        //TelnetStream2.println(str);
        break;
    case PRODUCTION:
        break;
    default:
        break;
    }
}

void debugPrintln(double str)
{
    switch (eDebugState)
    {
    case debugSERIAL:
        xSemaphoreTake(debugMutex, (TickType_t)20);
        printf("%f\n", str);
        xSemaphoreGive(debugMutex);        break;
    case debugBT:
        //SerialBT.println(temp);
        break;
    case debugTELNET:
        //TelnetStream2.println(temp);
        break;
    case PRODUCTION:
        break;
    default:
        break;
    }
}

void debugPrintln(int i)
{

    switch (eDebugState)
    {
    case debugSERIAL:
        xSemaphoreTake(debugMutex, (TickType_t)20);
        printf("%d\n", i);
        xSemaphoreGive(debugMutex);        break;
    //case debugBT:

    case debugTELNET:
        //TelnetStream2.print(str);
        break;
    case PRODUCTION:
        break;
    default:
        break;
    }
}

void debugPrintln(const char* str)
{
    switch (eDebugState)
    {
        case debugSERIAL:
            
            xSemaphoreTake(debugMutex, (TickType_t)20);
            printf("%s\n", str);
            xSemaphoreGive(debugMutex);
         
            break;
        //case debugBT:
            //SerialBT.println(str);
            //break;
        case debugTELNET:
            //TelnetStream2.println(str);
            break;
        case PRODUCTION:
            break;
        default:
            break;
    }
}

void debugPrintln(char * const str){
    switch (eDebugState)
    {
    case debugSERIAL:
        xSemaphoreTake(debugMutex, (TickType_t)20);
        printf("%s\n", str);
        xSemaphoreGive(debugMutex);
        break;
    //case debugBT:
    //SerialBT.println(str);
    //break;
    case debugTELNET:
        //TelnetStream2.println(str);
        break;
    case PRODUCTION:
        break;
    default:
        break;
    }
}

//#ifdef __cplusplus
//}
//#endif