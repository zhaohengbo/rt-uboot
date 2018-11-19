/*
 *  linux/drivers/clocksource/arm_arch_timer.c
 *
 *  Copyright (C) 2011 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <rthw.h>
#include <rtthread.h>

#include <asm/io.h>

#include "gic.h"

#define GCNT_BASE			0x004a1000
 
#define GCNT_CNTCV_LO		0x004a2000
#define GCNT_CNTCV_HI		0x004a2004
#define QTMR_CNTPCT_LO		0x0
#define QTMR_CNTPCT_HI		0x4
#define QTMR_CNTVCT_LO		0x8
#define QTMR_CNTVCT_HI		0xC
#define QTMR_CNTFRQ 		0x10
#define QTMR_CNTP_TVAL		0x28
#define QTMR_CNTP_CTL		0x2c
#define QTMR_CNTV_TVAL		0x38
#define QTMR_CNTV_CTL		0x3c

#define QTMR_LOAD_VAL		0x00FFFFFFFFFFFFFF

#define ARCH_TIMER_CTRL_ENABLE		(1 << 0)
#define ARCH_TIMER_CTRL_IT_MASK		(1 << 1)
#define ARCH_TIMER_CTRL_IT_STAT		(1 << 2)

#define TIMER_CLK_FREQ 48000000

#define SOC_IRQ_TIMER1 (16 + 2) //SECURE
#define SOC_IRQ_TIMER2 (16 + 3) //NO-SECURE
#define SOC_IRQ_TIMER3 (16 + 4) //VIRTUAL
#define SOC_IRQ_TIMER4 (16 + 1) //HYP
#define SOC_IRQ_TIMER_LEVEL IRQ_TYPE_LEVEL

void soc_timer_isr_handler(void)
{
	writel(TIMER_CLK_FREQ/RT_TICK_PER_SECOND, GCNT_BASE + QTMR_CNTP_TVAL);
}

void soc_timer_init(void)
{
	rt_uint32_t ctrl;
	
	ctrl = readl(GCNT_BASE + QTMR_CNTP_CTL);
	
	ctrl |= ARCH_TIMER_CTRL_ENABLE;
	
	writel(ctrl, GCNT_BASE + QTMR_CNTP_CTL);
	
	writel(TIMER_CLK_FREQ/RT_TICK_PER_SECOND, GCNT_BASE + QTMR_CNTP_TVAL);
}

extern void rt_hw_timer_isr(int vector, void *param);

void soc_timer_isr_install(void)
{
	rt_hw_interrupt_install(SOC_IRQ_TIMER2, rt_hw_timer_isr, RT_NULL, "secure-tick");
	rt_hw_interrupt_mask(SOC_IRQ_TIMER2);
	rt_hw_interrupt_config(SOC_IRQ_TIMER2,SOC_IRQ_TIMER_LEVEL);
    rt_hw_interrupt_umask(SOC_IRQ_TIMER2);
}