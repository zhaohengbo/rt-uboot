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

#include "soc_config.h"

static void rt_hw_timer_isr(int vector, void *param)
{
    rt_tick_increase();
    /* clear interrupt */
    soc_timer_isr_handler();
}

int rt_hw_timer_init(void)
{
    rt_uint32_t val;

    soc_timer_init();

    rt_hw_interrupt_install(SOC_IRQ_TIMER, rt_hw_timer_isr, RT_NULL, "tick");
    rt_hw_interrupt_umask(SOC_IRQ_TIMER);

    return 0;
}
INIT_BOARD_EXPORT(rt_hw_timer_init);