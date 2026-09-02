#include "lcd_lf60.h"

#ifdef TARGET_APT_850C_GD32F303RET6

#include "gd32_lcd_bus.h"
#include "pins.h"
#include "timers.h"

lcd_IC_t lcd_lf60_init(void)
{
  /* Native GD32 bus/pin setup. */
  gd32_lcd_bus_init();
  gd32_lcd_pin_set((uintptr_t)LCD_RESET__PORT, LCD_RESET__PIN);
  gd32_lcd_pin_set((uintptr_t)LCD_READ__PORT, LCD_READ__PIN);
  gd32_lcd_pin_reset((uintptr_t)LCD_CHIP_SELECT__PORT, LCD_CHIP_SELECT__PIN);

  /* Exact newer-850C LF60 initialization sequence from BIKEL MPe-ColorLCD
   * v1.1, whose 850C binary is confirmed working on this HMI.
   */
  delay_ms(120);

  lcd_write_command(0xE0);
  lcd_write_data_8bits(0x00);
  lcd_write_data_8bits(0x07);
  lcd_write_data_8bits(0x0F);
  lcd_write_data_8bits(0x07);
  lcd_write_data_8bits(0x15);
  lcd_write_data_8bits(0x09);
  lcd_write_data_8bits(0x3C);
  lcd_write_data_8bits(0x99);
  lcd_write_data_8bits(0x4B);
  lcd_write_data_8bits(0x09);
  lcd_write_data_8bits(0x10);
  lcd_write_data_8bits(0x0D);
  lcd_write_data_8bits(0x1C);
  lcd_write_data_8bits(0x1E);
  lcd_write_data_8bits(0x0F);

  lcd_write_command(0xE1);
  lcd_write_data_8bits(0x00);
  lcd_write_data_8bits(0x20);
  lcd_write_data_8bits(0x23);
  lcd_write_data_8bits(0x02);
  lcd_write_data_8bits(0x0F);
  lcd_write_data_8bits(0x06);
  lcd_write_data_8bits(0x34);
  lcd_write_data_8bits(0x45);
  lcd_write_data_8bits(0x43);
  lcd_write_data_8bits(0x04);
  lcd_write_data_8bits(0x0A);
  lcd_write_data_8bits(0x08);
  lcd_write_data_8bits(0x30);
  lcd_write_data_8bits(0x37);
  lcd_write_data_8bits(0x0F);

  lcd_write_command(0xC0);
  lcd_write_data_8bits(0x01);
  lcd_write_data_8bits(0x01);

  lcd_write_command(0xC1);
  lcd_write_data_8bits(0x41);

  lcd_write_command(0xC5);
  lcd_write_data_8bits(0x00);
  lcd_write_data_8bits(0x3F);
  lcd_write_data_8bits(0x80);

  lcd_write_command(0x36);
  lcd_write_data_8bits(0x48);

  lcd_write_command(0x3A);
  lcd_write_data_8bits(0x55);

  lcd_write_command(0xB0);
  lcd_write_data_8bits(0x00);

  lcd_write_command(0xB1);
  lcd_write_data_8bits(0xA0);
  lcd_write_data_8bits(0x11);

  lcd_write_command(0xB4);
  lcd_write_data_8bits(0x02);

  lcd_write_command(0xB6);
  lcd_write_data_8bits(0x02);
  lcd_write_data_8bits(0x02);

  lcd_write_command(0xBE);
  lcd_write_data_8bits(0x00);
  lcd_write_data_8bits(0x04);

  lcd_write_command(0xE9);
  lcd_write_data_8bits(0x00);

  lcd_write_command(0xF7);
  lcd_write_data_8bits(0xA9);
  lcd_write_data_8bits(0x51);
  lcd_write_data_8bits(0x2C);
  lcd_write_data_8bits(0x8A);

  lcd_write_command(0x11);
  delay_ms(120);

  lcd_write_command(0x29);

  lcd_write_command(0x36);
  lcd_write_data_8bits(0x48);

  return LCD_ST7796;
}

#else

lcd_IC_t lcd_lf60_init(void)
{
  return LCD_Unknown;
}

#endif
