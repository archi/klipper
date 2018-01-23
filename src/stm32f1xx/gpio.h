#ifndef __STM32F1_GPIO_H
#define __STM32F1_GPIO_H

#include <stdint.h>

#define GPIO(PORT, NUM) (((PORT)-'A') * 16 + (NUM))

#define GPIO2PORT(PIN) ((PIN) / 16)
#define GPIO2PIN(PIN)  ((PIN) & 15)
#define GPIO2BIT(PIN)  (1 << ((PIN) & 15))

typedef struct _gpio_peripheral_t {
    uint8_t        port;    // port
    uint8_t        pin;     // pin index
    uint8_t        func;    // Alternate configuration
    uint8_t        dir;     // 0 = in, 1 = out_10M, 2 = out_2M, 3 = out_50M
} _gpio_peripheral_t;


void gpio_peripheral(_gpio_peripheral_t const * const ptr,
                     uint8_t const pull_up);

struct gpio_out {
    void *regs;
    uint32_t bit;
};
struct gpio_out gpio_out_setup(uint8_t pin, uint8_t val);
void gpio_out_toggle(struct gpio_out g);
void gpio_out_write(struct gpio_out g, uint8_t val);

struct gpio_in {
    void *regs;
    uint32_t bit;
};
struct gpio_in gpio_in_setup(uint8_t pin, int8_t pull_up);
uint8_t gpio_in_read(struct gpio_in g);

struct gpio_adc {
    uint32_t channel;
};
struct gpio_adc gpio_adc_setup(uint8_t pin);
uint32_t gpio_adc_sample(struct gpio_adc g);
uint16_t gpio_adc_read(struct gpio_adc g);
void gpio_adc_cancel_sample(struct gpio_adc g);

#endif // gpio.h
