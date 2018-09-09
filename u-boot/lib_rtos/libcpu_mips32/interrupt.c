/*
 * File      : interrupt.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2011, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date                  Author       Notes
 * 2010-10-15     Bernard      first version
 * 2010-10-15     lgnq           modified for LS1B
 * 2013-03-29     aozima       Modify the interrupt interface implementations.
 * 2015-07-06     chinesebear modified for loongson 1c
 */

#include <rtthread.h>
#include <rthw.h>

#include "mipsregs.h"

extern rt_uint32_t rt_interrupt_nest;
rt_uint32_t rt_system_stack;
rt_uint32_t rt_interrupt_from_thread;
rt_uint32_t rt_interrupt_to_thread;
rt_uint32_t rt_thread_switch_interrupt_flag;

static struct rt_irq_desc irq_handle_table[8];
/*static*/ rt_uint32_t rt_hw_system_stack[0x1000];
void rt_interrupt_dispatch(rt_uint32_t sp);
void rt_hw_timer_handler(int vector, void *param);

extern void debug_on_serial(char * text,unsigned long pc_pointer);

/**
 * @addtogroup Loongson LS1B
 */

/*@{*/

static void rt_hw_syscall_handler(int vector, void *param)
{
    rt_kprintf("Unhandled syscall`	1 %d occured!!!\n", vector);
}

static void rt_hw_interrupt_handler(int vector, void *param)
{
    rt_kprintf("Unhandled interrupt %d occured!!!\n", vector);
}

/**
 * This function will initialize hardware interrupt
 */
void rt_hw_interrupt_init(void)
{
    rt_int32_t idx;
    rt_uint32_t c0_status = 0;

    // 设置协处理器0的状态寄存器SR的IM7-2，允许中断
    c0_status = read_c0_status();
    c0_status |= 0xFF00;
    write_c0_status(c0_status);

    rt_memset(irq_handle_table, 0x00, sizeof(irq_handle_table));
	
	for (idx = 0; idx < 2; idx ++)
    {
        irq_handle_table[idx].handler = rt_hw_syscall_handler;
    }
	
    for (idx = 2; idx < 7; idx ++)
    {
        irq_handle_table[idx].handler = rt_hw_interrupt_handler;
    }
	
	for (idx = 7; idx < 8; idx ++)
    {
        irq_handle_table[idx].handler = rt_hw_timer_handler;
    }

    /* init interrupt nest, and context in thread sp */
    rt_interrupt_nest = 0;
    rt_interrupt_from_thread = 0;
    rt_interrupt_to_thread = 0;
    rt_thread_switch_interrupt_flag = 0;
	rt_system_stack = (rt_uint32_t)&(rt_hw_system_stack) + sizeof(rt_hw_system_stack) - 4;
}

/**
 * This function will mask a interrupt.
 * @param vector the interrupt number
 */
void rt_hw_interrupt_mask(int vector)
{
    /* mask interrupt */
    volatile rt_uint32_t c0_status;
	
	if((vector < 0) || (vector > 7))
		return;
	c0_status = read_c0_status();
	c0_status |= 1<<(vector + 7);
	write_c0_status(c0_status);
}

/**
 * This function will un-mask a interrupt.
 * @param vector the interrupt number
 */
void rt_hw_interrupt_umask(int vector)
{
	volatile rt_uint32_t c0_status;
	rt_uint32_t temp_mask;
	
	if((vector < 0) || (vector > 7))
		return;
	c0_status = read_c0_status();
	temp_mask = c0_status & 0xFF00;
	temp_mask |= 1<<(vector + 7); 
	c0_status &= 0xFFFF00FF;
	c0_status |= temp_mask;
	write_c0_status(c0_status);
}

/**
 * This function will install a interrupt service routine to a interrupt.
 * @param vector the interrupt number
 * @param new_handler the interrupt service routine to be installed
 * @param old_handler the old interrupt service routine
 */
rt_isr_handler_t rt_hw_interrupt_install(int vector, rt_isr_handler_t handler,
                                         void *param, char *name)
{
    rt_isr_handler_t old_handler = RT_NULL;

    if (vector >= 0 && vector < 8)
    {
        old_handler = irq_handle_table[vector].handler;

#ifdef RT_USING_INTERRUPT_INFO
        rt_strncpy(irq_handle_table[vector].name, name, RT_NAME_MAX);
#endif /* RT_USING_INTERRUPT_INFO */
        irq_handle_table[vector].handler = handler;
        irq_handle_table[vector].param = param;
    }

    return old_handler;
}


/**
 * 执行中断处理函数
 * @IRQn 中断号
 */
void mips_do_IRQ(int IRQn)
{
    rt_isr_handler_t irq_func;
    void *param;

    // 找到中断处理函数
    irq_func = irq_handle_table[IRQn].handler;
    param    = irq_handle_table[IRQn].param;

    // 执行中断处理函数
    irq_func(IRQn, param);
    
#ifdef RT_USING_INTERRUPT_INFO
    irq_handle_table[IRQn].counter++;
#endif

    return ;
}

void mips_do_excpetion(int IRQn)
{
	//debug_on_serial("mips_do_excpetion",(rt_int32_t)&mips_do_excpetion);
	
	//debug_on_serial("c0_status",(rt_int32_t)c0_status);
	
	//debug_on_serial("c0_cause",(rt_int32_t)c0_cause);
		
	//debug_on_serial("c0_epc",(rt_int32_t)read_c0_epc());
	
    rt_kprintf("We can't handle excpetion now , so system halt!!!\n");
	
	while(1);
}

void rt_interrupt_dispatch(rt_uint32_t sp)
{
    volatile rt_uint32_t c0_status;
    volatile rt_uint32_t c0_cause;
    volatile rt_uint32_t cause_im;
    volatile rt_uint32_t status_im;
    volatile rt_uint32_t pending_im;
	volatile rt_uint32_t c0_ecptcode;

    /* check os timer */
    c0_status = read_c0_status();
    c0_cause = read_c0_cause();
	c0_ecptcode = (c0_cause&0x0000007C) >> 2;

    cause_im = c0_cause & ST0_IM;
    status_im = c0_status & ST0_IM;
    pending_im = cause_im & status_im;
	
	if(c0_ecptcode)
	{
		mips_do_excpetion(c0_ecptcode);
	}
	else
	{
		if (pending_im & CAUSEF_IP0)
    	{
        	mips_do_IRQ(0);
    	}
		
		if (pending_im & CAUSEF_IP1)
    	{
        	mips_do_IRQ(1);
    	}
		
		if (pending_im & CAUSEF_IP2)
    	{
        	mips_do_IRQ(2);
    	}
		
		if (pending_im & CAUSEF_IP3)
    	{
        	mips_do_IRQ(3);
    	}
		
		if (pending_im & CAUSEF_IP4)
    	{
        	mips_do_IRQ(4);
    	}
		
		if (pending_im & CAUSEF_IP5)
    	{
        	mips_do_IRQ(5);
    	}
		
		if (pending_im & CAUSEF_IP6)
    	{
        	mips_do_IRQ(6);
    	}
		
		if (pending_im & CAUSEF_IP7)
    	{
        	mips_do_IRQ(7);
    	}
	}
}

/*@}*/


