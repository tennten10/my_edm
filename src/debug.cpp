#include "a_config.h"
#include "Globals.h"
//#include "TelnetServer.h"
#include "esp_log.h"
//#include "driver/uart.h"
//#include <cstring>
#include <string.h>
#include <string>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// debugState is defined in a_config for ease of use. 
// Is enum so it can be changed as a program variable rather than needing to be compiled again
//
#ifdef SERIAL_DEBUG
    DEBUG eDebugState = debugSERIAL;
#endif
#ifdef BT_DEBUG
    DEBUG eDebugState = debugBT;
#endif
#ifdef TELNET_DEBUG
    DEBUG eDebugState = debugTELNET;
#endif

TaskHandle_t SerialMonitor_TH;
QueueHandle_t serialQueue;
extern TickType_t xBlockTime;

static const char *TAG = "MyModule";

void DebugSerialHandler_(void *pvParameters)
{
    char event[200];
    for (;;)
    {
        if (uxQueueMessagesWaiting(serialQueue) > 0)
        {
            xQueueReceive(serialQueue, &event, xBlockTime);

            ESP_LOGI(TAG, "%s", event);
        }
        vTaskDelay(5);
    }
}
void debugSetup()
{
    switch (eDebugState)
    {
    case debugSERIAL:
        ESP_LOGD(TAG, "Booting Serial Monitor...");
        serialQueue = xQueueCreate(75, 200);
        xTaskCreate(
            DebugSerialHandler_,    /* Task function. */
            "Serial Print Handler", /* String with name of task. */
            20000,                  /* Stack size in words, not bytes. */
            NULL,                   /* Parameter passed as input of the task */
            1,                      /* Priority of the task. */
            &SerialMonitor_TH       /* Task handle. */
        );
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
        ESP_LOGI(TAG, "Exiting Serial...");
        
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
        xQueueSend(serialQueue, &doo, xBlockTime);
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
/*void debugPrint(std::string str)
{
    switch (eDebugState)
    {
    case debugSERIAL:
        //Serial.print(str);
        static char doo[64];
        strcpy(doo, str.c_str());
        xQueueSend(serialQueue, &doo, xBlockTime);
        strcpy(doo, "");
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
        static char doo[64];
        strcpy(doo, str.c_str());
        xQueueSend(serialQueue, &doo, xBlockTime);
        strcpy(doo, "");
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
}*/
void debugPrint(double str)
{
    char temp[32]; // malloc(sizeof(char) * (32 + 1));// = NULL;
    sprintf(temp, "%f", str);
    
    switch (eDebugState)
    {
    case debugSERIAL:
        xQueueSend(serialQueue, &temp, xBlockTime);
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
    char temp[16];// = NULL;
    sprintf(temp, "%d", i);
    
    switch (eDebugState)
    {
    case debugSERIAL:
        xQueueSend(serialQueue, &temp, xBlockTime);
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
        static char doo[64];
        strcpy(doo, str.c_str());
        strcat(doo, "\n");
        xQueueSend(serialQueue, &doo, xBlockTime);
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
void debugPrint(std::string * str)
{
    switch (eDebugState)
    {
    case debugSERIAL:
        //Serial.print(str);
        static char doo[64];
        strcpy(doo, str->c_str());
        strcat(doo, "\n");
        xQueueSend(serialQueue, &doo, xBlockTime);
        strcpy(doo, "");
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


void debugPrintln(double str)
{
    char temp[16];
    sprintf(temp, "%f\n", str);
    switch (eDebugState)
    {
    case debugSERIAL:
        xQueueSend(serialQueue, &temp, xBlockTime);
        break;
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
    char temp[16];// = NULL;
    sprintf(temp, "%d\n", i);
    
    switch (eDebugState)
    {
    case debugSERIAL:
        xQueueSend(serialQueue, &temp, xBlockTime);
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

void debugPrintln(const char* str)
{
    switch (eDebugState)
    {
    case debugSERIAL:
        static char doo[64];
        strcpy(doo, str);
        strcat(doo, "\n");
        xQueueSend(serialQueue, &doo, xBlockTime);
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