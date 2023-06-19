/**
 * Implementation of platform-specific functions for F411 black pill board.
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
    .modem_exti_0 = {GPIOA, GPIO0},

    // Address pins
    .address0 = {GPIOB, GPIO10},
    .address1 = {GPIOB, GPIO11},
};

void platform_clocks_init(void) {
    // Enable clocks for GPIO banks we use
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);

    rcc_periph_clock_enable(RCC_SYSCFG);

    // Enable clocks for peripherals we use
    rcc_periph_clock_enable(RCC_USART1);
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_TIM2);
}

void platform_usart_init(void) {
    // Setup GPIO mode and Alternate Function mode for A9 (USART1_TX)
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO9);

    // Setup GPIO mode and Alternate Function mode for A10 (USART1_RX)
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO10);

    // Setup UART parameters
    usart_set_baudrate(platform_pinout.p_usart, 115200);
    usart_set_databits(platform_pinout.p_usart, 8);
    usart_set_stopbits(platform_pinout.p_usart, USART_STOPBITS_1);
    usart_set_parity(platform_pinout.p_usart, USART_PARITY_NONE);
    usart_set_flow_control(platform_pinout.p_usart, USART_FLOWCONTROL_NONE);
    usart_set_mode(platform_pinout.p_usart, USART_MODE_TX_RX);

    // enable interrupt and rx
    nvic_enable_irq(NVIC_USART1_IRQ);
    nvic_set_priority(NVIC_USART1_IRQ, 0);

    // Finally enable the USART
    usart_enable(platform_pinout.p_usart);
}

void platform_gpio_init(void) {
    // Setup modem reset pin
    gpio_mode_setup(platform_pinout.modem_rst.port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    platform_pinout.modem_rst.pin);

    // Setup address pins
    gpio_mode_setup(platform_pinout.address0.port, GPIO_MODE_INPUT, GPIO_PUPD_NONE,
                    platform_pinout.address0.pin);
    gpio_mode_setup(platform_pinout.address1.port, GPIO_MODE_INPUT, GPIO_PUPD_NONE,
                    platform_pinout.address1.pin);
}

void platform_spi_init(void) {
    gpio_mode_setup(platform_pinout.modem_ss.port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    platform_pinout.modem_ss.pin);
    gpio_mode_setup(platform_pinout.modem_sck.port, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    platform_pinout.modem_sck.pin);
    gpio_set_af(platform_pinout.modem_sck.port, GPIO_AF5, platform_pinout.modem_sck.pin);
    gpio_mode_setup(platform_pinout.modem_mosi.port, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    platform_pinout.modem_mosi.pin);
    gpio_set_af(platform_pinout.modem_mosi.port, GPIO_AF5, platform_pinout.modem_mosi.pin);
    gpio_mode_setup(platform_pinout.modem_miso.port, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    platform_pinout.modem_miso.pin);
    gpio_set_af(platform_pinout.modem_miso.port, GPIO_AF5, platform_pinout.modem_miso.pin);
    gpio_mode_setup(platform_pinout.modem_exti_0.port, GPIO_MODE_INPUT, GPIO_PUPD_NONE,
                    platform_pinout.modem_exti_0.pin);

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

// TODO: write something here to blink the blackpill LED
void platform_set_indicator(bool state) {
    return;
}
