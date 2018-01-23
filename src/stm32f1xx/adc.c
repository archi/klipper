#include "autoconf.h"
#include "board/irq.h" // irq_save
#include "command.h"   // shutdown
#include "sched.h"     // shutdown
#include "compiler.h"  // ARRAY_SIZE
#include "gpio.h"

#include <stm32f1xx_hal_rcc.h>
#include <stm32f1xx_hal_rcc_ex.h>
#include <stm32f1xx.h>
#include <stdint.h>

/*
typedef struct
{
  __IO uint32_t SR;
  __IO uint32_t CR1;
  __IO uint32_t CR2;
  __IO uint32_t SMPR1;
  __IO uint32_t SMPR2;
  __IO uint32_t JOFR1;
  __IO uint32_t JOFR2;
  __IO uint32_t JOFR3;
  __IO uint32_t JOFR4;
  __IO uint32_t HTR;
  __IO uint32_t LTR;
  __IO uint32_t SQR1;
  __IO uint32_t SQR2;
  __IO uint32_t SQR3;
  __IO uint32_t JSQR;
  __IO uint32_t JDR1;
  __IO uint32_t JDR2;
  __IO uint32_t JDR3;
  __IO uint32_t JDR4;
  __IO uint32_t DR;
} ADC_TypeDef;
ADC1
ADC2

typedef struct
{
  __IO uint32_t SR;               / *!< ADC status register,    used for ADC multimode (bits common to several ADC instances). Address offset: ADC1 base address         * /
  __IO uint32_t CR1;              / *!< ADC control register 1, used for ADC multimode (bits common to several ADC instances). Address offset: ADC1 base address + 0x04  * /
  __IO uint32_t CR2;              / *!< ADC control register 2, used for ADC multimode (bits common to several ADC instances). Address offset: ADC1 base address + 0x08  * /
  uint32_t  RESERVED[16];
  __IO uint32_t DR;               / *!< ADC data register,      used for ADC multimode (bits common to several ADC instances). Address offset: ADC1 base address + 0x4C  * /
} ADC_Common_TypeDef;
ADC12_COMMON
 */

/****************************************************************
 * Analog to Digital Converter (ADC) pins
 ****************************************************************/

/*
 * Analog Pins
 */
static const _gpio_peripheral_t adc_pins[10] = {
    { 0, 0, 0, 0 }, // ADC0
    { 0, 1, 0, 0 }, // ADC1
    { 0, 2, 0, 0 }, // ADC2
    { 0, 3, 0, 0 }, // ADC3
    { 0, 4, 0, 0 }, // ADC4
    { 0, 5, 0, 0 }, // ADC5
    { 0, 6, 0, 0 }, // ADC6
    { 0, 7, 0, 0 }, // ADC7
    { 1, 0, 0, 0 }, // ADC8
    { 1, 1, 0, 0 }  // ADC9
};

#define ADC_FREQ_MAX 12000000 // 12MHz, max is 13MHz
DECL_CONSTANT(ADC_MAX, 4095);


static uint32_t CFG_REG = 0;
static uint32_t CFG_CHAN = 0;

void
gpio_adc_init(void) {
    /* Init ADC HW */
    NVIC_DisableIRQ(ADC1_2_IRQn);
    NVIC_ClearPendingIRQ(ADC1_2_IRQn);

    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_ADC_CONFIG(RCC_ADCPCLK2_DIV6); // 72/6 = 12

    /*
      This mode is started either by
      setting the ADON bit in the ADC_CR2 register (for a regular channel only) or by external
      trigger (for a regular or injected channel), while the CONT bit is 0.
     */
    CFG_REG = (ADC_CR2_SWSTART |
               ADC_CR2_EXTSEL |  /* SW Start enabled */
               ADC_CR2_ADON      /* Enable ADC */
              );
    ADC1->SMPR1 = 0; // 0 = 1.5 cycles
    ADC1->SMPR2 = 0;
}
DECL_INIT(gpio_adc_init);

struct gpio_adc
gpio_adc_setup(uint8_t pin)
{
    uint8_t const in_port = GPIO2PORT(pin);
    uint8_t const in_pin  = GPIO2PIN(pin);
    // Find pin in adc_pins table

    int chan;
    for (chan=0; ; chan++) {
        if (chan >= ARRAY_SIZE(adc_pins))
            shutdown("Not a valid ADC pin");
        if (adc_pins[chan].port == in_port &&
            adc_pins[chan].pin == in_pin)
            break;
    }
    gpio_peripheral(&adc_pins[chan], 0);
    return (struct gpio_adc){ .channel = chan };
}

// Try to sample a value. Returns zero if sample ready, otherwise
// returns the number of clock ticks the caller should wait before
// retrying this function.
uint32_t
gpio_adc_sample(struct gpio_adc g)
{
    if (CFG_CHAN == 0) {
        // Start sample
        ADC1->SQR3 = g.channel;
        ADC1->CR2  = (CFG_REG);
        CFG_CHAN   = g.channel;
        goto need_delay;
    }
    if (CFG_CHAN != g.channel)
        // Sampling in progress on another channel
        goto need_delay;
    if ( ! (ADC1->SR & ADC_SR_EOC) ) // EOC (End Of Conversion)
        // Conversion still in progress
        goto need_delay;
    // Conversion ready
    return 0;
need_delay:
    return (CONFIG_CLOCK_FREQ / (ADC_FREQ_MAX));
}

// Read a value; use only after gpio_adc_sample() returns zero
uint16_t
gpio_adc_read(struct gpio_adc g)
{
    gpio_adc_cancel_sample(g);
    return (uint16_t)(ADC1->DR & 0xFFFF);
}

// Cancel a sample that may have been started with gpio_adc_sample()
void
gpio_adc_cancel_sample(struct gpio_adc g)
{
    CFG_CHAN = 0;
}
