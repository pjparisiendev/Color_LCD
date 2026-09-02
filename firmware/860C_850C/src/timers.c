/*
 * Bafang LCD 860C/850C firmware
 *
 * Copyright (C) Casainho, 2018, 2019, 2020
 *
 * Released under the GPL License, Version 3
 */

#ifdef TARGET_APT_850C_GD32F303RET6
#include "gd32f30x.h"
#include "gd32f30x_misc.h"
#include "gd32f30x_rcu.h"
#include "gd32f30x_timer.h"
#else
#include "stm32f10x.h"
#endif
#include "timers.h"
#include "main.h"
#include "state.h"

static volatile uint32_t _ms;
volatile uint32_t time_base_counter_1ms = 0;

void delay_ms (uint32_t ms)
{
  _ms = 1;
  while (ms >= _ms) ;
}

void SysTick_Handler(void) // runs every 1ms
{
  _ms++; // for delay_ms ()

  time_base_counter_1ms++;
}

void systick_init (void)
{
  /* Setup SysTick Timer for 1 millisecond interrupts, also enables Systick and Systick-Interrupt */
  if (SysTick_Config(SystemCoreClock / 1000))
  {
    /* Capture error */
    while (1);
  }
}

uint32_t get_time_base_counter_1ms (void)
{
  return time_base_counter_1ms;
}

// used for LCD backlight
void timer3_init(void)
{
#ifdef TARGET_APT_850C_GD32F303RET6
  timer_parameter_struct timer_initpara;
  timer_oc_parameter_struct timer_ocinitpara;

  rcu_periph_clock_enable(RCU_TIMER2);
  timer_deinit(TIMER2);
  timer_struct_para_init(&timer_initpara);
  timer_initpara.prescaler = 29U;
  timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
  timer_initpara.counterdirection = TIMER_COUNTER_UP;
  timer_initpara.period = 39999U;
  timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
  timer_init(TIMER2, &timer_initpara);

  timer_channel_output_struct_para_init(&timer_ocinitpara);
  timer_ocinitpara.outputstate = TIMER_CCX_ENABLE;
  timer_ocinitpara.ocpolarity = TIMER_OC_POLARITY_HIGH;
  timer_channel_output_config(TIMER2, TIMER_CH_1, &timer_ocinitpara);
  timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_1, 0U);
  timer_channel_output_mode_config(TIMER2, TIMER_CH_1, TIMER_OC_MODE_PWM0);
  timer_channel_output_shadow_config(TIMER2, TIMER_CH_1, TIMER_OC_SHADOW_ENABLE);
  timer_enable(TIMER2);
#else
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

  // reset TIM3
  TIM_DeInit(TIM3);

  /* Time Base configuration */
  // Keep the PWM rate at 100 Hz for each platform clock.
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  #ifdef TARGET_APT_850C_GD32F303RET6
  TIM_TimeBaseStructure.TIM_Prescaler = (30 - 1); /* 120 MHz timer clock */
  #else
  TIM_TimeBaseStructure.TIM_Prescaler = (32 - 1); /* legacy platform clock */
  #endif
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_Period = (40000 - 1);
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

  /* PWM1 Mode configuration: Channel2 */
  TIM_OCInitTypeDef TIM_OCInitStructure;
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = 0;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OC2Init(TIM3, &TIM_OCInitStructure);
  TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);

  /* TIM3 counter enable */
  TIM_Cmd(TIM3, ENABLE);
#endif
}

void timer3_set_compare(uint16_t compare)
{
#ifdef TARGET_APT_850C_GD32F303RET6
  timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_1, compare);
#else
  TIM_SetCompare2(TIM3, compare);
#endif
}

// every 100ms
#ifdef TARGET_APT_850C_GD32F303RET6
void TIMER3_IRQHandler(void) // GD32F30x name for the TIM4 vector
#else
void TIM4_IRQHandler(void)
#endif
{
#ifdef TARGET_APT_850C_GD32F303RET6
  if (timer_interrupt_flag_get(TIMER3, TIMER_INT_FLAG_UP) != RESET)
  {
    timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_UP);
    rt_processing();
  }
#else
  if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
  {
    /* Clear TIMx TIM_IT_Update pending interrupt bit */
    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

	  rt_processing();
  }
#endif
}

void timer4_init(void)
{
#ifdef TARGET_APT_850C_GD32F303RET6
  timer_parameter_struct timer_initpara;

  rcu_periph_clock_enable(RCU_TIMER3);
  timer_deinit(TIMER3);
  timer_struct_para_init(&timer_initpara);
  timer_initpara.prescaler = 1199U;
  timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
  timer_initpara.counterdirection = TIMER_COUNTER_UP;
  timer_initpara.period = 9999U;
  timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
  timer_init(TIMER3, &timer_initpara);
  nvic_irq_enable(TIMER3_IRQn, TIM4_INTERRUPT_PRIORITY, 0U);
  timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_UP);
  timer_interrupt_enable(TIMER3, TIMER_INT_UP);
  timer_enable(TIMER3);
#else
  // enable TIMx clock
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

  // reset TIMx
  TIM_DeInit(TIM4);

  /* Time base configuration */
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_TimeBaseStructure.TIM_Period = (10000 - 1);
  #ifdef TARGET_APT_850C_GD32F303RET6
  TIM_TimeBaseStructure.TIM_Prescaler = (1200 - 1); /* 120 MHz -> 10 Hz */
  #else
  TIM_TimeBaseStructure.TIM_Prescaler = (1280 - 1); /* legacy target -> 10 Hz */
  #endif
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit (TIM4, &TIM_TimeBaseStructure);

  /* Enable the TIMx global Interrupt */
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = TIM4_INTERRUPT_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init (&NVIC_InitStructure);

  /* TIMx TIM_IT_Update enable */
  TIM_ITConfig (TIM4, TIM_IT_Update, ENABLE);

  /* TIM4 counter enable */
  TIM_Cmd (TIM4, ENABLE);
#endif
}

void Display850C_rt_processing_stop(void)
{
#ifdef TARGET_APT_850C_GD32F303RET6
  timer_disable(TIMER3);
  timer_interrupt_disable(TIMER3, TIMER_INT_UP);
#else
  /* TIM4 counter disable */
  TIM_Cmd (TIM4, DISABLE);

  /* TIMx TIM_IT_Update disable */
  TIM_ITConfig (TIM4, TIM_IT_Update, DISABLE);
#endif
}

void Display850C_rt_processing_start(void)
{
#ifdef TARGET_APT_850C_GD32F303RET6
  timer_interrupt_enable(TIMER3, TIMER_INT_UP);
  timer_enable(TIMER3);
#else
  /* TIMx TIM_IT_Update enable */
  TIM_ITConfig (TIM4, TIM_IT_Update, ENABLE);

  /* TIM4 counter enable */
  TIM_Cmd (TIM4, ENABLE);
#endif
}
