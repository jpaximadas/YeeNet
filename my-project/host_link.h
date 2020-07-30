#pragma once
#include "uart.h"
#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct host_interface{
    ssize_t (*iface_write_bytes)(uint8_t *, size_t, bool ); //write bytes to the host
    uint16_t (*iface_get_command)(uint8_t **); //read a command
    void (*iface_release)(void); //release the interface
    FILE* fp; //TX only file pointer for convenience
};

extern struct host_interface link;

enum iface_setting{
    USART,
    USB
};

void host_link_init(enum iface_setting my_interface);

void host_link_parse(void);

enum element_type{
    SUBINSTRUCTION,
    INSTRUCTION,
    LEVEL_TERMINATOR
};

union next_step{
    void *function;
    struct command_tree_element *level;
};

struct command_tree_element{
    enum element_type type;
    char* word;
    union next_step my_next_step;
};