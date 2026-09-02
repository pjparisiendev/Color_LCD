#ifdef TARGET_APT_850C_GD32F303RET6

#include "gd32f30x.h"
#include "gd32f30x_rcu.h"
#include "gd32f30x_fmc.h"

void gd32_platform_early_init(void)
{
  /* Match the clocking used by the known-working BIKEL 850C firmware:
   * 8 MHz HXTAL x 13 = 104 MHz, AHB = SYSCLK, APB2 = HCLK, APB1 = HCLK/2.
   */
  rcu_deinit();
  rcu_osci_on(RCU_HXTAL);
  if (SUCCESS != rcu_osci_stab_wait(RCU_HXTAL))
  {
    while (1)
    {
    }
  }

  /* Two FMC wait states, matching the working firmware's flash latency. */
  FMC_WS = (FMC_WS & ~FMC_WS_WSCNT) | WS_WSCNT_2;

  rcu_ahb_clock_config(RCU_AHB_CKSYS_DIV1);
  rcu_apb2_clock_config(RCU_APB2_CKAHB_DIV1);
  rcu_apb1_clock_config(RCU_APB1_CKAHB_DIV2);

  rcu_pllpresel_config(RCU_PLLPRESRC_HXTAL);
  rcu_pll_config(RCU_PLLSRC_HXTAL_IRC48M, RCU_PLL_MUL13);
  rcu_osci_on(RCU_PLL_CK);
  if (SUCCESS != rcu_osci_stab_wait(RCU_PLL_CK))
  {
    while (1)
    {
    }
  }

  rcu_system_clock_source_config(RCU_CKSYSSRC_PLL);
  while (RCU_SCSS_PLL != rcu_system_clock_source_get())
  {
  }

  SystemCoreClockUpdate();

  rcu_periph_reset_disable(RCU_WWDGTRST);

  /* Working BIKEL 850C application layout: 20 KiB APT bootloader prefix. */
  SCB->VTOR = 0x08005000U;
  __DSB();
  __ISB();
}

void _init(void)
{
}

#endif
