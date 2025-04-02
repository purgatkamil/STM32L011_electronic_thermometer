#ifndef OLED_H
#define OLED_H

#include "stm32l0xx_hal.h"

void oled_init(void);
void oled_clear(void);
void oled_putc(uint8_t x, uint8_t y, char c);
void oled_print(uint8_t x, uint8_t y, const char *str);
void oled_display(void);

#endif
