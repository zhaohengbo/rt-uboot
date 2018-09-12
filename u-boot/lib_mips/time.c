/*
 * Copyright (C) 2015 Piotr Dymacz <piotr@dymacz.pl>
 * Copyright (C) 2003 Wolfgang Denk, DENX Software Engineering, <wd@denx.de>
 *
 * SPDX-License-Identifier:GPL-2.0
 */

#include <common.h>
#include <rtthread.h>
#include <rthw.h>

DECLARE_GLOBAL_DATA_PTR;

static inline void mips_compare_set(u32 v)
{
	asm volatile ("mtc0 %0, $11" : : "r" (v));
}

static inline void mips_count_set(u32 v)
{
	asm volatile ("mtc0 %0, $9" : : "r" (v));
}

static inline u32 mips_count_get(void)
{
	u32 count;

	asm volatile ("mfc0 %0, $9" : "=r" (count) :);

	return count;
}

/*
 * Timer without interrupts
 */
int timer_init(void)
{
	mips_compare_set(0);
	mips_count_set(0);

	return 0;
}

ulong get_timer(ulong base)
{
	return mips_count_get() - base;
}

void udelay(unsigned long usec)
{
	ulong tmo;
	ulong m;
	rt_uint32_t level;
	ulong start = get_timer(0);
	bd_t *bd = gd->bd;

	tmo = usec * (CFG_HZ / 1000000);
	if(!gd->rtos_status)
	{
		while ((ulong)((mips_count_get() - start)) < tmo)
			/* NOP */;	
	}
	else
	{
		if(tmo < (CFG_HZ/RT_TICK_PER_SECOND))
		{
			level = rt_hw_interrupt_disable();
			while ((ulong)((mips_count_get() - start)) < tmo)
				/* NOP */;	
			rt_hw_interrupt_enable(level);
		}
		else
		{
			m = usec/1000 + 1;
			rt_thread_delay(RT_TICK_PER_SECOND/1000 * m);
		}
	}
	
}
