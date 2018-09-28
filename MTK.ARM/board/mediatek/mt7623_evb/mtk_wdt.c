#include <common.h>
#include <config.h>

#include <asm/arch/mt_reg_base.h>
#include <asm/arch/mt_typedefs.h>
#include <asm/arch/mtk_wdt.h>

/*
 * Iverson 20140327
 *      We don't need the header file unless I need proloader arguments.
 */
 
#if 0
#include <asm/arch/boot_mode.h>
#endif
#include <asm/arch/mt_gpt.h>

#if ENABLE_WDT_MODULE

static unsigned int timeout;
static unsigned int is_rgu_trigger_rst = 0;

void mtk_wdt_disable(void)
{
    u32 tmp;

    tmp = DRV_Reg32(MTK_WDT_MODE);
    tmp &= ~MTK_WDT_MODE_ENABLE;       /* disable watchdog */
    tmp |= (MTK_WDT_MODE_KEY);         /* need key then write is allowed */
    DRV_WriteReg32(MTK_WDT_MODE,tmp);
}

static void mtk_wdt_reset(char mode)
{
    /* Watchdog Rest */
    unsigned int wdt_mode_val;
    DRV_WriteReg32(MTK_WDT_RESTART, MTK_WDT_RESTART_KEY); 

    wdt_mode_val = DRV_Reg32(MTK_WDT_MODE);
    /* clear autorestart bit: autoretart: 1, bypass power key, 0: not bypass power key */
    wdt_mode_val &=(~MTK_WDT_MODE_AUTO_RESTART);
	/* make sure WDT mode is hw reboot mode, can not config isr mode  */
	wdt_mode_val &=(~(MTK_WDT_MODE_IRQ|MTK_WDT_MODE_ENABLE | MTK_WDT_MODE_DUAL_MODE));
	
    if(mode){ /* mode != 0 means by pass power key reboot, We using auto_restart bit as by pass power key flag */
        wdt_mode_val = wdt_mode_val | (MTK_WDT_MODE_KEY|MTK_WDT_MODE_EXTEN|MTK_WDT_MODE_AUTO_RESTART);
        DRV_WriteReg32(MTK_WDT_MODE, wdt_mode_val);
       
    }else{
         wdt_mode_val = wdt_mode_val | (MTK_WDT_MODE_KEY|MTK_WDT_MODE_EXTEN);
         DRV_WriteReg32(MTK_WDT_MODE,wdt_mode_val); 
        
    }
    
    gpt_busy_wait_us(100);
    DRV_WriteReg32(MTK_WDT_SWRST, MTK_WDT_SWRST_KEY);
}



unsigned int mtk_wdt_check_status(void)
{
    unsigned int status;

    status = DRV_Reg32(MTK_WDT_STATUS);

    return status;
}

static void mtk_wdt_mode_config(	BOOL dual_mode_en, 
					BOOL irq, 
					BOOL ext_en, 
					BOOL ext_pol, 
					BOOL wdt_en )
{
	unsigned int tmp;
    
	printf(" mtk_wdt_mode LK config  mode value=%x\n",DRV_Reg32(MTK_WDT_MODE));
	tmp = DRV_Reg32(MTK_WDT_MODE);
	tmp |= MTK_WDT_MODE_KEY;

	// Bit 0 : Whether enable watchdog or not
	if(wdt_en == TRUE)
		tmp |= MTK_WDT_MODE_ENABLE;
	else
		tmp &= ~MTK_WDT_MODE_ENABLE;

	// Bit 1 : Configure extern reset signal polarity.
	if(ext_pol == TRUE)
		tmp |= MTK_WDT_MODE_EXT_POL;
	else
		tmp &= ~MTK_WDT_MODE_EXT_POL;

	// Bit 2 : Whether enable external reset signal
	if(ext_en == TRUE)
		tmp |= MTK_WDT_MODE_EXTEN;
	else
		tmp &= ~MTK_WDT_MODE_EXTEN;

	// Bit 3 : Whether generating interrupt instead of reset signal
	if(irq == TRUE)
		tmp |= MTK_WDT_MODE_IRQ;
	else
		tmp &= ~MTK_WDT_MODE_IRQ;

	// Bit 6 : Whether enable debug module reset
	if(dual_mode_en == TRUE)
		tmp |= MTK_WDT_MODE_DUAL_MODE;
	else
		tmp &= ~MTK_WDT_MODE_DUAL_MODE;

	// Bit 4: WDT_Auto_restart, this is a reserved bit, we use it as bypass powerkey flag.
	//		Because HW reboot always need reboot to kernel, we set it always.
	tmp |= MTK_WDT_MODE_AUTO_RESTART;

	DRV_WriteReg32(MTK_WDT_MODE,tmp);
	//dual_mode(1); //always dual mode
	//mdelay(100);
	printf(" mtk_wdt_mode_config LK  mode value=%x, tmp:%x\n",DRV_Reg32(MTK_WDT_MODE), tmp);

}


#if 0
static void mtk_wdt_mode_config(BOOL debug_en, 
                         BOOL irq, 
                         BOOL ext_en, 
                         BOOL ext_pol, 
                         BOOL wdt_en )
{
    unsigned short tmp;

    tmp = DRV_Reg32(MTK_WDT_MODE);
    tmp |= MTK_WDT_MODE_KEY;

    // Bit 0 : Whether enable watchdog or not
    if(wdt_en == TRUE)
        tmp |= MTK_WDT_MODE_ENABLE;
    else
        tmp &= ~MTK_WDT_MODE_ENABLE;

    // Bit 1 : Configure extern reset signal polarity.
    if(ext_pol == TRUE)
        tmp |= MTK_WDT_MODE_EXT_POL;
    else
        tmp &= ~MTK_WDT_MODE_EXT_POL;

    // Bit 2 : Whether enable external reset signal
    if(ext_en == TRUE)
        tmp |= MTK_WDT_MODE_EXTEN;
    else
        tmp &= ~MTK_WDT_MODE_EXTEN;

    // Bit 3 : Whether generating interrupt instead of reset signal
    if(irq == TRUE)
        tmp |= MTK_WDT_MODE_IRQ;
    else
        tmp &= ~MTK_WDT_MODE_IRQ;

    // Bit 6 : Whether enable debug module reset
    if(debug_en == TRUE)
        tmp |= MTK_WDT_MODE_DEBUG_EN;
    else
        tmp &= ~MTK_WDT_MODE_DEBUG_EN;

    DRV_WriteReg32(MTK_WDT_MODE,tmp);
}
#endif
void mtk_wdt_set_time_out_value(UINT32 value)
{
    /*
    * TimeOut = BitField 15:5
    * Key      = BitField  4:0 = 0x08
    */

    // sec * 32768 / 512 = sec * 64 = sec * 1 << 6
    timeout = (unsigned int)(value * ( 1 << 6) );
    timeout = timeout << 5; 
    DRV_WriteReg32(MTK_WDT_LENGTH, (timeout | MTK_WDT_LENGTH_KEY) );  
}

void mtk_wdt_restart(void)
{
    // Reset WatchDogTimer's counting value to time out value
    // ie., keepalive()

    DRV_WriteReg32(MTK_WDT_RESTART, MTK_WDT_RESTART_KEY);
}

void mtk_wdt_sw_reset(void)
{
    printf ("UB WDT SW RESET\n");
    //DRV_WriteReg32 (0x70025000, 0x2201);
    //DRV_WriteReg32 (0x70025008, 0x1971);
    //DRV_WriteReg32 (0x7002501C, 0x1209);
    mtk_wdt_reset(1);/* NOTE here, this reset will cause by pass power key */

    // system will reset 

    while (1)
    {
        printf ("UB SW reset fail ... \n");
    };
}

void mtk_wdt_hw_reset(void)
{
    printf("UB WDT_HW_Reset\n");

    // 1. set WDT timeout 1 secs, 1*64*512/32768 = 1sec
    mtk_wdt_set_time_out_value(1);

    // 2. enable WDT debug reset enable, generating irq disable, ext reset disable
    //    ext reset signal low, wdt enalbe
    mtk_wdt_mode_config(TRUE, FALSE, FALSE, FALSE, TRUE);

    // 3. reset the watch dog timer to the value set in WDT_LENGTH register
    mtk_wdt_restart();

    // 4. system will reset
    while(1);
} 

/**
 * For Power off and power on reset, the INTERVAL default value is 0x7FF.
 * We set Interval[1:0] to different value to distinguish different stage.
 * Enter pre-loader, we will set it to 0x0
 * Enter u-boot, we will set it to 0x1
 * Enter kernel, we will set it to 0x2
 * And the default value is 0x3 which means reset from a power off and power on reset
 */
#define POWER_OFF_ON_MAGIC	(0x3)
#define PRE_LOADER_MAGIC	(0x0)
#define U_BOOT_MAGIC		(0x1)
#define KERNEL_MAGIC		(0x2)
#define MAGIC_NUM_MASK		(0x3)
/**
 * If the reset is trigger by RGU(Time out or SW trigger), we hope the system can boot up directly;
 * we DO NOT hope we must press power key to reboot system after reset.
 * This message should tell pre-loader and u-boot, and we use Interval[2] to store this information.
 * And this information will be cleared when enter kernel.
 */
#define IS_POWER_ON_RESET	(0x1<<2)
#define RGU_TRIGGER_RESET_MASK	(0x1<<2)
void mtk_wdt_init(void)
{
    unsigned int interval_val = DRV_Reg32(MTK_WDT_INTERVAL);
    mtk_wdt_mode_config(FALSE, FALSE, FALSE, FALSE, FALSE);
    printf("UB wdt init\n");

    /* Update interval register value and check reboot flag */
    if( (interval_val & RGU_TRIGGER_RESET_MASK) == IS_POWER_ON_RESET )
        is_rgu_trigger_rst = 0; // Power off and power on reset
    else
        is_rgu_trigger_rst = 1; // RGU trigger reset

    interval_val &= ~(RGU_TRIGGER_RESET_MASK|MAGIC_NUM_MASK);
    interval_val |= (U_BOOT_MAGIC|IS_POWER_ON_RESET);

    /* Write back INTERVAL REG */
    DRV_WriteReg32(MTK_WDT_INTERVAL, interval_val);
	/* Setting timeout 10s */
	mtk_wdt_set_time_out_value(10);
	#if (!LK_WDT_DISABLE)

    /*
     * Iverson 20140327
     *      Only enable wdt enable mode in uboot.
     */
    
//	mtk_wdt_mode_config(TRUE, TRUE, TRUE, FALSE, TRUE);
    mtk_wdt_mode_config(TRUE, TRUE, TRUE, FALSE, TRUE);

	mtk_wdt_restart();
	#endif
    #if 0
	int i=0;
    //lk wdt test
	while(1)
	{
	  
	  i++;
	  mdelay(1000);
	  printf("UB wdt test %d\n" ,i);
      /* Dump RGU regisers */
    printf("==== fwq Dump RGU Reg ========\n");
    printf("RGU MODE:     %x\n", DRV_Reg32(MTK_WDT_MODE));
    printf("RGU LENGTH:   %x\n", DRV_Reg32(MTK_WDT_LENGTH));
    printf("RGU STA:      %x\n", DRV_Reg32(MTK_WDT_STATUS));
    printf("RGU INTERVAL: %x\n", DRV_Reg32(MTK_WDT_INTERVAL));
    printf("RGU SWSYSRST: %x\n", DRV_Reg32(MTK_WDT_SWSYSRST));
    printf("==== Dump RGU Reg End ====\n");

	if(i<60)
	{
	  printf("kick lk ext\n" );
	  mtk_wdt_restart();
	}
    }
	#endif
}

BOOL mtk_is_rgu_trigger_reset(void)
{
    if(is_rgu_trigger_rst)
        return TRUE;
    return FALSE;
}

/*
 * Iverson 20140327
 *      We don't need the function unless I need proloader arguments.
 */

#if 0
extern BOOT_ARGUMENT *g_boot_arg;
int mtk_wdt_boot_check(void)
{    
    int boot_reason;

    if (g_boot_arg->maggic_number == BOOT_ARGUMENT_MAGIC) {
    	boot_reason = g_boot_arg->boot_reason;
    	printf("WDT get boot reason is %d from pre-loader\n", boot_reason);

        if (boot_reason == BR_WDT) {
            return WDT_NORMAL_REBOOT;
        } else if (boot_reason == BR_WDT_BY_PASS_PWK) {
            return WDT_BY_PASS_PWK_REBOOT;
        } else {
            return WDT_NOT_WDT_REBOOT;
        }
    }

    return WDT_NOT_WDT_REBOOT;
}
#endif

void mtk_arch_reset(char mode)
{
    printf("UB mtk_arch_reset\n");

    mtk_wdt_reset(mode);

    while (1);
}
void rgu_swsys_reset(WD_SYS_RST_TYPE reset_type)
{
    if(WD_MD_RST == reset_type)
    {
	   unsigned int wdt_dbg_ctrl;
	   wdt_dbg_ctrl = DRV_Reg32(MTK_WDT_SWSYSRST);
	   wdt_dbg_ctrl |= MTK_WDT_SWSYS_RST_KEY;
	   wdt_dbg_ctrl |= 0x80;// 1<<7
	   DRV_WriteReg32(wdt_dbg_ctrl, MTK_WDT_SWSYSRST); 
	   udelay(1000);
	   wdt_dbg_ctrl = DRV_Reg32(MTK_WDT_SWSYSRST);
	   wdt_dbg_ctrl |= MTK_WDT_SWSYS_RST_KEY;
	   wdt_dbg_ctrl &= (~0x80);// ~(1<<7)
	   DRV_WriteReg32(wdt_dbg_ctrl, MTK_WDT_SWSYSRST); 
       printf("rgu pl md reset\n");
    }
}

#else
void mtk_wdt_init(void)
{
    printf("UB WDT Dummy init called\n");
}
BOOL mtk_is_rgu_trigger_reset()
{
    printf("UB Dummy mtk_is_rgu_trigger_reset called\n");
    return FALSE;
}
void mtk_arch_reset(char mode)
{
    printf("UB WDT Dummy arch reset called\n");
}

int mtk_wdt_boot_check(void)
{
    printf("UB WDT Dummy mtk_wdt_boot_check called\n");
    return WDT_NOT_WDT_REBOOT;
}

void mtk_wdt_disable(void)
{
   printf("UB WDT Dummy mtk_wdt_disable called\n");
}

void mtk_wdt_restart(void)
{
   printf("UB WDT Dummy mtk_wdt_restart called\n");
}
static void mtk_wdt_sw_reset(void)
{
  printf("UB WDT Dummy mtk_wdt_sw_reset called\n");
}

static void mtk_wdt_hw_reset(void)
{
  printf("UB WDT Dummy mtk_wdt_hw_reset called\n");
}
void rgu_swsys_reset(WD_SYS_RST_TYPE reset_type)
{
  printf("UB WDT Dummy rgu_swsys_reset called\n");
}



#endif
