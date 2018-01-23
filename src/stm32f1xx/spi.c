#include <stddef.h>   // NULL
#include "sched.h"
#include "autoconf.h"
#include "gpio.h"
#include "generic/spi.h"

#include <stm32f1xx_hal_rcc.h>
#include <stm32f1xx_hal_rcc_ex.h>
#include <stm32f1xx.h>


typedef struct SPI_mapping_t {
    _gpio_peripheral_t clk;
    _gpio_peripheral_t mosi;
    _gpio_peripheral_t miso;
} SPI_mapping_t;

#if (SPI == 1)
#define SPI_REG SPI1
#elif (SPI == 2)
#define SPI_REG SPI2
#else
    #error " Unknown SPI configuration!"
#endif

/*
 * SPI or SSP0/1
 *   * SSEL is handled by application using GPIO
 */
static const SPI_mapping_t g_pinsSPI[] = {
#if (SPI == 1)
#if (SPI_REMAP == 0)
    {
        // SPI - Default. Note: overlap with ADC pins!
        { 0, 5, 0b10, 3 }, // CLK  - PA.4  // Alternate function push-pull
        { 0, 7, 0b10, 3 }, // MOSI - PA.7  // Alternate function push-pull
        { 0, 6, 0b10, 0 }  // MISO - PA.6  // Input floating / Input pull-up
    }
#else
    {
        // SPI - Remap = 1
        { 1, 3, 0b10, 3 }, // CLK  - PB.3  // Alternate function push-pull
        { 1, 5, 0b10, 3 }, // MOSI - PB.5  // Alternate function push-pull
        { 1, 4, 0b10, 0 }  // MISO - PB.4  // Input floating / Input pull-up
    }
#endif
#elif (SPI == 2)
    {
        // SPI2
        { 1, 13, 0b10, 3 }, // CLK  - PB.13  // Alternate function push-pull
        { 1, 15, 0b10, 3 }, // MOSI - PB.15  // Alternate function push-pull
        { 1, 14, 0b10, 0 }  // MISO - PB.14  // Input floating / Input pull-up
    }
#endif
};


SPI_t spi_basic_config = 0;

static uint32_t
spi_get_clock(uint32_t const target_clock)
{
    uint32_t const spi_pclk = HAL_RCC_GetPCLK1Freq();
    uint32_t prescale = 0;
    // Find closest clock to target clock
    do {
        prescale++;
    } while ((target_clock * (2 * (1 << prescale))) < spi_pclk &&
             prescale <= 7);
    return prescale;
}


void
spi_init(void)
{
    /* Configure SCK, MISO and MOSI */
    gpio_peripheral(&g_pinsSPI[0].clk,  0);
    gpio_peripheral(&g_pinsSPI[0].mosi, 0);
    gpio_peripheral(&g_pinsSPI[0].miso, 1);

    // Power on the SPI
#if (SPI == 1)
    __HAL_RCC_SPI1_CLK_ENABLE();
#elif (SPI == 2)
    __HAL_RCC_SPI2_CLK_ENABLE();
#endif

#if (SPI_REMAP == 1)
    AFIO->MAPR  |= AFIO_MAPR_SPI1_REMAP;
#endif

    // Set SPI default settings
    spi_basic_config = spi_get_config(0, 4000000); // 4MHz, SPI Mode0
    spi_set_config(spi_basic_config); // Set default SPI config
}
DECL_INIT(spi_init);

SPI_t
spi_get_config(uint8_t const mode, uint32_t const clock)
{
    SPI_t config = 0; // | CR2 (16) | CR1 (16) |
    config |= SPI_CR1_MSTR; // Master
    config |= SPI_CR1_SPE; // SPI enabled

    switch(mode) {
        case 0:
            config |= (0);
            config |= (0);
            break;
        case 1:
            config |= (SPI_CR1_CPHA);
            break;
        case 2:
            config |= (SPI_CR1_CPOL);
            break;
        case 3:
            config |= (SPI_CR1_CPHA);
            config |= (SPI_CR1_CPOL);
            break;
    };

    // Calculate SPI prescaler
    config |= (spi_get_clock(clock) << SPI_CR1_BR_Pos) & SPI_CR1_BR;
    config &= 0xFFFF; // Store config , 0xFFFF

    // Set CR2:

    return config;
}

void
spi_set_config(SPI_t const config)
{
    SPI_REG->CR1 = (config & 0xFFFF); // Set config 1
    SPI_REG->CR2 = (config >> 16);    // Set config 2
}

void
spi_transfer_len(char *data, uint8_t len)
{
    uint16_t i;
    for (i = 0; i < len; i++) {
        data[i] = spi_transfer(data[i], 0);
    }
}

uint8_t
spi_transfer(uint8_t const data, uint8_t const last)
{
    // write byte with address and end transmission flag
    SPI_REG->DR = data;
    // wait for transmit register empty
    while (!(SPI_REG->SR & SPI_SR_TXE));
    // get data
    return (uint8_t)(SPI_REG->DR & SPI_DR_DR);
}
