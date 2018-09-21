/*
 * File      : board.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2012, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-11-20     Bernard    the first version
 */

#include <rthw.h>
#include <rtthread.h>

#include "board.h"

void idle_wfi(void)
{
    asm volatile ("wfi");
}

/**
 * This function will initialize beaglebone board
 */
void rt_hw_board_init(void)
{
    /* initialzie hardware interrupt */
    rt_hw_interrupt_init();
    rt_system_heap_init(HEAP_BEGIN, HEAP_END);

    rt_components_board_init();
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);

    rt_thread_idle_sethook(idle_wfi);
}

ALIGN(RT_ALIGN_SIZE)
static char thread_main_stack[0x40000];
struct rt_thread thread_main;
void rt_thread_entry_main(void* parameter)
{
	extern void main_loop(void);
    while (1)
    {
		for (;;)
			main_loop();
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
	
    rt_thread_startup(&thread_main);
    return 0;
}

void rtthread_startup(void)
{
	rt_hw_interrupt_disable();
	
    /* init board */
    rt_hw_board_init();

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