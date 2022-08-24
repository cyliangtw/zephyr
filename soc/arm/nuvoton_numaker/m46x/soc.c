/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/dt-bindings/clock/numaker_clock.h>
/* Hardware and starter kit includes. */
#include "NuMicro.h"

void z_arm_platform_init(void)
{
    SystemInit();

#ifndef CONFIG_CLOCK_CONTROL_NUMAKER_SCC
    /* Unlock protected registers */
    SYS_UnlockReg();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), hxt)
    /* Enable/disable HXT on request */
    if (DT_ENUM_IDX(DT_NODELABEL(scc), hxt) == NUMAKER_SCC_CLKSW_ENABLE) {
        CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
        CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);
    } else if (DT_ENUM_IDX(DT_NODELABEL(scc), hxt) == NUMAKER_SCC_CLKSW_DISABLE) {
        CLK_DisableXtalRC(CLK_PWRCTL_HXTEN_Msk);
    }
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), lxt)
    /* Enable/disable LXT on request */
    if (DT_ENUM_IDX(DT_NODELABEL(scc), lxt) == NUMAKER_SCC_CLKSW_ENABLE) {
        CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
        CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);
    } else if (DT_ENUM_IDX(DT_NODELABEL(scc), lxt) == NUMAKER_SCC_CLKSW_DISABLE) {
        CLK_DisableXtalRC(CLK_PWRCTL_LXTEN_Msk);
    }
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), hirc)
    /* Enable/disable HIRC on request */
    if (DT_ENUM_IDX(DT_NODELABEL(scc), hirc) == NUMAKER_SCC_CLKSW_ENABLE) {
        CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
        CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);
    } else if (DT_ENUM_IDX(DT_NODELABEL(scc), hirc) == NUMAKER_SCC_CLKSW_DISABLE) {
        CLK_DisableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    }
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), lirc)
    /* Enable/disable LIRC on request */
    if (DT_ENUM_IDX(DT_NODELABEL(scc), lirc) == NUMAKER_SCC_CLKSW_ENABLE) {
        CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
        CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);
    } else if (DT_ENUM_IDX(DT_NODELABEL(scc), lirc) == NUMAKER_SCC_CLKSW_DISABLE) {
        CLK_DisableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
    }
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), hirc48)
    /* Enable/disable HIRC48 on request */
    if (DT_ENUM_IDX(DT_NODELABEL(scc), hirc48) == NUMAKER_SCC_CLKSW_ENABLE) {
        CLK_EnableXtalRC(CLK_PWRCTL_HIRC48EN_Msk);
        CLK_WaitClockReady(CLK_STATUS_HIRC48STB_Msk);
    } else if (DT_ENUM_IDX(DT_NODELABEL(scc), hirc48) == NUMAKER_SCC_CLKSW_DISABLE) {
        CLK_DisableXtalRC(CLK_PWRCTL_HIRC48EN_Msk);
    }
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), clk_pclkdiv)
    /* Set CLK_PCLKDIV register on request */
    CLK->PCLKDIV = DT_PROP(DT_NODELABEL(scc), clk_pclkdiv);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), core_clock)
    /* Set core clock (HCLK) on request */
    CLK_SetCoreClock(DT_PROP(DT_NODELABEL(scc), core_clock));
#endif

     /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();
    
    /* Lock protected registers */
    SYS_LockReg();
#endif
}
