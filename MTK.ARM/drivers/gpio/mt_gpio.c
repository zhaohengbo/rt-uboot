/******************************************************************************
 * mt_gpio.c - MTKLinux GPIO Device Driver
 * 
 * Copyright 2008-2009 MediaTek Co.,Ltd.
 * 
 * DESCRIPTION:
 *     This file provid the other drivers GPIO relative functions
 *
 ******************************************************************************/

//#include <asm/arch/mt_reg_base.h>
#include <asm/arch/mt_gpio.h>
//#include <asm/arch/debug.h>

static const struct mtk_pin_info *mtk_pinctrl_get_gpio_array(int pin, int size,
	const struct mtk_pin_info pArray[])
{
	unsigned long i = 0;

	for (i = 0; i < size; i++) {
		if (pin == pArray[i].pin)
			return &pArray[i];
	}
	return NULL;
}

int mtk_pinctrl_set_gpio_value(int pin,	int value, int size, const struct mtk_pin_info pin_info[])
{
	unsigned int reg_set_addr, reg_rst_addr;
	const struct mtk_pin_info *spec_pin_info;

	spec_pin_info = mtk_pinctrl_get_gpio_array(pin, size, pin_info);

	if (spec_pin_info != NULL) {
		reg_set_addr = spec_pin_info->offset + 4;
		reg_rst_addr = spec_pin_info->offset + 8;
		if (value)
			GPIO_SET_BITS((1L << spec_pin_info->bit), (GPIO_BASE + reg_set_addr));
		else
			GPIO_SET_BITS((1L << spec_pin_info->bit), (GPIO_BASE + reg_rst_addr));
	} else {
		return -ERINVAL;
	}
	return 0;
}

int mtk_pinctrl_get_gpio_value(int pin, int size, const struct mtk_pin_info pin_info[])
{
	unsigned int reg_value, reg_get_addr;
	const struct mtk_pin_info *spec_pin_info;
	unsigned char bit_width, reg_bit;

	spec_pin_info = mtk_pinctrl_get_gpio_array(pin, size, pin_info);

	if (spec_pin_info != NULL) {
		reg_get_addr = spec_pin_info->offset;
		bit_width = spec_pin_info->width;
		reg_bit = spec_pin_info->bit;
		reg_value = GPIO_RD32(GPIO_BASE+  reg_get_addr);
		return ((reg_value >> reg_bit) & ((1 << bit_width) - 1));
	} else {
		return -ERINVAL;
	}
}

int mtk_pinctrl_update_gpio_value(int pin, unsigned char value, int size, const struct mtk_pin_info pin_info[])
{
	unsigned int reg_update_addr;
	unsigned int mask, reg_value;
	const struct mtk_pin_info *spec_update_pin;
	unsigned char bit_width;

	spec_update_pin = mtk_pinctrl_get_gpio_array(pin, size, pin_info);

	if (spec_update_pin != NULL) {
		reg_update_addr = spec_update_pin->offset;
		bit_width = spec_update_pin->width;
		mask = ((1 << bit_width) - 1) << spec_update_pin->bit;
		reg_value = GPIO_RD32(GPIO_BASE + reg_update_addr);
		reg_value &= ~(mask);
		reg_value |= (value << spec_update_pin->bit);
		GPIO_WR32((GPIO_BASE + reg_update_addr),reg_value);
	} else {
		return -ERINVAL;
	}
	return 0;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_dir_chip(unsigned long pin, unsigned long dir)
{
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    if (dir >= GPIO_DIR_MAX)
        return -ERINVAL;

	mtk_pinctrl_set_gpio_value(pin, dir, MAX_GPIO_PIN, mtk_pin_info_dir);
    return RSUCCESS;

}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_dir_chip(unsigned long pin)
{
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    return mtk_pinctrl_get_gpio_value(pin, MAX_GPIO_PIN, mtk_pin_info_dir);
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_smt_chip(unsigned long pin, unsigned long enable)
{
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

	mtk_pinctrl_set_gpio_value(pin, enable, MAX_GPIO_PIN, mtk_pin_info_smt);
    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_smt_chip(unsigned long pin)
{
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    return mtk_pinctrl_get_gpio_value(pin, MAX_GPIO_PIN, mtk_pin_info_dir);
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_select_chip(unsigned long pin, unsigned long select)
{
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    if (select == 1) {
        mtk_pinctrl_set_gpio_value(pin, 0, MAX_GPIO_PIN, mtk_pin_info_pd);
        mtk_pinctrl_set_gpio_value(pin, 1, MAX_GPIO_PIN, mtk_pin_info_pu);
    } else {
        mtk_pinctrl_set_gpio_value(pin, 0, MAX_GPIO_PIN, mtk_pin_info_pu);
        mtk_pinctrl_set_gpio_value(pin, 1, MAX_GPIO_PIN, mtk_pin_info_pd);
    }

    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_select_chip(unsigned long pin)
{
    unsigned long bit_pu,bit_pd,pull_sel;

    bit_pu = mtk_pinctrl_get_gpio_value(pin, MAX_GPIO_PIN, mtk_pin_info_pu);
    bit_pd = mtk_pinctrl_get_gpio_value(pin, MAX_GPIO_PIN, mtk_pin_info_pd);
    pull_sel = (bit_pd | bit_pu << 1);

	if (pull_sel == 0x02)
		pull_sel = GPIO_PULL_UP;
	else if (pull_sel == 0x01)
		pull_sel = GPIO_PULL_DOWN;
	else
		return -ERINVAL;

	return pull_sel;
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_out_chip(unsigned long pin, unsigned long output)
{
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    if (output >= GPIO_OUT_MAX)
        return -ERINVAL;

	mtk_pinctrl_set_gpio_value(pin, output, MAX_GPIO_PIN, mtk_pin_info_dataout);
    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_out_chip(unsigned long pin)
{
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    return mtk_pinctrl_get_gpio_value(pin, MAX_GPIO_PIN, mtk_pin_info_dataout);
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_in_chip(unsigned long pin)
{
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    return mtk_pinctrl_get_gpio_value(pin, MAX_GPIO_PIN, mtk_pin_info_datain);
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_mode_chip(unsigned long pin, unsigned long mode)
{
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    if (mode >= GPIO_MODE_MAX)
        return -ERINVAL;

    mtk_pinctrl_update_gpio_value(pin, mode, MAX_GPIO_PIN, mtk_pin_info_mode);
    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_mode_chip(unsigned long pin)
{
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    return mtk_pinctrl_get_gpio_value(pin, MAX_GPIO_PIN, mtk_pin_info_mode);
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_drving_chip(unsigned long pin,unsigned long drv)
{
	unsigned long drv_e4, drv_e8;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

	drv_e4 = drv & 0x1;
	drv_e8 = (drv & 0x2) >> 1;

	mtk_pinctrl_set_gpio_value(pin, drv_e4, MAX_GPIO_PIN, mtk_pin_info_drve4);
	mtk_pinctrl_set_gpio_value(pin, drv_e8, MAX_GPIO_PIN, mtk_pin_info_drve8);

    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_drving_chip(unsigned long pin)
{
	unsigned long drv_e4, drv_e8;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    drv_e4 = mtk_pinctrl_get_gpio_value(pin, MAX_GPIO_PIN, mtk_pin_info_drve4);
    drv_e8 = mtk_pinctrl_get_gpio_value(pin, MAX_GPIO_PIN, mtk_pin_info_drve8);

    return ((drv_e4)|(drv_e8<<1));
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
int mt_set_gpio_dir(unsigned long pin, unsigned long dir)
{
    return mt_set_gpio_dir_chip(pin,dir);
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_dir(unsigned long pin)
{
    return mt_get_gpio_dir_chip(pin);
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_select(unsigned long pin, unsigned long select)
{
    return mt_set_gpio_pull_select_chip(pin,select);
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_select(unsigned long pin)
{
    return mt_get_gpio_pull_select_chip(pin);
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_smt(unsigned long pin, unsigned long enable)
{
    return mt_set_gpio_smt_chip(pin,enable);
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_smt(unsigned long pin)
{
    return mt_get_gpio_smt_chip(pin);
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_out(unsigned long pin, unsigned long output)
{
    return mt_set_gpio_out_chip(pin,output);
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_out(unsigned long pin)
{
    return mt_get_gpio_out_chip(pin);
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_in(unsigned long pin)
{
    return mt_get_gpio_in_chip(pin);
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_mode(unsigned long pin, unsigned long mode)
{
    return mt_set_gpio_mode_chip(pin,mode);
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_mode(unsigned long pin)
{
    return mt_get_gpio_mode_chip(pin);
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_drving(unsigned long pin, unsigned long drv)
{
    return mt_set_gpio_drving_chip(pin,drv);
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_drving(unsigned long pin)
{
    return mt_get_gpio_drving_chip(pin);
}
