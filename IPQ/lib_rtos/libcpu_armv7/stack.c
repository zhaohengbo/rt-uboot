/*
 * File      : stack.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2011, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-09-23     Bernard      the first version
 * 2011-10-05     Bernard      add thumb mode
 */
#include <common.h>
#include <rtthread.h>
#include "armv7.h"

DECLARE_GLOBAL_DATA_PTR;

/**
 * @addtogroup AM33xx
 */
/*@{*/

/**
 * This function will initialize thread stack
 *
 * @param tentry the entry of thread
 * @param parameter the parameter of entry 
 * @param stack_addr the beginning stack address
 * @param texit the function will be called when thread exit
 *
 * @return stack address
 */
rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter,
	rt_uint8_t *stack_addr, void *texit)
{
	rt_uint32_t *stk;

	stk 	 = (rt_uint32_t*)stack_addr;
	*(stk) 	 = (rt_uint32_t)tentry;			/* entry point */
	*(--stk) = (rt_uint32_t)texit;			/* lr */
	*(--stk) = (rt_uint32_t)gd;				/* r12 */
	*(--stk) = (rt_uint32_t)gd;				/* r11 */
	*(--stk) = (rt_uint32_t)gd;				/* r10 */
	*(--stk) = (rt_uint32_t)gd;				/* r9 */
	*(--stk) = (rt_uint32_t)gd;				/* r8 */
	*(--stk) = (rt_uint32_t)gd;				/* r7 */
	*(--stk) = (rt_uint32_t)gd;				/* r6 */
	*(--stk) = (rt_uint32_t)gd;				/* r5 */
	*(--stk) = (rt_uint32_t)gd;				/* r4 */
	*(--stk) = (rt_uint32_t)gd;				/* r3 */
	*(--stk) = (rt_uint32_t)gd;				/* r2 */
	*(--stk) = (rt_uint32_t)gd;				/* r1 */
	*(--stk) = (rt_uint32_t)parameter;		/* r0 : argument */

	/* cpsr */
	if ((rt_uint32_t)tentry & 0x01)
		*(--stk) = SVCMODE | 0x20;			/* thumb mode */
	else
		*(--stk) = SVCMODE;					/* arm mode   */

	/* return task's current stack address */
	return (rt_uint8_t *)stk;
}

/*@}*/
