/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_gpio

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>

#include "gpio_utils.h"
#include "NuMicro.h"
#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_PCC
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/numaker_clock.h>
#endif

#include <zephyr/logging/log.h>


#define NU_MFP_POS(pinindex)    ((pinindex % 4) * 8)

LOG_MODULE_REGISTER(gpio_numaker, LOG_LEVEL_ERR);


struct gpio_numaker_config {
	struct gpio_driver_config common;
	uint32_t reg;
    uint32_t gpa_base;
    uint32_t size;
#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_PCC
    const struct device *clk_dev;
#endif
    uint32_t clk_modidx;
};

struct gpio_numaker_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};


static int gpio_numaker_configure(const struct device *dev,
			       gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_numaker_config *config = dev->config;
	struct gpio_numaker_data *data = dev->data;
    GPIO_T *gpio_base = (GPIO_T*)config->reg;
    uint32_t pinMfpMask = (0x1f << NU_MFP_POS(pin));
    uint32_t pinMask = BIT(pin);  // mask for pin index --> (0x01 << pin)
  
    ARG_UNUSED(data);
    
	/* Check for an invalid pin number */
	if (pin >= 15) {
		return -EINVAL;
	}

	/* Configure GPIO direction */

	switch (flags & GPIO_DIR_MASK) {
	case GPIO_INPUT:
        GPIO_SetMode(gpio_base, pinMask, GPIO_MODE_INPUT);
		break;
	case GPIO_OUTPUT:
        GPIO_SetMode(gpio_base, pinMask, GPIO_MODE_OUTPUT);
		break;
    case (GPIO_INPUT | GPIO_OUTPUT):
        GPIO_SetMode(gpio_base, pinMask, GPIO_MODE_QUASI);
		break;
	default:
		return -ENOTSUP;
	}

    if (flags & GPIO_LINE_OPEN_DRAIN) {
        GPIO_SetMode(gpio_base, pinMask, GPIO_MODE_OPEN_DRAIN);
    }
    
	/* Set Multi-function, default is GPIO */
    uint32_t port_index = (config->reg - DT_REG_ADDR(DT_NODELABEL(gpioa)) ) / DT_REG_SIZE(DT_NODELABEL(gpioa));
    uint32_t *GPx_MFPx = ((uint32_t *) &SYS->GPA_MFP0) + port_index * 4 + (pin / 4);
    // E.g.: SYS->GPA_MFP0  = (SYS->GPA_MFP0 & (~SYS_GPA_MFP0_PA0MFP_Msk) ) | SYS_GPA_MFP0_PA0MFP_SC0_CD  ;
    *GPx_MFPx  = (*GPx_MFPx & (~pinMfpMask)) | 0x00;
    
	/* Set pull control as pull-up, pull-down or pull-disable */
     
     if ((flags & GPIO_PULL_UP) != 0) {
         GPIO_SetPullCtl(gpio_base, pinMask, GPIO_PUSEL_PULL_UP);
	} else if ((flags & GPIO_PULL_DOWN) != 0) {
         GPIO_SetPullCtl(gpio_base, pinMask, GPIO_PUSEL_PULL_DOWN);
    } else {
         GPIO_SetPullCtl(gpio_base, pinMask, GPIO_PUSEL_DISABLE);
    }

	return 0;
}

static int gpio_numaker_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_numaker_config *config = dev->config;
    GPIO_T *gpio_base = (GPIO_T*)config->reg;

    /* Get raw bits of GPIO PIN data */
    *value = gpio_base->PIN;
    
	return 0;
}


static int gpio_numaker_port_set_masked_raw(const struct device *dev,
					 uint32_t mask,
					 uint32_t value)
{
	const struct gpio_numaker_config *config = dev->config;
    GPIO_T *gpio_base = (GPIO_T*)config->reg;

	gpio_base->DOUT = (gpio_base->DOUT & ~mask) | (mask & value);

	return 0;
}

static int gpio_numaker_port_set_bits_raw(const struct device *dev,
				       uint32_t mask)
{
	const struct gpio_numaker_config *config = dev->config;
    GPIO_T *gpio_base = (GPIO_T*)config->reg;

	/* Set raw bits of GPIO output data */
	gpio_base->DOUT |= mask;

	return 0;
}

static int gpio_numaker_port_clear_bits_raw(const struct device *dev,
					 gpio_port_pins_t mask)
{
	const struct gpio_numaker_config *config = dev->config;
    GPIO_T *gpio_base = (GPIO_T*)config->reg;

	/* Clear raw bits of GPIO data */
	gpio_base->DOUT &= ~mask;

	return 0;
}

static int gpio_numaker_port_toggle_bits(const struct device *dev,
				      gpio_port_pins_t mask)
{
	const struct gpio_numaker_config *config = dev->config;
    GPIO_T *gpio_base = (GPIO_T*)config->reg;

	/* Toggle raw bits of GPIO data */
	gpio_base->DOUT ^= mask;

	return 0;
}


static int gpio_numaker_pin_interrupt_configure(const struct device *dev,
					     gpio_pin_t pin,
					     enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	const struct gpio_numaker_config *config = dev->config;
    GPIO_T *gpio_base = (GPIO_T*)config->reg;
    uint32_t intAttr;
    
	if (mode == GPIO_INT_MODE_DISABLED) {
        GPIO_DisableInt(gpio_base, pin);
	} else {
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			intAttr = ((mode == GPIO_INT_MODE_EDGE) ? GPIO_INT_FALLING : GPIO_INT_LOW);
			break;
		case GPIO_INT_TRIG_HIGH:
			intAttr = ((mode == GPIO_INT_MODE_EDGE) ? GPIO_INT_RISING : GPIO_INT_HIGH);
			break;
		case GPIO_INT_TRIG_BOTH:
            if (mode != GPIO_INT_MODE_EDGE) return -ENOTSUP;
			intAttr = GPIO_INT_BOTH_EDGE;
			break;
		default:
			return -ENOTSUP;
		}
        GPIO_EnableInt(gpio_base, pin, intAttr);
    }

	return 0;
}

static int gpio_numaker_manage_callback(const struct device *dev,
				     struct gpio_callback *callback, bool set)
{
	struct gpio_numaker_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}


static const struct gpio_driver_api gpio_numaker_api = {
	.pin_configure = gpio_numaker_configure,
	.port_get_raw = gpio_numaker_port_get_raw,
	.port_set_masked_raw = gpio_numaker_port_set_masked_raw,
	.port_set_bits_raw = gpio_numaker_port_set_bits_raw,
	.port_clear_bits_raw = gpio_numaker_port_clear_bits_raw,
	.port_toggle_bits = gpio_numaker_port_toggle_bits,
	.pin_interrupt_configure = gpio_numaker_pin_interrupt_configure,
	.manage_callback = gpio_numaker_manage_callback
};

static void gpio_numaker_isr(const struct device *dev)
{
	const struct gpio_numaker_config *config = dev->config;
	struct gpio_numaker_data *data = dev->data;
    GPIO_T *gpio_base = (GPIO_T*)config->reg;
	uint32_t int_status;

    /* Get the int status  */
	int_status = gpio_base->INTSRC;

	/* Clear the port int status */
	gpio_base->INTSRC = int_status;

	gpio_fire_callbacks(&data->callbacks, dev, int_status);
}

#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_PCC
#define NUMAKER_DT_INST_CLOCK_INIT(n)                           \
    .clk_dev    = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),        \
    .clk_modidx = DT_INST_CLOCKS_CELL(n, clock_module_index),
#else
#define NUMAKER_DT_INST_CLOCK_INIT(n)                           \
    .clk_modidx = DT_INST_CLOCKS_CELL(n, clock_module_index),
#endif

#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_PCC
#define GPIO_NUMAKER_CLOCK_INIT(n)                  \
    do {                                            \
        const struct gpio_numaker_config *config = dev->config;                 \
        int err = clock_control_on(config->clk_dev, \
                               (clock_control_subsys_t) config->clk_modidx);    \
        if (err != 0) {                             \
            return err;                             \
        }                                           \
    } while (0)
#else
#define GPIO_NUMAKER_CLOCK_INIT(n)                  \
    do {                                            \
        const struct gpio_numaker_config *config = dev->config;                 \
        SYS_UnlockReg();                            \
        CLK_EnableModuleClock(config->clk_modidx);  \
        SYS_LockReg();                              \
    } while (0)
#endif

#define GPIO_NUMAKER_IRQ_INIT(n)						\
	do {								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    gpio_numaker_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
									\
		irq_enable(DT_INST_IRQN(n));				\
	} while (0)


#define GPIO_NUMAKER_DEFINE(n)						       \
	static const struct gpio_numaker_config gpio_numaker_config##n = {	       \
		.common = {						       \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),   \
		},							       \
		.reg = DT_INST_REG_ADDR(n),				       \
        .gpa_base = DT_REG_ADDR(DT_NODELABEL(gpioa)),    \
        .size = DT_REG_SIZE(DT_NODELABEL(gpioa)),         \
        NUMAKER_DT_INST_CLOCK_INIT(n)               \
	};								       \
									       \
	static struct gpio_numaker_data gpio_numaker_data##n;			       \
									       \
	static int gpio_numaker_init##n(const struct device *dev)	\
	{								\
        GPIO_NUMAKER_CLOCK_INIT(n); \
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 0),			\
			(GPIO_NUMAKER_IRQ_INIT(n);))			\
		return 0;						\
	}                                   \
	DEVICE_DT_INST_DEFINE(n, &gpio_numaker_init##n, NULL, &gpio_numaker_data##n,    \
			      &gpio_numaker_config##n, PRE_KERNEL_1,	       \
			      CONFIG_GPIO_INIT_PRIORITY, &gpio_numaker_api); 
								
                  
DT_INST_FOREACH_STATUS_OKAY(GPIO_NUMAKER_DEFINE)
