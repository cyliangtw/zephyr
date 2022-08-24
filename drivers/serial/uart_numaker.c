/*
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Copyright (c) 2022 Nuvoton Technology Corporation.
 */

#include <zephyr/drivers/uart.h>
#include <string.h>
#include "NuMicro.h"
#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_PCC
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/numaker_clock.h>
#endif
#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

#define DT_DRV_COMPAT nuvoton_numaker_uart

struct uart_numaker_config {
	UART_T *uart;
	uint32_t id_rst;
#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_PCC
    const struct device *clk_dev;
#endif
	uint32_t clk_modidx;
    uint32_t clk_src;
    uint32_t clk_div;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif
};

struct uart_numaker_data {
	const struct device *clock;
	struct uart_config ucfg;
};

static int uart_numaker_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_numaker_config *config = dev->config;
	uint32_t count;

	count = UART_Read(config->uart, c, 1);
	if (!count) {
		return -1;
	}

	return 0;
}

static void uart_numaker_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_numaker_config *config = dev->config;

	UART_Write(config->uart, &c, 1);
}

static int uart_numaker_err_check(const struct device *dev)
{
	return 0;
}

static inline int32_t uart_numaker_convert_stopbit(enum uart_config_stop_bits sb)
{
	switch (sb) {
	case UART_CFG_STOP_BITS_1:
		return UART_STOP_BIT_1;
	case UART_CFG_STOP_BITS_1_5:
		return UART_STOP_BIT_1_5;
	case UART_CFG_STOP_BITS_2:
		return UART_STOP_BIT_2;
	default:
		return -ENOTSUP;
	}
};

static inline int32_t uart_numaker_convert_datalen(enum uart_config_data_bits db)
{
	switch (db) {
	case UART_CFG_DATA_BITS_5:
		return UART_WORD_LEN_5;
	case UART_CFG_DATA_BITS_6:
		return UART_WORD_LEN_6;
	case UART_CFG_DATA_BITS_7:
		return UART_WORD_LEN_7;
	case UART_CFG_DATA_BITS_8:
		return UART_WORD_LEN_8;
	default:
		return -ENOTSUP;
	}
}

static inline uint32_t uart_numaker_convert_parity(enum uart_config_parity parity)
{
	switch (parity) {
	case UART_CFG_PARITY_ODD:
		return UART_PARITY_ODD;
	case UART_CFG_PARITY_EVEN:
		return UART_PARITY_EVEN;
	case UART_CFG_PARITY_MARK:
		return UART_PARITY_MARK;
	case UART_CFG_PARITY_SPACE:
		return UART_PARITY_SPACE;
	case UART_CFG_PARITY_NONE:
	default:
		return UART_PARITY_NONE;
	}
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_numaker_configure(const struct device *dev,
				  const struct uart_config *cfg)
{
	const struct uart_numaker_config *config = dev->config;
	struct uart_numaker_data *pData = dev->data;
	int32_t databits, stopbits;
	uint32_t parity;

	databits = uart_numaker_convert_datalen(cfg->data_bits);
	if (databits < 0) {
		return databits;
	}

	stopbits = uart_numaker_convert_stopbit(cfg->stop_bits);
	if (stopbits < 0) {
		return stopbits;
	}

	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_NONE) {
		UART_DisableFlowCtrl(config->uart);
	} else if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
		UART_EnableFlowCtrl(config->uart);
	} else {
		return -ENOTSUP;
	}

	parity = uart_numaker_convert_parity(cfg->parity);

	UART_SetLineConfig(config->uart, cfg->baudrate, databits, parity,
			   stopbits);

	memcpy(&pData->ucfg, cfg, sizeof(*cfg));

	return 0;
}

static int uart_numaker_config_get(const struct device *dev,
				   struct uart_config *cfg)
{
	struct uart_numaker_data *pData = dev->data;

	memcpy(cfg, &pData->ucfg, sizeof(*cfg));

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_numaker_init(const struct device *dev)
{
	const struct uart_numaker_config *config = dev->config;
	struct uart_numaker_data *pData = dev->data;
    int err;

	SYS_UnlockReg();

#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_PCC
    /* Equivalent to CLK_EnableModuleClock() */
    err = clock_control_on(config->clk_dev,
                           (clock_control_subsys_t) config->clk_modidx);
    if (err != 0) {
        SYS_LockReg();
        return err;
    }

    /* Equivalent to CLK_SetModuleClock() */
    struct numaker_pcc_module_clock_config mod_clk_cfg = {
        .clk_src    = config->clk_src,
        .clk_div    = config->clk_div,
    };
    err = clock_control_configure(config->clk_dev,
                                  (clock_control_subsys_t) config->clk_modidx,
                                  &mod_clk_cfg);
    if (err != 0) {
        SYS_LockReg();
        return err;
    }
#else
    /* Enable UART module clock */
    CLK_EnableModuleClock(config->clk_modidx);

    /* Select UART module clock source/divider */
    CLK_SetModuleClock(config->clk_modidx, config->clk_src, config->clk_div);
#endif

	/* Set pinctrl for UART0 RXD and TXD */
    /* Set multi-function pins for UART0 RXD and TXD */
#ifdef CONFIG_PINCTRL
	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}
#else
    SET_UART0_RXD_PB12();
    SET_UART0_TXD_PB13();    
#endif 

	SYS_LockReg();
  
	SYS_ResetModule(config->id_rst);
  
	UART_Open(config->uart, pData->ucfg.baudrate);

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/* API implementation: fifo_fill */
static int uart_numaker_fifo_fill(const struct device *dev,
			      const uint8_t *tx_data,
			      int size)
{
	int i = 0;

	return i;
}

/* API implementation: fifo_read */
static int uart_numaker_fifo_read(const struct device *dev,
			      uint8_t *rx_data,
			      const int size)
{
	int rx_count;

	return rx_count;
}

/* API implementation: irq_tx_enable */
static void uart_numaker_irq_tx_enable(const struct device *dev)
{

}

/* API implementation: irq_tx_disable */
static void uart_numaker_irq_tx_disable(const struct device *dev)
{

}

/* API implementation: irq_tx_ready */
static int uart_numaker_irq_tx_ready(const struct device *dev)
{

	return 1;
}

/* API implementation: irq_tx_complete */
static int uart_numaker_irq_tx_complete(const struct device *dev)
{

	return 1;
}

/* API implementation: irq_rx_enable */
static void uart_numaker_irq_rx_enable(const struct device *dev)
{

}

/* API implementation: irq_rx_disable */
static void uart_numaker_irq_rx_disable(const struct device *dev)
{

}

/* API implementation: irq_rx_ready */
static int uart_numaker_irq_rx_ready(const struct device *dev)
{
	return 1;
}

/* API implementation: irq_err_enable */
static void uart_numaker_irq_err_enable(const struct device *dev)
{

}

/* API implementation: irq_err_disable*/
static void uart_numaker_irq_err_disable(const struct device *dev)
{

}

/* API implementation: irq_is_pending */
static int uart_numaker_irq_is_pending(const struct device *dev)
{

}

/* API implementation: irq_update */
static int uart_numaker_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* nothing to be done */
	return 1;
}

/* API implementation: irq_callback_set */
static void uart_numaker_irq_callback_set(const struct device *dev,
				      uart_irq_callback_user_data_t cb,
				      void *cb_data)
{

}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static const struct uart_driver_api uart_numaker_driver_api = {
	.poll_in          = uart_numaker_poll_in,
	.poll_out         = uart_numaker_poll_out,
	.err_check        = uart_numaker_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure        = uart_numaker_configure,
	.config_get       = uart_numaker_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_numaker_fifo_fill,
	.fifo_read = uart_numaker_fifo_read,
	.irq_tx_enable = uart_numaker_irq_tx_enable,
	.irq_tx_disable = uart_numaker_irq_tx_disable,
	.irq_tx_ready = uart_numaker_irq_tx_ready,
	.irq_tx_complete = uart_numaker_irq_tx_complete,
	.irq_rx_enable = uart_numaker_irq_rx_enable,
	.irq_rx_disable = uart_numaker_irq_rx_disable,
	.irq_rx_ready = uart_numaker_irq_rx_ready,
	.irq_err_enable = uart_numaker_irq_err_enable,
	.irq_err_disable = uart_numaker_irq_err_disable,
	.irq_is_pending = uart_numaker_irq_is_pending,
	.irq_update = uart_numaker_irq_update,
	.irq_callback_set = uart_numaker_irq_callback_set,
#endif
};

#else
static const struct uart_driver_api uart_numaker_driver_api = {
	.poll_in          = uart_numaker_poll_in,
	.poll_out         = uart_numaker_poll_out,
	.err_check        = uart_numaker_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure        = uart_numaker_configure,
	.config_get       = uart_numaker_config_get,
#endif
};
#endif  /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_PCC
#define NUMAKER_DT_INST_CLOCK_INIT(inst)                            \
    .clk_dev    = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),         \
    .clk_modidx = DT_INST_CLOCKS_CELL(inst, clock_module_index),    \
    .clk_src    = DT_INST_CLOCKS_CELL(inst, clock_source),          \
    .clk_div    = DT_INST_CLOCKS_CELL(inst, clock_divider),
#else
#define NUMAKER_DT_INST_CLOCK_INIT(inst)                            \
    .clk_modidx = DT_INST_CLOCKS_CELL(inst, clock_module_index),    \
    .clk_src    = DT_INST_CLOCKS_CELL(inst, clock_source),          \
    .clk_div    = DT_INST_CLOCKS_CELL(inst, clock_divider),
#endif

#ifdef CONFIG_PINCTRL
#define PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n);
#define PINCTRL_INIT(n) .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),
#else
#define PINCTRL_DEFINE(n)
#define PINCTRL_INIT(n)
#endif

#define NUMAKER_INIT(inst)						\
	PINCTRL_DEFINE(inst)						\
									\
static const struct uart_numaker_config uart_numaker_cfg_##inst = {	\
	.uart = (UART_T *)DT_INST_REG_ADDR(inst),			\
	.id_rst = DT_INST_PROP(inst, reset),					\
    NUMAKER_DT_INST_CLOCK_INIT(inst)                        \
    PINCTRL_INIT(inst)							\
};									\
									\
static struct uart_numaker_data uart_numaker_data_##inst = {		\
	.ucfg = {							\
		.baudrate = DT_INST_PROP(inst, current_speed),		\
	},								\
};									\
									\
DEVICE_DT_INST_DEFINE(inst,						\
		    &uart_numaker_init,					\
		    NULL,						\
		    &uart_numaker_data_##inst,				\
		    &uart_numaker_cfg_##inst,				\
		    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,		\
		    &uart_numaker_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NUMAKER_INIT)
