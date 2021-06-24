// Status codes are defined in this file
// These are to communicate errors and successes with the app when they are connected

#define SB_OK                                   200
#define SB_OK_WITH_WIFI                         201

#define SB_WIFI_CONNECT_SUCCESS                 207
#define SB_WIFI_CONNECT_FAIL                    208
#define SB_INVALID_SSID                         209     // If unable to find SSID in available networks when scanning
#define SB_INVALID_PSWD                         210     // If unable to connect to network 
#define SB_WIFI_CRED_TIMEOUT                    205     // When the ssid or password don't receive their terminating character and too much time elapses
#define SB_SET_UNIT_SUCCESS                     300
#define SB_SET_UNIT_FAILED                      303 
#define SB_UPDATE_STARTED                       400
#define SB_UPDATE_FAILED_BATTERY                402
#define SB_UPDATE_FAILED_SERVER                 404
#define SB_UPDATE_FAILED_NO_WIFI_INFO           406
#define SB_UPDATE_FAILED_OTHER                  408
#define SB_STRAINGAUGE_1_ERROR                  601
#define SB_STRAINGAUGE_2_ERROR                  602
#define SB_STRAINGAUGE_3_ERROR                  603
#define SB_STRAINGAUGE_4_ERROR                  604
#define SB_UNSTABLE_STRAIN_GAUGE_VOLTAGE_SOURCE 700
#define SB_UNSTABLE_MAIN_VOLTAGE                701 
