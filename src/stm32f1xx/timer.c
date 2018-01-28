// SAM3x8e timer interrupt scheduling
//
// Copyright (C) 2016,2017  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "autoconf.h"
#include "board/irq.h" // irq_disable
#include "board/misc.h" // timer_read_time
#include "board/timer_irq.h" // timer_dispatch_many
#include "command.h" // DECL_SHUTDOWN
#include "sched.h" // DECL_INIT

#include <stm32f1xx_hal_rcc.h>
//#include <stm32f1xx_hal_tim.h>
#include <stm32f1xx.h>

void timer_isr(void);

static __attribute__((always_inline)) inline void
tc_clear_irq(void) {
    (void)TIM1->CCR1;
}

// Set the next irq time
static void
timer_set(uint32_t const value) {
    TIM1->CCR1 = value;
}

// Return the current time (in absolute clock ticks).
uint32_t
timer_read_time(void) {
    return TIM1->CNT; // Return current timer counter value
}

// Activate timer dispatch as soon as possible
void
timer_kick(void) {
    timer_set(TIM1->CNT + 200);
    tc_clear_irq();
}

void
timer_init(void)
{
    const uint16_t timPrescaler = 1; //(SystemCoreClock / 2) - 1 => 72MHz / 2 = 36MHz;
    uint32_t tmpcr1 = 0;

    NVIC_DisableIRQ(TIM1_BRK_IRQn);
    NVIC_DisableIRQ(TIM1_UP_IRQn);
    NVIC_DisableIRQ(TIM1_TRG_COM_IRQn);
    NVIC_DisableIRQ(TIM1_CC_IRQn);

    /* Enable clocks just in case */
    __HAL_RCC_TIM1_CLK_ENABLE();

    /* Set the Prescaler value */
    TIM1->PSC = timPrescaler;

    /* Enable the TIM CounterMatch interrupt */
    TIM1->DIER = (TIM_DIER_CC1IE);

    /* Enable the counter */
    tmpcr1 |= (TIM_CR1_CEN);
    TIM1->CR1 = tmpcr1;

    // Start timer
    NVIC_EnableIRQ(TIM1_CC_IRQn); // Enable IRQ
    timer_kick();
}
DECL_INIT(timer_init);

// IRQ handler
void TIM1_UP_IRQHandler(void)
{
    irq_disable();
    uint32_t const status = TIM1->SR; // Read pending IRQ
    tc_clear_irq();
    if (likely(status & (TIM_SR_CC1IF))) {
        uint32_t const next = timer_dispatch_many();
        timer_set(next);
    }
    irq_enable();
}
