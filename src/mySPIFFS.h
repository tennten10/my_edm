#ifndef _MYSPIFFS_H_
#define _MYSPIFFS_H_

#include "globals.h"
#include "eigen/Eigen/Dense"

WiFiStruct getActiveWifiInfo();
void setWiFiInfo(WiFiStruct wifi);
WiFiStruct defaultWiFiInfo();

bool saveStrainGaugeParams(const Eigen::Matrix3d *m0, const Eigen::Matrix3d *m1, const Eigen::Matrix3d *m2, const Eigen::Matrix3d *m3);
bool getStrainGaugeParams(const Eigen::Matrix3d& m0, const Eigen::Matrix3d& m1, const Eigen::Matrix3d& m2,const Eigen::Matrix3d& m3);

#endif