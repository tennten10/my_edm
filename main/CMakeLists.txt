FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)

idf_component_register(SRCS "main.cpp" ${app_sources} 
                            INCLUDE_DIRS "" "../components/lvgl" "../components/lvgl_esp32_drivers" "../components/esp-nimble-cpp"
                            REQUIRES soc nvs_flash ulp spiffs esp_adc_cal bt esp_https_ota app_update
                            # Embed the server root certificate into the final binary
                            EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem )
                            #LDFRAGMENTS "linker_fragment.lf")

set( ulp_app_name ulp_main)

set( ulp_s_sources "../ulp/button.S" "../ulp/wake_up.S")

set (ulp_exp_dep_srcs "main.cpp")

ulp_embed_binary(${ulp_app_name} "${ulp_s_sources}" "${ulp_exp_dep_srcs}")


#set(PROJECT_VER "0.1.0.1")



#register_component()

#if(CONFIG_EXAMPLE_EMBEDDED_CERTS)
#target_add_binary_data(${COMPONENT_TARGET} "certs/aws-root-ca.pem" TEXT)
#target_add_binary_data(${COMPONENT_TARGET} "certs/certificate.pem.crt" TEXT)
#target_add_binary_data(${COMPONENT_TARGET} "certs/private.pem.key" TEXT)
#endif()

#to add app size to binary header
#target_link_libraries(${COMPONENT_TARGET} "-u custom_app_desc")



#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    #COMMAND arm-none-eabi-size --format=berkeley "firmware.elf")