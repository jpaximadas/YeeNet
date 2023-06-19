/**
 * Implementation of platform-specific functions for F103 blue pill board.
 */

#include "platform/platform.h"
#include <libopencm3/cm3/assert.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/usart.h>

const struct platform_pinout_table platform_pinout = {
    // Peripherals
    .p_spi = SPI1,
    .p_usart = USART3,

    // Modem pins
    .modem_rst = {GPIOB, GPIO9},
    .modem_ss = {GPIOA, GPIO4},
    .modem_mosi = {GPIOA, GPIO7},
    .modem_miso = {GPIOA, GPIO6},
    .modem_sck = {GPIOA, GPIO5},
    .modem_exti_0 = {GPIOA, GPIO0},
    .modem_exti_1 = {GPIOA, GPIO1},
    .modem_exti_2 = {GPIOA, GPIO2},
    .modem_exti_3 = {GPIOA, GPIO3},
    .modem_exti_4 = {GPIOA, GPIO4},
    .modem_exti_5 = {GPIOA, GPIO5},

    // Address pins
    .address0 = {GPIOB, GPIO15},
    .address1 = {GPIOB, GPIO12},
    .address2 = {GPIOB, GPIO1},
    .address3 = {GPIOB, GPIO0},
    .address4 = {GPIOB, GPIO6},
    .address5 = {GPIOB, GPIO7},
    .address6 = {GPIOC, GPIO13},
    .address7 = {GPIOB, GPIO8},

    // LED pins
    .LED_R = {GPIOA, GPIO10},
    .LED_G = {GPIOA, GPIO9},
    .LED_B = {GPIOA, GPIO8},
};

void platform_clocks_init(void) {
    rcc_clock_setup_in_hse_8mhz_out_72mhz();

    // Enable clocks for GPIO banks we use
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);

    rcc_periph_clock_enable(RCC_AFIO);

    // Enable clocks for peripherals we use
    rcc_periph_clock_enable(RCC_USART3);
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_TIM2);
}

void platform_usart_init(void) {
    // Setup GPIO pin GPIO_USART3_RE_TX on GPIO port B for transmit
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART3_TX);

    // Setup GPIO pin GPIO_USART3_RE_RX on GPIO port B for receive
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_USART3_RX);

    // Setup UART parameters
    usart_set_baudrate(platform_pinout.p_usart, 115200);
    usart_set_databits(platform_pinout.p_usart, 8);
    usart_set_stopbits(platform_pinout.p_usart, USART_STOPBITS_1);
    usart_set_parity(platform_pinout.p_usart, USART_PARITY_NONE);
    usart_set_flow_control(platform_pinout.p_usart, USART_FLOWCONTROL_NONE);
    usart_set_mode(platform_pinout.p_usart, USART_MODE_TX_RX);

    // enable interrupt and rx
    nvic_enable_irq(NVIC_USART3_IRQ);
    nvic_set_priority(NVIC_USART3_IRQ, 0);

    // Finally enable the USART
    usart_enable(platform_pinout.p_usart);
}

void platform_gpio_init(void) {
    // set JTAG/SWD pins to input floating to prevent pins from being used
    // gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,GPIO13);
    // gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,GPIO14);

    // Setup modem reset pin
    gpio_set_mode(platform_pinout.modem_rst.port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                  platform_pinout.modem_rst.pin);
    gpio_set_mode(platform_pinout.LED_G.port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                  platform_pinout.LED_G.pin);

    // Setup address pins
    gpio_set_mode(platform_pinout.address0.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                  platform_pinout.address0.pin);
    gpio_set(platform_pinout.address0.port, platform_pinout.address0.pin);

    gpio_set_mode(platform_pinout.address1.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                  platform_pinout.address1.pin);
    gpio_set(platform_pinout.address1.port, platform_pinout.address1.pin);

    gpio_set_mode(platform_pinout.address2.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                  platform_pinout.address2.pin);
    gpio_set(platform_pinout.address2.port, platform_pinout.address2.pin);

    gpio_set_mode(platform_pinout.address3.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                  platform_pinout.address3.pin);
    gpio_set(platform_pinout.address3.port, platform_pinout.address3.pin);

    gpio_set_mode(platform_pinout.address4.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                  platform_pinout.address4.pin);
    gpio_set(platform_pinout.address4.port, platform_pinout.address4.pin);

    gpio_set_mode(platform_pinout.address5.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                  platform_pinout.address5.pin);
    gpio_set(platform_pinout.address5.port, platform_pinout.address5.pin);

    gpio_set_mode(platform_pinout.address6.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                  platform_pinout.address6.pin);
    gpio_set(platform_pinout.address6.port, platform_pinout.address6.pin);

    gpio_set_mode(platform_pinout.address7.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                  platform_pinout.address7.pin);
    gpio_set(platform_pinout.address7.port, platform_pinout.address7.pin);
}

void platform_spi_init(void) {
    gpio_set_mode(platform_pinout.modem_ss.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                  platform_pinout.modem_ss.pin);
    gpio_set_mode(platform_pinout.modem_sck.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  platform_pinout.modem_sck.pin);
    gpio_set_mode(platform_pinout.modem_mosi.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  platform_pinout.modem_mosi.pin);
    gpio_set_mode(platform_pinout.modem_miso.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                  platform_pinout.modem_miso.pin);

    gpio_set_mode(platform_pinout.modem_exti_0.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                  platform_pinout.modem_exti_0.pin);
    gpio_set_mode(platform_pinout.modem_exti_1.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                  platform_pinout.modem_exti_1.pin);
    gpio_set_mode(platform_pinout.modem_exti_2.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                  platform_pinout.modem_exti_2.pin);
    gpio_set_mode(platform_pinout.modem_exti_3.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                  platform_pinout.modem_exti_3.pin);
    gpio_set_mode(platform_pinout.modem_exti_4.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                  platform_pinout.modem_exti_4.pin);
    gpio_set_mode(platform_pinout.modem_exti_5.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                  platform_pinout.modem_exti_5.pin);

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

    exti_select_source(EXTI0, platform_pinout.modem_exti_0.port);
    exti_set_trigger(EXTI0, EXTI_TRIGGER_RISING);
    exti_enable_request(EXTI0);
}

void platform_set_indicator(bool state) {
    if (state) {
        gpio_clear(platform_pinout.LED_G.port, platform_pinout.LED_G.pin);
    } else {
        gpio_set(platform_pinout.LED_G.port, platform_pinout.LED_G.pin);
    }
}