// GPIO functions on sam3x8e
//
// Copyright (C) 2016,2017  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <stdint.h> // uint32_t
#include "autoconf.h" // CONFIG_CLOCK_FREQ
#include "board/irq.h" // irq_save
#include "command.h" // shutdown
#include "compiler.h" // ARRAY_SIZE
#include "gpio.h" // gpio_out_setup
#include <libmaple/gpio.h>
#include <libmaple/adc.h>
#include "sched.h" // sched_shutdown


/****************************************************************
 * Pin mappings
 ****************************************************************/

#define GPIO(PORT, NUM) (((PORT)-'A') * 16 + (NUM))
#define GPIO2PORT(PIN) ((PIN) / 16)
#define GPIO2BIT(PIN) (1<<((PIN) % 16))

static gpio_dev* digital_regs[4];

/****************************************************************
 * General Purpose Input Output (GPIO) pins
 ****************************************************************/

void
gpio_init_stm32 (void) {
    //GPIO{A,B,C,D} are no compile time constants
    digital_regs[0] = GPIOA;
    digital_regs[1] = GPIOB;
    digital_regs[2] = GPIOC;
    digital_regs[3] = GPIOD;
}
DECL_INIT(gpio_init_stm32);

struct gpio_out
gpio_out_setup(uint8_t pin, uint8_t val)
{
    if (GPIO2PORT(pin) >= ARRAY_SIZE(digital_regs))
        goto fail;
    gpio_dev *regs = digital_regs[GPIO2PORT(pin)];
    uint8_t bit = GPIO2BIT(pin);
    irqstatus_t flag = irq_save();
    gpio_set_mode(regs, bit, GPIO_OUTPUT_OD); //open drain (OD) or push-pull (PP)?
    gpio_write_bit(regs, bit, val);
    irq_restore(flag);
    return (struct gpio_out){ .regs=regs, .bit=bit };
fail:
    shutdown("Not an output pin");
}

void
gpio_out_toggle(struct gpio_out g)
{
    gpio_toggle_bit(g.regs, g.bit);
}

void
gpio_out_write(struct gpio_out g, uint8_t val)
{
    gpio_write_bit(g.regs, g.bit, val);
}


struct gpio_in
gpio_in_setup(uint8_t pin, int8_t pull_up)
{
    if (GPIO2PORT(pin) >= ARRAY_SIZE(digital_regs))
        goto fail;
    gpio_dev *regs = digital_regs[GPIO2PORT(pin)];
    uint8_t bit = GPIO2BIT(pin);
    irqstatus_t flag = irq_save();
    gpio_set_mode(regs, bit, pull_up ? GPIO_INPUT_PU : GPIO_INPUT_FLOATING); //or PD instead of FLOATING? 
    irq_restore(flag);
    return (struct gpio_in){ .regs=regs, .bit=bit };
fail:
    shutdown("Not an input pin");
}

uint8_t
gpio_in_read(struct gpio_in g)
{
    //TODO this cast uint32_t to uint8_t!
    return gpio_read_bit(g.regs, g.bit);
}


/****************************************************************
 * Analog to Digital Converter (ADC) pins
 ****************************************************************/

//STM32F103x8 and STM32F103xB
static const uint8_t adc_pins[] = {
    GPIO('A', 0), GPIO('A', 1), GPIO('A', 2), GPIO('A', 3), //ADC12_IN{0,1,2,3}
    GPIO('A', 4), GPIO('A', 5), GPIO('A', 6), GPIO('A', 7), //ADC12_IN{4,5,6,7}
    GPIO('B', 0), GPIO('B', 1), //ADC12_IN{8,9}
    GPIO('C', 0), GPIO('C', 1), GPIO('C', 2), GPIO('C', 3), //ADC12_IN{10,11,12,13}
    GPIO('C', 4), GPIO('C', 5), //ADC12_IN{14,15}
};

struct adc_status_t {
    adc_dev const *adc;
    uint8_t channel; //=255 -> free
};

struct adc_status_t adc_status[2];

#define ADC_FREQ_MAX 14000000
DECL_CONSTANT(ADC_MAX, (1<<12)-1);

void
adc_init_stm32(void)
{
    //ADC1 and ADC2 are no compile time constants
    adc_status[0].adc = ADC1;
    adc_status[0].channel = 255;
    adc_enable_single_swstart(ADC1);
    adc_set_sample_rate(ADC1, ADC_SMPR_1_5);
    adc_set_reg_seqlen(ADC1, 1);
    
    adc_status[1].adc = ADC2;
    adc_status[1].channel = 255;
    adc_enable_single_swstart(ADC2);
    adc_set_sample_rate(ADC2, ADC_SMPR_1_5);
    adc_set_reg_seqlen(ADC2, 1);
}
DECL_INIT(adc_init_stm32);

struct gpio_adc
gpio_adc_setup(uint8_t pin)
{
    // Find pin in adc_pins table
    int chan;
    for (chan=0; ; chan++) {
        if (chan >= ARRAY_SIZE(adc_pins))
            shutdown("Not a valid ADC pin");
        if (adc_pins[chan] == pin)
            break;
    }
    
    gpio_dev *regs = digital_regs[GPIO2PORT(pin)];
    uint8_t bit = GPIO2BIT(pin);
    //for the stm32f1, the ADC is ignored in adc_config_gpio
    adc_config_gpio(NULL, regs, bit);

    return (struct gpio_adc){.chan=chan};
}

// Try to sample a value. Returns zero if sample ready, otherwise
// returns the number of clock ticks the caller should wait before
// retrying this function.
uint32_t
gpio_adc_sample(struct gpio_adc g)
{

    for(int i = 0; i < 2; ++i) {
        struct adc_status_t* adc = &adc_status[i];
        if (adc->channel == g.chan) {
            if (adc->adc->regs->SR & ADC_SR_EOC)
                return 0;
            else
                goto need_delay;
        }
    }

    struct adc_status_t* adc = NULL;
    for(int i = 0; i < 2; ++i) {
        struct adc_status_t* adc_test = &adc_status[i];
        if (adc_test->channel == 255) {
            adc = adc_test;
            break;
        }
    }

    if (!adc)
        goto need_delay;

    adc->adc->regs->SQR3 = g.chan;
    adc->adc->regs->CR2 |= ADC_CR2_SWSTART;
    adc->channel = g.chan;    
need_delay:
    return ADC_FREQ_MAX * 1000ULL / CONFIG_CLOCK_FREQ;
}

// Read a value; use only after gpio_adc_sample() returns zero
uint16_t
gpio_adc_read(struct gpio_adc g)
{
    for(int i = 0; i < 2; ++i) {
        struct adc_status_t* adc = &adc_status[i];
        if (adc->channel == g.chan) {
            if (adc->adc->regs->SR & ADC_SR_EOC) {
                uint16_t val = (uint16_t)(adc->adc->regs->DR & ADC_DR_DATA);
                adc->channel = 255;
                return val;
            } else
                goto error;
        }
    }
error:
    shutdown("Tried sampling too early, or no ADC assigned to pin!");
}

// Cancel a sample that may have been started with gpio_adc_sample()
void
gpio_adc_cancel_sample(struct gpio_adc g)
{
    for(int i = 0; i < 2; ++i) {
        struct adc_status_t* adc = &adc_status[i];
        if (adc->channel == g.chan) {
            adc->channel = 255;
            //maybe we need to wait for the channel to finish sampling
        }
    }
}
