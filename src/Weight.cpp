//#include <BasicLinearAlgebra.h>

#include "PinDefs.h"
#include "globals.h"
#include "debug.h"
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
#include "eigen/Eigen/Dense"

TaskHandle_t weightHandler_TH;
QueueHandle_t weightQueue;
extern TickType_t xBlockTime;
extern SemaphoreHandle_t systemMutex;
SemaphoreHandle_t sgMutex;


//weight update rate, seconds
#define WEIGHT_UPDATE_RATE 0.5

// defined in Globals.h
// typedef enum {g, kg, oz, lb} Units;

double sg1 = 0;
double sg2 = 0;
double sg3 = 0;
double sg4 = 0;
double sg1_last = 0;
double sg2_last = 0;
double sg3_last = 0;
double sg4_last = 0;

// filter weight
float a = 0.9;

float conversion = 1.0;

double tareOffset=0.0;//[] = {0.0, 0.0, 0.0, 0.0};



Eigen::Vector4d mRawWeight = Eigen::Vector4d::Zero();
Eigen::Vector4d mTareOffset = Eigen::Vector4d::Zero();
Eigen::Vector4d mOutput = Eigen::Vector4d::Zero();

Eigen::Matrix3d mK_sg1 = Eigen::Matrix3d::Zero();
Eigen::Matrix3d mK_sg2 = Eigen::Matrix3d::Zero();
Eigen::Matrix3d mK_sg3 = Eigen::Matrix3d::Zero();
Eigen::Matrix3d mK_sg4 = Eigen::Matrix3d::Zero();



// BLA::Matrix<4> mRawWeight;
// BLA::Matrix<4> mTareOffset;
// BLA::Matrix<4> mOutput;


double ReadVoltage(adc1_channel_t pin){
  // from https://github.com/G6EJD/ESP32-ADC-Accuracy-Improvement-function/blob/master/ESP32_ADC_Read_Voltage_Accurate.ino
  // Use for more accurate reading on ESP32 ADC
  double reading = 1.0*adc1_get_raw(pin);; // Reference voltage is 3v3 so maximum reading is 3v3 = 4095 in range 0 to 4095
  if(reading < 1 || reading > 4095) return 0;
  // return -0.000000000009824 * pow(reading,3) + 0.000000016557283 * pow(reading,2) + 0.000854596860691 * reading + 0.065440348345433;
  return -0.000000000000016 * pow(reading,4) + 0.000000000118171 * pow(reading,3)- 0.000000301211691 * pow(reading,2)+ 0.001109019271794 * reading + 0.034143524634089;
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

void readSensors(){
  // read sensors Using dummy values until sensors are hooked up
  xSemaphoreTake(sgMutex, (TickType_t)10);
  sg1 = ReadVoltage(SG1);
  sg2 = ReadVoltage(SG2);
  sg3 = ReadVoltage(SG3);
  sg4 = ReadVoltage(SG4);
  debugPrint("[");
  debugPrint(std::string(sg1,5));
  debugPrint(",");
  debugPrint(std::string(sg2,5));
  debugPrint(",");
  debugPrint(std::string(sg3,5));
  debugPrint(",");
  debugPrint(std::string(sg4,5));
  debugPrintln("]");
  
  
  //averaging filter
  sg1 = (sg1* a) + (sg1_last * (1-a));
  sg2 = (sg2* a) + (sg2_last * (1-a));
  sg3 = (sg3* a) + (sg3_last * (1-a));
  sg4 = (sg4* a) + (sg4_last * (1-a));
  
  sg1_last = sg1;
  sg2_last = sg2;
  sg3_last = sg3;
  sg4_last = sg4;
  xSemaphoreGive(sgMutex);
  // 4095 0-3.3V
  // 0 and 0.1V, or between 3.2 and 3.3V
  // expect between 1.15 and 2.8V
}
std::string truncateWeight( double d){
  xSemaphoreTake(systemMutex, (TickType_t)10);
  Units u = _sys.eUnits;
  xSemaphoreGive(systemMutex);
  char str[16]="";
  switch(u){
    case g: // g.1
      sprintf(str, "%0.1f", d);
      return std::string(str);
    break;
    case kg: // kg.3?
      sprintf(str, "%0.3f", d);
      return std::string(str);
    break;
    case oz: // oz.1?
      sprintf(str, "%0.1f", d);
      return std::string(str);
    break;
    case lb: // lb.3?
      sprintf(str, "%0.1f", d);
      return std::string(str);
    break;
    default:
    debugPrintln("something wrong with truncate... made it to default");
    return "";
    break;
  }
  
  // on the big screen we can do maximum 5 characters? maybe, test it out
  // on the small screen we can do maximum 4 characters without changing fonts?
}

void setUnits(Units m){
  switch(m){
    // default system is in g
    case g: // grams
    conversion = 1;
    break;
    case kg: // kilograms
    conversion = 0.001;
    break;
    case oz: // oz
    conversion = 0.035274;
    break;
    case lb: // lbs
    conversion = 0.00220462;
    break;
    default:
    debugPrintln("Error: Units not set.");
    break;
  }
  //conversion *= 1000;
}

double getRawWeight(){
  // weight before conversions and tare offset
  xSemaphoreTake(sgMutex, (TickType_t)10);
  double weight = sg1 + sg2 + sg3+ sg4;
  
  mRawWeight(1) = sg1;
  mRawWeight(2) = sg2;
  mRawWeight(3) = sg3;
  mRawWeight(4) = sg4;
  xSemaphoreGive(sgMutex);
  return weight;
}

double getWeight(){
  //double weight = (getRawWeight() - tareOffset)* conversion;
  //xSemaphoreTake(systemMutex, (TickType_t)10);
  double weight = ((sg1-mTareOffset(0))*mK_sg1(2,2)+(sg2-mTareOffset(1))*mK_sg2(2,2)+(sg3-mTareOffset(2))*mK_sg3(2,2)+(sg4-mTareOffset(3))*mK_sg4(2,2))*conversion;
  /*xSemaphoreGive(sgMutex);
  debugPrint("weight: ");
  //double test = mTareOffset(0)*2.0;
  //debugPrintln(test);*/
  debugPrint( weight);
  return weight;
}

void tare(){
  tareOffset = getRawWeight();
  mTareOffset = {sg1, sg2, sg3, sg4};
}

void weightHandler_(void * pvParameters){
  char doo[16];
  std::string foo;
  double lastWeight = 0.0;
  double currentWeight= 0.0;
  for(;;){
    readSensors();
    currentWeight = getWeight();
    if(abs(currentWeight - lastWeight)>0.01){
     
      //if (asprintf (&foo, "%.5f", currentWeight) < 0){
      //  strcpy(foo, "0.000");
      //}
       
        //snprintf(foo, 16, "%.5f", currentWeight);
        
      foo = truncateWeight(currentWeight); // this crashes. Debug from here...
      
      strcpy(doo, foo.c_str());
      
      xQueueSend(weightQueue, &doo, xBlockTime);
      debugPrintln("Doing weight stuff");
      debugPrintln(foo);
      lastWeight = currentWeight;
    }
    vTaskDelay(WEIGHT_UPDATE_RATE*1000);
  }
  
}

void strainGaugeSetup(){
  
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(SG1,ADC_ATTEN_DB_11); // Will need to change the attenuation when the circuit gets upgraded to auto-ranging
  adc1_config_channel_atten(SG2,ADC_ATTEN_DB_11);
  adc1_config_channel_atten(SG3,ADC_ATTEN_DB_11);
  adc1_config_channel_atten(SG4,ADC_ATTEN_DB_11);
  
  
  getStrainGaugeParams(mK_sg1,mK_sg2, mK_sg3,mK_sg4 );

  //mRawWeight.Fill(0);
  //mTareOffset.Fill(0);
  //mOutput.Fill(0);

  sgMutex = xSemaphoreCreateMutex();
  
  mK_sg1(2,2) = 9.9099; // V/g
  mK_sg2(2,2) = 9.9099;
  mK_sg3(2,2) = 55.8559;
  mK_sg4(2,2) = 64.8649;
  
  //pinMode(enable_165, OUTPUT);
  //digitalWrite(enable_165, HIGH);
  
  vTaskDelay(300);
  xSemaphoreTake(systemMutex, (TickType_t)50);
  setUnits(_sys.eUnits);
  xSemaphoreGive(systemMutex);
  
  weightQueue = xQueueCreate(5, sizeof(uint32_t));
  
  if(weightQueue == NULL){
    debugPrintln("Error creating weightQueue");
  }else{
    debugPrintln("Weight queue created...");
  }
  
  xTaskCreate(    
        weightHandler_,          /* Task function. */
        "Weight Handler",        /* String with name of task. */
        20000,            /* Stack size in words, not bytes. */
        NULL,             /* Parameter passed as input of the task */
        0,                /* Priority of the task. */
        &weightHandler_TH              /* Task handle. */  
        );  
        
  debugPrintln("Weight thread created...");
  tare();

}

// Amplifier Gain
double amplifier(){
  double dV = 1;
  double Vout = 1.65+dV*(1+10000./5100.+2*10000./58.3);
  return Vout;
}


// Strain Gauge Calibration
// Calibration matrix created from setup proecdure: 
// Place 50g, 200g, 1000g, 5000g caligration weights 3x on scale 
// at each of 4 locations directly above the positioning of the gauges
// 
/*
 * physics:
 * strain at point on beam
 * 
 * converted to strain gauge
 * rated 2.0-2.2 gauge factor. Using 2.1. Defined as dR/R / e
 * ideal force conversion
 */

/*   TODO: Create calibration system and matrix solution each time
 *  Use function in mySPIFFS to write settings to file for retrieval 
 *
void Calibrate(float x, float y, float F){
  // Calibration sequence. Input x,y coordinates from bottom edge and left edge
  // transform into coordinate system around the center of pressure
  FindCP();
  [x, y] = transformCP(x, y);
  // Solve into one force and one torque
  Fz = F;
  Mx = F*x;
  My = F*y;
  
}
void FindCP(){
  // do routine thing here
}

float transformCP(float x, float y){
  // change these to get from FindCP()
  float X_cp = 180;
  float Y_cp = 110;

  float x_c = x - X_cp;
  float y_c = y - Y_cp;
  return x_c, y_c;
}

void solve(){
    BLA::Matrix<6> Input;
    BLA::Matrix<6> Reaction;
    BLA::Matrix<6,6> Calibration 
        = 10^6/GE * a_ij F'
        N / uV/v units
}
*/