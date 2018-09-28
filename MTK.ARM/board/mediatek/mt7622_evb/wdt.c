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
#include <asm/arch/mt6735.h>
#include <asm/arch/wdt.h>
#include <asm/arch/timer.h>

#define CONFIG_KICK_SPM_WDT

unsigned int g_rgu_status = RE_BOOT_REASON_UNKNOW;

//#ifdef CONFIG_HW_WATCHDOG
//#ifdef CFG_HW_WATCHDOG
#if CFG_HW_WATCHDOG
static unsigned int timeout;
static unsigned int reboot_from = RE_BOOT_FROM_UNKNOW; 
static unsigned int rgu_mode = 0;

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
#if CFG_USB_AUTO_DETECT
    wdt_mode_val &=(~MTK_WDT_MODE_AUTO_RESTART);
#endif
	/* make sure WDT mode is hw reboot mode, can not config isr mode  */
	//wdt_mode_val &=(~(MTK_WDT_MODE_IRQ|MTK_WDT_MODE_ENABLE | MTK_WDT_MODE_DUAL_MODE));
	
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


static unsigned int mtk_wdt_check_status(void)
{
    static unsigned int status=0;

    /**
     * Note:
     *   Because WDT_STA register will be cleared after writing WDT_MODE,
     *   we use a static variable to store WDT_STA.
     *   After reset, static varialbe will always be clear to 0,
     *   so only read WDT_STA when static variable is 0 is OK
     */
    if(0 == status)
        status = DRV_Reg32(MTK_WDT_STATUS);

    return status;
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
 * And this information will be cleared after uboot check it.
 */
#define IS_POWER_ON_RESET	(0x1<<2)
#define RGU_TRIGGER_RESET_MASK	(0x1<<2)

int mtk_wdt_boot_check(void);

static void mtk_wdt_check_trig_reboot_reason(void)
{
    unsigned int interval_val = DRV_Reg32(MTK_WDT_INTERVAL);

    reboot_from = RE_BOOT_FROM_UNKNOW;

    /* 1. Get reboot reason */
    if(0 != mtk_wdt_check_status()){
        /* Enter here means this reset is triggered by RGU(WDT) */
        printf("PL RGU RST: ");
        switch(interval_val&MAGIC_NUM_MASK)
        {
        case PRE_LOADER_MAGIC:
            reboot_from = RE_BOOT_FROM_PRE_LOADER;
            printf("P\n");
            break;
        case U_BOOT_MAGIC:
            reboot_from = RE_BOOT_FROM_U_BOOT;
            printf("U\n");
            break;
        case KERNEL_MAGIC:
            reboot_from = RE_BOOT_FROM_KERNEL;
            printf("K\n");
            break;
        default:
            printf("??\n"); // RGU reset, but not pr-loader, u-boot, kernel, from where???
            break;
        }
    }else{
        /* Enter here means reset may triggered by power off power on */
        if( (interval_val&MAGIC_NUM_MASK) == POWER_OFF_ON_MAGIC ){
            reboot_from = RE_BOOT_FROM_POWER_ON;
            printf("PL P ON\n");
        }else{
            printf("PL ?!\n"); // Not RGU trigger reset, and not defautl value, why?
        }
    }

    /* 2. Update interval register value and set reboot flag for u-boot */
    interval_val &= ~(RGU_TRIGGER_RESET_MASK|MAGIC_NUM_MASK);
    interval_val |= PRE_LOADER_MAGIC;
    if(reboot_from == RE_BOOT_FROM_POWER_ON)
    	interval_val |= IS_POWER_ON_RESET; // bit2==0 means RGU reset
    DRV_WriteReg32(MTK_WDT_INTERVAL, interval_val);

    /* 3. By pass power key info */
    if (mtk_wdt_boot_check() == WDT_BY_PASS_PWK_REBOOT) {
        printf("Find bypass powerkey flag\n");
    } else if (mtk_wdt_boot_check() == WDT_NORMAL_REBOOT) {
        printf("No bypass powerkey flag\n");
    } else {
        printf("WDT does not trigger reboot\n");
    }
}


static void mtk_wdt_mode_config(	BOOL dual_mode_en, 
					BOOL irq, 
					BOOL ext_en, 
					BOOL ext_pol, 
					BOOL wdt_en )
{
	unsigned int tmp;
    
	//printf(" mtk_wdt_mode_config  mode value=%x\n",DRV_Reg32(MTK_WDT_MODE));
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
	printf(" mtk_wdt_mode_config  mode value=%x, tmp:%x\n",DRV_Reg32(MTK_WDT_MODE), tmp);

}
//EXPORT_SYMBOL(mtk_wdt_mode_config);

static void mtk_wdt_set_time_out_value(UINT32 value)
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

void mtk_wdt_sw_reset (void)
{
    printf ("WDT SW RESET\n");
    //DRV_WriteReg32 (0x70025000, 0x2201);
    //DRV_WriteReg32 (0x70025008, 0x1971);
    //DRV_WriteReg32 (0x7002501C, 0x1209);
    mtk_wdt_reset(1);/* NOTE here, this reset will cause by pass power key */

    // system will reset 

    while (1)
    {
        printf ("SW reset fail ... \n");
    };
}

void mtk_wdt_hw_reset(void)
{
    printf("WDT_HW_Reset_test\n");

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

BOOL mtk_is_rgu_trigger_reset()
{
    if(reboot_from == RE_BOOT_FROM_POWER_ON)
        return FALSE;
    return TRUE;
}

int mtk_wdt_boot_check(void)
{
    unsigned int wdt_sta = mtk_wdt_check_status();

    #if 0
	if (wdt_sta == MTK_WDT_STATUS_HWWDT_RST) { /* For E1 bug, that SW reset value is 0xC000, we using "==" to check */
    	/* Time out reboot always by pass power key */
        printf ("TO reset, need bypass power key\n");
        return WDT_BY_PASS_PWK_REBOOT;

    } else if (wdt_sta & MTK_WDT_STATUS_SWWDT_RST) {
    #endif
    /* 
     * For DA download hope to timeout reboot, and boot to u-boot/kernel configurable reason,
     * we set both timeout reboot and software reboot can check whether bypass power key.
     */
    if (wdt_sta & (MTK_WDT_STATUS_HWWDT_RST|MTK_WDT_STATUS_SWWDT_RST|
		MTK_WDT_STATUS_SPM_THERMAL_RST|MTK_WDT_STATUS_SPMWDT_RST|
		MTK_WDT_STATUS_THERMAL_DIRECT_RST|MTK_WDT_STATUS_SECURITY_RST
		|MTK_WDT_STATUS_DEBUGWDT_RST)) {
        if (rgu_mode & MTK_WDT_MODE_AUTO_RESTART) {
            /* HW/SW reboot, and auto restart is set, means bypass power key */
            printf ("SW reset with bypass power key flag\n");
            return WDT_BY_PASS_PWK_REBOOT;
        } else {
            printf ("SW reset without bypass power key flag\n");
            return WDT_NORMAL_REBOOT;
        }
    }

    return WDT_NOT_WDT_REBOOT;
}

void mtk_wdt_init(void)
{
		unsigned wdt_sta;
		unsigned int wdt_dbg_ctrl;
		unsigned int nonrst;
    /* Dump RGU regisers */
    printf("==== Dump RGU Reg ========\n");
    printf("RGU MODE:     %x\n", DRV_Reg32(MTK_WDT_MODE));
    printf("RGU LENGTH:   %x\n", DRV_Reg32(MTK_WDT_LENGTH));
    printf("RGU STA:      %x\n", DRV_Reg32(MTK_WDT_STATUS));
    printf("RGU INTERVAL: %x\n", DRV_Reg32(MTK_WDT_INTERVAL));
    printf("RGU SWSYSRST: %x\n", DRV_Reg32(MTK_WDT_SWSYSRST));
    printf("==== Dump RGU Reg End ====\n");
    
    rgu_mode = DRV_Reg32(MTK_WDT_MODE);

    wdt_sta = mtk_wdt_check_status(); // This function will store the reset reason: Time out/ SW trigger
    
	if ((wdt_sta & MTK_WDT_STATUS_HWWDT_RST)&&(rgu_mode&MTK_WDT_MODE_AUTO_RESTART)) 
	{ /* For E1 bug, that SW reset value is 0xC000, we using "==" to check */
    	/* Time out reboot always by pass power key */
        g_rgu_status = RE_BOOT_BY_WDT_HW;

    } 
	else if (wdt_sta & MTK_WDT_STATUS_SWWDT_RST) 
	{
    	g_rgu_status = RE_BOOT_BY_WDT_SW;
    }
    else 
	{
    	g_rgu_status = RE_BOOT_REASON_UNKNOW;
    }

	if(wdt_sta & MTK_WDT_STATUS_IRQWDT_RST)
	{
	   g_rgu_status |= RE_BOOT_WITH_INTTERUPT;
	}
	if(wdt_sta & MTK_WDT_STATUS_SPM_THERMAL_RST)
	{
	   g_rgu_status |= RE_BOOT_BY_SPM_THERMAL;
	}
	if(wdt_sta & MTK_WDT_STATUS_SPMWDT_RST)
	{
	   g_rgu_status |= RE_BOOT_BY_SPM;
	}
	if(wdt_sta & MTK_WDT_STATUS_THERMAL_DIRECT_RST)
	{
	   g_rgu_status |= RE_BOOT_BY_THERMAL_DIRECT;
	}
	if(wdt_sta & MTK_WDT_STATUS_DEBUGWDT_RST)
	{
	   g_rgu_status |= RE_BOOT_BY_DEBUG;
	}
	if(wdt_sta & MTK_WDT_STATUS_SECURITY_RST)
	{
	   g_rgu_status |= RE_BOOT_BY_SECURITY;
	}
	
    printf ("RGU: g_rgu_satus:%d\n", g_rgu_status);	    
    mtk_wdt_mode_config(FALSE, FALSE, FALSE, FALSE, FALSE); // Wirte Mode register will clear status register
    mtk_wdt_check_trig_reboot_reason();
    /* Setting timeout 10s */
    mtk_wdt_set_time_out_value(16);

#if (!CFG_APWDT_DISABLE)
    /* Config HW reboot mode */
    mtk_wdt_mode_config(TRUE, TRUE, TRUE, FALSE, TRUE); 
    mtk_wdt_restart();
#endif
    /*
	* E3 ECO
       * reset will delay 2ms after set SW_WDT in register
	*/
	nonrst = READ_REG(MTK_WDT_NONRST_REG);
	nonrst = (nonrst | (1<<29)); 
	WRITE_REG(nonrst, MTK_WDT_NONRST_REG);
	printf("WDT NONRST=0x%x\n", READ_REG(MTK_WDT_NONRST_REG));
    // set mcu_lath_en requir by pl owner confirmed by RGU DE
    
    /*SW workaround close DEBUG IRQ for C2K*/
	nonrst = READ_REG(MTK_WDT_REQ_IRQ_EN);
	nonrst &= (~(1<<19)); 
	nonrst |= MTK_WDT_REQ_IRQ_KEY; 
	WRITE_REG(nonrst, MTK_WDT_REQ_IRQ_EN);
	
	/*disable spm_thermal bit, becaue spm_thermal had been remove from HW*/
	nonrst = READ_REG(MTK_WDT_REQ_IRQ_EN);
	nonrst &= (~(1<<0)); /*disale spm_thermal irq*/
	nonrst |= MTK_WDT_REQ_IRQ_KEY; 
	WRITE_REG(nonrst, MTK_WDT_REQ_IRQ_EN);
	printf("WDT IRQ_EN=0x%x\n", READ_REG(MTK_WDT_REQ_IRQ_EN));

	nonrst = READ_REG(MTK_WDT_REQ_MODE);
	nonrst &= (~(1<<0)); /*disale spm_thermal reset*/
	nonrst |= 0x33000000; 
	WRITE_REG(nonrst, MTK_WDT_REQ_MODE);
	printf("WDT REQ_EN=0x%x\n", READ_REG(MTK_WDT_REQ_MODE));
	
    wdt_dbg_ctrl = READ_REG(MTK_WDT_DEBUG_CTL);
    wdt_dbg_ctrl |= MTK_RG_MCU_LATH_EN;
    wdt_dbg_ctrl |= MTK_DEBUG_CTL_KEY;
    WRITE_REG(wdt_dbg_ctrl, MTK_WDT_DEBUG_CTL);  
    printf("RGU %s:MTK_WDT_DEBUG_CTL(%x)\n", __func__,wdt_dbg_ctrl);
}

void mtk_arch_reset(char mode)
{
    printf("mtk_arch_reset at pre-loader!\n");

    //mtk_wdt_hw_reset();
    mtk_wdt_reset(mode);

    while (1);
}
int rgu_dram_reserved(int enable)
{
    volatile unsigned int tmp, ret = 0;
    if(1 == enable) 
    {
        /* enable ddr reserved mode */
        tmp = READ_REG(MTK_WDT_MODE);
        tmp |= (MTK_WDT_MODE_DDR_RESERVE|MTK_WDT_MODE_KEY);
        WRITE_REG(tmp, MTK_WDT_MODE);                        
                        
    } else if(0 == enable)
    {
        /* disable ddr reserved mode, set reset mode, 
               disable watchdog output reset signal */
        tmp = READ_REG(MTK_WDT_MODE);
        tmp &= (~MTK_WDT_MODE_DDR_RESERVE);
        tmp |= MTK_WDT_MODE_KEY;
        WRITE_REG(tmp, MTK_WDT_MODE);        
    } else 
    {
        printf("Wrong input %d, should be 1(enable) or 0(disable) in %s\n", enable, __func__);
        ret = -1;
    }
	printf("RGU %s:MTK_WDT_MODE(%x)\n", __func__,tmp);
    return ret;
}

int rgu_is_reserve_ddr_enabled(void)
{
  unsigned int wdt_mode;
  wdt_mode = READ_REG(MTK_WDT_MODE);
  if(wdt_mode & MTK_WDT_MODE_DDR_RESERVE)
  {
    return 1;
  } 
  else
  {
    return 0;
  }   
}

int rgu_is_dram_slf(void)
{
  unsigned int wdt_dbg_ctrl;
  wdt_dbg_ctrl = READ_REG(MTK_WDT_DEBUG_CTL);
  printf("DDR is in self-refresh. %x\n", wdt_dbg_ctrl);
  if(wdt_dbg_ctrl & MTK_DDR_SREF_STA)
  {
    //printf("DDR is in self-refresh. %x\n", wdt_dbg_ctrl);
    return 1;
  }
  else
  {  
    //printf("DDR is not in self-refresh. %x\n", wdt_dbg_ctrl);
    return 0;
  }
}

void rgu_release_rg_dramc_conf_iso(void)
{
  unsigned int wdt_dbg_ctrl;
  wdt_dbg_ctrl = READ_REG(MTK_WDT_DEBUG_CTL);
  wdt_dbg_ctrl &= (~MTK_RG_CONF_ISO);
  wdt_dbg_ctrl |= MTK_DEBUG_CTL_KEY;
  WRITE_REG(wdt_dbg_ctrl, MTK_WDT_DEBUG_CTL);  
  printf("RGU %s:MTK_WDT_DEBUG_CTL(%x)\n", __func__,wdt_dbg_ctrl);
}

void rgu_release_rg_dramc_iso(void)
{
  unsigned int wdt_dbg_ctrl;
  wdt_dbg_ctrl = READ_REG(MTK_WDT_DEBUG_CTL);
  wdt_dbg_ctrl &= (~MTK_RG_DRAMC_ISO);
  wdt_dbg_ctrl |= MTK_DEBUG_CTL_KEY;
  WRITE_REG(wdt_dbg_ctrl, MTK_WDT_DEBUG_CTL);  
  printf("RGU %s:MTK_WDT_DEBUG_CTL(%x)\n", __func__,wdt_dbg_ctrl);
}

void rgu_release_rg_dramc_sref(void)
{
  unsigned int wdt_dbg_ctrl;
  wdt_dbg_ctrl = READ_REG(MTK_WDT_DEBUG_CTL);
  wdt_dbg_ctrl &= (~MTK_RG_DRAMC_SREF);
  wdt_dbg_ctrl |= MTK_DEBUG_CTL_KEY;
  WRITE_REG(wdt_dbg_ctrl, MTK_WDT_DEBUG_CTL);  
  printf("RGU %s:MTK_WDT_DEBUG_CTL(%x)\n", __func__,wdt_dbg_ctrl);
}
int rgu_is_reserve_ddr_mode_success(void)
{
  unsigned int wdt_dbg_ctrl;
  wdt_dbg_ctrl = READ_REG(MTK_WDT_DEBUG_CTL);
  if(wdt_dbg_ctrl & MTK_DDR_RESERVE_RTA)
  {
    printf("WDT DDR reserve mode success! %x\n",wdt_dbg_ctrl);
    return 1;
  }
  else
  {  
    printf("WDT DDR reserve mode FAIL! %x\n",wdt_dbg_ctrl);
    return 0;
  }  
}

void rgu_swsys_reset(WD_SYS_RST_TYPE reset_type)
{
    if(WD_MD_RST == reset_type)
    {
	   unsigned int wdt_dbg_ctrl;
	   wdt_dbg_ctrl = READ_REG(MTK_WDT_SWSYSRST);
	   wdt_dbg_ctrl |= MTK_WDT_SWSYS_RST_KEY;
	   wdt_dbg_ctrl |= 0x80;// 1<<7
	   WRITE_REG(wdt_dbg_ctrl, MTK_WDT_SWSYSRST); 
	   udelay(1000);
	   wdt_dbg_ctrl = READ_REG(MTK_WDT_SWSYSRST);
	   wdt_dbg_ctrl |= MTK_WDT_SWSYS_RST_KEY;
	   wdt_dbg_ctrl &= (~0x80);// ~(1<<7)
	   WRITE_REG(wdt_dbg_ctrl, MTK_WDT_SWSYSRST); 
       printf("rgu pl md reset\n");
    }
}

#else // Using dummy WDT functions
void mtk_wdt_init(void)
{
    printf("PL WDT Dummy init called\n");
}
BOOL mtk_is_rgu_trigger_reset()
{
    printf("PL Dummy mtk_is_rgu_trigger_reset called\n");
    return FALSE;
}
void mtk_arch_reset(char mode)
{
    printf("PL WDT Dummy arch reset called\n");
}

int mtk_wdt_boot_check(void)
{
    printf("PL WDT Dummy mtk_wdt_boot_check called\n");
    return 0;
}

void mtk_wdt_disable(void)
{
   printff("UB WDT Dummy mtk_wdt_disable called\n");
}

void mtk_wdt_restart(void)
{
   printff("UB WDT Dummy mtk_wdt_restart called\n");
}
static void mtk_wdt_sw_reset(void)
{
  printf("UB WDT Dummy mtk_wdt_sw_reset called\n");
}

static void mtk_wdt_hw_reset(void)
{
  printf("UB WDT Dummy mtk_wdt_hw_reset called\n");
}

int rgu_dram_reserved(int enable)
{
    volatile unsigned int  ret = 0;
    
	printf("dummy RGU %s \n", __func__);
    return ret;
}

int rgu_is_reserve_ddr_enabled(void)
{
   printf("dummy RGU %s \n", __func__);  
   return 0;
}

int rgu_is_dram_slf(void)
{
  printf("dummy RGU %s \n", __func__);  
   return 0;
}

void rgu_release_rg_dramc_conf_iso(void)
{
  printf("dummy RGU %s \n", __func__);  
}

void rgu_release_rg_dramc_iso(void)
{
  printf("dummy RGU %s \n", __func__);  
}

void rgu_release_rg_dramc_sref(void)
{
  printf("dummy RGU %s \n", __func__);  
}
int rgu_is_reserve_ddr_mode_success(void)
{
  printf("dummy RGU %s \n", __func__);  
  return 0;
}
void rgu_swsys_reset(WD_SYS_RST_TYPE reset_type)

{
	printf("dummy RGU %s \n", __func__);

}


#endif
