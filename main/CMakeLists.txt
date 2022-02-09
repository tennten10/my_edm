FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)

idf_component_register(SRCS "main.cpp" ${app_sources} #"BLE.cpp" "globals.cpp" "mySPIFFS.cpp" "System.cpp" "display.cpp" 
                            INCLUDE_DIRS "" "../components/lvgl" "../components/lvgl_esp32_drivers" "../components/Eigen" "../components/DendoStepper"  )
                            #REQUIRES soc nvs_flash spiffs esp_adc_cal bt esp_https_ota app_update ulp lvgl) 

                            