/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include <common.h>
#include <config.h>

#include <asm/arch/typedefs.h>
#include <asm/arch/timer.h>
#include <asm/arch/mt6735.h>

#define GPT4_CON                    ((P_U32)(GPT_BASE+0x0040))
#define GPT4_CLK                    ((P_U32)(GPT_BASE+0x0044))
#define GPT4_DAT                    ((P_U32)(GPT_BASE+0x0048))

#define GPT4_EN                     0x0001
#define GPT4_FREERUN                0x0030
#define GPT4_SYS_CLK                0x0000
#define GPT4_CLK_DIV1               0X0000
#define GPT4_CLK_DIV2               0X0001

#define GPT4_MAX_TICK_CNT   ((U32)0xFFFFFFFF)

#if CFG_FPGA_PLATFORM
// 12MHz setting
#define GPT4_CLK_SETTING    (GPT4_SYS_CLK|GPT4_CLK_DIV1)
#define GPT4_MAX_US_TIMEOUT ((U32)357770775)    // 0xFFFFFFFF * 83.3ns / 1000
#define GPT4_MAX_MS_TIMEOUT ((U32)357770)       // 0xFFFFFFFF * 83.3ns / 1000000
#define GPT4_1US_TICK       ((U32)12)           //    1000 / 83.3ns = 12.004
#define GPT4_1MS_TICK       ((U32)12004)        // 1000000 / 83.3ns = 12004.801
// 12MHz: 1us = 12.004 ticks
#define TIME_TO_TICK_US(us) ((us)*GPT4_1US_TICK + ((us)*4 + (1000-1))/1000)
// 12MHz: 1ms = 12004.801 ticks
#define TIME_TO_TICK_MS(ms) ((ms)*GPT4_1MS_TICK + ((ms)*801 + (1000-1))/1000)
#else
/*
// 6.5MHz setting (13/2) MHz
#define GPT4_CLK_SETTING    (GPT4_SYS_CLK|GPT4_CLK_DIV2)
#define GPT4_MAX_US_TIMEOUT ((U32)660737768)    // 0xFFFFFFFF * 153.84ns / 1000
#define GPT4_MAX_MS_TIMEOUT ((U32)660737)       // 0xFFFFFFFF * 153.84ns / 1000000
#define GPT4_1US_TICK       ((U32)6)            //    1000 / 153.84ns = 6.500
#define GPT4_1MS_TICK       ((U32)6500)         // 1000000 / 153.84ns = 6500.260
// 6.5MHz: 1us = 6.500 ticks
#define TIME_TO_TICK_US(us) ((us)*GPT4_1US_TICK + ((us)*500 + (1000-1))/1000)
// 6.5MHz: 1ms = 6500.260 ticks
#define TIME_TO_TICK_MS(ms) ((ms)*GPT4_1MS_TICK + ((ms)*260 + (1000-1))/1000)
*/

// 13MHz setting
#define GPT4_CLK_SETTING    (GPT4_SYS_CLK)
#define GPT4_MAX_US_TIMEOUT ((U32)330382100)    // 0xFFFFFFFF /13d
#define GPT4_MAX_MS_TIMEOUT ((U32)330382)       // 0xFFFFFFFF /13000d

#define GPT4_1US_TICK       ((U32)13)           //    1000 / 76.92ns = 13.000
#define GPT4_1MS_TICK       ((U32)13000)        // 1000000 / 76.92ns = 13000.520
// 13MHz: 1us = 13.000 ticks
#define TIME_TO_TICK_US(us) ((us)*GPT4_1US_TICK + ((us)*0 + (1000-1))/1000)
// 13MHz: 1ms = 13000.520 ticks
#define TIME_TO_TICK_MS(ms) ((ms)*GPT4_1MS_TICK + ((ms)*520 + (1000-1))/1000)

#endif

#define MS_TO_US            1000
#define CFG_HZ              100
#define MAX_REG_MS          GPT4_MAX_MS_TIMEOUT

#define GPT_SET_BITS(BS,REG)       ((*(volatile U32*)(REG)) |= (U32)(BS))
#define GPT_CLR_BITS(BS,REG)       ((*(volatile U32*)(REG)) &= ~((U32)(BS)))

static volatile U32 timestamp;
static volatile U32 lastinc;


//===========================================================================
// GPT4 fixed 13MHz counter
//===========================================================================
static void gpt_power_on (bool bPowerOn)
{
// XGPT no need to control PDN setting
}

static void gpt4_start (void)
{
    *GPT4_CLK = (GPT4_CLK_SETTING);
    *GPT4_CON = (GPT4_EN|GPT4_FREERUN);
}

static void gpt4_stop (void)
{
    *GPT4_CON = 0x0; // disable
    *GPT4_CON = 0x2; // clear counter
}

static void gpt4_init (bool bStart)
{
    // power on GPT
    gpt_power_on (TRUE);

    // clear GPT4 first
    gpt4_stop ();

    // enable GPT4 without lock
    if (bStart)
    {
        gpt4_start ();
    }
}

U32 gpt4_get_current_tick (void)
{
    return (*GPT4_DAT);
}

bool gpt4_timeout_tick (U32 start_tick, U32 timeout_tick)
{
    register U32 cur_tick;
    register U32 elapse_tick;

    // get current tick
    cur_tick = gpt4_get_current_tick ();

    // check elapse time
    if (start_tick <= cur_tick)
    {
        elapse_tick = cur_tick - start_tick;
    }
    else
    {
        elapse_tick = (GPT4_MAX_TICK_CNT - start_tick) + cur_tick;
    }

    // check if timeout
    if (timeout_tick <= elapse_tick)
    {
        // timeout
        return TRUE;
    }

    return FALSE;
}

//===========================================================================
// us interface
//===========================================================================
U32 gpt4_tick2time_us (U32 tick)
{
    return ((tick + (GPT4_1US_TICK - 1)) / GPT4_1US_TICK);
}

U32 gpt4_time2tick_us (U32 time_us)
{
    if (GPT4_MAX_US_TIMEOUT <= time_us)
    {
        return GPT4_MAX_TICK_CNT;
    }
    else
    {
        return TIME_TO_TICK_US (time_us);
    }
}

//===========================================================================
// ms interface
//===========================================================================
static U32 gpt4_tick2time_ms (U32 tick)
{
    return ((tick + (GPT4_1MS_TICK - 1)) / GPT4_1MS_TICK);
}

static U32 gpt4_time2tick_ms (U32 time_ms)
{
    if (GPT4_MAX_MS_TIMEOUT <= time_ms)
    {
        return GPT4_MAX_TICK_CNT;
    }
    else
    {
        return TIME_TO_TICK_MS (time_ms);
    }
}

//===========================================================================
// busy wait
//===========================================================================
void gpt_busy_wait_us (U32 timeout_us)
{
    U32 start_tick, timeout_tick;

    // get timeout tick
    timeout_tick = gpt4_time2tick_us (timeout_us);
    start_tick = gpt4_get_current_tick ();

    // wait for timeout
    while (!gpt4_timeout_tick (start_tick, timeout_tick));
}

void gpt_busy_wait_ms (U32 timeout_ms)
{
    U32 start_tick, timeout_tick;

    // get timeout tick
    timeout_tick = gpt4_time2tick_ms (timeout_ms);
    start_tick = gpt4_get_current_tick ();

    // wait for timeout
    while (!gpt4_timeout_tick (start_tick, timeout_tick));
}

//======================================================================


void reset_timer_masked (void)
{
    lastinc = gpt4_tick2time_ms (*GPT4_DAT);
    timestamp = 0;
}

ulong get_timer_masked (void)
{
    volatile U32 now;

    now = gpt4_tick2time_ms (*GPT4_DAT);

    if (now >= lastinc)
    {
        timestamp = timestamp + now - lastinc;        /* normal */
    }
    else
    {
        timestamp = timestamp + MAX_REG_MS - lastinc + now;   /* overflow */
    }
    lastinc = now;

    return timestamp;
}

void reset_timer (void)
{
    reset_timer_masked ();
}

#define MAX_TIMESTAMP_MS  0xffffffff

ulong get_timer (ulong base)
{
    ulong current_timestamp = 0;
    ulong temp = 0;

    current_timestamp = get_timer_masked ();

    if (current_timestamp >= base)
    {                         /* timestamp normal */
        return (current_timestamp - base);
    }
    /* timestamp overflow */
    //print("return = 0x%x\n",MAX_TIMESTAMP_MS - ( base - current_timestamp ));
    temp = base - current_timestamp;

    return (MAX_TIMESTAMP_MS - temp);
}

void set_timer (ulong ticks)
{
    timestamp = ticks;
}

/*
 * Iverson 20150522 : 
 *      The function is ported from LK. but multiple definition with <src>\lib\Time.c
 */
#if 0

/* delay msec mseconds */
void mdelay (unsigned long msec)
{
   gpt_busy_wait_ms(msec);
}

/* delay usec useconds */
void udelay (unsigned long usec)
{
   gpt_busy_wait_us(usec);
}
#endif

/*
 * Iverson 20150522 :
 *      This is a very importmant function, uboot use the function for bootmenu countdown counter.
 *      The function is not defined in preloader. We should port it by ourself.
 */
 
/* delay usec useconds */
void __udelay (unsigned long usec)
{
      gpt_busy_wait_us(usec);
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On ARM it just returns the timer value.
 */
unsigned long long get_ticks (void)
{
    return (unsigned long long) get_timer (0);
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk (void)
{
    ulong tbclk;
    tbclk = CFG_HZ;
    return tbclk;
}

void mtk_timer_init (void)
{
    gpt4_init (TRUE);
    // init timer system
    reset_timer ();
}
