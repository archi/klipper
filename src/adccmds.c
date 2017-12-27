// Commands for controlling GPIO analog-to-digital input pins
//
// Copyright (C) 2016  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.
#include "autoconf.h"

#include "basecmd.h" // oid_alloc
#include "board/gpio.h" // struct gpio_adc
#include "board/irq.h" // irq_disable
#include "command.h" // DECL_COMMAND
#include "sched.h" // DECL_TASK

#if (CONFIG_SIMULATOR == 1 && CONFIG_MACH_LINUX == 1)
#include <stdio.h>
#endif
#ifdef __LPC176x__
#include "lpc176x/pins_MKS.h"
#endif

struct analog_in {
    struct timer timer;
    uint32_t rest_time, sample_time, next_begin_time;
    uint16_t value, min_value, max_value;
    struct gpio_adc pin;
    uint8_t state, sample_count;
};

static struct task_wake analog_wake;

static uint_fast8_t
analog_in_event(struct timer *timer)
{
    struct analog_in *a = container_of(timer, struct analog_in, timer);
    uint32_t sample_delay = gpio_adc_sample(a->pin);
    if (sample_delay) {
        a->timer.waketime += sample_delay;
        return SF_RESCHEDULE;
    }
    uint16_t value = gpio_adc_read(a->pin);
    uint8_t state = a->state;
    if (state >= a->sample_count) {
        state = 0;
    } else {
        value += a->value;
    }
    a->value = value;
    a->state = state+1;
    if (a->state < a->sample_count) {
        a->timer.waketime += a->sample_time;
        return SF_RESCHEDULE;
    }
    if (a->value < a->min_value || a->value > a->max_value) {
#if (CONFIG_SIMULATOR == 1 && CONFIG_MACH_LINUX == 1)
        printf("ADC error %u (min: %u, max: %u)\n",
               a->value, a->min_value, a->max_value);
#endif
        shutdown("ADC out of range");
    }
    sched_wake_task(&analog_wake);
    a->next_begin_time += a->rest_time;
    a->timer.waketime = a->next_begin_time;
    return SF_RESCHEDULE;
}

void
command_config_analog_in(uint32_t *args)
{
    struct analog_in *a = oid_alloc(args[0],
                                    command_config_analog_in,
                                    sizeof(*a));
    a->timer.func = analog_in_event;
    a->pin        = gpio_adc_setup(args[1]);
    a->state      = 1;
#ifdef __LPC176x__
    serial_uart_printf("ADC pin configured %d\n", args[1]);
#endif
}
DECL_COMMAND(command_config_analog_in, "config_analog_in oid=%c pin=%u");

void
command_query_analog_in(uint32_t *args)
{
    struct analog_in *a = oid_lookup(args[0], command_config_analog_in);
    sched_del_timer(&a->timer);
    gpio_adc_cancel_sample(a->pin);
    a->next_begin_time = args[1];
    a->timer.waketime  = a->next_begin_time;
    a->sample_time     = args[2];
    a->sample_count    = args[3];
    a->state           = a->sample_count + 1;
    a->rest_time       = args[4];
    a->min_value       = args[5];
    a->max_value       = args[6];
    if (! a->sample_count) {
        shutdown("ADC sample_count error!");
        return;
    }
    sched_add_timer(&a->timer);
}
DECL_COMMAND(command_query_analog_in,
             "query_analog_in oid=%c clock=%u sample_ticks=%u sample_count=%c"
             " rest_ticks=%u min_value=%hu max_value=%hu");

void
analog_in_task(void)
{
    if (!sched_check_wake(&analog_wake))
        return;
    uint8_t oid;
    struct analog_in *a;
    foreach_oid(oid, a, command_config_analog_in) {
        if (a->state != a->sample_count)
            continue;
        irq_disable();
        if (a->state != a->sample_count) {
            irq_enable();
            continue;
        }
        uint16_t const value = a->value;
        uint32_t const next_begin_time = a->next_begin_time;
        a->state++;
        irq_enable();
        sendf("analog_in_state oid=%c next_clock=%u value=%hu"
              , oid, next_begin_time, value);
    }
}
DECL_TASK(analog_in_task);

void
analog_in_shutdown(void)
{
    uint8_t oid;
    struct analog_in *a;
    foreach_oid(oid, a, command_config_analog_in) {
        gpio_adc_cancel_sample(a->pin);
#if 0
        if (a->sample_count) {
            a->state = a->sample_count + 1;
            a->next_begin_time += a->rest_time;
            a->timer.waketime = a->next_begin_time;
            sched_add_timer(&a->timer);
        }
#endif
#if (CONFIG_SIMULATOR == 1 && CONFIG_MACH_LINUX == 1)
        printf("ADC shutdown! oid %u, sample_cnt %u\n",
               oid, a->sample_count);
#endif
    }
}
DECL_SHUTDOWN(analog_in_shutdown);
