#include <stddef.h>   // NULL
#include "autoconf.h"
#include "gpio.h"
#include "generic/spi.h"
#include "sched.h"
#include <sam3x8e.h>

#define REGPTR     SPI0
#define PERIPH_ID  ID_SPI0

#define CHANNEL    0 // Use same channel for all

#define SPCK  GPIO('A', 27)
#define MOSI  GPIO('A', 26)
#define MISO  GPIO('A', 25)

SPI_t spi_basic_config = 0;


void
spi_init(void)
{
    /* Configure SCK, MISO and MOSI */
    gpio_peripheral('A', PIO_PA25A_SPI0_MISO, 'A', 0);
    gpio_peripheral('A', PIO_PA26A_SPI0_MOSI, 'A', 0);
    gpio_peripheral('A', PIO_PA27A_SPI0_SPCK, 'A', 0);
    //gpio_in_setup(MISO, 0); // needed?

    // Power ON SPI
    if (!(PMC->PMC_PCSR0 & (1u << PERIPH_ID))) {
        PMC->PMC_PCER0 = (1 << PERIPH_ID);
    }

    REGPTR->SPI_CR  = SPI_CR_SPIDIS;
    /* Execute a software reset of the SPI twice */
    REGPTR->SPI_CR  = SPI_CR_SWRST;
    REGPTR->SPI_CR  = SPI_CR_SWRST;
    // set master mode, peripheral select, fault detection
    REGPTR->SPI_MR |= SPI_MR_MSTR;;
    REGPTR->SPI_MR |= SPI_MR_MODFDIS;
    REGPTR->SPI_MR &= ~(SPI_MR_LLB);
    REGPTR->SPI_MR |= SPI_MR_PS;
    REGPTR->SPI_MR &= ~(SPI_MR_PCSDEC);
    REGPTR->SPI_CR  = SPI_CR_SPIEN;

    REGPTR->SPI_CSR[0] = 0;
    REGPTR->SPI_CSR[1] = 0;
    REGPTR->SPI_CSR[2] = 0;
    REGPTR->SPI_CSR[3] = 0;
    REGPTR->SPI_CR = SPI_CR_SPIEN;

    spi_basic_config = spi_get_config(0, 4000000);
}
DECL_INIT(spi_init);

SPI_t
spi_get_config(uint8_t const mode, uint32_t const clock)
{
    SPI_t config = 0;
    uint8_t clockDiv;
    if (clock < (CHIP_FREQ_CPU_MAX / 255)) {
        clockDiv = 255;
    } else if (clock >= (CHIP_FREQ_CPU_MAX / 2)) {
        clockDiv = 2;
    } else {
        clockDiv = (CHIP_FREQ_CPU_MAX / (clock + 1)) + 1;
    }

    // CSAAT  : Chip Select Active After Transfer
    // DLYBCT : Delay Between Consecutive Transfers
    config = SPI_CSR_CSAAT | SPI_CSR_DLYBCT(1);  // Will be written to SPI_CSRx register

    switch(mode) {
        case 0:
            config |= 0;
            break;
        case 1:
            config |= SPI_CSR_NCPHA;
            break;
        case 2:
            config |= SPI_CSR_CPOL;
            break;
        case 3:
            config |= SPI_CSR_NCPHA;
            config |= SPI_CSR_CPOL;
            break;
    };

    //config &= ~(SPI_CSR_SCBR_Msk); // Clear current
    config |= SPI_CSR_SCBR(clockDiv);

    return config;
}

void
spi_set_config(SPI_t const config)
{
    REGPTR->SPI_CSR[CHANNEL] = config;
}

void
spi_transfer_len(char *data, uint8_t len)
{
    uint16_t _lenTmp = len - 1;
    uint16_t i;
    for (i = 0; i < _lenTmp; i++) {
        data[i] = spi_transfer(data[i], 0); // CONTINUE = 0
    }
    data[i] = spi_transfer(data[i], 1); // LAST = 1
}

uint8_t
spi_transfer(uint8_t const data, uint8_t const last)
{
    uint32_t tmp = SPI_TDR_TD(data) | SPI_TDR_PCS(CHANNEL);
    if (last) tmp |= SPI_TDR_LASTXFER;
    // write byte with address and end transmission flag
    REGPTR->SPI_TDR = tmp;
    // wait for transmit register empty
    while ((REGPTR->SPI_SR & SPI_SR_TDRE) == 0);
    // wait for receive register
    while ((REGPTR->SPI_SR & SPI_SR_RDRF) == 0);
    // get data
    return REGPTR->SPI_RDR;
}
