/**
 * Implementation of platform-specific functions for F103 blue pill board.
 */

#include "platform/platform.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/usart.h>

const struct platform_pinout_table platform_pinout = {
    // Peripherals
    .p_spi = SPI1,
    .p_usart = USART1,

    // Modem pins
    .modem_rst = {GPIOB, GPIO9},
    .modem_ss = {GPIOA, GPIO1},
    .modem_mosi = {GPIOA, GPIO7},
    .modem_miso = {GPIOA, GPIO6},
    .modem_sck = {GPIOA, GPIO5},
    .modem_irq = {GPIOA, GPIO0},

    // Address pins
    .address0 = {GPIOB, GPIO10},
    .address1 = {GPIOB, GPIO11},

    // Indicator pin
    .indicator ={GPIOC, GPIO13}
};

void platform_clocks_init(void) {
    rcc_clock_setup_in_hse_8mhz_out_72mhz();

    // Enable clocks for GPIO banks we use
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);

    rcc_periph_clock_enable(RCC_AFIO);

    // Enable clocks for peripherals we use
    rcc_periph_clock_enable(RCC_USART1);
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_TIM2);
}

void platform_usart_init(void) {
    // Setup GPIO pin GPIO_USART1_RE_TX on GPIO port A for transmit
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
              GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);

    // Setup GPIO pin GPIO_USART1_RE_RX on GPIO port A for receive
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
              GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

    // Setup UART parameters
    usart_set_baudrate(platform_pinout.p_usart, 115200);
    usart_set_databits(platform_pinout.p_usart, 8);
    usart_set_stopbits(platform_pinout.p_usart, USART_STOPBITS_1);
    usart_set_parity(platform_pinout.p_usart, USART_PARITY_NONE);
    usart_set_flow_control(platform_pinout.p_usart, USART_FLOWCONTROL_NONE);
    usart_set_mode(platform_pinout.p_usart, USART_MODE_TX_RX);

    //enable interrupt and rx
    nvic_enable_irq(NVIC_USART1_IRQ);
    nvic_set_priority(NVIC_USART1_IRQ, 0);
    
    // Finally enable the USART
    usart_enable(platform_pinout.p_usart);
}

void platform_gpio_init(void) {
    // Setup modem reset pin
    gpio_set_mode(platform_pinout.modem_rst.port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, platform_pinout.modem_rst.pin);

    // Setup address pins
	gpio_set_mode(platform_pinout.address0.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, platform_pinout.address0.pin);
	gpio_set_mode(platform_pinout.address1.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, platform_pinout.address1.pin);

    // Setup indicator pin
    gpio_set_mode(platform_pinout.indicator.port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, platform_pinout.indicator.pin);
}

void platform_spi_init(void) {
    gpio_set_mode(platform_pinout.modem_ss.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, platform_pinout.modem_ss.pin);
    gpio_set_mode(platform_pinout.modem_sck.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, platform_pinout.modem_sck.pin);
    gpio_set_mode(platform_pinout.modem_mosi.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, platform_pinout.modem_mosi.pin);
    gpio_set_mode(platform_pinout.modem_miso.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, platform_pinout.modem_miso.pin);
    gpio_set_mode(platform_pinout.modem_irq.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, platform_pinout.modem_irq.pin);

    spi_reset(platform_pinout.p_spi);

    spi_init_master(platform_pinout.p_spi, SPI_CR1_BAUDRATE_FPCLK_DIV_128, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
            SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);

    spi_enable_software_slave_management(platform_pinout.p_spi);
    spi_set_nss_high(platform_pinout.p_spi);

    spi_enable(platform_pinout.p_spi);
}

void platform_irq_init(void) {
    // TODO: implement other pins for irq
    // assume EXTI0

    nvic_enable_irq(NVIC_EXTI0_IRQ);  // interrupt on PA0
    nvic_set_priority(NVIC_EXTI0_IRQ, 1);

    exti_select_source(EXTI0, platform_pinout.modem_irq.port);
    exti_set_trigger(EXTI0, EXTI_TRIGGER_RISING);
    exti_enable_request(EXTI0);
}

void platform_set_indicator(bool state){
    if(state){
        gpio_clear(platform_pinout.indicator.port,platform_pinout.indicator.pin);
    }else{
        gpio_set(platform_pinout.indicator.port,platform_pinout.indicator.pin);
    }
}