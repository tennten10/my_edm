#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "stdio.h"
#include <string>


#include "mySPIFFS.h"
#include "globals.h"
#include "Dense" //"Eigen/Dense"

//static bool spiffs_init_flag = false;
esp_vfs_spiffs_conf_t conf;
esp_err_t ret;

//#define DEFAULT_SCAN_LIST_SIZE CONFIG_EXAMPLE_SCAN_LIST_SIZE

// return the wifi info set to active from memory
// WiFiStruct getActiveWifiInfo(){
//     WiFiStruct wfi[20];
//     //println("getActiveWiFiInfo");
//     if(!esp_spiffs_mounted(conf.partition_label)){
//         //println("Initializing SPIFFS");
//         conf = {
//             .base_path = "/spiffs",
//             .partition_label = NULL,
//             .max_files = 5,
//             .format_if_mount_failed = true
//         };
//         ret = esp_vfs_spiffs_register(&conf);

//         if (ret != ESP_OK) {
//             if (ret == ESP_FAIL) {
//                 //println("Failed to mount or format filesystem");
//             } else if (ret == ESP_ERR_NOT_FOUND) {
//                 //println("Failed to find SPIFFS partition");
//             } else if (ret == ESP_ERR_INVALID_STATE){
//                 //println("Invalid State");
//                 // ESP_ERR_INVALID_STATE if already mounted or partition is encrypted
//             } else {
//                 //print("Failed to initialize SPIFFS (");
//                 //print(ret);
//                 //print(")");
//             }
//             //return (WiFiStruct){0,"",""};
//             return WiFiStruct(0,"","");
//         }
        
//     }else{
//         //println("SPIFFS already initialized?");
//     }
 
//     //println("Opening file");
//     int i = 0;
//     int j = -1;
//     FILE* f = fopen("/spiffs/config.txt", "r"); 
//     if (f == NULL) {
//         //println("Failed to open file for reading");
//         //return (WiFiStruct){0,"",""};
//         return WiFiStruct(0,"","");
//     }
//     //println("after file opening");


//     //get next line
//     char line[100]={};
    
//     // populate from text file into struct for easier processing 
//     while(fgets(line, sizeof(line), f)){
//         if(i > 20){
//             //print("too many entries in wifi text file");
//             break;
//         }
//         wfi[i].active = atoi(strtok(line, ","));
//         strcpy(wfi[i].ssid, strtok(NULL, ","));
//         strcpy(wfi[i].pswd, strtok(NULL, ","));
//         if(wfi[i].active == 1){
//             j = i;
//             //println("j=i");
//         }
//         i++;
//     }
//     if(feof(f)){
//         fclose(f);
//         //println("SPIFFS file closed.");
//         //print(i);
//         //println(" stored networks");
//         if(j >= 0){
//             //print(wfi[j].ssid);
//             //println(" is active");
//             return wfi[j];
//         }else{
//             //println("No Active Networks");
//             //return (WiFiStruct){0,"",""};
//             return WiFiStruct(0,"","");
//         }

//     }else {
//         //println("unknown error, look into spiffs file read in getActiveWiFiInfo()");
//         fclose(f);
//         //return (WiFiStruct){0,"",""};
//         return WiFiStruct(0,"","");
//     }

// }

// // save wifi info into memory and overwrite if an entry already exists
// void setWiFiInfo(WiFiStruct wifi){
//     WiFiStruct wfi[20];
//     int matchedIndex = -1;
//     int i = 0;
//     //println("setWiFiInfo");
//     if(!esp_spiffs_mounted(conf.partition_label)){
        
//         conf = {
//             .base_path = "/spiffs",
//             .partition_label = NULL,
//             .max_files = 5,
//             .format_if_mount_failed = true
//         };
//         ret = esp_vfs_spiffs_register(&conf);

//         if (ret != ESP_OK) {
//             if (ret == ESP_FAIL) {
//                 //println("Failed to mount or format filesystem");
//             } else if (ret == ESP_ERR_NOT_FOUND) {
//                 //println("Failed to find SPIFFS partition");
//             } else if (ret == ESP_ERR_NOT_FOUND) {
//                 //println("Failed to find SPIFFS partition");
//             } else {
//                 //print("Failed to initialize SPIFFS (");
//                 //print(ret);
//                 //print(")");
//             }
//             return;
//         }
        
//     }else{
//         //println("SPIFFS already initialized?");
//     }

//     //println("Opening file");
  
//     FILE* f = fopen("/spiffs/config.txt", "r"); 
//     if (f == NULL) {
//         //println("Failed to open file for reading");
//         return;
//     }

//     //get next line
//     char line[100];
//     while(fgets(line, sizeof(line), f)){
//         if(i > 20){
//             //print("too many entries in wifi text file");
//             break;
//         }
        
//         wfi[i].active = atoi(strtok(line, ","));
//         strcpy(wfi[i].ssid, strtok(NULL, ","));
//         strcpy(wfi[i].pswd, strtok(NULL, ","));
//         i++;
//     }
//     if(feof(f)){
//         //println("end of file. closing normally.");
//     }else{
//         //println("other error?");
//     }
//     fclose(f);
//     //println("file closed.");

//     // compare to known networks and change password as necessary...
//     for(int k = 0; k<i; k++){ 
//         if(strcmp(wifi.ssid, wfi[k].ssid) == 0){
//             matchedIndex = k;
//             strcpy(wfi[k].pswd, wifi.pswd);
//         }
//     }
//     // write updated file and actives
//     f = fopen("/spiffs/config.txt", "w");
//     // if not changing current entry, copy with all active zeros and print again at the end
//     if(matchedIndex < 0){
//         for(int m = 0; m <i; m++){
//             fprintf(f, "%d,%s,%s,\n", 0, wfi[m].ssid, wfi[m].pswd); 
//         }
//         fprintf(f, "%d,%s,%s,\n",1, wifi.ssid, wifi.pswd);
//     }else{  // else, make changes for modified entry accordingly
//         for(int m = 0; m<i; m++){
//             if(m == matchedIndex){
//                 fprintf(f, "%d,%s,%s,\n",1, wfi[m].ssid, wfi[m].pswd);
//             }else{
//                 fprintf(f, "%d,%s,%s,\n",0, wfi[m].ssid, wfi[m].pswd);
//             }
//         }
//     }
//     fclose(f);
//     //println("file closed.");
// }




// // Compare saved wifi info to the networks that are available and set a saved one to active if it is available
// WiFiStruct availableWiFiInfo(){
//     WiFiStruct wfi[20];
//     //println("default WiFi Info");
    
//     if(!esp_spiffs_mounted(conf.partition_label)){
//         //println("Initializing SPIFFS");
//         conf = {
//             .base_path = "/spiffs",
//             .partition_label = NULL,
//             .max_files = 5,
//             .format_if_mount_failed = true
//         };
//         ret = esp_vfs_spiffs_register(&conf);

//         if (ret != ESP_OK) {
//             if (ret == ESP_FAIL) {
//                 //println("Failed to mount or format filesystem");
//             } else if (ret == ESP_ERR_NOT_FOUND) {
//                 //println("Failed to find SPIFFS partition");
//             } else if (ret == ESP_ERR_NOT_FOUND) {
//                 //println("Failed to find SPIFFS partition");
//             } else {
//                 //print("Failed to initialize SPIFFS (");
//                 //print(ret);
//                 //print(")");
//             }
//             //return (WiFiStruct){0,"",""};
//             return WiFiStruct(0,"","");
//         }
        
//     }else{
//         //println("SPIFFS already initialized?");
//     }

//     //println("Opening file");
//     // variables to store indexes
//     int i = 0;
//     int j = 0;
//     int thisone[20]={0};
//     //println("before opening file");
//     FILE* f = fopen("/spiffs/config.txt", "r");
//     //println("after opening file"); 
//     if (f == NULL) {
//         //println("Failed to open file for reading");
//         //return (WiFiStruct){0,"",""};
//         return WiFiStruct(0,"","");
//     }
//     //println("after checking if file openend... it did btw");
//     char line[100];
//     //println("after allocating line");
//     // populate from text file into struct for easier processing 
//     while(fgets(line, sizeof(line), f)){
//         //println("populating wifiStructs");
//         if(i > 20){
//             //print("too many entries in wifi text file");
//             break;
//         }
//         wfi[i].active = atoi(strtok(line, ","));
//         strcpy(wfi[i].ssid, strtok(NULL, ","));
//         strcpy(wfi[i].pswd, strtok(NULL, ","));
//         printf("%d,%s,%s\n", wfi[i].active, wfi[i].ssid, wfi[i].pswd);
//         i++;
//         // TODO: maybe in the future find a way to catch an error of an empty line. Otherwise the file should end with a comma on the same line as the last entry
//     }
//     if(feof(f)){
//         //println("file read to end. functioning as expected.");
//     }else{
//         //println("unknown file error in availableWiFiInfo");
//     }
//     fclose(f);
//     //println("file closed.");
    

//     uint16_t networkNum = 20; //DEFAULT_SCAN_LIST_SIZE; // set to max number of networks, return value later is actual number
//     //println("file closed.");
//     //wifi_ap_record_t apRecords[networkNum]; //={};
//     std::string apRecords[networkNum]={};
//     //ch
//     //println("file closed.");
//     //char temp[33];
//     //println("file closed.");
//     scanNetworks((uint16_t&)networkNum, apRecords);
//     //println("file closed..");
//     //printf("%d\n", networkNum);
//     //println(wfi[2].active);
//     if(networkNum > 0){
//         for(int p=0; p < (int)networkNum; p++){
            
//             //println(apRecords[p]);
            

//             for(int k = 0; k < i; k++){
//                 // compare to known networks
//                 // TODO: double check to make sure this returns the right types to compare... 
//                 // apRecords.ssid is uint8_t, which I'm assuming is just the ascii number. use memcpy instead of strcpy.
                
                
//                 if(strcmp(apRecords[p].c_str(), wfi[k].ssid)==0){
//                     //println("compare successful");
//                     // If active already, default to this one and change nothing

                    
//                     if(wfi[k].active == 1){
//                         fclose(f);
//                         //free(apRecords);
//                         //println("already active");
//                         //print(wfi[k].active);
//                         //print(" ");
//                         //print(wfi[k].ssid);
//                         //print(" ");
//                         //println(wfi[k].pswd);
//                         return wfi[k];
//                     }
//                     //wfi[k].active = 1;
//                     // collect indexes of available network connections
//                     thisone[j]=k; 
//                     //println(wfi[k].active);
//                     j++;
//                     //print("this should be 1: ");
//                     //println(wfi[2].active);
                    
//                 }

//             }
//         }

//     }else{
//         //println("No networks found...");
//     }
//     //println(wfi[thisone[0]].active);
//     // This loop updates the active flag in the file
//     if(j>0){ // this means that at least one was matched
//         f = fopen("/spiffs/config.txt", "w");    
//         for(int m = 0; m <i; m++){
//             // for now, just going with the first match and ignoring multiples
//             if(m == thisone[0]){
//                 fprintf(f, "%d,%s,%s,\n", 1, wfi[m].ssid, wfi[m].pswd); 

//             }else{
//                 fprintf(f, "%d,%s,%s,\n", 0, wfi[m].ssid, wfi[m].pswd); 
//             } 
//         }
//         fclose(f);
//         //free(apRecords);
//         //print("j > 0 active\n j= ");
//         //println(j);
//         //println(thisone[0]);
//         //print(wfi[thisone[0]].active);
//         //print(".");
//         //print(wfi[thisone[0]].ssid);
//         //print(".");
//         //println(wfi[thisone[0]].pswd);
//         return wfi[thisone[0]];
//     }else{
//         //free(apRecords);
//         //println("returning empty");
//         //return (WiFiStruct){0,"",""}; // might need to return NULL. Not sure. 
//         return WiFiStruct(0,"","");
//     }

// }

bool getStrainGaugeParams(const Eigen::Matrix3d& m0, const Eigen::Matrix3d& m1, const Eigen::Matrix3d& m2, const Eigen::Matrix3d& m3){
    // if file exists and am able to open...
    Eigen::Matrix3d m[4] = {const_cast<Eigen::Matrix3d&>(m0), const_cast<Eigen::Matrix3d&>(m1), const_cast<Eigen::Matrix3d&>(m2), const_cast<Eigen::Matrix3d&>(m3)};
    //println("getStrainGaugeParams");
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
                //println("Failed to mount or format filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                //println("Failed to find SPIFFS partition");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                //println("Failed to find SPIFFS partition");
            } else {
                //print("Failed to initialize SPIFFS (");
                //print(ret);
                //print(")");
            }
            return false;
        }
        size_t total = 0, used = 0;
        ret = esp_spiffs_info(conf.partition_label, &total, &used);
        
        if (ret != ESP_OK) {
            //printf( "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        } else {
            //printf("Partition size: total: %d, used: %d", total, used);
        }
    }
    //println("Opening file");
    FILE* f = fopen("/spiffs/calib.txt", "r"); 
    if (f == NULL) {
        //println("Failed to open file for reading");
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
                ////printf("at matrix population %d %d row \n", row, col);
            }
            
        }
        
    }
    fclose(f);
    //println("Closing SPIFFS file");
    return true;
}

bool saveStrainGaugeParams(Eigen::Matrix3d *m0, Eigen::Matrix3d *m1, Eigen::Matrix3d *m2,Eigen::Matrix3d *m3){
    // if file exists and am able to open...
    Eigen::Matrix3d* m[4] = {m0, m1, m2, m3};
    //println("saveStrainGaugeParams");
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
                //println("Failed to mount or format filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                //println("Failed to find SPIFFS partition");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                //println("Failed to find SPIFFS partition");
            } else {
                //print("Failed to initialize SPIFFS (");
                //print(ret);
                //print(")");
            }
            return false;
        }
    }
    //println("Opening file");
    FILE* f = fopen("/spiffs/calib.txt", "w"); 
    if (f == NULL) {
        //println("Failed to open file for reading");
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
    //println("Closing spiffs file");
    return true;
}



Data getSaveData(){
    // if file exists and am able to open...
    Data d = {0,0,0,0};
    //println("getSaveData");
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
                //println("Failed to mount or format filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                //println("Failed to find SPIFFS partition");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                //println("Failed to find SPIFFS partition");
            } else {
                //print("Failed to initialize SPIFFS (");
                //print(ret);
                //print(")");
            }
            return d;
        }
        size_t total = 0, used = 0;
        ret = esp_spiffs_info(conf.partition_label, &total, &used);
        
        if (ret != ESP_OK) {
            printf( "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        } else {
            printf("Partition size: total: %d, used: %d", total, used);
        }
    }
    //println("Opening file");
    FILE* f = fopen("/spiffs/save.txt", "r"); 
    if (f == NULL) {
        //println("Failed to open file for reading");
        return d;
        
    }
    
    char line[100];

    fgets(line, sizeof(line), f); // throw away header line
    //println(line);
    strcpy(line,"");
    fgets(line, sizeof(line), f);
    //println(line);
    std::string s = std::string(strtok(line, ","));
    // println(s);
    //d.u = stringToUnits(s);
    d.intensity = std::stoi(strtok(NULL, ",")); // intensity
    d.red = std::stoi(strtok(NULL, ",")); // red
    d.green = std::stoi(strtok(NULL, ",")); // green
    d.blue = std::stoi(strtok(NULL, ",")); // blue

    fclose(f);
    //println("Closing SPIFFS file");
    return d;
}

bool setSaveData(Data d){
    // if file exists and am able to open...
    
    //println("setSaveData");
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
                //println("Failed to mount or format filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                //println("Failed to find SPIFFS partition");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                //println("Failed to find SPIFFS partition");
            } else {
                //print("Failed to initialize SPIFFS (");
                //print(ret);
                //print(")");
            }
            return false;
        }
    }
    //println("Opening file");
    FILE* f = fopen("/spiffs/save.txt", "w"); 
    if (f == NULL) {
        //println("Failed to open file for reading");
        return false;
    }
         
    fprintf(f, "intensity,red,blue,green\n%d,%d,%d,%d", d.intensity, d.red, d.green, d.blue );
       
    fclose(f);
    //println("Closing spiffs file");
    return true;
}