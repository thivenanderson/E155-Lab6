/*
 * DS1722 temperature sensor driver
 * Target: STM32L432KC, CMSIS register access only (no HAL)
 *
 * Pin mapping (SPI1, AF5):
 *   PA5 = SCK
 *   PA6 = MISO  (connects to DS1722 SDO)
 *   PB5 = MOSI  (connects to DS1722 SDI)
 *   PB0 = CE    (GPIO, active HIGH)
 */

#include "stm32l4xx.h"
#include <stdint.h>

/* ---- DS1722 registers (SPI addresses) ---- */
#define DS1722_REG_CFG_READ   0x00
#define DS1722_REG_TEMP_LSB   0x01   /* MSB is at 0x02; burst reads auto-increment */
#define DS1722_REG_CFG_WRITE  0x80

/* Config register: 1 1 1 1SHOT R2 R1 R0 SD */
#define DS1722_CFG_BASE       0xE0
#define DS1722_CFG_RES(bits)  ((uint8_t)(((bits) - 8) << 1))   /* 8..12 bits */

/* CE is active HIGH on the DS1722 */
#define DS1722_CE_HIGH()  (GPIOB->BSRR = GPIO_BSRR_BS0)
#define DS1722_CE_LOW()   (GPIOB->BSRR = GPIO_BSRR_BR0)

/* Exchange one byte. Byte-wide DR access is required on the L4 so the
   FIFO doesn't pack/expect 16-bit frames. */
static uint8_t spi1_xfer(uint8_t tx)
{
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    *(volatile uint8_t *)&SPI1->DR = tx;
    while (!(SPI1->SR & SPI_SR_RXNE)) { }
    return *(volatile uint8_t *)&SPI1->DR;
}

void ds1722_init(uint8_t resolution_bits)
{
    /* Clocks */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* PA5 (SCK) and PA6 (MISO) -> alternate function 5 */
    GPIOA->MODER &= ~(GPIO_MODER_MODE5 | GPIO_MODER_MODE6);
    GPIOA->MODER |=  (GPIO_MODER_MODE5_1 | GPIO_MODER_MODE6_1);
    GPIOA->AFR[0] &= ~((0xFUL << 20) | (0xFUL << 24));
    GPIOA->AFR[0] |=  ((5UL << 20) | (5UL << 24));

    /* PB5 (MOSI) -> alternate function 5 */
    GPIOB->MODER &= ~GPIO_MODER_MODE5;
    GPIOB->MODER |=  GPIO_MODER_MODE5_1;
    GPIOB->AFR[0] &= ~(0xFUL << 20);
    GPIOB->AFR[0] |=  (5UL << 20);

    /* PB0 -> push-pull output for CE, idle low */
    GPIOB->MODER &= ~GPIO_MODER_MODE0;
    GPIOB->MODER |=  GPIO_MODER_MODE0_0;
    DS1722_CE_LOW();

    /* SPI1: master, software NSS, mode 1 (CPOL=0, CPHA=1), MSB first,
       fPCLK/4 (1 MHz at the default 4 MHz MSI; DS1722 max is 2 MHz) */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                SPI_CR1_CPHA | (1U << SPI_CR1_BR_Pos);
    SPI1->CR2 = (7U << SPI_CR2_DS_Pos) | SPI_CR2_FRXTH;   /* 8-bit frames */
    SPI1->CR1 |= SPI_CR1_SPE;

    /* Write config: continuous conversion (SD = 0), chosen resolution */
    DS1722_CE_HIGH();
    spi1_xfer(DS1722_REG_CFG_WRITE);
    spi1_xfer(DS1722_CFG_BASE | DS1722_CFG_RES(resolution_bits));
    while (SPI1->SR & SPI_SR_BSY) { }
    DS1722_CE_LOW();
}

/* Returns temperature as signed fixed-point, 8 fractional bits
   (degrees C = raw / 256.0). Resolution is 1/16 degC at 12 bits. */
int16_t ds1722_read_raw(void)
{
    DS1722_CE_HIGH();
    spi1_xfer(DS1722_REG_TEMP_LSB);          /* address phase */
    uint8_t lsb = spi1_xfer(0xFF);           /* 0x01: fractional bits in [7:4] */
    uint8_t msb = spi1_xfer(0xFF);           /* 0x02: signed integer part */
    while (SPI1->SR & SPI_SR_BSY) { }
    DS1722_CE_LOW();

    return (int16_t)(((uint16_t)msb << 8) | lsb);
}

float ds1722_read_celsius(void)
{
    return (float)ds1722_read_raw() / 256.0f;
}

/* Example usage:
 *
 *   ds1722_init(12);          // 12-bit resolution
 *   // wait ~1.2 s for the first conversion
 *   float t = ds1722_read_celsius();
 */
