#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)


CFLAGS += -DLV_LVGL_H_INCLUDE_SIMPLE

COMPONENT_SRCDIRS := . \
	configs \
	fonts \
	images 
	#components

COMPONENT_ADD_INCLUDEDIRS := $(COMPONENT_SRCDIRS) ../   

# COMPONENT_EMBED_TXTFILES :=  ${PROJECT_PATH}/server_certs/ca_cert.pem
