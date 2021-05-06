#ifndef WEIGHT_H
#define WEIGHT_H

/* #ifdef __cplusplus
extern "C"{
#endif */

#include "a_config.h"
#include "globals.h"
#include "Eigen/Sparse"
#include <string>

void strainGaugeSetup();
// read all 4 pins
//void doStrainGaugeStuff();
//
//float convert(String from, String to);
//void readSensors();
//void setUnits(Units m);
//double getWeight();
//double readVoltage(int pin);
//void weightHandler_(void * pvParameters);
//void tare();

/*   */ 

template <typename T>
class ThreadXb
{
public:
    ThreadXb( uint16_t _stackDepth, UBaseType_t _priority, const char *_name = "")
    {
        xTaskCreate(task, _name, _stackDepth, this, _priority, &this->taskHandle);
    }

    TaskHandle_t GetHandle()
    {
        return this->taskHandle;
    }
    void Main()
    {
        static_cast<T&>(*this).Main();
    }

private:
    static void task(void *_params)
    {
        ThreadXb *p = static_cast<ThreadXb*>(_params);
        p->Main();
    }

    TaskHandle_t taskHandle;
};

class WeightX : public ThreadXb<WeightX>
{
public:
    WeightX() : ThreadXb{5000, 1, "ButtonHandler"}                               
    {
        //Main();
    }
    ~WeightX(){} 
    
    void tare();
    void sleepPreparation();
    void setWeightUpdateRate(int r); // update rate in milliseconds
    double getWeight();
    void Main();

private:
    
    void setUnits(Units m);
    double getWeight();
    double getRawWeight();
    double readVoltage(int pin);
    double ReadVoltage(adc1_channel_t pin);
    void readSensors();
    std::string truncateWeight(double d);
    //QueueHandle_t weightQueue;

    Eigen::Matrix3d mK_sg1;
    Eigen::Matrix3d mK_sg2;
    Eigen::Matrix3d mK_sg3;
    Eigen::Matrix3d mK_sg4;

    double sg1 = 0;
    double sg2 = 0;
    double sg3 = 0;
    double sg4 = 0;
    double sg1_last = 0;
    double sg2_last = 0;
    double sg3_last = 0;
    double sg4_last = 0;

    float a = 0.9;

    float conversion = 1.0;
};


/* #ifdef __cplusplus
}
#endif */

#endif