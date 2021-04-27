FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)

idf_component_register(SRCS "main.cpp" ${app_sources} 
                            INCLUDE_DIRS "" "../components/lvgl" "../components/lvgl_esp32_drivers" "../components/esp-nimble-cpp"
                            REQUIRES soc nvs_flash ulp spiffs esp_adc_cal bt)

set( ulp_app_name ulp_main)

set( ulp_s_sources "../ulp/button.S" "../ulp/wake_up.S")

set (ulp_exp_dep_srcs "main.cpp")

ulp_embed_binary(${ulp_app_name} "${ulp_s_sources}" "${ulp_exp_dep_srcs}")



