// Main starting point for SAM3x8e boards.
//
// Copyright (C) 2016,2017  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "command.h" // DECL_CONSTANT
#include "sched.h" // sched_main

//#include <stm32f1xx_hal_rcc.h>
#include <stm32f1xx.h>
#include <core_cm3.h>

#include <string.h>

extern void serial_init(void);
extern void serial_uart_init(void);

DECL_CONSTANT(MCU, "stm32f1xx");


/****************************************************************
 * misc functions
 ****************************************************************/

void
command_reset(uint32_t *args)
{
    NVIC_SystemReset();
}
DECL_COMMAND_FLAGS(command_reset, HF_IN_SHUTDOWN, "reset");

// Main entry point
int main(void) {
    /*__HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();*/
    SystemInit();
    SystemCoreClockUpdate();

    //serial_uart_init();
    serial_init();

    sched_main();
    return 0;
}

void __firstStart(void) {
}
