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
#include "eeprom.h"
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
  volatile uint32_t ui32_ms_loop_counter_1;

#ifndef TARGET_APT_850C_GD32F303RET6
  SetSysClockTo104Mhz();
#endif
#ifdef TARGET_APT_850C_GD32F303RET6
  gd32_platform_early_init();
#else
  RCC_APB1PeriphResetCmd(RCC_APB1Periph_WWDG, DISABLE);
  /* Preserve the APT bootloader region on legacy bootloader builds. */
#ifdef USE_WITH_BOOTLOADER
  NVIC_SetVectorTable(NVIC_VectTab_FLASH, (uint32_t) 0x4000);
#endif
#endif

  pins_init();
  system_power(1);
#ifdef LCD_BRINGUP_DIAGNOSTIC
  timer3_init();
  lcd_init();
  while (1)
  {
    volatile uint32_t delay;
    UG_FillScreen(C_RED);
    for (delay = 0; delay < 8000000U; delay++) { __NOP(); }
    UG_FillScreen(C_GREEN);
    for (delay = 0; delay < 8000000U; delay++) { __NOP(); }
    UG_FillScreen(C_BLUE);
    for (delay = 0; delay < 8000000U; delay++) { __NOP(); }
    UG_FillScreen(C_WHITE);
    for (delay = 0; delay < 8000000U; delay++) { __NOP(); }
  }
#endif
  adc_init();
  systick_init();
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
    // because of continue; at the end of each if code block that will stop the while (1) loop there,
    // the first if block code will have the higher priority over any others
    ui32_timer_base_counter_1ms = get_time_base_counter_1ms();
    if((ui32_timer_base_counter_1ms - ui32_ms_loop_counter_1) > 20) // every 20ms
    {
      ui32_ms_loop_counter_1 = ui32_timer_base_counter_1ms;

#ifdef TARGET_APT_850C_GD32F303RET6
      bafang_runtime_tick(20U);
#endif

      // next 2 lines takes about 11ms to execute (main menu). Measured on 2019.03.04.
      main_idle();
      continue;
    }
  }
}

#ifndef TARGET_APT_850C_GD32F303RET6
void SetSysClockTo128Mhz(void)
{
  ErrorStatus HSEStartUpStatus;

  /* SYSCLK, HCLK, PCLK2 and PCLK1 configuration -----------------------------*/
  /* RCC system reset(for debug purpose) */
  RCC_DeInit();

  /* Enable HSE */
  RCC_HSEConfig(RCC_HSE_ON);

  /* Wait till HSE is ready */
  HSEStartUpStatus = RCC_WaitForHSEStartUp();

  if (HSEStartUpStatus == SUCCESS)
  {
    /* Enable Prefetch Buffer */
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

    /* Flash 2 wait state */
    FLASH_SetLatency(FLASH_Latency_2);

    /* HCLK = SYSCLK */
    RCC_HCLKConfig(RCC_SYSCLK_Div1);

    /* PCLK2 = HCLK */
    RCC_PCLK2Config(RCC_HCLK_Div1);

    /* PCLK1 = HCLK/2 */
    RCC_PCLK1Config(RCC_HCLK_Div2);

    /* PLLCLK = 8MHz * 16 = 128 MHz */
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_13);

    /* Enable PLL */
    RCC_PLLCmd(ENABLE);

    /* Wait till PLL is ready */
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
    {
    }

    /* Select PLL as system clock source */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

    /* Wait till PLL is used as system clock source */
    while(RCC_GetSYSCLKSource() != 0x08)
    {
    }
  }
  else
  { /* If HSE fails to start-up, the application will have wrong clock configuration.
       User can add here some code to deal with this error */

    /* Go to infinite loop */
    while (1)
    {
    }
  }
}
#endif
