/*
 * File      : interrupt.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2013-2014, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2013-07-06     Bernard      first version
 */

#include <rthw.h>
#include <rtthread.h>

extern int _vector_undef;
extern void vector_undef(void);
extern int _vector_swi;
extern void vector_swi(void);
extern int _vector_pabt;
extern void vector_pabt(void);
extern int _vector_dabt;
extern void vector_dabt(void);
extern int _vector_resv;
extern void vector_resv(void);
extern int _vector_irq;
extern void vector_irq(void);
extern int _vector_fiq;
extern void vector_fiq(void);

extern int _rt_interrupt_from_thread;
extern int _rt_interrupt_to_thread;
extern int _rt_thread_switch_interrupt_flag;

extern rt_uint32_t rt_interrupt_from_thread;
extern rt_uint32_t rt_interrupt_to_thread;
extern rt_uint32_t rt_thread_switch_interrupt_flag;

extern int _stack_top;
extern int stack_top;

void rt_relocation_patch(void)
{
	_vector_undef = (rt_uint32_t)&vector_undef;
	_vector_swi = (rt_uint32_t)&vector_swi;
	_vector_pabt = (rt_uint32_t)&vector_pabt;
	_vector_dabt = (rt_uint32_t)&vector_dabt;
	_vector_resv = (rt_uint32_t)&vector_resv;
	_vector_irq = (rt_uint32_t)&vector_irq;
	_vector_fiq = (rt_uint32_t)&vector_fiq;
	
	_rt_interrupt_from_thread = (rt_uint32_t)&rt_interrupt_from_thread;
	_rt_interrupt_to_thread = (rt_uint32_t)&rt_interrupt_to_thread;
	_rt_thread_switch_interrupt_flag = (rt_uint32_t)&rt_thread_switch_interrupt_flag;
	
	_stack_top = (rt_uint32_t)&stack_top;
}
