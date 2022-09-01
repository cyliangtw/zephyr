/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NUMAKER_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NUMAKER_CLOCK_H_

/**
 * @brief Enable/disable oscillators or other clocks
 */
#define NUMAKER_SCC_CLKSW_UNTOUCHED     0
#define NUMAKER_SCC_CLKSW_ENABLE        1
#define NUMAKER_SCC_CLKSW_DISABLE       2

/**
 * @brief Module clock configuration structure
 */
struct numaker_pcc_module_clock_config {
    uint32_t clk_src;   /*!< Same as u32ClkSrc on invoking BSP CLK driver CLK_SetModuleClock() */
    uint32_t clk_div;   /*!< Same as u32ClkDiv on invoking BSP CLK driver CLK_SetModuleClock() */
};

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NUMAKER_CLOCK_H_ */
