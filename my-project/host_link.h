#pragma once
#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>

struct host_interface{
    void (*write_bytes)(uint8_t *, uint16_t, bool ); //write bytes to the host
    void (*get_command)(uint8_t *, uint16_t); //read a command
    FILE* fp; //TX only file pointer for convenience
};

enum iface_setting{
    USART,
    USB
}

bool host_link_init(enum iface_setting my_interface);