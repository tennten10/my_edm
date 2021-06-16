FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)

idf_component_register(SRCS "BLE.cpp" "globals.cpp" "mySPIFFS.cpp" "System.cpp" "display.cpp" "main.cpp" ${app_sources} 
                            INCLUDE_DIRS "" "../components/lvgl" "../components/lvgl_esp32_drivers" "../components/esp-nimble-cpp" "../components/Eigen"  )
                            #REQUIRES soc nvs_flash spiffs esp_adc_cal bt esp_https_ota app_update ulp lvgl) 
                            # Embed the server root certificate into the final binary
                            #EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem )
                            

set( ulp_app_name ulp_main)

set( ulp_s_sources "../ulp/button.S" "../ulp/wake_up.S")

set (ulp_exp_dep_srcs "main.cpp")

ulp_embed_binary(${ulp_app_name} "${ulp_s_sources}" "${ulp_exp_dep_srcs}")



#if(CONFIG_EXAMPLE_EMBEDDED_CERTS)
#target_add_binary_data(${COMPONENT_TARGET} "certs/aws-root-ca.pem" TEXT)
#target_add_binary_data(${COMPONENT_TARGET} "certs/certificate.pem.crt" TEXT)
#target_add_binary_data(${COMPONENT_TARGET} "certs/private.pem.key" TEXT)
#endif()
