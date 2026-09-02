/*
 * Bafang LCD 860C/850C firmware
 *
 * Copyright (C) Casainho, 2018, 2019, 2020
 *
 * Released under the GPL License, Version 3
 */

#include <math.h>
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stdio.h"
#include "main.h"
#include "utils.h"
#include "pins.h"
#include "lcd.h"
#include "buttons.h"
#include "eeprom.h"
#include "usart1.h"
#include "ugui.h"
#include "rtc.h"
#include "fonts.h"
#include "state.h"
#include "timers.h"
#include "ugui_driver/ugui_display_8x0c.h"
#ifdef TARGET_APT_850C_GD32F303RET6
#include "lcd_lf60.h"
#endif

#define BATTERY_SOC_START_X 8
#define BATTERY_SOC_START_Y 4
#define BATTERY_SOC_BAR_WITH 7
#define BATTERY_SOC_BAR_HEIGHT 24
#define BATTERY_SOC_CONTOUR 1

volatile lcd_vars_t m_lcd_vars =
{
  .ui32_main_screen_draw_static_info = 1,
  .lcd_screen_state = LCD_SCREEN_MAIN,
  .ui8_lcd_menu_counter_1000ms_state = 0,
  .main_screen_state = MAIN_SCREEN_STATE_MAIN,
};

lcd_IC_t g_lcd_ic_type;

void power_off_management(void);
void lcd_power_off(uint8_t updateDistanceOdo);
void power_off_management(void);

void lcd_init(void)
{
#ifdef TARGET_APT_850C_GD32F303RET6
  /* Do not run the historical LB60/ILI9481 autodetection path on the E2.3
   * target. Use the exact LF60 sequence copied from the working BIKEL 850C.
   */
  g_lcd_ic_type = lcd_lf60_init();
#else
  g_lcd_ic_type = display_8x0C_lcd_init();
#endif
  UG_FillScreen(C_BLACK);

  set_lcd_backlight();
}

void power_off_management(void)
{
  if(buttons_get_onoff_long_click_event() &&
    m_lcd_vars.lcd_screen_state == LCD_SCREEN_MAIN &&
    buttons_get_up_state() == 0 &&
    buttons_get_down_state() == 0)
  {
    lcd_power_off(1);
  }
}

void lcd_set_backlight_intensity(uint8_t ui8_intensity)
{
  if (ui8_intensity == 0U)
  {
    timer3_set_compare(0U);
    return;
  }

  ui8_intensity /= 5;

  if(ui8_intensity < 1)
  {
    ui8_intensity = 1;
  }
  else if(ui8_intensity > 20)
  {
    ui8_intensity = 20;
  }

  timer3_set_compare(((uint16_t) ui8_intensity) * 2000);
}

void lcd_power_off(uint8_t updateDistanceOdo)
{
  ui_vars.ui32_wh_x10_offset = ui_vars.ui32_wh_x10;
  eeprom_write_variables ();

  UG_FillScreen(0);
  lcd_set_backlight_intensity(0);

  system_power(0);

  while(1) ;
}

volatile lcd_vars_t* get_lcd_vars(void)
{
  return &m_lcd_vars;
}
