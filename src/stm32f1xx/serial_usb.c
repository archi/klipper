// lpc176x serial port
//

#include <string.h>     // memmove
#include "autoconf.h"   // CONFIG_SERIAL_BAUD
#include "board/gpio.h" // gpio_peripheral
#include "board/io.h"   // readl
#include "board/irq.h"  // irq_save
#include "board/misc.h" // console_sendf
#include "command.h"    // DECL_CONSTANT
#include "sched.h"      // DECL_INIT


DECL_CONSTANT(SERIAL_BAUD, CONFIG_SERIAL_BAUD);


/****************************************************************
 * RX and TX FIFOs
 ****************************************************************/
#define SERIAL_BUFFER_SIZE 1024
static char receive_buf[SERIAL_BUFFER_SIZE];
static uint32_t receive_pos = 0;
static char transmit_buf[SERIAL_BUFFER_SIZE];
static uint32_t transmit_pos = 0, transmit_max = 0;



/****************************************************************
 * Serial hardware
 ****************************************************************/

void serial_init(void) {
    receive_pos = 0;
    transmit_pos = transmit_max = 0;

    // TODO Write INIT!
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
    uint8_t pop_count = 0;
    uint32_t rpos = readl(&receive_pos);
    int8_t ret = command_find_block(receive_buf, rpos, &pop_count);
    if (ret > 0) {
        command_dispatch(receive_buf, pop_count);
    }
    if (ret) {
        console_pop_input(pop_count);
    }
}
DECL_TASK(console_task);

// Encode and transmit a "response" message
void
console_sendf(const struct command_encoder *ce, va_list args)
{
    // Verify space for message
    uint32_t tpos = readl(&transmit_pos), tmax = readl(&transmit_max);
    if (tpos >= tmax) {
        tpos = tmax = 0;
        writel(&transmit_max, 0);
        writel(&transmit_pos, 0);
    }
    uint32_t max_size = ce->max_size;
    if (tmax + max_size > SERIAL_BUFFER_SIZE) {
        if (tmax + max_size - tpos > SERIAL_BUFFER_SIZE)
            // Not enough space for message
            return;
        // Disable TX irq and move buffer
        writel(&transmit_max, 0);
        tpos = readl(&transmit_pos);
        tmax -= tpos;
        memmove(&transmit_buf[0], &transmit_buf[tpos], tmax);
        writel(&transmit_pos, 0);
        writel(&transmit_max, tmax);
        //USBHwNakIntEnable(INACK_BI); // Enable TX ISR
    }

    // Generate message
    char *buf = &transmit_buf[tmax];
    uint32_t msglen = command_encodef(buf, ce, args);
    command_add_frame(buf, msglen);

    // Start message transmit
    writel(&transmit_max, tmax + msglen);
    //USBHwNakIntEnable(INACK_BI); // Enable TX ISR
}
