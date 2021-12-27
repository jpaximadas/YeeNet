PROJECT = yeenet_router_firmware
BUILD_DIR = bin

SHARED_DIR = ../shared

CFILES = main.c
CFILES += api.c \
		  modem_hl.c \
		  modem_ll.c \
		  otf_cobs.c \
		  uart.c \
		  modem_ll_config.c \
		  util.c\
		  callback_timer.c \
		  address.c \
		  packet_handler.c \
		  host_link.c \
		  platform/platform.c \
		  packet_buffer.c \
		  payload_buffer.c \
		  host_link_buffer.c \
		  host_link_modem.c \
		  

AFILES += api-asm.S

OOCD_FILE = board/common.cfg

include ../yeenet.mk

# You shouldn't have to edit anything below here.
VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=../libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk

upload : all
	st-flash --reset write $(PROJECT).bin 0x8000000