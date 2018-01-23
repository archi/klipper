#include "sched.h" // DECL_CONSTANT
#include "command.h" // shutdown

#include <stm32f1xx_hal_wwdg.h>

/* WWDG handler declaration */
static WWDG_HandleTypeDef   WwdgHandle;

/****************************************************************
 * watchdog handler
 ****************************************************************/

void
watchdog_reset(void)
{
    (void)HAL_WWDG_Refresh(&WwdgHandle);
}
DECL_TASK(watchdog_reset);

void
watchdog_init(void)
{
    WwdgHandle.Instance = WWDG;
    WwdgHandle.Init.Prescaler = WWDG_PRESCALER_8;
    WwdgHandle.Init.Window    = 80U;
    WwdgHandle.Init.Counter   = 127U;
    (void)HAL_WWDG_Init(&WwdgHandle);
}
DECL_INIT(watchdog_init);
