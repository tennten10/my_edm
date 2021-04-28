#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

CFLAGS+= -DLV_LVGL_H_INCLUDE_SIMPLE


COMPONENT_ADD_INCLUDEDIRS += configs
COMPONENT_EMBED_TXTFILES :=  ${PROJECT_PATH}/server_certs/ca_cert.pem

$(call compile_only_if,$(CONFIG_SB_V1_HALF_ILI9341),configs/PinDefs_V1.o)
$(call compile_only_if,$(CONFIG_LV_TFT_DISPLAY_CONTROLLER_RA8875),configs/PinDefs_V3.o)
$(call compile_only_if,$(CONFIG_SB_V6_FULL_ILI9341),configs/PinDefs_V6.o)