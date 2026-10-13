// DS1722.h
//Thiven Anderson
//thanderson@g.hmc.edu
//10/12/2026
//Header file for DS1722 driver
#ifndef DS1722_H
#define DS1722_H

#include <stdint.h>

void initDS1722(void);
void setDS1722Resolution(uint8_t resolution);
float readDS1722Temp(void);

#endif