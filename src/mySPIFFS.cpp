#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "stdio.h"
#include <string>
#include "esp_wifi.h"

#include "mySPIFFS.h"
#include "globals.h"
#include "debug.h"
#include "eigen/Eigen/Dense"

//#define DEFAULT_SCAN_LIST_SIZE CONFIG_EXAMPLE_SCAN_LIST_SIZE

WiFiStruct getActiveWifiInfo(){
    WiFiStruct wfi[20];
    debugPrintln("Initializing SPIFFS");
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            debugPrintln("Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            debugPrintln("Failed to find SPIFFS partition");
        } else {
            debugPrint("Failed to initialize SPIFFS (");
            debugPrint(ret);
            debugPrint(")");
        }
        return (WiFiStruct){0,"",""};
    }
    debugPrintln("Opening file");
    int i = 0;
    int j = -1;
    FILE* f = fopen("/spiffs/config.txt", "r"); 
    if (f == NULL) {
        debugPrintln("Failed to open file for reading");
        return (WiFiStruct){0,"",""};
    }


    //get next line
    char line[100];
    
    // populate from text file into struct for easier processing 
    while(!feof(f)){
        if(i > 20){
            debugPrint("too many entries in wifi text file");
            break;
        }
        fgets(line, sizeof(line), f);
        wfi[i].active = atoi(strtok(line, ","));
        strcpy(wfi[i].ssid, strtok(NULL, ","));
        strcpy(wfi[i].pswd, strtok(NULL, ","));
        if(wfi[i].active == 1){
            j = i;
        }
        i++;
    }
    fclose(f);
    debugPrint(i+1);
    debugPrintln(" stored networks");
    if(j){
        debugPrint(wfi[j].ssid);
        debugPrintln(" is active");
        return wfi[j];
    }else{
        debugPrintln("No Active Networks");
        return (WiFiStruct){0,"",""};
    }
}

void setWiFiInfo(WiFiStruct wifi){
    WiFiStruct wfi[20];
    int matchedIndex = -1;
    int i = 0;
    debugPrintln("Initializing SPIFFS");
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            debugPrintln("Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            debugPrintln("Failed to find SPIFFS partition");
        } else {
            debugPrint("Failed to initialize SPIFFS (");
            debugPrint(ret);
            debugPrint(")");
        }
        return;
    }
    debugPrintln("Opening file");
  
    FILE* f = fopen("/spiffs/config.txt", "r"); 
    if (f == NULL) {
        debugPrintln("Failed to open file for reading");
        return;
    }

    //get next line
    char line[100];
    while(!feof(f)){
        if(i > 20){
            debugPrint("too many entries in wifi text file");
            break;
        }
        fgets(line, sizeof(line), f);
        wfi[i].active = atoi(strtok(line, ","));
        strcpy(wfi[i].ssid, strtok(NULL, ","));
        strcpy(wfi[i].pswd, strtok(NULL, ","));
        i++;
    }
    fclose(f);

    // compare to known networks and change password as necessary...
    for(int k = 0; k<i; k++){ 
        if(strcmp(wifi.ssid, wfi[k].ssid) ==0){
            matchedIndex = k;
            strcpy(wfi[k].pswd, wifi.pswd);
        }
    }
    // write updated file and actives
    f = fopen("/spiffs/config.txt", "w");
    // if not changing current entry, copy with all active zeros and print again at the end
    if(matchedIndex < 0){
        for(int m = 0; m <i; m++){
            fprintf(f, "%d,%s,%s,\n", 0, wfi[m].ssid, wfi[m].pswd); 
        }
        fprintf(f, "%d,%s,%s,\n",1, wifi.ssid, wifi.pswd);
    }else{  // else, make changes for modified entry accordingly
        for(int m = 0; m<i; m++){
            if(m == matchedIndex){
                fprintf(f, "%d,%s,%s,\n",1, wfi[m].ssid, wfi[m].pswd);
            }else{
                fprintf(f, "%d,%s,%s,\n",0, wfi[m].ssid, wfi[m].pswd);
            }
        }
    }
    fclose(f);
}





WiFiStruct defaultWiFiInfo(){
    WiFiStruct wfi[20];
    debugPrintln("Initializing SPIFFS");
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            debugPrintln("Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            debugPrintln("Failed to find SPIFFS partition");
        } else {
            debugPrint("Failed to initialize SPIFFS (");
            debugPrint(ret);
            debugPrint(")");
        }
        return (WiFiStruct){0,"",""};
    }
    debugPrintln("Opening file");
    // variables to store indexes
    int i = 0;
    int j = 0;
    int thisone[10]={0};
    FILE* f = fopen("/spiffs/config.txt", "r"); 
    if (f == NULL) {
        debugPrintln("Failed to open file for reading");
        return (WiFiStruct){0,"",""};
    }
    
    char line[100];
    // populate from text file into struct for easier processing 
    while(!feof(f)){
        if(i > 20){
            debugPrint("too many entries in wifi text file");
            break;
        }
        //get next line
        fgets(line, sizeof(line), f);
        wfi[i].active = atoi(strtok(line, ","));
        strcpy(wfi[i].ssid, strtok(NULL, ","));
        strcpy(wfi[i].pswd, strtok(NULL, ","));
        i++;
    }
    fclose(f);
    

    uint16_t networkNum = 20; //DEFAULT_SCAN_LIST_SIZE; // set to max number of networks, return value later is actual number
    wifi_ap_record_t apRecords[networkNum];
    // wifi_scan_config_t* w_scan = {};
    esp_wifi_scan_start(NULL, true);
    esp_wifi_scan_stop();
    
    esp_wifi_scan_get_ap_records(&networkNum, apRecords);
    if(networkNum > 0){
        for(int l=0; l < (int)networkNum; l++){
            for(int k = 0; k < i; k++){
                // compare to known networks
                // TODO: double check to make sure this returns the right types to compare... 
                // apRecords.ssid is uint8_t, which I'm assuming is just the ascii number. Will casting to char fix it?
                if(strcmp((char*)apRecords[l].ssid, wfi[k].ssid)==0){
                    
                    // If active already, default to this one and change nothing
                    if(wfi[k].active == 1){
                        fclose(f);
                        return wfi[k];
                    }
                    // collect indexes of available network connections
                    thisone[j]=i;
                    j++;
                    
                }else{
                    // sort of redundant, but doing it anyways in case previous connection isn't available
                    wfi[k].active = 0;
                }

            }
        }

    }else{
        debugPrintln("No networks found...");
    }
    if(j>0){
        f = fopen("/spiffs/config.txt", "w");    
        for(int m = 0; m <i; m++){
            // for now, just going with the first match and ignoring multiples
            if(m == thisone[0]){
                fprintf(f, "%d,%s,%s,\n", 1, wfi[m].ssid, wfi[m].pswd); 

            }else{
                fprintf(f, "%d,%s,%s,\n", 0, wfi[m].ssid, wfi[m].pswd); 
            } 
        }
        fclose(f);
        return wfi[thisone[0]];
    }else{
        return (WiFiStruct){0,"",""}; // might need to return NULL. Not sure. 
    }
}

bool getStrainGaugeParams(const Eigen::Matrix3d& m0, const Eigen::Matrix3d& m1, const Eigen::Matrix3d& m2, const Eigen::Matrix3d& m3){
    // if file exists and am able to open...
    Eigen::Matrix3d m[4] = {const_cast< Eigen::Matrix3d& >(m0), const_cast< Eigen::Matrix3d& >(m1), const_cast< Eigen::Matrix3d& >(m2), const_cast< Eigen::Matrix3d& >(m3)};
    debugPrintln("Initializing SPIFFS");
    if(!esp_spiffs_mounted("/spiffs")){
        esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
        };
        esp_err_t ret = esp_vfs_spiffs_register(&conf);
        if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                debugPrintln("Failed to mount or format filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                debugPrintln("Failed to find SPIFFS partition");
            } else {
                debugPrint("Failed to initialize SPIFFS (");
                debugPrint(ret);
                debugPrint(")");
            }
            return false;
        }
    }
    debugPrintln("Opening file");
    FILE* f = fopen("/spiffs/calib.txt", "r"); 
    if (f == NULL) {
        debugPrintln("Failed to open file for reading");
        return false;
    }
    
    char line[100];
    for(int i = 0; i < 4; i++){ // document line corresponding to one matrix
        //for(int j = 0; j<9; j++){ // single row with all matrix numbers
            for(int row = 0; row < 3; row++){
                for(int col = 0; col < 3; col++){
                    fgets(line, sizeof(line), f);
                    m[i](row,col) = std::stod(strtok(line, ","));
                }
            }
        //}
    }
    fclose(f);
    return true;
}

bool saveStrainGaugeParams(Eigen::Matrix3d *m0, Eigen::Matrix3d *m1, Eigen::Matrix3d *m2,Eigen::Matrix3d *m3){
    // if file exists and am able to open...
    Eigen::Matrix3d* m[4] = {m0, m1, m2, m3};
    debugPrintln("Initializing SPIFFS");
    if(!esp_spiffs_mounted("/spiffs")){
        esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
        };
        esp_err_t ret = esp_vfs_spiffs_register(&conf);
        if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                debugPrintln("Failed to mount or format filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                debugPrintln("Failed to find SPIFFS partition");
            } else {
                debugPrint("Failed to initialize SPIFFS (");
                debugPrint(ret);
                debugPrint(")");
            }
            return false;
        }
    }
    debugPrintln("Opening file");
    FILE* f = fopen("/spiffs/calib.txt", "w"); 
    if (f == NULL) {
        debugPrintln("Failed to open file for reading");
        return false;
    }
    
    for(int i = 0; i < 4; i++){ // document line corresponding to one matrix
        //for(int j = 0; j<9; j++){ // single row with all matrix numbers
            for(int row = 0; row < 3; row++){
                for(int col = 0; col < 3; col++){
                    
                    fprintf(f, "%0.5f,", m[i]->coeffRef(row,col));
                }
                fprintf(f, "\n");
            }
        //}
    }
    fclose(f);
    return true;
}










