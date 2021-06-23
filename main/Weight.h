#ifndef WEIGHT_H
#define WEIGHT_H

/* #ifdef __cplusplus
extern "C"{
#endif */

#include "a_config.h"
#include "globals.h"
#include "Dense" //"Eigen/Dense"
#include <string>
#include "mySPIFFS.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/adc.h"
#include "math.h"
#include "driver/gpio.h"
#include "esp_adc_cal.h"


struct WeightStruct{
    double w1;
    double w2;
    double w3;
    double w4;
    double total;
};


/*   */ 

template <typename T>
class ThreadXb
{
public:
    ThreadXb( uint16_t _stackDepth, UBaseType_t _priority, const char *_name = "")
    {
        xTaskCreate(task, _name, _stackDepth, this, _priority, &this->taskHandle);
    }
    virtual ~ThreadXb(){
        vTaskDelete(taskHandle);
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
/*************************************/

class WeightX : public ThreadXb<WeightX>
{
public:
    WeightX() : ThreadXb{5000, 1, "WeightHandler"}                               
    {
        debugPrint("inside weight initialization: ");
        debugPrintln((int)esp_timer_get_time()/1000);
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(SG1,ADC_ATTEN_DB_11); // Will need to change the attenuation when the circuit gets upgraded to auto-ranging
        adc1_config_channel_atten(SG2,ADC_ATTEN_DB_11);
        adc1_config_channel_atten(SG3,ADC_ATTEN_DB_11);
        adc1_config_channel_atten(SG4,ADC_ATTEN_DB_11);

        // Retrieving calibration values from spiffs file
        getStrainGaugeParams(mK_sg1,mK_sg2, mK_sg3, mK_sg4 );

        // temporary calibration values
        mK_sg1(2,2) = 55.9099*1; // V/g
        mK_sg2(2,2) = 55.9099*1;
        mK_sg3(2,2) = 55.8559*1;
        mK_sg4(2,2) = 64.8649*1;

        //pinMode(enable_165, OUTPUT);
        //digitalWrite(enable_165, HIGH);

        
        debugPrintln("after weight initialization");
        
    }
    
    virtual ~WeightX(){
        vQueueDelete(weightQueue);
    }
    
    void tare();
    void sleepPreparation();
    void setWeightUpdateRate(int r); // update rate in milliseconds
    std::string getWeightStr();
    Units getLocalUnits(){
        return localUnits;
    }
    void setLocalUnits(Units u){
        localUnits = u;
        conversion = units2conversionfactor(u);
    }

    void CalibrateParameters();

    void runTheoreticalWeight(double a, double b, double c){
        theoreticalWeight(a,b,c);
    }
    void Main();

private:
    SemaphoreHandle_t sgMutex; 
    QueueHandle_t weightQueue; 
    //void setUnits(Units m);
    double getRawWeight();
    
    double ReadVoltage(adc1_channel_t pin);
    void readSensors();
    double getWeight();
    std::string truncateWeight(double d);
    

    double units2conversionfactor(Units u){
        double ret = 0.0;
        switch(u){
            case g:
                ret = 1.0;
            break;
            case kg:
                ret = 0.001;
            break;
            case oz:
                ret = 0.035274;
            break;
            case lb:
                ret = 0.0022046208235381;
            break;
            default:
                ret = 1.0;
            break;
        }
        return ret;
    }

    //Eigen::Array4d mRawWeight = Eigen::Array4d::Zero();
    Eigen::Array4d mTareOffset = Eigen::Array4d::Zero();
    //Eigen::Array4d mOutput = Eigen::Array4d::Zero();


    Eigen::Matrix3d mK_sg1 = Eigen::Matrix3d::Identity();
    Eigen::Matrix3d mK_sg2 = Eigen::Matrix3d::Identity();
    Eigen::Matrix3d mK_sg3 = Eigen::Matrix3d::Identity();
    Eigen::Matrix3d mK_sg4 = Eigen::Matrix3d::Identity();

    WeightStruct rawWeight = {0.,0.,0.,0.,0.}; // use sgMutex when using this
    WeightStruct output = {0.,0.,0.,0.,0.};

    double sg1 = 0.; // use sgMutex when using these
    double sg2 = 0.;
    double sg3 = 0.;
    double sg4 = 0.;
    double sg1_last = 0.;
    double sg2_last = 0.;
    double sg3_last = 0.;
    double sg4_last = 0.;

    float a = 0.5;

    double conversion = 300.0;
    double getUnitPrecision(Units local){
        //make sure this is consistent with truncateWeight
        switch(local){
            case g:
            return 0.1;
            case kg:
            return 0.001;
            case oz:
            return 0.1;
            case lb:
            return 0.01;
            default:
            return 0.1;
        }
    }

    Units localUnits;
    int weight_update_rate = WEIGHT_UPDATE_RATE;

    Eigen::Matrix2d theoreticalWeight(double g, double x, double y);
    void inverseWeight();
};


/* #ifdef __cplusplus
}
#endif */

#endif