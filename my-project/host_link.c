
#include "host_link.h"
#include "uart.h"
#include <string.h>

#define COMMAND_DELIMITER 0x3A //ascii colon
#define COMMAND_BUFFER_SIZE 20

struct host_interface link;

void host_link_init(enum iface_setting my_interface){
    switch(my_interface){
        case USB:
        {
            //TODO
        }
        case USART:
        {
            link.fp = usart1_setup(115200);
            link.iface_get_command = &usart1_get_command;
            link.iface_write_bytes = &usart1_write_bytes;
            link.iface_release = &usart1_release;
            break;
        }
    }
}

void host_link_echo(uint8_t *arg_str, uint16_t arg_len){
    (*link.iface_write_bytes)(arg_str,arg_len,true);
}

struct command_tree_element command_tree_top_level[] = {
    {
        .type = INSTRUCTION,
        .word = "ECHO",
        .next_step.function = &host_link_echo
    },
    {
        .type = LEVEL_TERMINATOR
    }
};

bool word_comp(uint8_t *command, char *word){
    
}

uint8_t find_in_level(uint8_t * command, struct command_tree_element *level){
    uint8_t pos = 0;
    while(level[pos].type != LEVEL_TERMINATOR){
        if(word_comp(command,level[pos].word)){return pos;}
        pos++;
    }
    return pos;
}


uint8_t *host_link_command;
void host_link_parse(void){

    //get the length and guard against empty command
    uint16_t len = (uint16_t) (*link.iface_get_command)(&host_link_command);
    if(!len){
        (*link.iface_release)();
        return;
    }

    uint16_t host_link_command_pos = 0; //start at the beginning of the command
    
    host_link_command_pos = 0; //start at the 
    struct command_tree_element *cur_level = command_tree_top_level;
    bool searching_tree = true;
    //traverse the command subcommand by subcommand
    while(host_link_command_pos < len && searching_tree){
        uint8_t elem_pos;
        switch(host_link_command[host_link_command_pos]){
            case ' ':
                searching_tree = false;
                //TODO report error
                break;
            case ':':
                elem_pos = find_in_level(&host_link_command[host_link_command_pos+1],cur_level);
                switch(cur_level[elem_pos].type){
                    case INSTRUCTION:
                        searching_tree = false;
                        ( *(cur_level[elem_pos].my_next_step.function) )( host_link_command, len);
                        break;
                    case SUBINSTRUCTION:
                        cur_level = cur_level[elem_pos].my_next_step.level;
                    case LEVEL_TERMINATOR:
                        searching_tree = false;
                        //TODO report error
                }
                break;
            default:
                break;
        }
        host_link_command_pos++;
        
    }

    (*link.iface_release)();
    
}