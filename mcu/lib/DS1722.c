// DS1722.c
//Thiven Anderson
//thanderson@g.hmc.edu
//10/12/2026
//Functions to configure the DS1722 temp sensor driver

#include "STM32L432KC.h"
#include "DS1722.h"

#define DS1722_CE PB6
#define DS1722_CONFIG 0x80
#define DS1722_TEMP_LSB 0x01

void setDS1722Resolution(uint8_t resolution) {
    if (resolution < 8 || resolution > 12) return;

    uint8_t config = (resolution - 8) << 1; //define register value based on res

    digitalWrite(DS1722_CE, PIO_HIGH); //active high
    //multiple byte burst transfer to address of config reg to set resolution
    spiSendReceive(DS1722_CONFIG);
    spiSendReceive(config);

    while (SPI1->SR & SPI_SR_BSY);
    digitalWrite(DS1722_CE, PIO_LOW);
}

void initDS1722(void) {
    setDS1722Resolution(12); //initialize at 12 bit res
}

float readDS1722Temp(void) {
    digitalWrite(DS1722_CE, PIO_HIGH); //active high
    //send dummy bits to address temperature bytes to read them
    spiSendReceive(DS1722_TEMP_LSB);
    uint8_t lsb = spiSendReceive(0x00); //read lsb of temp
    uint8_t msb = spiSendReceive(0x00); //read msb of temp

    while (SPI1->SR & SPI_SR_BSY);
    digitalWrite(DS1722_CE, PIO_LOW);

    int16_t raw_temp = (int16_t)(((uint16_t)msb << 8) | lsb); //stitch together bytes
    
    return raw_temp / 256.0f; //convert to temperature
}