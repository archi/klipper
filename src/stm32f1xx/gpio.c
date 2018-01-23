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
#include "sched.h" // sched_shutdown

//#include <stm32f1xx_hal_rcc.h>
#include <stm32f1xx.h>


#ifndef _BV
#define _BV(_b) (1 << (_b))
#endif

/*
typedef struct
{
  __IO uint32_t CRL;
  __IO uint32_t CRH;
  __IO uint32_t IDR;
  __IO uint32_t ODR;
  __IO uint32_t BSRR;
  __IO uint32_t BRR;
  __IO uint32_t LCKR;
} GPIO_TypeDef;
*/

/****************************************************************
 * Pin mappings
 ****************************************************************/

#define NUM_PORTS      5
#define MAX_PIN_VALUE  (NUM_PORTS * 16)

/****************************************************************
 * General Purpose Input Output (GPIO) pins
 ****************************************************************/

void
gpio_peripheral(_gpio_peripheral_t const * const ptr,
                uint8_t const pull_up)
{
    uint32_t const pin_idx = ptr->pin;
    GPIO_TypeDef * const regs = &GPIOA[ptr->port];

    uint32_t const cfg = (ptr->dir | (ptr->func << 2));

    irqstatus_t flag = irq_save();
    if (7 < pin_idx) {
        regs->CRH |= (cfg << (pin_idx - 8));
    } else {
        regs->CRL |= (cfg << pin_idx);
    }
    if (ptr->dir == 0 && ptr->func == 0b10) {
        if (pull_up) {
            regs->ODR |= _BV(pin_idx); // pull-up
        } else {
            regs->ODR &= ~(_BV(pin_idx)); // pull-down
        }
    }
    irq_restore(flag);
}


struct gpio_out
gpio_out_setup(uint8_t pin, uint8_t val)
{
    if (pin >= MAX_PIN_VALUE)
        goto fail;
    uint32_t const port_idx = GPIO2PORT(pin);
    uint32_t const pin_idx = GPIO2PIN(pin);
    uint32_t const bit = _BV(pin_idx);
    GPIO_TypeDef * const regs = &GPIOA[port_idx];
    uint32_t const cfg = 0b0101; // General purpose output (10MHz) with open-drain

    irqstatus_t flag = irq_save();
    // Config!
    if (7 < pin_idx) {
        regs->CRH |= (cfg << (pin_idx - 8));
    } else {
        regs->CRL |= (cfg << pin_idx);
    }
    irq_restore(flag);
    regs->BSRR = bit << (val ? 0 : 16);
    return (struct gpio_out){ .regs=regs, .bit=bit };
fail:
    shutdown("Not an output pin");
}

void
gpio_out_toggle(struct gpio_out g)
{
    GPIO_TypeDef * const regs = g.regs;
    regs->BSRR = g.bit << ((regs->ODR & g.bit) ? 0 : 16);
}

void
gpio_out_write(struct gpio_out g, uint8_t val)
{
    GPIO_TypeDef * const regs = g.regs;
    regs->BSRR = g.bit << (val ? 0 : 16);
}

struct gpio_in
gpio_in_setup(uint8_t pin, int8_t pull_up)
{
    if (pin >= MAX_PIN_VALUE)
        goto fail;
    uint32_t const port_idx = GPIO2PORT(pin);
    uint32_t const pin_idx = GPIO2PIN(pin);
    uint32_t const bit = _BV(pin_idx);
    GPIO_TypeDef * const regs = &GPIOA[port_idx];

    uint32_t cfg = 0;
    if (pull_up) {
        cfg |= 0b1000; // General purpose input with pull-up / pull-down
    } else {
        cfg |= 0b0100; // General purpose input with floating
    }

    irqstatus_t flag = irq_save();
    if (7 < pin_idx) {
        regs->CRH |= (cfg << (pin_idx - 8));
    } else {
        regs->CRL |= (cfg << pin_idx);
    }
    if (pull_up)
        regs->ODR |= bit; // pull-up
    else
        regs->ODR &= ~bit; // pull-down
    irq_restore(flag);
    return (struct gpio_in){ .regs=regs, .bit=bit };
fail:
    shutdown("Not an input pin");
}

uint8_t
gpio_in_read(struct gpio_in g)
{
    GPIO_TypeDef * const regs = g.regs;
    return !!(regs->IDR & g.bit);
}
