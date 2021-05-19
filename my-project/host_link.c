#include "host_link.h"
#include "uart.h"
#include <sys/types.h>
#include <libopencm3/cm3/scb.h>
#include "host_link_buffer.h"
#include "host_link_modem.h"

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

uint16_t test;
void host_link_echo(uint8_t *command, uint16_t len){
    //link.iface_write_bytes(command,len); //send back what was received
    test = 0;
    for(uint16_t i = 0; i<len; i++){
        test += link.iface_write_byte(command[i]);
    }
    
}

void host_link_reset(uint8_t *command, uint16_t len);

void host_link_reset(uint8_t *command, uint16_t len){
    scb_reset_system();
}

#define NUM_COMMANDS 14
void (*commands[])(uint8_t *,uint16_t len) = {
    &host_link_reset,
    &host_link_echo,
    &host_link_buffer_pop,
    &host_link_buffer_cap,
    &host_link_modem_setup,
    &host_link_modem_listen,
    &host_link_modem_load_payload,
    &host_link_modem_load_and_transmit,
    &host_link_modem_transmit,
    &host_link_modem_standby,
    &host_link_modem_is_clear,
    &host_link_modem_get_last_payload_rssi,
    &host_link_modem_get_last_payload_snr,
    &host_link_modem_get_airtime_usec
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
  
    //look up the command in the table and pass the command
    void (*command)(uint8_t *,uint16_t len) = commands[*cur_command]; 
    //move the command pointer forward
    cur_command++;
    len--;
    command(cur_command,len);
    //release the interface once the command is done
    link.iface_release();
    
    
}