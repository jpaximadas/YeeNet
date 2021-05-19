#pragma once
#include "uart.h"
#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define PACKET_BUFFER_LENGTH 3

struct host_interface{
    ssize_t (*iface_write_bytes)(uint8_t *, size_t); //write bytes to the host
    ssize_t (*iface_write_byte)(uint8_t); //write a byte
    uint16_t (*iface_get_command)(uint8_t **); //read a command
    void (*iface_release)(void); //release the interface
};

extern struct host_interface link;

enum iface_setting{
    USART,
    USB
};

enum iface_status_byte{
    GOOD=0,
    COBS_ERROR=1,
    PARSE_ERROR=2
};

void host_link_init(enum iface_setting my_interface);

void host_link_parse(void);

