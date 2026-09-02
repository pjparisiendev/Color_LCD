/*
 * Bafang LCD 860C/850C firmware
 *
 * Copyright (C) Casainho, 2018, 2019, 2020
 *
 * Released under the GPL License, Version 3
 */

#ifndef _MAIN_H_
#define _MAIN_H_

/* Existing project constants intentionally preserved. */
#define USART1_INTERRUPT_PRIORITY       3
#define USART1_DMA_INTERRUPT_PRIORITY   4
#define TIM4_INTERRUPT_PRIORITY         5
#define RTC_INTERRUT_PRIORITY           6

/* The E2.3/GD32 target uses the newer 850C LF60 panel path proven by the
 * working BIKEL 850C v1.1 binary. Legacy targets remain unchanged.
 */
#if defined(TARGET_APT_850C_GD32F303RET6) && !defined(DISPLAY_850C_LF60)
#define DISPLAY_850C_LF60
#endif

#endif // _MAIN_H_
