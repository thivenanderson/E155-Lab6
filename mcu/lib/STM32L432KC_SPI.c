// STM32L432KC_SPI.c
//Thiven Anderson
//thanderson@g.hmc.edu
//10/12/2026
//Functions to configure the SPI driver

#include "STM32L432KC.h"
#include "STM32L432KC_SPI.h"

void initSPI(int br, int cpol, int cpha) {
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    // Configure pin modes as ALT function
    pinMode(PA4, GPIO_OUTPUT); // CE
    pinMode(PA5, GPIO_ALT); // SCK
    pinMode(PA6, GPIO_ALT); // MISO
    pinMode(PA7, GPIO_ALT); // MOSI

    digitalWrite(PA4, PIO_LOW); // Disable DS1722

    // Configure correct alternate functions
    GPIOA->AFR[0] &= ~((0xF << GPIO_AFRL_AFSEL5_Pos)
                       |(0xF << GPIO_AFRL_AFSEL6_Pos)
                       |(0xF << GPIO_AFRL_AFSEL7_Pos));   
    GPIOA->AFR[0] |= ((0b0101 << GPIO_AFRL_AFSEL5_Pos)
                       |(0b0101 << GPIO_AFRL_AFSEL6_Pos) 
                       |(0b0101 << GPIO_AFRL_AFSEL7_Pos)); 

    // Configure SPI1 as master with software NSS
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
              | (br << SPI_CR1_BR_Pos)
              | (cpol << SPI_CR1_CPOL_Pos)
              | (cpha << SPI_CR1_CPHA_Pos);
    // 8-bit data frames and 8-bit RX threshold
    SPI1->CR2 = (0b0111 << SPI_CR2_DS_Pos) | SPI_CR2_FRXTH;

    // Enable SPI1
    SPI1->CR1 |= SPI_CR1_SPE;
}

char spiSendReceive(char send) {
// Wait until transmit buffer is ready
    while (!(SPI1->SR & SPI_SR_TXE));

    // Send 8 bits
    *(__IO uint8_t *) & SPI1->DR = (uint8_t)send;

    // Wait until data is received
    while (!(SPI1->SR & SPI_SR_RXNE));

    // Read and return received data
    return *(__IO uint8_t *)&SPI1->DR;
}