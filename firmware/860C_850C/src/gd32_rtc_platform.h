#pragma once

#include <stdbool.h>
#include <stdint.h>

bool gd32_rtc_platform_init(void);
uint32_t gd32_rtc_counter_get(void);
void gd32_rtc_counter_set(uint32_t value);
void gd32_rtc_clear_second_irq(void);
