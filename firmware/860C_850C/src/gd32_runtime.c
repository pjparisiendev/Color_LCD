#ifdef TARGET_APT_850C_GD32F303RET6

#include "gd32f30x.h"
#include "gd32f30x_rcu.h"

void gd32_platform_early_init(void)
{
  rcu_periph_reset_disable(RCU_WWDGTRST);
  SCB->VTOR = 0x08004000U;
  __DSB();
  __ISB();
}

/* Required by the official GD32 GCC startup before main(). */
void _init(void)
{
}

#endif
