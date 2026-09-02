#include "gd32_lcd_bus.h"

#ifdef TARGET_APT_850C_GD32F303RET6

#include "gd32f30x_gpio.h"

/* E2.3 LCD interface inherited from the documented 850C board routing:
 * PB0..PB15 data, PC3 command/data, PC4 CS, PC5 WR, PC6 reset, PC7 RD. */
void gd32_lcd_bus_init(void)
{
  gpio_pin_remap_config(GPIO_SWJ_SWDPENABLE_REMAP, ENABLE);
  gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ,
      GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
  gd32_lcd_bus_output();
}

void gd32_lcd_bus_input(void)
{
  gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_ALL);
}

void gd32_lcd_bus_output(void)
{
  gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_ALL);
}

void gd32_lcd_pin_set(uintptr_t port, uint16_t pin)
{
  gpio_bit_set((uint32_t)port, pin);
}

void gd32_lcd_pin_reset(uintptr_t port, uint16_t pin)
{
  gpio_bit_reset((uint32_t)port, pin);
}

void gd32_lcd_bus_write(uint16_t value)
{
  GPIO_OCTL(GPIOB) = value;
}

uint16_t gd32_lcd_bus_read(void)
{
  return (uint16_t)GPIO_ISTAT(GPIOB);
}

#else

void gd32_lcd_bus_init(void) {}
void gd32_lcd_bus_input(void) {}
void gd32_lcd_bus_output(void) {}
void gd32_lcd_pin_set(uintptr_t port, uint16_t pin) {(void)port; (void)pin;}
void gd32_lcd_pin_reset(uintptr_t port, uint16_t pin) {(void)port; (void)pin;}
void gd32_lcd_bus_write(uint16_t value) {(void)value;}
uint16_t gd32_lcd_bus_read(void) {return 0U;}

#endif
