cmake_minimum_required(VERSION 3.5)

if (NOT DEFINED PROJECT_NAME)
    #set(PROJECT_VER "0.1.0.1")
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(SudoBoard)
else()
    message(FATAL_ERROR "LV PORT ESP32: This must be a project's main CMakeLists.txt.")
endif()
