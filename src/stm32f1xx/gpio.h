#ifndef __STM32F1XX_GPIO_H
#define __STM32F1XX_GPIO_H

#include <stdint.h>

//forward declarations
struct gpio_dev;
struct adc_dev;

void gpio_peripheral(char bank, uint32_t bit, char ptype, uint32_t pull_up);

struct gpio_out {
    struct gpio_dev *regs;
    uint8_t bit;
};

struct gpio_out gpio_out_setup(uint8_t pin, uint8_t val);
void gpio_out_toggle(struct gpio_out g);
void gpio_out_write(struct gpio_out g, uint8_t val);

struct gpio_in {
    struct gpio_dev *regs;
    uint8_t bit;
};
struct gpio_in gpio_in_setup(uint8_t pin, int8_t pull_up);
uint8_t gpio_in_read(struct gpio_in g);

struct gpio_adc {
    struct adc_dev *adc;
    uint8_t chan;
};
struct gpio_adc gpio_adc_setup(uint8_t pin);
uint32_t gpio_adc_sample(struct gpio_adc g);
uint16_t gpio_adc_read(struct gpio_adc g);
void gpio_adc_cancel_sample(struct gpio_adc g);

#endif // gpio.h
