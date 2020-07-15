
#include "host_link.h"
#include "uart.h"

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


uint8_t *command;
uint8_t instruction_buffer[COMMAND_BUFFER_SIZE];
void host_link_parse(void){
    uint16_t len = (uint16_t) (*link.iface_get_command)(&command);
    if(!len){ return; }

    fprintf(link.fp ,"got: ");
    usart1_write_bytes(command,len,false);
    fprintf(link.fp,"\r\n");
    usart1_terminate();
    usart1_release();

    
    
}