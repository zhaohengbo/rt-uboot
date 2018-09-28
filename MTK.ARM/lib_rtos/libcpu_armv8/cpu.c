/*
 * File      : cpu.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-09-15     Bernard      first version
 */

#include <rthw.h>
#include <rtthread.h>

/**
 * @addtogroup ARM CPU
 */
/*@{*/

#define DISABLE_INTERRUPTS()									\
	__asm volatile ( "MSR DAIFSET, #2" ::: "memory" );				\
	__asm volatile ( "DSB SY" );									\
	__asm volatile ( "ISB SY" );

#define ENABLE_INTERRUPTS()										\
	__asm volatile ( "MSR DAIFCLR, #2" ::: "memory" );				\
	__asm volatile ( "DSB SY" );									\
	__asm volatile ( "ISB SY" );

/** shutdown CPU */
void rt_hw_cpu_shutdown()
{
	rt_uint32_t level;
	rt_kprintf("shutdown...\n");

	level = rt_hw_interrupt_disable();
	while (1)
	{
		RT_ASSERT(0);
	}
}

volatile rt_hw_interrupt_lock_count;

rt_base_t rt_hw_interrupt_disable()
{
	DISABLE_INTERRUPTS();
	rt_hw_interrupt_lock_count ++;
}


void rt_hw_interrupt_enable(rt_base_t level)
{
	rt_hw_interrupt_lock_count --;
	if(!rt_hw_interrupt_lock_count)
		ENABLE_INTERRUPTS();
}

/*@}*/
