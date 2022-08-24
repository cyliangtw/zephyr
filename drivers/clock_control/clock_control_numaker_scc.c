/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_scc

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/numaker_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_numaker_scc, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

struct numaker_scc_config {
    uint32_t    clk_base;
    int         hxt;
    int         lxt;
    int         hirc;
    int         lirc;
    int         hirc48;
    uint32_t    clk_pclkdiv;
    uint32_t    core_clock;
};

static inline int numaker_scc_on(const struct device *dev,
                                 clock_control_subsys_t subsys)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(subsys);
    return -ENOTSUP;
}

static inline int numaker_scc_off(const struct device *dev,
					              clock_control_subsys_t subsys)
{
	ARG_UNUSED(dev);
    ARG_UNUSED(subsys);
    return -ENOTSUP;
}

static inline int numaker_scc_get_rate(const struct device *dev,
					                   clock_control_subsys_t subsys,
					                   uint32_t *rate)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(subsys);
    ARG_UNUSED(rate);
    return -ENOTSUP;
}

static inline int numaker_scc_set_rate(const struct device *dev,
		                               clock_control_subsys_t subsys,
		                               clock_control_subsys_rate_t rate)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(subsys);
    ARG_UNUSED(rate);
    return -ENOTSUP;
}

static inline int numaker_scc_configure(const struct device *dev,
                                        clock_control_subsys_t subsys,
                                        void *data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(subsys);
    ARG_UNUSED(data);
    return -ENOTSUP;
}

/* System clock controller driver registration */
static struct clock_control_driver_api numaker_scc_api = {
    .on             = numaker_scc_on,
    .off            = numaker_scc_off,
    .get_rate       = numaker_scc_get_rate,
    .set_rate       = numaker_scc_set_rate,
    .configure      = numaker_scc_configure,
};

/* At most one compatible with status "okay" */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
        "Requires at most one compatible with status \"okay\"");

#define LOG_OSC_SW(osc, sw)                         \
    if (sw == NUMAKER_SCC_CLKSW_ENABLE) {           \
        LOG_INF("Enable " #osc);                    \
    } else if (sw == NUMAKER_SCC_CLKSW_DISABLE) {   \
        LOG_INF("Disable " #osc);                   \
    }

static int numaker_scc_init(const struct device *dev)
{
    const struct numaker_scc_config *cfg = dev->config;

    LOG_INF("CLK base: 0x%08x", cfg->clk_base);
#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), hxt)
    LOG_OSC_SW(HXT, cfg->hxt);
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), lxt)
    LOG_OSC_SW(LXT, cfg->lxt);
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), hirc)
    LOG_OSC_SW(HIRC, cfg->hirc);
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), lirc)
    LOG_OSC_SW(LIRC, cfg->lirc);
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), hirc48)
    LOG_OSC_SW(HIRC48, cfg->hirc48);
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), clk_pclkdiv)
    LOG_INF("CLK_PCLKDIV: 0x%08x", cfg->clk_pclkdiv);
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), core_clock)
    LOG_INF("Core clock: %d (Hz)", cfg->core_clock);
#endif

    SYS_UnlockReg();

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), hxt)
    /* Enable/disable HXT on request */
    if (cfg->hxt == NUMAKER_SCC_CLKSW_ENABLE) {
        CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
        CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);
    } else if (cfg->hxt == NUMAKER_SCC_CLKSW_DISABLE) {
        CLK_DisableXtalRC(CLK_PWRCTL_HXTEN_Msk);
    }
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), lxt)
    /* Enable/disable LXT on request */
    if (cfg->lxt == NUMAKER_SCC_CLKSW_ENABLE) {
        CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
        CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);
    } else if (cfg->lxt == NUMAKER_SCC_CLKSW_DISABLE) {
        CLK_DisableXtalRC(CLK_PWRCTL_LXTEN_Msk);
    }
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), hirc)
    /* Enable/disable HIRC on request */
    if (cfg->hirc == NUMAKER_SCC_CLKSW_ENABLE) {
        CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
        CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);
    } else if (cfg->hirc == NUMAKER_SCC_CLKSW_DISABLE) {
        CLK_DisableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    }
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), lirc)
    /* Enable/disable LIRC on request */
    if (cfg->lirc == NUMAKER_SCC_CLKSW_ENABLE) {
        CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
        CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);
    } else if (cfg->lirc == NUMAKER_SCC_CLKSW_DISABLE) {
        CLK_DisableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
    }
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), hirc48)
    /* Enable/disable HIRC48 on request */
    if (cfg->hirc48 == NUMAKER_SCC_CLKSW_ENABLE) {
        CLK_EnableXtalRC(CLK_PWRCTL_HIRC48EN_Msk);
        CLK_WaitClockReady(CLK_STATUS_HIRC48STB_Msk);
    } else if (cfg->hirc48 == NUMAKER_SCC_CLKSW_DISABLE) {
        CLK_DisableXtalRC(CLK_PWRCTL_HIRC48EN_Msk);
    }
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), clk_pclkdiv)
    /* Set CLK_PCLKDIV register on request */
    CLK->PCLKDIV = cfg->clk_pclkdiv;
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), core_clock)
    /* Set core clock (HCLK) on request */
    CLK_SetCoreClock(cfg->core_clock);
#endif

    /* Update System Core Clock */
    SystemCoreClockUpdate();

    SYS_LockReg();

    LOG_INF("SystemCoreClock: %d (Hz)", SystemCoreClock);

	return 0;
}

#define NUMICRO_SCC_INIT(inst)			            \
    static const struct numaker_scc_config numaker_scc_config_ ## inst = {  \
        .clk_base       = DT_INST_REG_ADDR(inst),                       \
        .hxt            = DT_INST_ENUM_IDX_OR(inst, hxt, NUMAKER_SCC_CLKSW_UNTOUCHED),      \
        .lxt            = DT_INST_ENUM_IDX_OR(inst, lxt, NUMAKER_SCC_CLKSW_UNTOUCHED),      \
        .hirc           = DT_INST_ENUM_IDX_OR(inst, hirc, NUMAKER_SCC_CLKSW_UNTOUCHED),     \
        .lirc           = DT_INST_ENUM_IDX_OR(inst, lirc, NUMAKER_SCC_CLKSW_UNTOUCHED),     \
        .hirc48         = DT_INST_ENUM_IDX_OR(inst, hirc48, NUMAKER_SCC_CLKSW_UNTOUCHED),   \
        .clk_pclkdiv    = DT_INST_PROP_OR(inst, clk_pclkdiv, 0),        \
        .core_clock     = DT_INST_PROP_OR(inst, core_clock, 0),         \
    };                                                                  \
                                                                        \
    DEVICE_DT_INST_DEFINE(inst,					    \
                &numaker_scc_init,                  \
                NULL,                               \
                NULL,                               \
                &numaker_scc_config_ ## inst,       \
                PRE_KERNEL_1,                       \
                CONFIG_CLOCK_CONTROL_INIT_PRIORITY, \
                &numaker_scc_api);

DT_INST_FOREACH_STATUS_OKAY(NUMICRO_SCC_INIT);
