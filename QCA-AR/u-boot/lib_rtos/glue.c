/*
 * File      : board.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006-2012, RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2010-06-25     Bernard      first version
 * 2011-08-08     lgnq            modified for Loongson LS1B
 * 2015-07-06     chinesebear  modified for Loongson LS1C
 */

#include <rtthread.h>
#include <rthw.h>
#include <rtos/rtos_data.h>
#include <common.h>

#include "glue.h"
#include "./libcpu_mips32/mipsregs.h"

/**
 * @addtogroup Loongson LS1B
 */
 
extern void rt_hw_vector_init(rd_t *rd);
static rt_uint32_t boot_c0status;

/*@{*/

/**
 * This is the timer interrupt service routine.
 */
void rt_hw_timer_handler(void)
{
	unsigned int count;

	count = read_c0_compare();
	write_c0_compare(count);
	write_c0_count(0);

	/* increase a OS tick */
	rt_tick_increase();
}

/**
 * This function will initial OS timer
 */
void rt_hw_timer_init(rd_t *rd)
{
	//write_c0_compare(CPU_HZ/2/RT_TICK_PER_SECOND);
	write_c0_compare(rd->cpu_hz/RT_TICK_PER_SECOND);
	write_c0_count(0);
}

void rt_thread_shutdown_os(void)
{
	write_c0_status(boot_c0status);
	write_c0_compare(0);
	write_c0_count(0);
}

/**
 * This function will initial sam7s64 board.
 */
void rt_hw_board_init(rd_t *rd)
{
	/* init vector */
	rt_hw_vector_init(rd);
	
	/* init cache */
	//rt_hw_cache_init();
	
	/* init hardware interrupt */
	rt_hw_interrupt_init();

	boot_c0status = read_c0_status();
	
	/* clear bev */
	write_c0_status(read_c0_status()&(~(1<<22)));
	
	write_c0_status(read_c0_status()&0xFFFFFFE1);
	
	write_c0_cause(read_c0_cause() & 0xFFFFFF83);

	//invalidate_writeback_dcache_all();
	//invalidate_icache_all();
	
#ifdef RT_USING_HEAP
	rt_system_heap_init((void*)rd->rtos_mem_start, (void*)rd->rtos_mem_end);
#endif

#ifdef RT_USING_SERIAL
	/* init hardware UART device */
	rt_hw_uart_init();
#endif

#ifdef RT_USING_CONSOLE
	/* set console device */
	rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif
    /* init operating system timer */	
    rt_hw_timer_init(rd);

#ifdef RT_USING_FPU
    /* init hardware fpu */
    rt_hw_fpu_init();
#endif

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

    rt_kprintf("current sr: 0x%08x\n", read_c0_status());
}

ALIGN(RT_ALIGN_SIZE)
static char thread_main_stack[0x40000];
struct rt_thread thread_main;
void rt_thread_entry_main(void* parameter)
{
	extern void main_loop(void);
	extern void all_led_on(void);
	extern void all_led_off(void);
	extern void rt_thread_idle_excute(void);
    while (1)
    {
		for (;;)
			main_loop();
    }
}

ALIGN(RT_ALIGN_SIZE)
static char thread_led_stack[0x400];
struct rt_thread thread_led;
void rt_thread_entry_led(void* parameter)
{
	extern void all_led_on(void);
	extern void all_led_off(void);
    while (1)
    {
		all_led_on();
		rt_thread_delay(RT_TICK_PER_SECOND);
		all_led_off();
		rt_thread_delay(RT_TICK_PER_SECOND);
    }
}

int rt_application_init(void)
{
	rt_thread_init(&thread_main,
                   "thread_main",
                   rt_thread_entry_main,
                   RT_NULL,
                   &thread_main_stack[0],
                   sizeof(thread_main_stack),12,5);
	
	rt_thread_init(&thread_led,
                   "thread_led",
                   rt_thread_entry_led,
                   RT_NULL,
                   &thread_led_stack[0],
                   sizeof(thread_led_stack),10,4);
	
    rt_thread_startup(&thread_main);
	rt_thread_startup(&thread_led);
    return 0;
}

void rtthread_startup(rd_t *rd)
{
	rt_hw_interrupt_disable();
	
    /* init board */
    rt_hw_board_init(rd);

    /* show version */
    rt_show_version();

    /* init tick */
    rt_system_tick_init();

    /* init kernel object */
    rt_system_object_init();

    /* init timer system */
    rt_system_timer_init();

    /* init scheduler system */
    rt_system_scheduler_init();

    /* init application */
    rt_application_init();

    /* init timer thread */
    rt_system_timer_thread_init();

#ifdef RT_USING_FINSH
    /* init finsh */
    finsh_system_init();
    finsh_set_device("uart");
#endif

    /* init idle thread */
    rt_thread_idle_init();

    /* start scheduler */
    rt_system_scheduler_start();

    /* never reach here */
    while(1)
		;
}

/*@}*/
