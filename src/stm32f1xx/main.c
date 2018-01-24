// Main starting point for SAM3x8e boards.
//
// Copyright (C) 2016,2017  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "command.h" // DECL_CONSTANT
#include <libmaple/iwdg.h> //iwdg_feed/iwdg_init
#include <libmaple/nvic.h> //nvic_sys_reset 
#include <libmaple/usb_cdcacm.h> 
#include "sched.h" // sched_main

DECL_CONSTANT(MCU, "stm32f1x3");


/****************************************************************
 * watchdog handler
 ****************************************************************/

void
watchdog_reset(void)
{
    iwdg_feed();
}
DECL_TASK(watchdog_reset);

void
watchdog_init(void)
{
    //should be about 500ms
    iwdg_init(IWDG_PRE_32, 500);
}
DECL_INIT(watchdog_init);

/****************************************************************
 * misc functions
 ****************************************************************/

void
command_reset(uint32_t *args)
{
    nvic_sys_reset();
}
DECL_COMMAND_FLAGS(command_reset, HF_IN_SHUTDOWN, "reset");

// Main entry point
int
main(void)
{
    usb_cdcacm_enable(GPIOA, (uint8_t)0);
   // sched_main();
    return 0;
}
