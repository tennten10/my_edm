#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := SudoBoard
#EXTRA_COMPONENT_DIRS := $(abspath lvgl_esp32_drivers)	$(abspath lvgl) $(abspath esp-nimble-cpp) $(abspath Eigen)
include $(IDF_PATH)/make/project.mk
