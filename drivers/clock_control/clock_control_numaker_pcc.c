/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_pcc

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/numaker_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_numaker_pcc, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

struct numaker_pcc_config {
    uint32_t clk_base;
};

static inline int numaker_pcc_on(const struct device *dev,
                                 clock_control_subsys_t subsys)
{
    ARG_UNUSED(dev);

    uint32_t clk_modidx = (uint32_t) subsys;

    SYS_UnlockReg();

    CLK_EnableModuleClock(clk_modidx);

    SYS_LockReg();

    return 0;
}

static inline int numaker_pcc_off(const struct device *dev,
					              clock_control_subsys_t subsys)
{
	ARG_UNUSED(dev);

    uint32_t clk_modidx = (uint32_t) subsys;

    SYS_UnlockReg();

    CLK_DisableModuleClock(clk_modidx);

    SYS_LockReg();

    return 0;
}

static inline int numaker_pcc_get_rate(const struct device *dev,
					                   clock_control_subsys_t subsys,
					                   uint32_t *rate)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(subsys);
    ARG_UNUSED(rate);
    return -ENOTSUP;
}

static inline int numaker_pcc_set_rate(const struct device *dev,
		                               clock_control_subsys_t subsys,
		                               clock_control_subsys_rate_t rate)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(subsys);
    ARG_UNUSED(rate);
    return -ENOTSUP;
}

static inline int numaker_pcc_configure(const struct device *dev,
                                        clock_control_subsys_t subsys,
                                        void *data)
{
    ARG_UNUSED(dev);

    uint32_t clk_modidx = (uint32_t) subsys;
    struct numaker_pcc_module_clock_config *mod_clk_cfg = (struct numaker_pcc_module_clock_config *) data;

    SYS_UnlockReg();

    CLK_SetModuleClock(clk_modidx, mod_clk_cfg->clk_src, mod_clk_cfg->clk_div);

    SYS_LockReg();

    return 0;
}

/* Peripheral clock controller driver registration */
static struct clock_control_driver_api numaker_pcc_api = {
    .on             = numaker_pcc_on,
    .off            = numaker_pcc_off,
    .get_rate       = numaker_pcc_get_rate,
    .set_rate       = numaker_pcc_set_rate,
    .configure      = numaker_pcc_configure,
};

/* At most one compatible with status "okay" */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
        "Requires at most one compatible with status \"okay\"");

static int numaker_pcc_init(const struct device *dev)
{
    const struct numaker_pcc_config *cfg = dev->config;

    LOG_INF("CLK base: 0x%08x", cfg->clk_base);

	return 0;
}

#define NUMICRO_PCC_INIT(inst)			                    \
    static const struct numaker_pcc_config numaker_pcc_config_ ## inst = {  \
        .clk_base   = DT_REG_ADDR(DT_INST_PARENT(inst)),    \
    };                                                      \
                                                            \
    DEVICE_DT_INST_DEFINE(inst,					            \
                &numaker_pcc_init,                          \
                NULL,                                       \
                NULL,                                       \
                &numaker_pcc_config_ ## inst,               \
                PRE_KERNEL_1,                               \
                CONFIG_CLOCK_CONTROL_INIT_PRIORITY,         \
                &numaker_pcc_api);

DT_INST_FOREACH_STATUS_OKAY(NUMICRO_PCC_INIT);
