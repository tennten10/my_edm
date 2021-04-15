cmake_minimum_required(VERSION 3.5)

if (NOT DEFINED PROJECT_NAME)
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(_SUDOBOARD_)
else()
    message(FATAL_ERROR "LV PORT ESP32: This must be a project's main CMakeLists.txt.")
endif()

message(STATUS CONFIG_TESTOPTION_YAY: ${CONFIG_TESTOPTION_YAY})
