//#include "a_config.h"
#include "globals.h"
#include "main.h"
#include "mySPIFFS.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"
#include "math.h"
#include <string>
#include <stdio.h>
//#include "Eigen/Dense"
#include "Dense"
#include "_adc.h"
#include <iostream>
#include "System.h"

extern SystemX *_sys;

double ADCX::ReadVoltage(adc1_channel_t pin)
{
    // from https://github.com/G6EJD/ESP32-ADC-Accuracy-Improvement-function/blob/master/ESP32_ADC_Read_Voltage_Accurate.ino
    // Use for more accurate reading on ESP32 ADC
    double reading = 1.0 * adc1_get_raw(pin); // Reference voltage is 3v3 so maximum reading is 3v3 = 4095 in range 0 to 4095
    if (reading < 1 || reading > 4095)
        return 0;
    // return -0.000000000009824 * pow(reading,3) + 0.000000016557283 * pow(reading,2) + 0.000854596860691 * reading + 0.065440348345433;
    return -0.000000000000016 * pow(reading, 4) + 0.000000000118171 * pow(reading, 3) - 0.000000301211691 * pow(reading, 2) + 0.001109019271794 * reading + 0.034143524634089;
} // Added an improved polynomial, use either, comment out as required
/* ADC readings v voltage
 *  y = -0.000000000009824x3 + 0.000000016557283x2 + 0.000854596860691x + 0.065440348345433
 // Polynomial curve match, based on raw data thus:
 *   464     0.5
 *  1088     1.0
 *  1707     1.5
 *  2331     2.0
 *  2951     2.5 
 *  3775     3.0
 *  
 */

void ADCX::readSensors()
{
    // read sensors Using dummy values until sensors are hooked up
    if (xSemaphoreTake(sgMutex, (TickType_t)10) == pdTRUE)
    {
        sg1 = ReadVoltage(SG1);
        sg2 = ReadVoltage(SG2);
        sg3 = ReadVoltage(SG3);
        sg4 = ReadVoltage(SG4);
        // char ch[36]={};
        // sprintf(ch, "[%.5f,%.5f,%.5f,%.5f]", sg1,sg2,sg3,sg4);
        // [//println(ch);

        //averaging filter
        sg1 = (sg1 * a) + (sg1_last * (1 - a));
        sg2 = (sg2 * a) + (sg2_last * (1 - a));
        sg3 = (sg3 * a) + (sg3_last * (1 - a));
        sg4 = (sg4 * a) + (sg4_last * (1 - a));

        sg1_last = sg1;
        sg2_last = sg2;
        sg3_last = sg3;
        sg4_last = sg4;
        xSemaphoreGive(sgMutex);
        // TODO: uncomment this when V3 is wired up fully. Otherwise spams the phone
        // if(isBtConnected()){
        //     if(sg1_last < 0.04 || sg1_last > 3.0){
        //     updateBTStatus(SB_STRAINGAUGE_1_ERROR);
        //     }
        //     if(sg2_last < 0.04 || sg2_last > 3.0){
        //     updateBTStatus(SB_STRAINGAUGE_2_ERROR);
        //     }
        //     if(sg3_last < 0.04 || sg3_last > 3.0){
        //     updateBTStatus(SB_STRAINGAUGE_3_ERROR);
        //     }
        //     if(sg4_last < 0.04 || sg4_last > 3.0){
        //     updateBTStatus(SB_STRAINGAUGE_4_ERROR);
        //     }
        // }
        
    }
    else
    {
        //println("could not get sgMutex in readSensors");
    }

    // 4095 0-3.3V
    // 0 and 0.1V, or between 3.2 and 3.3V
    // expect between 1.15 and 2.8V
}

std::string ADCX::truncateWeight(double d)
{
    //print("this is passed to truncate: ");
    //println(d);

    char str[16] = "";


    return "";



    // on the big screen we can do maximum 5 characters? maybe, test it out
    // on the small screen we can do maximum 4 characters without changing fonts?
}

double ADCX::getRawWeight()
{
    // weight before conversions and tare offset
    static double ret = 0;
    if (xSemaphoreTake(sgMutex, (TickType_t)10) == pdTRUE)
    {
        rawWeight.w1 = sg1 * mK_sg1(2, 2);
        rawWeight.w2 = sg2 * mK_sg2(2, 2);
        rawWeight.w3 = sg3 * mK_sg3(2, 2);
        rawWeight.w4 = sg4 * mK_sg4(2, 2);
        rawWeight.total = rawWeight.w1 + rawWeight.w2 + rawWeight.w3 + rawWeight.w4;
        ret = rawWeight.total;
        xSemaphoreGive(sgMutex);
    }
    else
    {
        //println("unable to get weight semaphore in getRawWeight. Returning last value?");
    }
    return ret;
}

double ADCX::getWeight()
{
    // adjusting for tare and units
    static double weight = 0;
    if (getRawWeight() > 0.0)
    { // just checking that it's returning something non-zero
        if (xSemaphoreTake(sgMutex, (TickType_t)10) == pdTRUE)
        {
            weight = (abs(rawWeight.w1 - mTareOffset(0)) + abs(rawWeight.w2 - mTareOffset(1)) + abs(rawWeight.w3 - mTareOffset(2)) + abs(rawWeight.w4 - mTareOffset(3))) * 10*conversion;
            xSemaphoreGive(sgMutex);
        }
        else
        {
            //println("could not get sgMutex in getWeight");
        }
    }
    //print("rawWeight: ");
    ////println(rawWeight.total);

    //print("weight: ");
    ////println( weight);

    return weight;
}

std::string ADCX::getWeightStr()
{
    char temp[16];
    // message already truncated to proper size for units
    int qWaiting = (int)uxQueueMessagesWaiting(adcQueue);

    if (qWaiting > 0)
    {
        xQueueReceive(adcQueue, &temp, (TickType_t)20);

        return std::string(temp);
    }
    return std::string("-1");
}

void ADCX::tare()
{
    //println("start of Tare. Raw weight: ");
    //println(getRawWeight());
    if (xSemaphoreTake(sgMutex, (TickType_t)50) == pdTRUE)
    {
        //println("after tare semaphore.");
        mTareOffset(0) = rawWeight.w1;
        mTareOffset(1) = rawWeight.w2;
        mTareOffset(2) = rawWeight.w3;
        mTareOffset(3) = rawWeight.w4;
        xSemaphoreGive(sgMutex);
    }
    else
    {
        //println("Could not get sgMutex in tare function");
    }

    //println("end of tare");
}

void ADCX::setWeightUpdateRate(int r)
{
    weight_update_rate = r;
}

void ADCX::sleepPreparation()
{
    // TODO: turn off low voltage regulator so it doesn't waste power, maybe deinitialize other stuff?

    return;
}

void ADCX::Main()
{
    //print("Weight main begin: ");
    //println((int)esp_timer_get_time() / 1000);
    char doo[16];
    char dump[16];
    std::string foo;
    double lastWeight = 0.0;
    double currentWeight = 0.0;
    adcQueue = xQueueCreate(7, 16 * sizeof(char));
    if (adcQueue == 0)
    {
        //println("________________FAILED TO CREATE ADCQUEUE_____________________________");
    }
    sgMutex = xSemaphoreCreateMutex();
    long t = esp_timer_get_time() / 1000;
    bool b = false;
    //print("Weight main end: ");
    //println((int)esp_timer_get_time() / 1000);
    for (;;)
    {
        ////println("adcLoop");
        readSensors();
        b = ((esp_timer_get_time() / 1000 - t) > (weight_update_rate * 1000));
        if (b)
        {
            currentWeight = getWeight();
            ////println(currentWeight);
            if (abs(currentWeight - lastWeight) > 0.1)
            {

                foo = truncateWeight(currentWeight);

                strcpy(doo, foo.c_str());
                
                if (uxQueueMessagesWaiting(adcQueue) > 4)
                {

                    xQueueReceive(adcQueue, &dump, (TickType_t)20);
                    strcpy(dump, "");
                }
                xQueueSend(adcQueue, &doo, (TickType_t)0);
                lastWeight = currentWeight;
            }
            _sys->callbackFlag = true;
            t = esp_timer_get_time() / 1000;
        }

        vTaskDelay(15);
    }
}

// Amplifier Gain
// differential voltage input -> voltage output
double amplifier(double dV)
{
    // double dV = 1;
    double Vout = 1.65 + dV * (1 + 10000. / 5100. + 2 * 10000. / 58.3);
    return Vout;
}
// voltage output -> differential voltage input
double inverseAmplifier(double Vout)
{
    double dV = (Vout - 1.65) / (1 + 10000. / 5100. + 2 * 10000. / 58.3);
    return dV;
}

// converts strain value to weight value for a single
// e -> w
double conversion(double raw_value)
{
    double ret = 0;
    double h, w;                           // units: cm
    double E_aluminum = 70.3 * pow(10, 9); // Pa
    double v_aluminum = 0.345;

    h = .2;
    w = 0.8;


    double I = pow(h, 3) * w / 12.0; // cm^4
    return ret;
}

// w & (x,y) -> [w,w /n w,w] or all weights
// origin is in center of device
Eigen::Matrix2d ADCX::theoreticalWeight(double g, double x, double y)
{
    // https://www.scirp.org/pdf/am_2015032417562679.pdf
    //or...
    //https://ntrs.nasa.gov/api/citations/20040045162/downloads/20040045162.pdf
    Eigen::Matrix2d ret = Eigen::Matrix2d::Identity();

// assuming all gauges are identical

    // Center of mass: ( millimeters )
    double CMx = 0.81;
    double CMy = -11.19;
    double CMz = -1.41;
    // Note: had to switch coordinate system for this model

    // strain gauge location, positive values ( millimeters )
    double X = 192.28;
    double Y = 109.05;
    double surfaceZ = 30.90;

    // equations to solve,
    // double Ex = 0; // sums of all reaction components
    // double Ey = 0; // sums of all reaction components
    // double Ez = Wself + g + Rsg1 + Rsg2 + Rsg3 + Rsg4;
    // double Mx = Wself*Y + g*y + (Rsg1+Rsg2)*dY  -(Rsg3 + Rsg4)*dY;
    // double My = Wself*X + g*x + (Rsg2+Rsg4)*dX  -(Rsg1 + Rsg3)*dX;



    return ret;
}

