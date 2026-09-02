#include "adc.h"
#ifdef TARGET_APT_850C_GD32F303RET6
#include "gd32f30x_adc.h"
#include "gd32f30x_gpio.h"
#include "gd32f30x_rcu.h"
#else
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_adc.h"
#include "pins.h"
#endif
#include "stdio.h"

/**
 * Per @casainho
 *
 * PA4
 * ADC01_IN4, measure battery voltage. Battery voltage first goes over D5 and then R29 and R31 forms the voltage divider. R29 is connected in one side to GND.

 R31 = 200kohms; R29 = 10kohms. Vout = Vin * (R2 / (R2+R1)); Vout = Vin * 0,048.
 */
void adc_init() {
#ifdef TARGET_APT_850C_GD32F303RET6
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_ADC0);
	rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV8);

	gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_4);
	adc_deinit(ADC0);
	adc_mode_config(ADC_MODE_FREE);
	adc_special_function_config(ADC0, ADC_SCAN_MODE, DISABLE);
	adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);
	adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
	adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 1U);
	adc_regular_channel_config(ADC0, 0U, ADC_CHANNEL_4, ADC_SAMPLETIME_239POINT5);
	adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL,
	                                   ADC0_1_2_EXTTRIG_REGULAR_NONE);
	adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);
	adc_enable(ADC0);
	adc_calibration_enable(ADC0);
	adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
#else
	GPIO_InitTypeDef ginit;
	ginit.GPIO_Pin = GPIO_Pin_4;
	ginit.GPIO_Mode = GPIO_Mode_AIN; //GPIO Pin as analog Mode
	ginit.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_Init(GPIOA, &ginit); // GPIO Initialization

	ADC_DeInit(ADC1); //Deinitialize the ADC to reconfigure it
	ADC_InitTypeDef ADC_InitStruct;
	ADC_StructInit(&ADC_InitStruct);
	ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_Init(ADC1, &ADC_InitStruct);
	ADC_Cmd(ADC1, ENABLE);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 1, ADC_SampleTime_239Cycles5);

	// Queue up a conversion
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
#endif
}

#define NUM_STEPS 4096 // 12 bit

uint16_t battery_voltage_10x_get() {
  static uint8_t ui8_first_time = 1;
  uint32_t rawVoltage;

  // the very first measure need to be discarded as it seems to have a wrong value
  if (ui8_first_time) {
    ui8_first_time = 0;

#ifdef TARGET_APT_850C_GD32F303RET6
    while (adc_flag_get(ADC0, ADC_FLAG_EOC) == RESET)
      ;

    rawVoltage = (uint32_t) adc_regular_data_read(ADC0);
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
#else
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET)
      ;

    rawVoltage = (uint32_t) ADC_GetConversionValue(ADC1);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE); // start a conversion for next time.
#endif
  }

#ifdef TARGET_APT_850C_GD32F303RET6
	while (adc_flag_get(ADC0, ADC_FLAG_EOC) == RESET)
		;

	rawVoltage = (uint32_t) adc_regular_data_read(ADC0);
#else
	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET)
		;

	rawVoltage = (uint32_t) ADC_GetConversionValue(ADC1);
#endif
	uint32_t v_1000x = (3300UL * rawVoltage) / NUM_STEPS; // voltage at input pin x1000
	uint32_t busvolt_10x = (v_1000x * 2083) / 1000 / 10;

#ifdef TARGET_APT_850C_GD32F303RET6
	adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
#else
	ADC_SoftwareStartConvCmd(ADC1, ENABLE); // start a conversion for next time.
#endif
	return busvolt_10x;
}
