#include <linux/types.h>

#include <asm/arch/mt_typedefs.h>
#include <asm/arch/mt_reg_base.h>

typedef volatile unsigned int* P_U32;

#define GPT4_CON                    ((P_U32)(APMCU_GPTIMER_BASE+0x0040))
#define GPT4_CLK                    ((P_U32)(APMCU_GPTIMER_BASE+0x0044))
#define GPT4_DAT                    ((P_U32)(APMCU_GPTIMER_BASE+0x0048))

//Iverson_debug
#define GPT6__CON                   ((P_U32)(APMCU_GPTIMER_BASE+0x0060))
#define GPT6__CLK                   ((P_U32)(APMCU_GPTIMER_BASE+0x0064))

#define GPT4_EN                     0x0001
#define GPT4_FREERUN                0x0030
#define GPT4_SYS_CLK                0x0000



#define GPT4_MAX_TICK_CNT   ((U32)0xFFFFFFFF)

// 13MHz setting
#define GPT4_MAX_US_TIMEOUT ((U32)330382100)    // 0xFFFFFFFF /13d
#define GPT4_MAX_MS_TIMEOUT ((U32)330382)       // 0xFFFFFFFF /13000d

#define GPT4_1US_TICK       ((U32)13)           //    1000 / 76.92ns = 13.000
#define GPT4_1MS_TICK       ((U32)13000)        // 1000000 / 76.92ns = 13000.520
// 13MHz: 1us = 13.000 ticks
#define TIME_TO_TICK_US(us) ((us)*GPT4_1US_TICK + ((us)*0 + (1000-1))/1000)
// 13MHz: 1ms = 13000.520 ticks
#define TIME_TO_TICK_MS(ms) ((ms)*GPT4_1MS_TICK + ((ms)*520 + (1000-1))/1000)


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

/*
static void gpt_power_on (bool bPowerOn)
{
    #define AP_PERI_GLOBALCON_PDN0 (PERI_CON_BASE+0x10)
    if(!bPowerOn){
        GPT_SET_BITS(1<<13, AP_PERI_GLOBALCON_PDN0);
    }else{
    	GPT_CLR_BITS(1<<13, AP_PERI_GLOBALCON_PDN0);
    }
}
*/
static void gpt4_start (void)
{
    *GPT4_CLK = (GPT4_SYS_CLK);
    *GPT4_CON = (GPT4_EN|GPT4_FREERUN);
}

static void gpt6_start(void)
{
	*GPT6__CLK = (GPT4_SYS_CLK);
	*GPT6__CON = (GPT4_FREERUN | GPT4_EN);
}

static void gpt4_stop (void)
{
    *GPT4_CON = 0x0; // disable
    *GPT4_CON = 0x2; // clear counter
}

static void gpt6_stop(void)
{
	*GPT6__CON = 0x0; /* disable */
	*GPT6__CON = 0x2; /* clear counter */
}

static void gpt4_init (bool bStart)
{
    // power on GPT
    //gpt_power_on (TRUE);

    // clear GPT4 first
    gpt4_stop ();

    // enable GPT4 without lock
    if (bStart)
    {
        gpt4_start ();
    }
}

void gpt6_init(bool bStart)
{
	/* power on GPT */
	/* gpt_power_on(TRUE); */

	/* clear GPT6 first */
	gpt6_stop ();

	/* enable GPT6 without lock */
	if (bStart) {
		gpt6_start();
	}
}

U32 gpt4_get_current_tick (void)
{
     U32 cnt1, cnt2, cnt3,value1;

     cnt1 = (*GPT4_DAT);
     cnt2 = (*GPT4_DAT);
     cnt3 = (*GPT4_DAT);
     if(cnt2 < cnt1)
    {
        if(cnt1 < cnt3)
	        value1 = cnt1;
         else
        {
               value1 = ((cnt2 > cnt3) ? cnt2 :cnt3);
        }
     }
      else
     {
         if(cnt2 < cnt3)
             value1 = cnt2;
         else
        {
            value1= ((cnt1 > cnt3) ? cnt1 :cnt3);
        }
     }
     return value1;
    //return (*GPT4_DAT);
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
U32 gpt4_tick2time_ms (U32 tick)
{
    return ((tick + (GPT4_1MS_TICK - 1)) / GPT4_1MS_TICK);
}

static U32 gpt4_time2tick_ms (U32 time_ms)
{
    if (GPT4_MAX_MS_TIMEOUT <= time_ms)
    {
        return GPT4_MAX_MS_TIMEOUT;
    }
    else
    {
        return TIME_TO_TICK_MS (time_ms);
    }
}

//===========================================================================
// bust wait
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
    register U32 cur_tick;
    // get current tick
    cur_tick = gpt4_get_current_tick ();
    lastinc = gpt4_tick2time_ms (cur_tick);
    timestamp = 0;
}

ulong get_timer_masked (void)
{
    volatile U32 now;
    register U32 cur_tick;
    // get current tick
    cur_tick = gpt4_get_current_tick ();
    now = gpt4_tick2time_ms (cur_tick);

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
    //dbg_print("return = 0x%x\n",MAX_TIMESTAMP_MS - ( base - current_timestamp ));
    temp = base - current_timestamp;

    return (MAX_TIMESTAMP_MS - temp);
}

void set_timer (ulong ticks)
{
    timestamp = ticks;
}

/*
 * Iverson 20140326 : 
 *      The function is ported from LK. but multiple definition with <src>\lib\Time.c
 */
#if 0
/* delay msec mseconds */
void mdelay (unsigned long msec)
{
       gpt_busy_wait_ms(msec);
}
#endif

/* delay usec useconds */
void __udelay (unsigned long usec)
{
      gpt_busy_wait_us(usec);
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On ARM it just returns the timer value.
 */
unsigned long long get_ticks(void)
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
    gpt4_init(TRUE);
    gpt6_init(TRUE); 
    // init timer system
    reset_timer ();
}

