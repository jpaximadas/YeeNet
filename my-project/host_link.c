#include "host_link.h"
#include "uart.h"
#include <string.h>

struct host_interface link;

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
}

void host_link_echo(uint8_t *command, uint16_t len);

void host_link_echo(uint8_t *command, uint16_t len){
    (*link.iface_write_bytes)(command,len,true);
}

struct clist_elem clist_top_level[] = {
    {
        .my_type = (enum elem_type) 1 //number of commands in the clist
    },
    {
        .my_type = COMMAND,
        .my_next_step = &host_link_echo
    }
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
            link.iface_release();
        }
    }
    
}