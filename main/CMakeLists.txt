FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)

idf_component_register(SRCS "main.cpp" ${app_sources} #"BLE.cpp" "globals.cpp" "mySPIFFS.cpp" "System.cpp" "display.cpp" 
                            INCLUDE_DIRS "" "../components/lvgl" "../components/lvgl_esp32_drivers" "../components/Eigen"  )
                            #REQUIRES soc nvs_flash spiffs esp_adc_cal bt esp_https_ota app_update ulp lvgl) 
                            # Embed the server root certificate into the final binary
                            #EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem )
                            


#if(CONFIG_EXAMPLE_EMBEDDED_CERTS)
#target_add_binary_data(${COMPONENT_TARGET} "certs/aws-root-ca.pem" TEXT)
#target_add_binary_data(${COMPONENT_TARGET} "certs/certificate.pem.crt" TEXT)
#target_add_binary_data(${COMPONENT_TARGET} "certs/private.pem.key" TEXT)
#endif()
