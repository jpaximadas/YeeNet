#pragma once
#include "uart.h"
#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct host_interface{
    ssize_t (*iface_write_bytes)(uint8_t *, size_t); //write bytes to the host
    uint16_t (*iface_get_command)(uint8_t **); //read a command
    void (*iface_release)(void); //release the interface
};

extern struct host_interface link;

enum iface_setting{
    USART,
    USB
};

void host_link_init(enum iface_setting my_interface);

void host_link_parse(void);

enum elem_type{
    COMMAND_LIST,
    COMMAND,
};

union next_step{
    void (*function)(uint8_t*, uint16_t);
    struct clist_elem *next_clist;
};


struct clist_elem{
    enum elem_type my_type;
    union next_step my_next_step;
};