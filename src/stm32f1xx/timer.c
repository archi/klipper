// SAM3x8e timer interrupt scheduling
//
// Copyright (C) 2016,2017  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "board/irq.h" // irq_disable
#include "board/misc.h" // timer_read_time
#include "board/timer_irq.h" // timer_dispatch_many
#include "command.h" // DECL_SHUTDOWN
#include <libmaple/timer.h>
#include <libmaple/../../timer_private.h>
#include "sched.h" // DECL_INIT

#define TIMER TIMER1
#define IRQ 0
#define CHANNEL 0

// Set the next irq time
static void
timer_set(uint32_t value)
{
    timer_set_count(TIMER, 0);
    timer_set_compare(TIMER, CHANNEL, value); //this will be cast ot uint16!
}

// Return the current time (in absolute clock ticks).
uint32_t
timer_read_time(void)
{
    return timer_get_count(TIMER);
}

// Activate timer dispatch as soon as possible
void
timer_kick(void)
{
    timer_set_reload(TIMER, timer_get_compare(TIMER, CHANNEL) - 2);
}

// IRQ handler
static void __visible __aligned(16) // aligning helps stabilize perf benchmarks
timer_handler(void)
{
    timer_disable_irq(TIMER, IRQ);
    uint32_t next = timer_dispatch_many();
    timer_set(next);
    timer_enable_irq(TIMER, IRQ);
}

void
timer_init_stm32(void)
{
    timer_init(TIMER);
    timer_set_mode(TIMER, CHANNEL, TIMER_OUTPUT_COMPARE); 
    timer_enable_irq(TIMER, IRQ);
    timer_attach_interrupt(TIMER, IRQ, &timer_handler);
    timer_kick();
}
DECL_INIT(timer_init_stm32);
