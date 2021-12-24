#include "host_link.h"
#include "uart.h"
#include <sys/types.h>
#include <libopencm3/cm3/scb.h>
#include "host_link_buffer.h"
#include "host_link_modem.h"
#include "address.h"

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
            link.iface_write_byte = &usart1_write_byte;
            link.iface_release = &usart1_release;
            break;
        }
    }
    link.iface_write_byte(GOOD); //let the host know that the controller is ready (especially if a reset was performed)
    link.iface_release();
}

void host_link_echo(uint8_t *command, uint16_t len);


void host_link_echo(uint8_t *command, uint16_t len){
    //link.iface_write_bytes(command,len); //send back what was received
    for(uint16_t i = 0; i<len; i++){
        link.iface_write_byte(command[i]);
    }
    
}

void host_link_reset(uint8_t *command, uint16_t len);

void host_link_reset(uint8_t *command, uint16_t len){
    scb_reset_system();
}

void host_link_get_local_address(uint8_t *command, uint16_t len){
    link.iface_write_byte(local_address_get());
}

#define NUM_COMMANDS 18
void (*commands[])(uint8_t *,uint16_t len) = {
    &host_link_reset, //0
    &host_link_echo, //1
    &host_link_buffer_pop, //2
    &host_link_buffer_cap, //3
    &host_link_modem_setup, //4
    &host_link_modem_listen, //5
    &host_link_modem_load_payload, //6
    &host_link_modem_load_and_transmit, //7
    &host_link_modem_transmit, //8
    &host_link_modem_standby, //9
    &host_link_modem_is_clear, //10
    &host_link_modem_get_last_payload_rssi, //11
    &host_link_modem_get_last_payload_snr, //12
    &host_link_modem_get_airtime_usec, //13
    &host_link_modem_set_modulation, //14
    &host_link_get_local_address, //15
    &host_link_buffer_get_n_overflow, //16
    &host_link_buffer_reset_n_overflow //17
};

uint8_t *cur_command;
void host_link_parse(void){
    //update command pointer
    uint16_t len = (uint16_t) (*link.iface_get_command)(&cur_command);
    //do nothing and return if retval of iface_get_command is NULL
    if(!len){
        return;
    }
    //check if command is out of bounds for the command list
    if(cur_command[0]>=NUM_COMMANDS){
        link.iface_write_byte(PARSE_ERROR);
        link.iface_release();
        return;
    }
    //report a successful parse
    link.iface_write_byte(GOOD);
  
    //look up the command in the table
    void (*command)(uint8_t *,uint16_t len) = commands[*cur_command]; 
    //move the command pointer forward
    cur_command++;
    len--;
    command(cur_command,len);
    //release the interface once the command is done
    link.iface_release();
    
    
}