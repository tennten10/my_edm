#include "a_config.h"
#include "globals.h"
#include "main.h"
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
#include "Eigen/Sparse"
#include "Weight.h"



QueueHandle_t weightQueue;
extern TickType_t xBlockTime;

SemaphoreHandle_t sgMutex;

double aSGs[4] = {0};

//weight update rate, seconds
#define WEIGHT_UPDATE_RATE 0.5

// defined in Globals.h
// typedef enum {g, kg, oz, lb} Units;

// double sg1 = 0;
// double sg2 = 0;
// double sg3 = 0;
// double sg4 = 0;
// double sg1_last = 0;
// double sg2_last = 0;
// double sg3_last = 0;
// double sg4_last = 0;

// // filter weight
// float a = 0.9;

// float conversion = 1.0;

double tareOffset=0.0;  //[] = {0.0, 0.0, 0.0, 0.0};



// Eigen::Vector4d mRawWeight = Eigen::Vector4d::Zero();
// Eigen::Vector4d mTareOffset = Eigen::Vector4d::Zero();
// Eigen::Vector4d mOutput = Eigen::Vector4d::Zero();


// Eigen::Matrix3d mK_sg1 = Eigen::Matrix3d::Identity();
// Eigen::Matrix3d mK_sg2 = Eigen::Matrix3d::Identity();
// Eigen::Matrix3d mK_sg3 = Eigen::Matrix3d::Identity();
// Eigen::Matrix3d mK_sg4 = Eigen::Matrix3d::Identity();



double WeightX::ReadVoltage(adc1_channel_t pin){
  // from https://github.com/G6EJD/ESP32-ADC-Accuracy-Improvement-function/blob/master/ESP32_ADC_Read_Voltage_Accurate.ino
  // Use for more accurate reading on ESP32 ADC
  double reading = 1.0*adc1_get_raw(pin); // Reference voltage is 3v3 so maximum reading is 3v3 = 4095 in range 0 to 4095
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

void WeightX::readSensors(){
  // read sensors Using dummy values until sensors are hooked up
  xSemaphoreTake(sgMutex, (TickType_t)10);
  sg1 = ReadVoltage(SG1);
  sg2 = ReadVoltage(SG2);
  sg3 = ReadVoltage(SG3);
  sg4 = ReadVoltage(SG4);
  char ch[36]={};
  sprintf(ch, "[%.5f,%.5f,%.5f,%.5f]", sg1,sg2,sg3,sg4);
  debugPrintln(ch);

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

std::string WeightX::truncateWeight( double d){
  //xSemaphoreTake(systemMutex, (TickType_t)10);
  //Units u = _sys.eUnits;
  //xSemaphoreGive(systemMutex);
  char str[16]="";
  switch(localUnits){
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

void WeightX::setUnits(Units m){
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

double WeightX::getRawWeight(){
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

double WeightX::getWeight(){
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

std::string WeightX::getWeightStr(){
  return truncateWeight(getWeight());;
}

void WeightX::tare(){
  readSensors();
  xSemaphoreTake(sgMutex, (TickType_t)10);
  //tareOffset = getRawWeight();
  //mTareOffset(0) = sg1;
  //mTareOffset(1) = sg2;
  //mTareOffset(2) = sg3;
  //mTareOffset(3) = sg4;
  mTareOffset = mRawWeight;
  xSemaphoreGive(sgMutex);
}

void WeightX::Main(){
  char doo[16];
  std::string foo;
  double lastWeight = 0.0;
  double currentWeight= 0.0;
  weightQueue = xQueueCreate(5, sizeof(uint32_t));
  sgMutex = xSemaphoreCreateMutex();
    
  for(;;){
    readSensors();
    currentWeight = getWeight();
    if(abs(currentWeight - lastWeight)>0.01){
        
      foo = truncateWeight(currentWeight); // this crashes. Debug from here...? Maybe not anymore?
      
      strcpy(doo, foo.c_str());
      
      xQueueSend(weightQueue, &doo, xBlockTime);
      debugPrintln("Doing weight stuff");
      debugPrintln(foo);
      lastWeight = currentWeight;
    }
    
    vTaskDelay(WEIGHT_UPDATE_RATE*1000);
  }
  
}


// Amplifier Gain
// differential voltage input -> voltage output
double amplifier(double dV){
  // double dV = 1;
  double Vout = 1.65+dV*(1+10000./5100.+2*10000./58.3);
  return Vout;
}
// voltage output -> differential voltage input
double inverseAmplifier(double Vout){
  double dV = (Vout - 1.65)/(1+10000./5100.+2*10000./58.3);
  return dV;
}

// // converts strain value to weight value for a single 
// // e -> w
// double conversion(double raw_value){
//   double h, w; // units: cm
//   double E_aluminum = 70.3*10^9; // Pa
//   double v_aluminum = 0.345;
//   #ifdef CONFIG_SB_V1_HALF_ILI9341
//   h  = .2;
//   w = 0.8;
//   #endif
//   #ifdef CONFIG_SB_V3_ST7735S
//   h = 0.25;
//   w = 1.0; 
//   #endif
//   #ifdef CONFIG_SB_V6_FULL_ILI9341
//   h = 0.2;
//   w = 1.0; 
//   // L = 18mm
//   #endif
//   double I = h^3*w/12; // cm^4

// }

// // w & (x,y) -> [w,w /n w,w] or all weights
// // origin is in center of device
// Eigen::Matrix2d WeightX::theoreticalWeight(double g, double x, double y){
//   // https://www.scirp.org/pdf/am_2015032417562679.pdf
//   Eigen::Matrix2d ret = Eigen::Zero();
  
//   // assuming all gauges are identical
//   #ifdef CONFIG_SB_V1_HALF_ILI9341
//   // Center of mass: ( millimeters )
// 	double X = 0.81;
// 	double Y = -11.19;
// 	double Z = -1.41;
//   // Note: had to switch coordinate system for this model
  
//   // strain gauge location, positive values ( millimeters )
//   double dX = 192.28
//   double dY = 109.05
//   double surfaceZ = 30.90;

//   // equations to solve, 
//   double Ex = 0; // sums of all reaction components
//   double Ey = 0; // sums of all reaction components
//   double Ez = Wself + g + Rsg1 + Rsg2 + Rsg3 + Rsg4;
//   double Mx = Wself*Y + g*y + (Rsg1+Rsg2)*dY  -(Rsg3 + Rsg4)*dY;
//   double My = Wself*X + g*x + (Rsg2+Rsg4)*dX  -(Rsg1 + Rsg3)*dX;


//   #endif
//   #ifdef CONFIG_SB_V3_ST7735S 
//   Eigen::Matrix4d matrix (1,1,1,1,
//                           Y, Y, -Y, -Y,
//                           X, -X, X, -X,
//                           );
//   #endif
//   #ifdef CONFIG_SB_V6_FULL_ILI9341

//   #endif
  
//   return ret;
// }

// void inverseWeight(){
//   // turns weight into voltage levels
//   // not sure if I need this... because I can compare to raw weight values

// }

// // Strain Gauge Calibration
// // Calibration matrix created from setup proecdure: 
// // Place 50g, 200g, 1000g, 5000g caligration weights 3x on scale 
// // at each of 4 locations directly above the positioning of the gauges
// // 
// /*
//  * physics:
//  * strain at point on beam
//  * 
//  * converted to strain gauge
//  * rated 2.0-2.2 gauge factor. Using 2.1. Defined as dR/R / e
//  * ideal force conversion
//  */

// /*   TODO: Create calibration system and matrix solution each time
//  *  Use function in mySPIFFS to write settings to file for retrieval 
//  *
// void Calibrate(float x, float y, float F){
//   // Calibration sequence. Input x,y coordinates from bottom edge and left edge
//   // transform into coordinate system around the center of pressure
//   FindCP();
//   [x, y] = transformCP(x, y);
//   // Solve into one force and one torque
//   Fz = F;
//   Mx = F*x;
//   My = F*y;
  
// }
// void FindCP(){
//   // do routine thing here
// }

// float transformCP(float x, float y){
//   // change these to get from FindCP()
//   float X_cp = 180;
//   float Y_cp = 110;

//   float x_c = x - X_cp;
//   float y_c = y - Y_cp;
//   return x_c, y_c;
// }

// void solve(){
//     BLA::Matrix<6> Input;
//     BLA::Matrix<6> Reaction;
//     BLA::Matrix<6,6> Calibration 
//         = 10^6/GE * a_ij F'
//         N / uV/v units
// }
// */

// (ex-e), gxy/2, gzx/2
// gxy/2, ey-e, gyz/2
// gzx/2,  gyz/2, ez-e

// sx + sy = E/(1-v)(ex+ey)

// (ex-e), gxy/2, 0
// gxy/2,  ey-e,  0
// 0,      0,     ez-e

// since sg is on only one axis we'll assume all on (y?) axis 

// if we want to combine it all into one, we'll transform them all into the same coordinate system then add them, but for now let's work individually

// SB prototype V1 and V3 have configurations tat looks like this  i-------------i
//                                                                 |  -       -  |
//                                                                 |  _       _  |   y
//                                                                 |_____________|   |__ x


// Strain value from gauge -> stress value from material properties - > force value from geometry
// So, V1


