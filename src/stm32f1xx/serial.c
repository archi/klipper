// sam3x8e serial port
//
// Copyright (C) 2016,2017  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <string.h> // memmove
#include "autoconf.h" // CONFIG_SERIAL_BAUD
#include "board/gpio.h" // gpio_peripheral
#include "board/io.h" // readl
#include "board/irq.h" // irq_save
#include "board/misc.h" // console_sendf
#include "command.h" // DECL_CONSTANT
#include "libmaple/usb_cdcacm.h" // UART
#include "sched.h" // DECL_INIT

#define SERIAL_BUFFER_SIZE 96
static char receive_buf[SERIAL_BUFFER_SIZE];
static uint32_t receive_pos;
static char transmit_buf[SERIAL_BUFFER_SIZE];


/****************************************************************
 * Serial hardware
 ****************************************************************/

DECL_CONSTANT(SERIAL_BAUD, CONFIG_SERIAL_BAUD);

void rx_hook(unsigned, void*);

void
serial_init(void)
{
    //GPIO and bit can be found as BOARD_USB_DISC_{DEV,BIT} in stm32duino boards.h
    usb_cdcacm_enable(GPIOA, (uint8_t)0);
    usb_cdcacm_set_hooks(USB_CDCACM_HOOK_RX, rx_hook);
}
DECL_INIT(serial_init);

void __visible
rx_hook(unsigned hook, void* dunno)
{
    (void)hook; (void)dunno;

    while (usb_cdcacm_data_available()) {
        uint8_t data;
        usb_cdcacm_rx(&data, 1);
        if (data == MESSAGE_SYNC)
            sched_wake_tasks();
        if (receive_pos >= sizeof(receive_buf))
            // Serial overflow - ignore it as crc error will force retransmit
            return;
        receive_buf[receive_pos++] = data;
        return;
    }
}


/****************************************************************
 * Console access functions
 ****************************************************************/

// Remove from the receive buffer the given number of bytes
static void
console_pop_input(uint32_t len)
{
    uint32_t copied = 0;
    for (;;) {
        uint32_t rpos = readl(&receive_pos);
        uint32_t needcopy = rpos - len;
        if (needcopy) {
            memmove(&receive_buf[copied], &receive_buf[copied + len]
                    , needcopy - copied);
            copied = needcopy;
            sched_wake_tasks();
        }
        irqstatus_t flag = irq_save();
        if (rpos != readl(&receive_pos)) {
            // Raced with irq handler - retry
            irq_restore(flag);
            continue;
        }
        receive_pos = needcopy;
        irq_restore(flag);
        break;
    }
}

// Process any incoming commands
void
console_task(void)
{
    uint8_t pop_count;
    uint32_t rpos = readl(&receive_pos);
    int8_t ret = command_find_block(receive_buf, rpos, &pop_count);
    if (ret > 0)
        command_dispatch(receive_buf, pop_count);
    if (ret)
        console_pop_input(pop_count);
}
DECL_TASK(console_task);

// Encode and transmit a "response" message
void
console_sendf(const struct command_encoder *ce, va_list args)
{
    // Generate message
    uint32_t msglen = command_encodef(transmit_buf, ce, args);
    command_add_frame(transmit_buf, msglen);
    usb_cdcacm_tx((uint8_t*)transmit_buf, msglen);
}
