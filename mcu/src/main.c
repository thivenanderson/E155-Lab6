#include "stm32l4xx.h"
#include <stdint.h>

/* Driver functions from ds1722.c */
void    ds1722_init(uint8_t resolution_bits);
int16_t ds1722_read_raw(void);
float   ds1722_read_celsius(void);

/* Crude busy-wait; roughly 1 s per 1,000,000 at the default 4 MHz clock */
static void delay(volatile uint32_t n)
{
    while (n--) { }
}

volatile float temperature_c;   /* volatile so you can watch it in the debugger */

int main(void)
{
    ds1722_init(12);            /* 12-bit resolution */
    delay(1500000);             /* wait for first conversion (~1.2 s) */

    while (1) {
        temperature_c = ds1722_read_celsius();
        delay(500000);
    }
}