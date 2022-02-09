cmake_minimum_required(VERSION 3.5)

if (NOT DEFINED PROJECT_NAME)
    
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(my_edm)
    spiffs_create_partition_image(spiffs data FLASH_IN_PROJECT)
else()
    message(FATAL_ERROR "LV PORT ESP32: This must be a project's main CMakeLists.txt.")
endif()
