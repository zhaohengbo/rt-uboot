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

#include <common.h>
#include <rthw.h>
#include <rtthread.h>

#include "glue.h"

ALIGN(RT_ALIGN_SIZE)
static char thread_main_stack[0x20000];
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

ALIGN(RT_ALIGN_SIZE)
static char thread_led_stack[0x400];
struct rt_thread thread_led;
void rt_thread_entry_led(void* parameter)
{
	extern void all_led_on(void);
	extern void all_led_off(void);
    while (1)
    {
		//all_led_on();
		rt_thread_delay(RT_TICK_PER_SECOND);
		//all_led_off();
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


/*@}*/
#ifdef RT_USING_IDLE_HOOK
static void idle_wfi(void)
{
    asm volatile ("wfi");
}
#endif

/**
 * This function will initialize beaglebone board
 */
void rt_hw_board_init(void)
{
	extern void rt_relocation_patch(void);
	extern void stack_setup(void);
	rt_relocation_patch();
	stack_setup();
	
    /* initialzie hardware interrupt */
    rt_hw_interrupt_init();
	
#ifdef RT_USING_HEAP
	rt_system_heap_init((void*)gd->rtos_mem_start, (void*)gd->rtos_mem_end);
#endif

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#else
	extern void rt_hw_mmu_init(void);
	extern int rt_hw_timer_init(void);
	rt_hw_timer_init();
	//rt_hw_mmu_init();
#endif
	
#ifdef RT_USING_CONSOLE
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif
#ifdef RT_USING_IDLE_HOOK
    rt_thread_idle_sethook(idle_wfi);
#endif
}

void rtthread_startup(void)
{
	//return;
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
