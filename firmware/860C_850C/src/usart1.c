/*
 * Bafang LCD 860C/850C firmware
 *
 * Copyright (C) Casainho, 2018, 2019, 2020
 *
 * Released under the GPL License, Version 3
 */

#include <string.h>
#include "usart1.h"
#ifdef TARGET_APT_850C_GD32F303RET6
#include "gd32f30x_dma.h"
#include "gd32f30x_gpio.h"
#include "gd32f30x_misc.h"
#include "gd32f30x_rcu.h"
#include "gd32f30x_usart.h"
#else
#include "stm32f10x.h"
#include "pins.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#endif
#include "lcd.h"
#include "utils.h"
#include "usart1.h"
#include "main.h"
#include "uart.h"
#include "bafang_runtime.h"

uint8_t ui8_rx_buffer[UART_NUMBER_DATA_BYTES_TO_RECEIVE];
volatile uint8_t ui8_received_package_flag = 0;

void usart1_init(void)
{
#ifdef TARGET_APT_850C_GD32F303RET6
  dma_parameter_struct dma_init_struct;

  rcu_periph_clock_enable(RCU_GPIOA);
  rcu_periph_clock_enable(RCU_AF);
  rcu_periph_clock_enable(RCU_USART0);
  rcu_periph_clock_enable(RCU_DMA0);

  gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
  gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);

  dma_deinit(DMA0, DMA_CH3);
  dma_struct_para_init(&dma_init_struct);
  dma_init_struct.periph_addr = (uint32_t)&USART_DATA(USART0);
  dma_init_struct.memory_addr = (uint32_t)uart_get_tx_buffer();
  dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
  dma_init_struct.number = UART_NUMBER_DATA_BYTES_TO_SEND;
  dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
  dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
  dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
  dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
  dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
  dma_init(DMA0, DMA_CH3, &dma_init_struct);

  usart_deinit(USART0);
  /* Confirmed by passive captures of both factory-link directions. */
  usart_baudrate_set(USART0, 1200U);
  usart_word_length_set(USART0, USART_WL_8BIT);
  usart_stop_bit_set(USART0, USART_STB_1BIT);
  usart_parity_config(USART0, USART_PM_NONE);
  usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
  usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
  usart_receive_config(USART0, USART_RECEIVE_ENABLE);
  usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
  nvic_irq_enable(USART0_IRQn, USART1_INTERRUPT_PRIORITY, 1U);
  /* Controller TX stays disabled until BBS02B baud/framing is verified. */
#ifndef CONTROLLER_TX_LOCKED
  usart_dma_transmit_config(USART0, USART_TRANSMIT_DMA_ENABLE);
#endif
  usart_interrupt_enable(USART0, USART_INT_RBNE);
  usart_enable(USART0);
#else
  NVIC_InitTypeDef NVIC_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  DMA_InitTypeDef DMA_InitStructure;

  // enable GPIO clock
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  DMA_DeInit(DMA1_Channel4);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(USART1->DR);
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) uart_get_tx_buffer();
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_BufferSize = UART_NUMBER_DATA_BYTES_TO_SEND;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel4, &DMA_InitStructure);

  // USART pins
  GPIO_InitStructure.GPIO_Pin = USART1_RX__PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(USART1__PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = USART1_TX__PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(USART1__PORT, &GPIO_InitStructure);

  USART_DeInit(USART1);
  USART_InitStructure.USART_BaudRate = 19200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART1, &USART_InitStructure);

  // enable the USART Interrupt
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USART1_INTERRUPT_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  USART_ClearITPendingBit(USART1, USART_IT_RXNE);
  USART_ClearITPendingBit(USART1, USART_IT_TC);

  // enable the USART
  USART_Cmd(USART1, ENABLE);

  DMA_Cmd(DMA1_Channel4, ENABLE);
  USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
  USART_Cmd(USART1, ENABLE);

  // enable USART Receive and Transmit interrupts
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
#endif
}

// USART1 Tx and Rx interrupt handler.
#ifdef TARGET_APT_850C_GD32F303RET6
void USART0_IRQHandler(void)
#else
void USART1_IRQHandler(void)
#endif
{
  uint8_t ui8_byte_received;
  static uint8_t ui8_state_machine = 0;
  static uint8_t ui8_rx[UART_NUMBER_DATA_BYTES_TO_RECEIVE];
  static uint8_t ui8_rx_cnt = 0;
  uint8_t ui8_i;
  uint16_t ui16_crc_rx;

  // The interrupt may be from Tx, Rx, or both.
#ifdef TARGET_APT_850C_GD32F303RET6
  if (usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE_ORERR) == SET &&
      usart_flag_get(USART0, USART_FLAG_ORERR) == SET)
  {
    (void)usart_data_receive(USART0);
    return;
  }
  else if (usart_interrupt_flag_get(USART0, USART_INT_FLAG_TBE) == SET)
  {
    usart_interrupt_disable(USART0, USART_INT_TBE);
    return;
  }
  else if (usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE) == SET)
  {
    ui8_byte_received = (uint8_t)usart_data_receive(USART0);
    bafang_runtime_receive_byte(ui8_byte_received);
    return;
#else
  if (USART_GetITStatus(USART1, USART_IT_ORE) == SET)
  {
    USART_ReceiveData(USART1); // get ride of this interrupt flag
    return;
  }
  else if (USART_GetITStatus(USART1, USART_IT_TXE) == SET)
  {
    USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
    return;
  }
  else if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
  {
    // receive byte
    ui8_byte_received = (uint8_t) USART1->DR;
#endif

    switch (ui8_state_machine)
    {
      case 0:
      if (ui8_byte_received == 0x43) { // see if we get start package byte
        ui8_rx[0] = ui8_byte_received;
        ui8_state_machine = 1;
      }
      else
        ui8_state_machine = 0;
      break;

      case 1:
        ui8_rx[1] = ui8_byte_received;
        ui8_state_machine = 2;
      break;

      case 2:
      ui8_rx[ui8_rx_cnt + 2] = ui8_byte_received;
      ++ui8_rx_cnt;

      // reset if it is the last byte of the package and index is out of bounds
      if (ui8_rx_cnt >= ui8_rx[1])
      {
        ui8_rx_cnt = 0;
        ui8_state_machine = 0;

        // just to make easy next calculations
        ui16_crc_rx = 0xffff;
        for (ui8_i = 0; ui8_i < ui8_rx[1]; ui8_i++)
        {
          crc16(ui8_rx[ui8_i], &ui16_crc_rx);
        }

        // if CRC is correct read the package
        if (((((uint16_t) ui8_rx[ui8_rx[1] + 1]) << 8) +
              ((uint16_t) ui8_rx[ui8_rx[1]])) == ui16_crc_rx)
        {
          // copy to the other buffer only if we processed already the last package
          if (ui8_received_package_flag == 0)
          {
            ui8_received_package_flag = 1;

            // store the received data to rx_buffer
            memcpy(ui8_rx_buffer, ui8_rx, ui8_rx[1] + 2);
          }
        }
      }
    }
  }
}

void usart1_start_dma_transfer(uint8_t ui8_len)
{
#if defined(TARGET_APT_850C_GD32F303RET6) && defined(CONTROLLER_TX_LOCKED)
  (void)ui8_len;
#elif defined(TARGET_APT_850C_GD32F303RET6)
  dma_channel_disable(DMA0, DMA_CH3);
  dma_transfer_number_config(DMA0, DMA_CH3, ui8_len);
  dma_channel_enable(DMA0, DMA_CH3);
#else
  DMA_Cmd(DMA1_Channel4, DISABLE);
  DMA_SetCurrDataCounter(DMA1_Channel4, ui8_len);
  DMA_Cmd(DMA1_Channel4, ENABLE);
#endif
}

uint8_t usart1_send_bafang_read(const uint8_t *frame, uint8_t length)
{
#ifdef TARGET_APT_850C_GD32F303RET6
  uint8_t valid = 0U;

  if (frame == NULL)
    return 0U;
  if (length == 2U && frame[0] == 0x11U &&
      (frame[1] == 0x20U || frame[1] == 0x0AU ||
       frame[1] == 0x08U || frame[1] == 0x11U))
    valid = 1U;
  if (!valid)
    return 0U;

  memcpy(uart_get_tx_buffer(), frame, length);
  dma_channel_disable(DMA0, DMA_CH3);
  dma_transfer_number_config(DMA0, DMA_CH3, length);
  usart_dma_transmit_config(USART0, USART_TRANSMIT_DMA_ENABLE);
  dma_channel_enable(DMA0, DMA_CH3);
  return 1U;
#else
  (void)frame;
  (void)length;
  return 0U;
#endif
}

uint8_t* usart1_get_rx_buffer(void)
{
  return ui8_rx_buffer;
}

uint8_t usart1_received_package(void)
{
  return ui8_received_package_flag;
}

void usart1_reset_received_package(void)
{
  ui8_received_package_flag = 0;
}
