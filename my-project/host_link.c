#include "host_link.h"
#include "uart.h"
#include <sys/types.h>
#include <libopencm3/cm3/scb.h>
#include "host_link_buffer.h"
#include "host_link_modem.h"

struct host_interface link;

uint8_t rdy_good = 0x00;
uint8_t rdy_bad = 0x01;

void host_link_init(enum iface_setting my_interface){
    switch(my_interface){
        case USB:
        {
            //TODO
        }
        case USART:
        {
            usart1_host_link_init();
            link.iface_get_command = &usart1_get_command;
            link.iface_write_bytes = &usart1_write_bytes;
            link.iface_release = &usart1_release;
            break;
        }
    }
    link.iface_write_bytes(&rdy_good,1); //let the host know that the controller is ready (especially if a reset was performed)
    link.iface_release();
}

void host_link_echo(uint8_t *command, uint16_t len);

void host_link_echo(uint8_t *command, uint16_t len){
    link.iface_write_bytes(command,len);
}

void host_link_reset(uint8_t *command, uint16_t len);

void host_link_reset(uint8_t *command, uint16_t len){
    scb_reset_system();
}
/*
struct clist_elem modem_commands[] = {
    {
        .my_type = (enum elem_type) 9
    },
    {
        .my_type = COMMAND,
        .my_next_step = &host_link_modem_setup
    },
    {
        .my_type = COMMAND,
        .my_next_step = &host_link_modem_listen
    },
    {
        .my_type = COMMAND,
        .my_next_step = &host_link_modem_load_payload
    },
    {
        .my_type = COMMAND,
        .my_next_step = &host_link_modem_load_and_transmit
    },
    {
        .my_type = COMMAND,
        .my_next_step = &host_link_modem_transmit
    },
    {
        .my_type = COMMAND,
        .my_next_step = &host_link_modem_is_clear
    },
    {
        .my_type = COMMAND,
        .my_next_step = &host_link_modem_get_last_payload_rssi
    },
    {
        .my_type = COMMAND,
        .my_next_step = &host_link_modem_get_last_payload_snr
    },
    {
        .my_type = COMMAND,
        .my_next_step = &host_link_modem_get_airtime_usec
    },
};

struct clist_elem buffer_commands[] = {
    {
        .my_type = (enum elem_type) 2
    },
    {
        .my_type = COMMAND,
        .my_next_step = &host_link_buffer_pop
    },
    {
        .my_type = COMMAND,
        .my_next_step = &host_link_buffer_cap
    }
};*/

struct clist_elem clist_top_level[] = {
    [0] = {4,0},
    [1] = { COMMAND, &host_link_echo },
    [2] = { COMMAND, &host_link_reset }
    /*,
    {
        .my_type = COMMAND_LIST,
        .my_next_step = modem_commands
    },
    {
        .my_type = COMMAND_LIST,
        .my_next_step = buffer_commands
    }*/
};

//TODO make sure same command doesn't get run twice?
uint8_t *cur_command;
void host_link_parse(void){

    //start at top level command list
    struct clist_elem *cur_clist = clist_top_level;

    //update command pointer
    uint16_t len = (uint16_t) (*link.iface_get_command)(&cur_command);
    if(!len){
        return;
    }
    
    

    //loop over the command
    for(;;){
    
        //guard against a zero length command
        if(!len){
            link.iface_release();
            return;
        }
        //guard against out of bounds
        if(cur_command[0] < (uint8_t) cur_clist[0].my_type){
            enum elem_type target_elem_type = cur_clist[cur_command[0]+1].my_type; //look up command
            union next_step target_elem_next_step = cur_clist[cur_command[0]+1].my_next_step;
            cur_command++; //move the buffer base pointer forward to ignore the parsed command
            len--; //make the length reflect this change
            switch(target_elem_type){
                case COMMAND:
                    link.iface_write_bytes(&rdy_good,1); //since a command was reached, the decode was successful
                    //execute the command
                    target_elem_next_step.function(cur_command, len);
                    link.iface_release();
                    return;
                case COMMAND_LIST:
                    //jump to the next command list
                    cur_clist = target_elem_next_step.next_clist;
                    break;
            }
        } else {
            link.iface_write_bytes(&rdy_bad,1);//out of bounds sends a decode error
            link.iface_release();
        }
    }
    
}