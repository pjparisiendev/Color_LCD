/*
 * Bafang LCD 860C/850C firmware
 *
 * Copyright (C) Casainho, 2018, 2019, 2020
 *
 * Released under the GPL License, Version 3
 */

#include <math.h>
#include "stdio.h"
#include "main.h"
#include "utils.h"
#include "screen.h"
#include "rtc.h"
#include "fonts.h"
#include "uart.h"
#include "mainscreen.h"
#include "eeprom.h"
#include "buttons.h"
#include "lcd.h"
#include "adc.h"
#include "ugui.h"
#include "configscreen.h"
#include "battery_gui.h"
#include "state.h"
#include "eeprom.h"

Field batteryField = FIELD_CUSTOM(renderBattery);

uint8_t ui8_g_configuration_clock_hours;
uint8_t ui8_g_configuration_clock_minutes;

static void mainScreenOnEnter() {
	// Set the font preference for this screen
	editable_label_font = &SMALL_TEXT_FONT;
	editable_value_font = &SMALL_TEXT_FONT;
	editable_units_font = &SMALL_TEXT_FONT;
}

static void drawSpeedGaugeStatic(void) {
  const int16_t cx = 160, cy = 145;
  for (uint8_t i = 0; i < 24; i++) {
    float angle1 = (155.0f + (230.0f * i / 24.0f)) * 3.1415926f / 180.0f;
    float angle2 = (155.0f + (230.0f * (i + 1) / 24.0f)) * 3.1415926f / 180.0f;
    UG_COLOR color = i < 12 ? C_GREEN : (i < 19 ? C_YELLOW : C_RED);
    UG_DrawLine(cx + (int16_t)(cosf(angle1) * 101), cy + (int16_t)(sinf(angle1) * 101),
                cx + (int16_t)(cosf(angle2) * 101), cy + (int16_t)(sinf(angle2) * 101), color);
    UG_DrawLine(cx + (int16_t)(cosf(angle1) * 100), cy + (int16_t)(sinf(angle1) * 100),
                cx + (int16_t)(cosf(angle2) * 100), cy + (int16_t)(sinf(angle2) * 100), color);
  }
  for (uint8_t i = 0; i <= 12; i++) {
    float angle = (155.0f + (230.0f * i / 12.0f)) * 3.1415926f / 180.0f;
    UG_DrawLine(cx + (int16_t)(cosf(angle) * 91), cy + (int16_t)(sinf(angle) * 91),
                cx + (int16_t)(cosf(angle) * 98), cy + (int16_t)(sinf(angle) * 98), C_LIGHT_GRAY);
  }
}

void mainScreenOnDirtyClean() {
  UG_FontSelect(&FONT_10X16);
  UG_SetBackcolor(C_BLACK);
  UG_SetForecolor(C_WHITE);

  // Modern high-contrast dashboard surfaces.
  UG_FillScreen(C_BLACK);
  UG_FillRoundFrame(6, 5, 313, 36, 6, 0x1082);
  UG_FillRoundFrame(6, 43, 313, 208, 9, 0x1082);
  UG_FillRoundFrame(12, 220, 150, 272, 7, 0x18C3);
  UG_FillRoundFrame(170, 220, 308, 272, 7, 0x18C3);
  UG_FillRoundFrame(12, 288, 150, 340, 7, 0x18C3);
  UG_FillRoundFrame(170, 288, 308, 340, 7, 0x18C3);
  UG_FillRoundFrame(12, 358, 308, 449, 7, 0x1082);
  drawSpeedGaugeStatic();

  // wheel speed
  if(ui_vars.ui8_units_type == 0)
  {
    UG_PutString(254, 62 , "KM/H");
  }
  else
  {
    UG_PutString(254, 62 , "MPH");
  }

  // if street mode is enable, show ASSIST with regular color otherwise use orange color
  UG_COLOR assist_color;
  if ((assistLevelField.rw->visibility == FieldTransitionVisible) ||
      (assistLevelField.rw->visibility == FieldVisible)) {
    if (ui_vars.ui8_street_mode_enabled) {
      assist_color = MAIN_SCREEN_FIELD_LABELS_COLOR;
    } else {
      assist_color = C_ORANGE_RED;
    }
  }

  // if street mode feature is disabled, show with regular color
  if (ui_vars.ui8_street_mode_function_enabled == 0)
    assist_color = MAIN_SCREEN_FIELD_LABELS_COLOR;

  // if fieldAlternate is enable, do not show ASSIST
  if ((fieldAlternate.rw->visibility == FieldTransitionVisible) ||
      (fieldAlternate.rw->visibility == FieldVisible)) {
    UG_PutString(20, 62, "      ");
  } else {
    UG_SetForecolor(assist_color);
    UG_PutString(20, 62, "ASSIST");
  }
}

void mainScreenOnPostUpdate(void) {
  static int16_t old_x1 = 160, old_y1 = 145, old_x2 = 160, old_y2 = 145;
  static uint16_t old_speed_x10 = UINT16_MAX;
  if (rt_vars.ui16_wheel_speed_x10 != old_speed_x10) {
    old_speed_x10 = rt_vars.ui16_wheel_speed_x10;
    float speed = old_speed_x10 / 10.0f;
    if (speed > 60.0f) speed = 60.0f;
    float angle = (155.0f + (230.0f * speed / 60.0f)) * 3.1415926f / 180.0f;
    UG_DrawLine(old_x1, old_y1, old_x2, old_y2, 0x1082);
    drawSpeedGaugeStatic();
    old_x1 = 160 + (int16_t)(cosf(angle) * 82); old_y1 = 145 + (int16_t)(sinf(angle) * 82);
    old_x2 = 160 + (int16_t)(cosf(angle) * 101); old_y2 = 145 + (int16_t)(sinf(angle) * 101);
    UG_DrawLine(old_x1, old_y1, old_x2, old_y2, C_WHITE);
  }
  // because printing numbers of wheel speed will make dirty the dot, always print it
  // wheel speed print dot
  UG_FillRoundFrame(233, 128, 238, 133, 2, C_WHITE);
}

/**
 * Appears at the bottom of all screens, includes status msgs or critical fault alerts
 * FIXME - get rid of this nasty define - instead add the concept of Subscreens, so that the battery bar
 * at the top and the status bar at the bottom can be shared across all screens
 */
#define STATUS_BAR \
{ \
    .x = 4, .y = SCREEN_HEIGHT - 18, \
    .width = 0, .height = -1, \
    .field = &warnField, \
    .font = &SMALL_TEXT_FONT, \
}

#define BATTERY_BAR \
  { \
      .x = 0, .y = 0, \
      .width = -1, .height = -1, \
      .field = &batteryField, \
  }, \
  { \
      .x = 8 + ((7 + 1 + 1) * 10) + (1 * 2) + 10, .y = 2, \
      .width = -5, .height = -1, \
      .font = &REGULAR_TEXT_FONT, \
      .align_x = AlignLeft, \
      .unit_align_x = AlignLeft, \
      .field = &socField \
  }, \
	{ \
		.x = 234, .y = 2, \
		.width = -5, .height = -1, \
		.font = &REGULAR_TEXT_FONT, \
		.unit_align_x = AlignRight, \
		.field = &timeField \
	}

//
// Screenscommon/src/state.c
//
Screen mainScreen1 = {
  .onPress = mainScreenOnPress,
  .onEnter = mainScreenOnEnter,
  .onDirtyClean = mainScreenOnDirtyClean,
  .onPostUpdate = mainScreenOnPostUpdate,

  .fields = {
    BATTERY_BAR,
    {
      .x = 12, .y = 88,
      .width = 78, .height = 72,
      .field = &assistLevelField,
      .font = &BIG_NUMBERS_TEXT_FONT,
      .label_align_x = AlignHidden,
      .align_x = AlignCenter,
      .unit_align_x = AlignRight,
      .unit_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 12, .y = 88,
      .width = 78, .height = 72,
      .field = &fieldAlternate,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .align_x = AlignCenter,
      .inset_y = 12,
      .unit_align_x = AlignRight,
      .unit_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 88, .y = 60,
      .width = 145, // 2 digits
      .height = 99,
      .field = &wheelSpeedIntegerField,
      .font = &HUGE_NUMBERS_TEXT_FONT,
      .label_align_x = AlignHidden,
      .align_x = AlignRight,
      .unit_align_x = AlignRight,
      .unit_align_y = AlignTop,
      .show_units = Hide,
      .border = BorderNone,
    },
    {
      .x = 240, .y = 87,
      .width = 58, // 1 digit
      .height = 72,
      .field = &wheelSpeedDecimalField,
      .font = &BIG_NUMBERS_TEXT_FONT,
      .label_align_x = AlignHidden,
      .align_x = AlignCenter,
      .unit_align_x = AlignCenter,
      .unit_align_y = AlignTop,
      .show_units = Hide,
      .border = BorderNone,
    },
    {
      .x = 12, .y = 220,
      .width = 137,
      .height = 52,
      .align_x = AlignCenter,
      .inset_y = 12,
      .inset_x = 0,
      .field = &custom1,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 170, .y = 220,
      .width = 137,
      .height = 52,
      .align_x = AlignCenter,
      .inset_y = 12,
      .inset_x = 0,
      .field = &custom2,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 12, .y = 288,
      .width = 137,
      .height = 52,
      .align_x = AlignCenter,
      .inset_y = 12,
      .inset_x = 0,
      .field = &custom3,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 170, .y = 288,
      .width = 137,
      .height = 52,
      .align_x = AlignCenter,
      .inset_y = 12,
      .inset_x = 0,
      .field = &custom4,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .border = BorderNone,
    },
    STATUS_BAR,
    {
      .field = NULL
    }
  }
};

Screen mainScreen2 = {
  .onPress = mainScreenOnPress,
  .onEnter = mainScreenOnEnter,
  .onDirtyClean = mainScreenOnDirtyClean,
  .onPostUpdate = mainScreenOnPostUpdate,

  .fields = {
    BATTERY_BAR,
    {
      .x = 12, .y = 88,
      .width = 78, .height = 72,
      .field = &assistLevelField,
      .font = &BIG_NUMBERS_TEXT_FONT,
      .label_align_x = AlignHidden,
      .align_x = AlignCenter,
      .unit_align_x = AlignRight,
      .unit_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 12, .y = 88,
      .width = 78, .height = 72,
      .field = &fieldAlternate,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .align_x = AlignCenter,
      .inset_y = 12,
      .unit_align_x = AlignRight,
      .unit_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 88, .y = 60,
      .width = 145, // 2 digits
      .height = 99,
      .field = &wheelSpeedIntegerField,
      .font = &HUGE_NUMBERS_TEXT_FONT,
      .label_align_x = AlignHidden,
      .align_x = AlignRight,
      .unit_align_x = AlignRight,
      .unit_align_y = AlignTop,
      .show_units = Hide,
      .border = BorderNone,
    },
    {
      .x = 240, .y = 87,
      .width = 58, // 1 digit
      .height = 72,
      .field = &wheelSpeedDecimalField,
      .font = &BIG_NUMBERS_TEXT_FONT,
      .label_align_x = AlignHidden,
      .align_x = AlignCenter,
      .unit_align_x = AlignCenter,
      .unit_align_y = AlignTop,
      .show_units = Hide,
      .border = BorderNone,
    },
    {
      .x = 12, .y = 220,
      .width = 137,
      .height = 52,
      .align_x = AlignCenter,
      .inset_y = 12,
      .inset_x = 0,
      .field = &custom5,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 170, .y = 220,
      .width = 137,
      .height = 52,
      .align_x = AlignCenter,
      .inset_y = 12,
      .inset_x = 0,
      .field = &custom6,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 12, .y = 288,
      .width = 137,
      .height = 52,
      .align_x = AlignCenter,
      .inset_y = 12,
      .inset_x = 0,
      .field = &custom7,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 170, .y = 288,
      .width = 137,
      .height = 52,
      .align_x = AlignCenter,
      .inset_y = 12,
      .inset_x = 0,
      .field = &custom8,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .border = BorderNone,
    },
    STATUS_BAR,
    {
      .field = NULL
    }
  }
};

Screen mainScreen3 = {
  .onPress = mainScreenOnPress,
  .onEnter = mainScreenOnEnter,
  .onDirtyClean = mainScreenOnDirtyClean,
  .onPostUpdate = mainScreenOnPostUpdate,

  .fields = {
    BATTERY_BAR,
    {
      .x = 12, .y = 88,
      .width = 78, .height = 72,
      .field = &assistLevelField,
      .font = &BIG_NUMBERS_TEXT_FONT,
      .label_align_x = AlignHidden,
      .align_x = AlignCenter,
      .unit_align_x = AlignRight,
      .unit_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 12, .y = 88,
      .width = 78, .height = 72,
      .field = &fieldAlternate,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .align_x = AlignCenter,
      .inset_y = 12,
      .unit_align_x = AlignRight,
      .unit_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 88, .y = 60,
      .width = 145, // 2 digits
      .height = 99,
      .field = &wheelSpeedIntegerField,
      .font = &HUGE_NUMBERS_TEXT_FONT,
      .label_align_x = AlignHidden,
      .align_x = AlignRight,
      .unit_align_x = AlignRight,
      .unit_align_y = AlignTop,
      .show_units = Hide,
      .border = BorderNone,
    },
    {
      .x = 240, .y = 87,
      .width = 58, // 1 digit
      .height = 72,
      .field = &wheelSpeedDecimalField,
      .font = &BIG_NUMBERS_TEXT_FONT,
      .label_align_x = AlignHidden,
      .align_x = AlignCenter,
      .unit_align_x = AlignCenter,
      .unit_align_y = AlignTop,
      .show_units = Hide,
      .border = BorderNone,
    },
    {
      .x = 12, .y = 220,
      .width = 137,
      .height = 52,
      .align_x = AlignCenter,
      .inset_y = 12,
      .inset_x = 0,
      .field = &custom9,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 170, .y = 220,
      .width = 137,
      .height = 52,
      .align_x = AlignCenter,
      .inset_y = 12,
      .inset_x = 0,
      .field = &custom10,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 12, .y = 288,
      .width = 137,
      .height = 52,
      .align_x = AlignCenter,
      .inset_y = 12,
      .inset_x = 0,
      .field = &custom11,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 170, .y = 288,
      .width = 137,
      .height = 52,
      .align_x = AlignCenter,
      .inset_y = 12,
      .inset_x = 0,
      .field = &custom12,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .border = BorderNone,
    },
    STATUS_BAR,
    {
      .field = NULL
    }
  }
};

Screen mainScreen4 = {
  .onPress = mainScreenOnPress,
  .onEnter = mainScreenOnEnter,
  .onDirtyClean = mainScreenOnDirtyClean,
  .fields = {
    BATTERY_BAR,
    {
      .x = 12, .y = 58,
      .width = 296, .height = 385,
      .field = &graph1,
    },
    STATUS_BAR,
    { .field = NULL }
  }
};


// Screens in a loop, shown when the user short presses the power button
Screen *screens[] = { &mainScreen1, &mainScreen2, &mainScreen3, &mainScreen4, NULL };

// Show our battery graphic
void battery_display() {
	static uint8_t old_soc = 0xff;

  if (ui8_g_battery_soc != old_soc) {
    old_soc = ui8_g_battery_soc;
    batteryField.rw->dirty = true;
  }
}

void clock_time(void) {
  rtc_time_t *p_rtc_time;

  // get current time
  p_rtc_time = rtc_get_time();
  ui8_g_configuration_clock_hours = p_rtc_time->ui8_hours;
  ui8_g_configuration_clock_minutes = p_rtc_time->ui8_minutes;

  // force to be [0 - 12] depending on SI or Ipmerial units
  if (ui_vars.ui8_units_type) {
    if(ui8_g_configuration_clock_hours > 12) {
      ui8_g_configuration_clock_hours -= 12;
    }
  }
}
void onSetConfigurationClockHours(uint32_t v) {
  static rtc_time_t rtc_time;

  // save the new clock time
  rtc_time.ui8_hours = v;
  rtc_time.ui8_minutes = ui8_g_configuration_clock_minutes;
  rtc_set_time(&rtc_time);
}

void onSetConfigurationClockMinutes(uint32_t v) {
  static rtc_time_t rtc_time;

  // save the new clock time
  rtc_time.ui8_hours = ui8_g_configuration_clock_hours;
  rtc_time.ui8_minutes = v;
  rtc_set_time(&rtc_time);
}

void onSetConfigurationDisplayLcdBacklightOnBrightness(uint32_t v) {

  ui_vars.ui8_lcd_backlight_on_brightness = v;
  set_lcd_backlight();
}

void onSetConfigurationDisplayLcdBacklightOffBrightness(uint32_t v) {

  ui_vars.ui8_lcd_backlight_off_brightness = v;
  set_lcd_backlight();
}
