/*
 * Bafang LCD 860C/850C firmware
 *
 * Copyright (C) Casainho, 2018, 2019, 2020
 *
 * Released under the GPL License, Version 3
 */

#ifdef TARGET_APT_850C_GD32F303RET6
#include "gd32f30x_gpio.h"
#include "gd32f30x_rcu.h"
#else
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "pins.h"
#endif
#include "stdio.h"

#ifdef TARGET_APT_850C_GD32F303RET6
/* Static E2.3 candidate pin-map conflict checks. Physical confirmation is
 * still required, but accidental software overlap now fails the build. */
#define PA_OUTPUTS (GPIO_PIN_3 | GPIO_PIN_7 | GPIO_PIN_9)
#define PA_INPUTS  (GPIO_PIN_4 | GPIO_PIN_10 | GPIO_PIN_15)
#define PC_OUTPUTS (GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7)
#define PC_INPUTS  (GPIO_PIN_11 | GPIO_PIN_12)
typedef char apt_pin_map_no_io_conflict[
    ((PA_OUTPUTS & PA_INPUTS) == 0U && (PC_OUTPUTS & PC_INPUTS) == 0U) ? 1 : -1];
typedef char apt_pin_map_preserves_swd[
    (((PA_OUTPUTS | PA_INPUTS) & (GPIO_PIN_13 | GPIO_PIN_14)) == 0U) ? 1 : -1];
#endif

void pins_init (void)
{
#ifdef TARGET_APT_850C_GD32F303RET6
  rcu_periph_clock_enable(RCU_GPIOA);
  rcu_periph_clock_enable(RCU_GPIOB);
  rcu_periph_clock_enable(RCU_GPIOC);
  rcu_periph_clock_enable(RCU_AF);

  /* Preload safe levels before changing output modes. PC1 holds display power;
   * USB charging remains off until system_power() explicitly enables it. */
  gpio_bit_set(GPIOC, GPIO_PIN_1);
  gpio_bit_reset(GPIOA, GPIO_PIN_3);
  gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
  gpio_init(GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, GPIO_PIN_12);
  gpio_init(GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, GPIO_PIN_11);
  gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, GPIO_PIN_15);
  gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
  gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3);
#else
  // enable clocks
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                         RCC_APB2Periph_GPIOB |
                         RCC_APB2Periph_GPIOC |
						 RCC_APB2Periph_ADC1 |
                         RCC_APB2Periph_AFIO,
                         ENABLE);

  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Pin = SYSTEM_POWER_ON_OFF__PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(SYSTEM_POWER_ON_OFF__PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = BUTTON_ONOFF__PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(BUTTON_ONOFF__PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = BUTTON_UP__PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(BUTTON_UP__PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = BUTTON_DOWN__PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(BUTTON_DOWN__PORT, &GPIO_InitStructure);

#ifdef DISPLAY_860C
  GPIO_InitStructure.GPIO_Pin = BUTTON_M__PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(BUTTON_M__PORT, &GPIO_InitStructure);
#endif

  GPIO_InitStructure.GPIO_Pin = LCD_BACKLIGHT__PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(LCD_BACKLIGHT__PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = USB_CHARGE__PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(USB_CHARGE__PORT, &GPIO_InitStructure);
#endif
}

void system_power(uint32_t ui32_state)
{
  if(ui32_state)
  {
#ifdef TARGET_APT_850C_GD32F303RET6
    gpio_bit_set(GPIOC, GPIO_PIN_1);
    gpio_bit_set(GPIOA, GPIO_PIN_3);
#else
    GPIO_SetBits(SYSTEM_POWER_ON_OFF__PORT, SYSTEM_POWER_ON_OFF__PIN);
    GPIO_SetBits(USB_CHARGE__PORT, USB_CHARGE__PIN);
#endif
  }
  else
  {
#ifdef TARGET_APT_850C_GD32F303RET6
    gpio_bit_reset(GPIOC, GPIO_PIN_1);
    gpio_bit_reset(GPIOA, GPIO_PIN_3);
#else
    GPIO_ResetBits(SYSTEM_POWER_ON_OFF__PORT, SYSTEM_POWER_ON_OFF__PIN);
    GPIO_ResetBits(USB_CHARGE__PORT, USB_CHARGE__PIN);
#endif
  }
}
