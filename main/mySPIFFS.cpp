#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "stdio.h"
#include <string>
#include "esp_wifi.h"


#include "mySPIFFS.h"
#include "myWiFi.h"
#include "globals.h"
#include "debug.h"
#include "Eigen/Dense"

//static bool spiffs_init_flag = false;
esp_vfs_spiffs_conf_t conf;
esp_err_t ret;

//#define DEFAULT_SCAN_LIST_SIZE CONFIG_EXAMPLE_SCAN_LIST_SIZE

// return the wifi info set to active from memory
WiFiStruct getActiveWifiInfo(){
    WiFiStruct wfi[20];
    debugPrintln("getActiveWiFiInfo");
    if(!esp_spiffs_mounted(conf.partition_label)){
        debugPrintln("Initializing SPIFFS");
        conf = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = true
        };
        ret = esp_vfs_spiffs_register(&conf);

        if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                debugPrintln("Failed to mount or format filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                debugPrintln("Failed to find SPIFFS partition");
            } else if (ret == ESP_ERR_INVALID_STATE){
                debugPrintln("Invalid State");
                // ESP_ERR_INVALID_STATE if already mounted or partition is encrypted
            } else {
                debugPrint("Failed to initialize SPIFFS (");
                debugPrint(ret);
                debugPrint(")");
            }
            //return (WiFiStruct){0,"",""};
            return WiFiStruct(0,"","");
        }
        
    }else{
        debugPrintln("SPIFFS already initialized?");
    }
 
    debugPrintln("Opening file");
    int i = 0;
    int j = -1;
    FILE* f = fopen("/spiffs/config.txt", "r"); 
    if (f == NULL) {
        debugPrintln("Failed to open file for reading");
        //return (WiFiStruct){0,"",""};
        return WiFiStruct(0,"","");
    }
    debugPrintln("after file opening");


    //get next line
    char line[100]={};
    
    // populate from text file into struct for easier processing 
    while(fgets(line, sizeof(line), f)){
        if(i > 20){
            debugPrint("too many entries in wifi text file");
            break;
        }
        wfi[i].active = atoi(strtok(line, ","));
        strcpy(wfi[i].ssid, strtok(NULL, ","));
        strcpy(wfi[i].pswd, strtok(NULL, ","));
        if(wfi[i].active == 1){
            j = i;
            debugPrintln("j=i");
        }
        i++;
    }
    if(feof(f)){
        fclose(f);
        debugPrintln("SPIFFS file closed.");
        debugPrint(i);
        debugPrintln(" stored networks");
        if(j >= 0){
            debugPrint(wfi[j].ssid);
            debugPrintln(" is active");
            return wfi[j];
        }else{
            debugPrintln("No Active Networks");
            //return (WiFiStruct){0,"",""};
            return WiFiStruct(0,"","");
        }

    }else {
        debugPrintln("unknown error, look into spiffs file read in getActiveWiFiInfo()");
        fclose(f);
        //return (WiFiStruct){0,"",""};
        return WiFiStruct(0,"","");
    }

}

// save wifi info into memory and overwrite if an entry already exists
void setWiFiInfo(WiFiStruct wifi){
    WiFiStruct wfi[20];
    int matchedIndex = -1;
    int i = 0;
    debugPrintln("setWiFiInfo");
    if(!esp_spiffs_mounted(conf.partition_label)){
        
        conf = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = true
        };
        ret = esp_vfs_spiffs_register(&conf);

        if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                debugPrintln("Failed to mount or format filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                debugPrintln("Failed to find SPIFFS partition");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                debugPrintln("Failed to find SPIFFS partition");
            } else {
                debugPrint("Failed to initialize SPIFFS (");
                debugPrint(ret);
                debugPrint(")");
            }
            return;
        }
        
    }else{
        debugPrintln("SPIFFS already initialized?");
    }

    debugPrintln("Opening file");
  
    FILE* f = fopen("/spiffs/config.txt", "r"); 
    if (f == NULL) {
        debugPrintln("Failed to open file for reading");
        return;
    }

    //get next line
    char line[100];
    while(fgets(line, sizeof(line), f)){
        if(i > 20){
            debugPrint("too many entries in wifi text file");
            break;
        }
        
        wfi[i].active = atoi(strtok(line, ","));
        strcpy(wfi[i].ssid, strtok(NULL, ","));
        strcpy(wfi[i].pswd, strtok(NULL, ","));
        i++;
    }
    if(feof(f)){
        debugPrintln("end of file. closing normally.");
    }else{
        debugPrintln("other error?");
    }
    fclose(f);
    debugPrintln("file closed.");

    // compare to known networks and change password as necessary...
    for(int k = 0; k<i; k++){ 
        if(strcmp(wifi.ssid, wfi[k].ssid) == 0){
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
    debugPrintln("file closed.");
}




// Compare saved wifi info to the networks that are available and set a saved one to active if it is available
WiFiStruct availableWiFiInfo(){
    WiFiStruct wfi[20];
    debugPrintln("default WiFi Info");
    
    if(!esp_spiffs_mounted(conf.partition_label)){
        debugPrintln("Initializing SPIFFS");
        conf = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = true
        };
        ret = esp_vfs_spiffs_register(&conf);

        if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                debugPrintln("Failed to mount or format filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                debugPrintln("Failed to find SPIFFS partition");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                debugPrintln("Failed to find SPIFFS partition");
            } else {
                debugPrint("Failed to initialize SPIFFS (");
                debugPrint(ret);
                debugPrint(")");
            }
            //return (WiFiStruct){0,"",""};
            return WiFiStruct(0,"","");
        }
        
    }else{
        debugPrintln("SPIFFS already initialized?");
    }

    debugPrintln("Opening file");
    // variables to store indexes
    int i = 0;
    int j = 0;
    int thisone[20]={0};
    debugPrintln("before opening file");
    FILE* f = fopen("/spiffs/config.txt", "r");
    debugPrintln("after opening file"); 
    if (f == NULL) {
        debugPrintln("Failed to open file for reading");
        //return (WiFiStruct){0,"",""};
        return WiFiStruct(0,"","");
    }
    debugPrintln("after checking if file openend... it did btw");
    char line[100];
    debugPrintln("after allocating line");
    // populate from text file into struct for easier processing 
    while(fgets(line, sizeof(line), f)){
        debugPrintln("populating wifiStructs");
        if(i > 20){
            debugPrint("too many entries in wifi text file");
            break;
        }
        wfi[i].active = atoi(strtok(line, ","));
        strcpy(wfi[i].ssid, strtok(NULL, ","));
        strcpy(wfi[i].pswd, strtok(NULL, ","));
        printf("%d,%s,%s\n", wfi[i].active, wfi[i].ssid, wfi[i].pswd);
        i++;
        // TODO: maybe in the future find a way to catch an error of an empty line. Otherwise the file should end with a comma on the same line as the last entry
    }
    if(feof(f)){
        debugPrintln("file read to end. functioning as expected.");
    }else{
        debugPrintln("unknown file error in availableWiFiInfo");
    }
    fclose(f);
    debugPrintln("file closed.");
    

    uint16_t networkNum = 20; //DEFAULT_SCAN_LIST_SIZE; // set to max number of networks, return value later is actual number
    debugPrintln("file closed.");
    //wifi_ap_record_t apRecords[networkNum]; //={};
    std::string apRecords[networkNum]={};
    //ch
    debugPrintln("file closed.");
    //char temp[33];
    debugPrintln("file closed.");
    scanNetworks((uint16_t&)networkNum, apRecords);
    debugPrintln("file closed..");
    printf("%d\n", networkNum);
    debugPrintln(wfi[2].active);
    if(networkNum > 0){
        for(int p=0; p < (int)networkNum; p++){
            
            debugPrintln(apRecords[p]);
            

            for(int k = 0; k < i; k++){
                // compare to known networks
                // TODO: double check to make sure this returns the right types to compare... 
                // apRecords.ssid is uint8_t, which I'm assuming is just the ascii number. Will casting to char fix it?
                
                
                if(strcmp(apRecords[p].c_str(), wfi[k].ssid)==0){
                    debugPrintln("compare successful");
                    // If active already, default to this one and change nothing

                    
                    if(wfi[k].active == 1){
                        fclose(f);
                        //free(apRecords);
                        debugPrintln("already active");
                        debugPrint(wfi[k].active);
                        debugPrint(" ");
                        debugPrint(wfi[k].ssid);
                        debugPrint(" ");
                        debugPrintln(wfi[k].pswd);
                        return wfi[k];
                    }
                    //wfi[k].active = 1;
                    // collect indexes of available network connections
                    thisone[j]=k; 
                    debugPrintln(wfi[k].active);
                    j++;
                    debugPrint("this should be 1: ");
                    debugPrintln(wfi[2].active);
                    
                }

            }
        }

    }else{
        debugPrintln("No networks found...");
    }
    debugPrintln(wfi[thisone[0]].active);
    // This loop updates the active flag in the file
    if(j>0){ // this means that at least one was matched
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
        //free(apRecords);
        debugPrint("j > 0 active\n j= ");
        debugPrintln(j);
        debugPrintln(thisone[0]);
        debugPrint(wfi[thisone[0]].active);
        debugPrint(".");
        debugPrint(wfi[thisone[0]].ssid);
        debugPrint(".");
        debugPrintln(wfi[thisone[0]].pswd);
        return wfi[thisone[0]];
    }else{
        //free(apRecords);
        debugPrintln("returning empty");
        //return (WiFiStruct){0,"",""}; // might need to return NULL. Not sure. 
        return WiFiStruct(0,"","");
    }

}

bool getStrainGaugeParams(const Eigen::Matrix3d& m0, const Eigen::Matrix3d& m1, const Eigen::Matrix3d& m2, const Eigen::Matrix3d& m3){
    // if file exists and am able to open...
    Eigen::Matrix3d m[4] = {const_cast<Eigen::Matrix3d&>(m0), const_cast<Eigen::Matrix3d&>(m1), const_cast<Eigen::Matrix3d&>(m2), const_cast<Eigen::Matrix3d&>(m3)};
    debugPrintln("getStrainGaugeParams");
    if(!esp_spiffs_mounted(conf.partition_label)){
        conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
        };
        ret = esp_vfs_spiffs_register(&conf);
        
        if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                debugPrintln("Failed to mount or format filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                debugPrintln("Failed to find SPIFFS partition");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                debugPrintln("Failed to find SPIFFS partition");
            } else {
                debugPrint("Failed to initialize SPIFFS (");
                debugPrint(ret);
                debugPrint(")");
            }
            return false;
        }
        size_t total = 0, used = 0;
        ret = esp_spiffs_info(conf.partition_label, &total, &used);
        
        if (ret != ESP_OK) {
            printf( "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        } else {
            printf("Partition size: total: %d, used: %d", total, used);
        }
    }
    debugPrintln("Opening file");
    FILE* f = fopen("/spiffs/calib.txt", "r"); 
    if (f == NULL) {
        debugPrintln("Failed to open file for reading");
        return false;
        //f = fopen("/spiffs/calib.txt", "w");
        //if(f == NULL)
        //{
        //    return false;
        //}
        //fprintf(f, ",,,\n");
        //fclose(f);
        //f = fopen("/spiffs/calib.txt", "r");
    }
    
    char line[100];
    for(int i = 0; i < 4; i++){ // document line corresponding to one matrix
        
        for(int row = 0; row < 3; row++){
            for(int col = 0; col < 3; col++){
                fgets(line, sizeof(line), f);
                m[i](row,col) = std::stod(strtok(line, ","));
                //printf("at matrix population %d %d row \n", row, col);
            }
            
        }
        
    }
    fclose(f);
    debugPrintln("Closing SPIFFS file");
    return true;
}

bool saveStrainGaugeParams(Eigen::Matrix3d *m0, Eigen::Matrix3d *m1, Eigen::Matrix3d *m2,Eigen::Matrix3d *m3){
    // if file exists and am able to open...
    Eigen::Matrix3d* m[4] = {m0, m1, m2, m3};
    debugPrintln("saveStrainGaugeParams");
    if(!esp_spiffs_mounted(conf.partition_label)){
        conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
        };
        ret = esp_vfs_spiffs_register(&conf);
        if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                debugPrintln("Failed to mount or format filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                debugPrintln("Failed to find SPIFFS partition");
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
    debugPrintln("Closing spiffs file");
    return true;
}

