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
#include "adc.h"

#define MIN_BATTERY_VOLTAGE 300U

#ifdef TARGET_APT_850C_GD32F303RET6
#define PA_OUTPUTS (GPIO_PIN_3 | GPIO_PIN_7 | GPIO_PIN_9)
#define PA_INPUTS  (GPIO_PIN_4 | GPIO_PIN_10 | GPIO_PIN_15)
#define PC_OUTPUTS (GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7)
#define PC_INPUTS  (GPIO_PIN_11 | GPIO_PIN_12)
typedef char apt_pin_map_no_io_conflict[
    ((PA_OUTPUTS & PA_INPUTS) == 0U && (PC_OUTPUTS & PC_INPUTS) == 0U) ? 1 : -1];
typedef char apt_pin_map_preserves_swd[
    (((PA_OUTPUTS | PA_INPUTS) & (GPIO_PIN_13 | GPIO_PIN_14)) == 0U) ? 1 : -1];
#endif

void power_latch_test_run(void)
{
#ifdef TARGET_APT_850C_GD32F303RET6
  /* Deliberately bypass the normal platform/LCD/application initialization.
   * If execution reaches here, prove it by latching PC1 HIGH and blinking the
   * PA7 backlight pin forever using GPIO only. */
  rcu_periph_clock_enable(RCU_GPIOA);
  rcu_periph_clock_enable(RCU_GPIOC);

  gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
  gpio_bit_reset(GPIOC, GPIO_PIN_1);

  for (volatile uint32_t i = 0U; i < 100000U; i++)
  {
    __NOP();
  }

  gpio_bit_set(GPIOC, GPIO_PIN_1);

  gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);

  while (1)
  {
    gpio_bit_set(GPIOA, GPIO_PIN_7);
    for (volatile uint32_t i = 0U; i < 8000000U; i++)
    {
      __NOP();
    }

    gpio_bit_reset(GPIOA, GPIO_PIN_7);
    for (volatile uint32_t i = 0U; i < 8000000U; i++)
    {
      __NOP();
    }
  }
#else
  while (1)
  {
  }
#endif
}

void pins_init (void)
{
#ifdef TARGET_APT_850C_GD32F303RET6
  rcu_periph_clock_enable(RCU_GPIOA);
  rcu_periph_clock_enable(RCU_GPIOB);
  rcu_periph_clock_enable(RCU_GPIOC);
  rcu_periph_clock_enable(RCU_AF);

  /* Match the physically working BIKEL 850C startup exactly here: PC1 is
   * configured as the system-power output and is initially driven LOW.
   * main() waits 500 ms and system_power(1) then creates the required LOW->HIGH
   * transition on the latch. Preloading PC1 HIGH prevented that transition. */
  gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
  gpio_bit_reset(GPIOC, GPIO_PIN_1);

  gpio_init(GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, GPIO_PIN_12);
  gpio_init(GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, GPIO_PIN_11);
  gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, GPIO_PIN_15);

  gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);

  gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3);
  gpio_bit_reset(GPIOA, GPIO_PIN_3);
#else
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
  GPIO_ResetBits(SYSTEM_POWER_ON_OFF__PORT, SYSTEM_POWER_ON_OFF__PIN);

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
  uint16_t batteryVoltage = 0U;

  if(ui32_state)
  {
#ifdef TARGET_APT_850C_GD32F303RET6
    gpio_bit_set(GPIOC, GPIO_PIN_1);

    /* BIKEL delays before deciding whether to enable the USB-charge rail and
     * only enables it with a valid battery voltage. Keep PA3 out of the basic
     * power-latch operation. */
    for (uint32_t i = 0U; i < 1000000U; i++)
    {
      __NOP();
    }

    batteryVoltage = battery_voltage_10x_get();
    batteryVoltage = battery_voltage_10x_get();
    if (batteryVoltage > MIN_BATTERY_VOLTAGE)
    {
      gpio_bit_set(GPIOA, GPIO_PIN_3);
    }
    else
    {
      gpio_bit_reset(GPIOA, GPIO_PIN_3);
    }
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
