#include "gd32_rtc_platform.h"

#ifdef TARGET_APT_850C_GD32F303RET6

#include "gd32f30x_misc.h"
#include "gd32f30x_pmu.h"
#include "gd32f30x_rcu.h"
#include "gd32f30x_rtc.h"

#define RTC_SECONDS_PER_DAY 86400U

static bool rtc_wait_lwoff(void)
{
  uint32_t timeout = 1000000U;
  while (((RTC_CTL & RTC_CTL_LWOFF) == 0U) && (--timeout != 0U)) {}
  return timeout != 0U;
}

static bool rtc_sync(void)
{
  uint32_t timeout = 1000000U;
  RTC_CTL &= ~RTC_CTL_RSYNF;
  while (((RTC_CTL & RTC_CTL_RSYNF) == 0U) && (--timeout != 0U)) {}
  return timeout != 0U;
}

static void rtc_write_counter(uint32_t value)
{
  if (!rtc_wait_lwoff())
    return;
  RTC_CTL |= RTC_CTL_CMF;
  RTC_CNTH = (value >> 16) & 0xFFFFU;
  RTC_CNTL = value & 0xFFFFU;
  RTC_CTL &= ~RTC_CTL_CMF;
  (void)rtc_wait_lwoff();
}

bool gd32_rtc_platform_init(void)
{
  uint32_t source;

  rcu_periph_clock_enable(RCU_PMU);
  rcu_periph_clock_enable(RCU_BKPI);
  PMU_CTL |= PMU_CTL_BKPWEN;

  source = RCU_BDCTL & RCU_BDCTL_RTCSRC;
  if (source == RCU_RTCSRC_NONE) {
    rcu_osci_on(RCU_LXTAL);
    if (rcu_osci_stab_wait(RCU_LXTAL) != SUCCESS)
      return false;
    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
    rcu_periph_clock_enable(RCU_RTC);
    if (!rtc_sync() || !rtc_wait_lwoff())
      return false;
    RTC_CTL |= RTC_CTL_CMF;
    RTC_PSCH = (32767U >> 16) & 0xFU;
    RTC_PSCL = 32767U & 0xFFFFU;
    RTC_CTL &= ~RTC_CTL_CMF;
    if (!rtc_wait_lwoff())
      return false;
  } else {
    rcu_periph_clock_enable(RCU_RTC);
    if (!rtc_sync())
      return false;
  }

  RTC_INTEN |= RTC_INT_SECOND;
  RTC_CTL &= ~RTC_FLAG_SECOND;
  nvic_irq_enable(RTC_IRQn, 1U, 1U);
  if (gd32_rtc_counter_get() >= RTC_SECONDS_PER_DAY)
    rtc_write_counter(gd32_rtc_counter_get() % RTC_SECONDS_PER_DAY);
  return true;
}

uint32_t gd32_rtc_counter_get(void)
{
  uint32_t high_a;
  uint32_t high_b;
  uint32_t low;
  do {
    high_a = RTC_CNTH & 0xFFFFU;
    low = RTC_CNTL & 0xFFFFU;
    high_b = RTC_CNTH & 0xFFFFU;
  } while (high_a != high_b);
  return (high_a << 16) | low;
}

void gd32_rtc_counter_set(uint32_t value)
{
  rtc_write_counter(value % RTC_SECONDS_PER_DAY);
}

void gd32_rtc_clear_second_irq(void)
{
  RTC_CTL &= ~RTC_FLAG_SECOND;
  NVIC_ClearPendingIRQ(RTC_IRQn);
}

#else

bool gd32_rtc_platform_init(void) { return false; }
uint32_t gd32_rtc_counter_get(void) { return 0U; }
void gd32_rtc_counter_set(uint32_t value) { (void)value; }
void gd32_rtc_clear_second_irq(void) {}

#endif
