/*
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2009-2012,2014, The Linux Foundation. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <rthw.h>
#include <rtthread.h>

#include <asm/io.h>

#include "gic.h"

#define MSM_TMR_BASE        0x0200A000
 
#define TIMER_MATCH_VAL					0x0000
#define TIMER_COUNT_VAL					0x0004
#define TIMER_ENABLE					0x0008
#define TIMER_CLEAR			    		0x000C
#define TIMER_ENABLE_CLR_ON_MATCH_EN	2
#define TIMER_ENABLE_EN					1
#define DGT_CLK_CTL			    		0x10
#define DGT_CLK_CTL_DIV_4				0x3
#define TIMER_STS_GPT0_CLR_PEND			0x400

#define TIMER_CLK_FREQ 25000000

#define SOC_IRQ_TIMER1 (16 + 1) //GPT0
#define SOC_IRQ_TIMER2 (16 + 2) //GPT1
#define SOC_IRQ_TIMER3 (16 + 3) //GPT2
#define SOC_IRQ_TIMER_LEVEL IRQ_TYPE_EDGE

static rt_uint32_t *sts_base;
static rt_uint32_t *event_base;
static rt_uint32_t *source_base;

static rt_uint32_t soc_read_timer_count(void)
{
	return readl(source_base + TIMER_COUNT_VAL);
}

static void soc_timer_set_value(rt_uint32_t value)
{
	rt_uint32_t ctrl;
	
	ctrl = readl(event_base + TIMER_ENABLE);

	ctrl &= ~TIMER_ENABLE_EN;
	writel(ctrl, event_base + TIMER_ENABLE);

	writel(ctrl, event_base + TIMER_CLEAR);
	
	value += soc_read_timer_count();
	
	writel(value, event_base + TIMER_MATCH_VAL);

	if (sts_base)
		while (readl(sts_base) & TIMER_STS_GPT0_CLR_PEND)
			;

	writel(ctrl | TIMER_ENABLE_EN, event_base + TIMER_ENABLE);
}

void soc_timer_isr_handler(void)
{
	soc_timer_set_value(TIMER_CLK_FREQ/RT_TICK_PER_SECOND);
}

void soc_timer_init(void)
{
	rt_uint32_t ctrl;
	
	event_base = MSM_TMR_BASE + 0x4;
	sts_base = MSM_TMR_BASE + 0x88;
	source_base = MSM_TMR_BASE + 0x24;
	
	writel(DGT_CLK_CTL_DIV_4, source_base + DGT_CLK_CTL);
	
	ctrl = readl(event_base + TIMER_ENABLE);
	ctrl &= ~(TIMER_ENABLE_EN | TIMER_ENABLE_CLR_ON_MATCH_EN);
	writel(ctrl, event_base + TIMER_ENABLE);
	
	soc_timer_set_value(TIMER_CLK_FREQ/RT_TICK_PER_SECOND);
}

extern void rt_hw_timer_isr(int vector, void *param);

void soc_timer_isr_install(void)
{
	rt_hw_interrupt_install(SOC_IRQ_TIMER1, rt_hw_timer_isr, RT_NULL, "gpt0-tick");
	rt_hw_interrupt_mask(SOC_IRQ_TIMER1);
	rt_hw_interrupt_config(SOC_IRQ_TIMER1,SOC_IRQ_TIMER_LEVEL);
    rt_hw_interrupt_umask(SOC_IRQ_TIMER1);
}