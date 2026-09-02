/*
 * Bafang LCD 860C/850C firmware
 *
 * Copyright (C) Casainho, 2018, 2019, 2020
 *
 * Released under the GPL License, Version 3
 */

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "system_stm32f10x.h"
#include "stdio.h"
#include "stdbool.h"

#include "main.h"
#include "pins.h"
#include "lcd.h"
#include "buttons.h"
#include "eeprom.h"
#include "timers.h"
#include "timer.h"
#include "usart1.h"
#include "ugui.h"
#include "utils.h"
#include "rtc.h"
#include "stm32f10x_usart.h"
#include "mainscreen.h"
#include "configscreen.h"
#include "ugui_driver/ugui_display_8x0c.h"
#include "bafang_runtime.h"
#ifdef TARGET_APT_850C_GD32F303RET6
#include "gd32_runtime.h"
#endif

#ifndef TARGET_APT_850C_GD32F303RET6
void SetSysClockTo128Mhz(void);
#endif
void adc_init();

int main(void)
{
  volatile uint32_t ui32_timer_base_counter_1ms;
  volatile uint32_t ui32_ms_loop_counter_1 = 0U;

#ifdef TARGET_APT_850C_GD32F303RET6
  gd32_platform_early_init();
#else
  SetSysClockTo128Mhz();
  RCC_APB1PeriphResetCmd(RCC_APB1Periph_WWDG, DISABLE);
#ifdef USE_WITH_BOOTLOADER
  NVIC_SetVectorTable(NVIC_VectTab_FLASH, (uint32_t) 0x5000);
#endif
#endif

  /* Follow the startup order used by the known-working BIKEL 850C firmware. */
  systick_init();
  pins_init();
  delay_ms(500);
  adc_init();
  system_power(1);

#ifdef LCD_BRINGUP_DIAGNOSTIC
  timer3_init();
  lcd_init();

  /* Diagnostic builds skip EEPROM/UI state initialization, so do not depend
   * on set_lcd_backlight() finding a valid saved brightness. Force the same
   * PA7 PWM channel fully visible before drawing test colors. */
  lcd_set_backlight_intensity(100U);

  while (1)
  {
    UG_FillScreen(C_RED);
    delay_ms(1000);
    UG_FillScreen(C_GREEN);
    delay_ms(1000);
    UG_FillScreen(C_BLUE);
    delay_ms(1000);
    UG_FillScreen(C_WHITE);
    delay_ms(1000);
  }
#endif

  usart1_init();
  bafang_runtime_init();
  eeprom_init();
  rtc_init();
  timer3_init(); // drives LCD backlight
  lcd_init();
  timer4_init();
  screen_init();

  screenShow(&bootScreen);

  while(1)
  {
    ui32_timer_base_counter_1ms = get_time_base_counter_1ms();
    if((ui32_timer_base_counter_1ms - ui32_ms_loop_counter_1) > 20)
    {
      ui32_ms_loop_counter_1 = ui32_timer_base_counter_1ms;

#ifdef TARGET_APT_850C_GD32F303RET6
      bafang_runtime_tick(20U);
#endif

      main_idle();
      continue;
    }
  }
}

#ifndef TARGET_APT_850C_GD32F303RET6
void SetSysClockTo128Mhz(void)
{
  ErrorStatus HSEStartUpStatus;

  RCC_DeInit();
  RCC_HSEConfig(RCC_HSE_ON);
  HSEStartUpStatus = RCC_WaitForHSEStartUp();

  if (HSEStartUpStatus == SUCCESS)
  {
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
    FLASH_SetLatency(FLASH_Latency_2);
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK2Config(RCC_HCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div2);
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_16);
    RCC_PLLCmd(ENABLE);

    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
    {
    }

    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    while(RCC_GetSYSCLKSource() != 0x08)
    {
    }
  }
  else
  {
    while (1)
    {
    }
  }
}
#endif
