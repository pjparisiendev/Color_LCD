#pragma once

#include <stdint.h>

void gd32_lcd_bus_init(void);
void gd32_lcd_bus_input(void);
void gd32_lcd_bus_output(void);
void gd32_lcd_pin_set(uintptr_t port, uint16_t pin);
void gd32_lcd_pin_reset(uintptr_t port, uint16_t pin);
void gd32_lcd_bus_write(uint16_t value);
uint16_t gd32_lcd_bus_read(void);
