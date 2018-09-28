/******************************************************************************
* mtk_nand.c - MTK NAND Flash Device Driver
 *
* Copyright 2009-2012 MediaTek Co.,Ltd.
 *
* DESCRIPTION:
* 	This file provid the other drivers nand relative functions
 *
* modification history
* ----------------------------------------
* v3.0, 11 Feb 2010, mtk
* ----------------------------------------
******************************************************************************/
#include <configs/autoconf.h>
#if !defined (ON_BOARD_NAND_FLASH_COMPONENT)
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/dma-mapping.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/mm.h>
#include <linux/xlog.h>
#include <asm/io.h>
#include <asm/cacheflush.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <mach/mtk_nand.h>
#include <mach/dma.h>
#include <mach/devs.h>
#include <mach/mt_reg_base.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_clkmgr.h>
#include <mach/mtk_nand.h>
#include <mach/bmt.h>
#include <mach/mt_irq.h>
//#include "partition.h"
#include <asm/system.h>
#include "partition_define.h"
#include <mach/mt_boot.h>
//#include "../../../../../../source/kernel/drivers/aee/ipanic/ipanic.h"
#include <linux/rtc.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#ifdef CONFIG_PWR_LOSS_MTK_SPOH
#include <mach/power_loss_test.h>
#endif
#include <mach/nand_device_list.h>
#else
#include <common.h>
#include <linux/string.h>
#include <config.h>
#include <malloc.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand_ecc.h>
#include <asm/arch/nand/mtk_nand.h>
#include <asm/arch/mt_reg_base.h>
#include <asm/arch/mt_typedefs.h>
#include <asm/arch/mt_clkmgr.h>
#include <asm/arch/nand/bmt.h>
//#include <asm/arch/nand/part.h>
#include <asm/arch/mt_irq.h>
#include <asm/arch/nand/nand_device_list.h>
#include <asm/arch/nand/partition_define.h>
#include <asm/arch/mt_gpio.h>
//#include <sys/types.h>
#define printk	printf
typedef unsigned short* P_U16;
typedef unsigned long* P_U32;
typedef unsigned short uint16;

#define PERICFG_BASE (0xF0003000)
#define NFI_RANDOM_ENSEED01_TS_REG32 ((volatile P_U32)(NFI_BASE+0x024C))
#define NFI_RANDOM_ENSEED02_TS_REG32 ((volatile P_U32)(NFI_BASE+0x0250))
#define NFI_RANDOM_ENSEED03_TS_REG32 ((volatile P_U32)(NFI_BASE+0x0254))
#define NFI_RANDOM_ENSEED04_TS_REG32 ((volatile P_U32)(NFI_BASE+0x0258))
#define NFI_RANDOM_ENSEED05_TS_REG32 ((volatile P_U32)(NFI_BASE+0x025C))
#define NFI_RANDOM_ENSEED06_TS_REG32 ((volatile P_U32)(NFI_BASE+0x0260))
#define NFI_RANDOM_DESEED01_TS_REG32 ((volatile P_U32)(NFI_BASE+0x0264))
#define NFI_RANDOM_DESEED02_TS_REG32 ((volatile P_U32)(NFI_BASE+0x0268))
#define NFI_RANDOM_DESEED03_TS_REG32 ((volatile P_U32)(NFI_BASE+0x026C))
#define NFI_RANDOM_DESEED04_TS_REG32 ((volatile P_U32)(NFI_BASE+0x0270))
#define NFI_RANDOM_DESEED05_TS_REG32 ((volatile P_U32)(NFI_BASE+0x0274))
#define NFI_RANDOM_DESEED06_TS_REG32 ((volatile P_U32)(NFI_BASE+0x0278))
/* NAND driver */
struct mtk_nand_host_hw {
    unsigned int nfi_bus_width;         /* NFI_BUS_WIDTH */
    unsigned int nfi_access_timing;     /* NFI_ACCESS_TIMING */
    unsigned int nfi_cs_num;            /* NFI_CS_NUM */
    unsigned int nand_sec_size;         /* NAND_SECTOR_SIZE */
    unsigned int nand_sec_shift;        /* NAND_SECTOR_SHIFT */
    unsigned int nand_ecc_size;
    unsigned int nand_ecc_bytes;
    unsigned int nand_ecc_mode;
};
#define NFI_DEFAULT_ACCESS_TIMING        0x30c77fff//(0x44333)
#define NFI_CS_NUM				(1)
struct mtk_nand_host_hw mtk_nand_hw = {
    .nfi_bus_width = 8,         /* NFI_BUS_WIDTH */
    .nfi_access_timing = NFI_DEFAULT_ACCESS_TIMING,     /* NFI_ACCESS_TIMING */
    .nfi_cs_num = NFI_CS_NUM,            /* NFI_CS_NUM */
    .nand_sec_size = 512,         /* NAND_SECTOR_SIZE */
    .nand_sec_shift = 9,        /* NAND_SECTOR_SHIFT */
    .nand_ecc_size = 2048,
    .nand_ecc_bytes = 32,
    .nand_ecc_mode = NAND_ECC_HW
};
#define xlog_printk	fprintf
#define ANDROID_LOG_WARN	stderr
#define ANDROID_LOG_INFO	stderr
#define MTD_MAX_OOBFREE_ENTRIES      16	
#define NFI_CNRNB_REG16             ((volatile P_U16)(NFI_BASE+0x0044))
#define RAND_START_ADDR	0
#define NFI_DEFAULT_CS                          (0)
#define mb() __asm__ __volatile__("": : :"memory")
#define ASSERT	assert
extern int nand_get_device(struct nand_chip *chip, struct mtd_info *mtd, int new_state);
#endif

#define VERSION  	"v2.1 Fix AHB virt2phys error"
#define MODULE_NAME	"# MTK NAND #"
#define PROCNAME    "driver/nand"

#define __INTERNAL_USE_AHB_MODE__ 	(1)

#if defined (MT7623_FPGA_BOARD)
#define CFG_FPGA_PLATFORM (1)
#define CFG_RANDOMIZER    (0) // for randomizer code
#define CFG_PERFLOG_DEBUG (0) // for performance log
#define CFG_2CS_NAND    (0) // for 2CS nand
#define CFG_COMBO_NAND    (0) // for Combo nand
#define CONFIG_CLKMGR_BRINGUP
#else
#define CFG_FPGA_PLATFORM (0) // for fpga by bean
#define CFG_RANDOMIZER    (1) // for randomizer code
#define CFG_PERFLOG_DEBUG (0) // for performance log
#define CFG_2CS_NAND    (1) // for 2CS nand
#define CFG_COMBO_NAND    (1) // for Combo nand
#define CONFIG_CLKMGR_BRINGUP
#define MTK_COMBO_NAND_SUPPORT (1)
#endif

#define NFI_TRICKY_CS  (1)  // must be 1 or > 1?

#define PERI_NFI_CLK_SOURCE_SEL ((volatile P_U32)(PERICFG_BASE+0x424))
#define PERI_NFI_MAC_CTRL ((volatile P_U32)(PERICFG_BASE+0x428))
#define NFI_PAD_1X_CLOCK (1) //nfi1X

#if !defined (CONFIG_NAND_BOOTLOADER)
void show_stack(struct task_struct *tsk, unsigned long *sp);
#endif
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq,unsigned int polarity);

extern struct mtd_partition g_pasStatic_Partition[PART_MAX_COUNT];

#if defined(MTK_MLC_NAND_SUPPORT)
bool MLC_DEVICE = TRUE;// to build pass xiaolei
#else
bool MLC_DEVICE = FALSE;
#endif

#if defined(NAND_OTP_SUPPORT)

#define SAMSUNG_OTP_SUPPORT     1
#define OTP_MAGIC_NUM           0x4E3AF28B
#define SAMSUNG_OTP_PAGE_NUM    6

static const unsigned int Samsung_OTP_Page[SAMSUNG_OTP_PAGE_NUM] = { 0x15, 0x16, 0x17, 0x18, 0x19, 0x1b };

static struct mtk_otp_config g_mtk_otp_fuc;
static spinlock_t g_OTPLock;

#define OTP_MAGIC           'k'

/* NAND OTP IO control number */
#define OTP_GET_LENGTH 		_IOW(OTP_MAGIC, 1, int)
#define OTP_READ 	        _IOW(OTP_MAGIC, 2, int)
#define OTP_WRITE 			_IOW(OTP_MAGIC, 3, int)

#define FS_OTP_READ         0
#define FS_OTP_WRITE        1

/* NAND OTP Error codes */
#define OTP_SUCCESS                   0
#define OTP_ERROR_OVERSCOPE          -1
#define OTP_ERROR_TIMEOUT            -2
#define OTP_ERROR_BUSY               -3
#define OTP_ERROR_NOMEM              -4
#define OTP_ERROR_RESET              -5

struct mtk_otp_config
{
    u32(*OTPRead) (u32 PageAddr, void *BufferPtr, void *SparePtr);
    u32(*OTPWrite) (u32 PageAddr, void *BufferPtr, void *SparePtr);
    u32(*OTPQueryLength) (u32 * Length);
};

struct otp_ctl
{
    unsigned int QLength;
    unsigned int Offset;
    unsigned int Length;
    char *BufferPtr;
    unsigned int status;
};
#endif

#define ERR_RTN_SUCCESS   1
#define ERR_RTN_FAIL      0
#define ERR_RTN_BCH_FAIL -1

#define NFI_SET_REG32(reg, value) \
do {	\
	g_value = (DRV_Reg32(reg) | (value));\
	DRV_WriteReg32(reg, g_value); \
} while(0)

#define NFI_SET_REG16(reg, value) \
do {	\
	g_value = (DRV_Reg16(reg) | (value));\
	DRV_WriteReg16(reg, g_value); \
} while(0)

#define NFI_CLN_REG32(reg, value) \
do {	\
	g_value = (DRV_Reg32(reg) & (~(value)));\
	DRV_WriteReg32(reg, g_value); \
} while(0)

#define NFI_CLN_REG16(reg, value) \
do {	\
	g_value = (DRV_Reg16(reg) & (~(value)));\
	DRV_WriteReg16(reg, g_value); \
} while(0)

#define NFI_WAIT_STATE_DONE(state) do{;}while (__raw_readl(NFI_STA_REG32) & state)
#define NFI_WAIT_TO_READY()  do{;}while (!(__raw_readl(NFI_STA_REG32) & STA_BUSY2READY))
#define FIFO_PIO_READY(x)  (0x1 & x)
#define WAIT_NFI_PIO_READY(timeout) \
    do {\
    while( (!FIFO_PIO_READY(DRV_Reg(NFI_PIO_DIRDY_REG16))) && (--timeout) );\
    } while(0);


#define NAND_SECTOR_SIZE (512)
#define OOB_PER_SECTOR      (16)
#define OOB_AVAI_PER_SECTOR (8)

#if defined(MTK_COMBO_NAND_SUPPORT)
	// BMT_POOL_SIZE is not used anymore
#else
	#ifndef PART_SIZE_BMTPOOL
	#define BMT_POOL_SIZE (80)
	#else
	#define BMT_POOL_SIZE (PART_SIZE_BMTPOOL)
	#endif
#endif
#if defined (CONFIG_NAND_BOOTLOADER)
u32 g_bmt_sz = 0;
bool bBCHRetry = TRUE;
#endif
u8 ecc_threshold;

/*******************************************************************************
 * Gloable Varible Definition
 *******************************************************************************/
#if CFG_PERFLOG_DEBUG
struct nand_perf_log
{
    unsigned int ReadPageCount;
    suseconds_t  ReadPageTotalTime;
    unsigned int ReadBusyCount;
    suseconds_t  ReadBusyTotalTime;
    unsigned int ReadDMACount;
    suseconds_t  ReadDMATotalTime;

    unsigned int ReadSubPageCount;
    suseconds_t  ReadSubPageTotalTime;
    
    unsigned int WritePageCount;
    suseconds_t  WritePageTotalTime;
    unsigned int WriteBusyCount;
    suseconds_t  WriteBusyTotalTime;
    unsigned int WriteDMACount;
    suseconds_t  WriteDMATotalTime;

    unsigned int EraseBlockCount;
    suseconds_t  EraseBlockTotalTime;

};
#endif
#ifdef PWR_LOSS_SPOH

#define PL_TIME_RAND_PROG(chip, page_addr, time) do { \
    if(host->pl.nand_program_wdt_enable == 1){ \
        PL_TIME_RAND(page_addr, time, host->pl.last_prog_time);} \
    else \
        time=0; \
    } while(0)
    
#define PL_TIME_RAND_ERASE(chip, page_addr, time) do { \
    if(host->pl.nand_erase_wdt_enable == 1){ \
        PL_TIME_RAND(page_addr, time, host->pl.last_erase_time); \
        if(time != 0) \
        printk(KERN_ERR "[MVG_TEST]: Erase reset in %d us\n", time);} \
    else \
        time=0; \
    } while(0)

#define PL_TIME_PROG(duration) do {  \
    host->pl.last_prog_time = duration; \
    } while(0)

#define PL_TIME_ERASE(duration) do { \
    host->pl.last_erase_time = duration; \
    } while(0)


#define PL_TIME_PROG_WDT_SET(WDT) do {  \
    host->pl.nand_program_wdt_enable = WDT; \
    } while(0)

#define PL_TIME_ERASE_WDT_SET(WDT) do { \
    host->pl.nand_erase_wdt_enable = WDT; \
    } while(0)

#define PL_NAND_BEGIN(time) PL_BEGIN(time)

#define PL_NAND_RESET(time) PL_RESET(time)

#define PL_NAND_END(pl_time_write, duration) PL_END(pl_time_write, duration)


#else

#define PL_TIME_RAND_PROG(chip, page_addr, time)
#define PL_TIME_RAND_ERASE(chip, page_addr, time)

#define PL_TIME_PROG(duration)
#define PL_TIME_ERASE(duration)

#define PL_TIME_PROG_WDT_SET(WDT)
#define PL_TIME_ERASE_WDT_SET(WDT)

#define PL_NAND_BEGIN(time)
#define PL_NAND_RESET(time)
#define PL_NAND_END(pl_time_write, duration)

#endif

#if CFG_PERFLOG_DEBUG
static struct nand_perf_log g_NandPerfLog={0};
static struct timeval g_NandLogTimer={0};
#endif

#ifdef NAND_PFM
static suseconds_t g_PFM_R = 0;
static suseconds_t g_PFM_W = 0;
static suseconds_t g_PFM_E = 0;
static u32 g_PFM_RNum = 0;
static u32 g_PFM_RD = 0;
static u32 g_PFM_WD = 0;
static struct timeval g_now;

#define PFM_BEGIN(time) \
do_gettimeofday(&g_now); \
(time) = g_now;

#define PFM_END_R(time, n) \
do_gettimeofday(&g_now); \
g_PFM_R += (g_now.tv_sec * 1000000 + g_now.tv_usec) - (time.tv_sec * 1000000 + time.tv_usec); \
g_PFM_RNum += 1; \
g_PFM_RD += n; \
MSG(PERFORMANCE, "%s - Read PFM: %lu, data: %d, ReadOOB: %d (%d, %d)\n", MODULE_NAME , g_PFM_R, g_PFM_RD, g_kCMD.pureReadOOB, g_kCMD.pureReadOOBNum, g_PFM_RNum);

#define PFM_END_W(time, n) \
do_gettimeofday(&g_now); \
g_PFM_W += (g_now.tv_sec * 1000000 + g_now.tv_usec) - (time.tv_sec * 1000000 + time.tv_usec); \
g_PFM_WD += n; \
MSG(PERFORMANCE, "%s - Write PFM: %lu, data: %d\n", MODULE_NAME, g_PFM_W, g_PFM_WD);

#define PFM_END_E(time) \
do_gettimeofday(&g_now); \
g_PFM_E += (g_now.tv_sec * 1000000 + g_now.tv_usec) - (time.tv_sec * 1000000 + time.tv_usec); \
MSG(PERFORMANCE, "%s - Erase PFM: %lu\n", MODULE_NAME, g_PFM_E);
#else
#define PFM_BEGIN(time)
#define PFM_END_R(time, n)
#define PFM_END_W(time, n)
#define PFM_END_E(time)
#endif

#define TIMEOUT_1   0x1fff
#define TIMEOUT_2   0x8ff
#define TIMEOUT_3   0xffff
#define TIMEOUT_4   0xffff      //5000   //PIO

#define NFI_ISSUE_COMMAND(cmd, col_addr, row_addr, col_num, row_num) \
   do { \
      DRV_WriteReg(NFI_CMD_REG16,cmd);\
      while (DRV_Reg32(NFI_STA_REG32) & STA_CMD_STATE);\
      DRV_WriteReg32(NFI_COLADDR_REG32, col_addr);\
      DRV_WriteReg32(NFI_ROWADDR_REG32, row_addr);\
      DRV_WriteReg(NFI_ADDRNOB_REG16, col_num | (row_num<<ADDR_ROW_NOB_SHIFT));\
      while (DRV_Reg32(NFI_STA_REG32) & STA_ADDR_STATE);\
   }while(0);

//-------------------------------------------------------------------------------
static struct completion g_comp_AHB_Done;
static struct NAND_CMD g_kCMD;
bool g_bInitDone;
static int g_i4Interrupt;
static bool g_bcmdstatus;
//static bool g_brandstatus;
static u32 g_value = 0;
static int g_page_size;
static int g_block_size;
static u32 PAGES_PER_BLOCK = 255;
static bool g_bSyncOrToggle = false;
static int g_iNFI2X_CLKSRC = ARMPLL;
#if defined (CONFIG_NAND_BOOTLOADER)
#define MAX_FLASH	(sizeof(gen_FlashTable)/sizeof(flashdev_info))
#define gen_FlashTable_p	gen_FlashTable
unsigned int flash_number=MAX_FLASH;
#else
extern unsigned int flash_number;
extern flashdev_info gen_FlashTable_p[MAX_FLASH];
extern int part_num;
#endif
#if CFG_2CS_NAND
bool g_b2Die_CS = FALSE; // for nand base
static bool g_bTricky_CS = FALSE;
static u32 g_nanddie_pages = 0;
#endif

#if __INTERNAL_USE_AHB_MODE__
BOOL g_bHwEcc = true;
#else
BOOL g_bHwEcc = true;
#endif
#define LPAGE 16384
#define LSPARE 2048

static u8 *local_buffer_16_align;   // 16 byte aligned buffer, for HW issue
__attribute__((aligned(64))) static u8 local_buffer[LPAGE + LSPARE];
static u8 *temp_buffer_16_align;   // 16 byte aligned buffer, for HW issue
__attribute__((aligned(64))) static u8 temp_buffer[LPAGE + LSPARE];
//static u8 *bean_buffer_16_align;   // 16 byte aligned buffer, for HW issue
//__attribute__((aligned(64))) static u8 bean_buffer[LPAGE + LSPARE];
#if defined (CONFIG_NAND_BOOTLOADER)
static int mtk_nand_read_oob(struct mtd_info *mtd, struct nand_chip *chip, int page, int sndcmd);
static int mtk_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip, u8 * buf, int page);
#endif
extern struct mtd_perf_log g_MtdPerfLog;

extern void nand_release_device(struct mtd_info *mtd);
#if !defined (CONFIG_NAND_BOOTLOADER)
extern int nand_get_device(struct mtd_info *mtd, int new_state);
#endif
bool mtk_nand_SetFeature(struct mtd_info *mtd, u16 cmd, u32 addr, u8 *value,  u8 bytes);
bool mtk_nand_GetFeature(struct mtd_info *mtd, u16 cmd, u32 addr, u8 *value,  u8 bytes);

#if CFG_2CS_NAND
static int mtk_nand_cs_check(struct mtd_info *mtd, u8 *id, u16 cs);
static u32 mtk_nand_cs_on(struct nand_chip *nand_chip, u16 cs, u32 page);
#endif

#if defined (CONFIG_NAND_BOOTLOADER)
bmt_struct *g_bmt = NULL;
#else
static bmt_struct *g_bmt;
#endif
struct mtk_nand_host *host;
static u8 g_running_dma = 0;
#ifdef DUMP_NATIVE_BACKTRACE
static u32 g_dump_count = 0;
#endif
//extern struct mtd_partition g_pasStatic_Partition[];//to build pass xiaolei
//int part_num = PART_NUM;//to build pass xiaolei  NUM_PARTITIONS;
#ifdef PMT
extern void part_init_pmt(struct mtd_info *mtd, u8 * buf);
extern struct mtd_partition g_exist_Partition[PART_MAX_COUNT];
#endif
int manu_id;
int dev_id;

static u8 local_oob_buf[LSPARE];

#ifdef _MTK_NAND_DUMMY_DRIVER_
int dummy_driver_debug;
#endif

flashdev_info devinfo;

enum NAND_TYPE_MASK{
    TYPE_ASYNC = 0x0,
    TYPE_TOGGLE = 0x1,
    TYPE_SYNC = 0x2,
    TYPE_RESERVED = 0x3,
    TYPE_MLC = 0x4, // 1b0
    TYPE_SLC = 0x4, // 1b1
};

u32 MICRON_TRANSFER(u32 pageNo);
u32 SANDISK_TRANSFER(u32 pageNo);
u32 HYNIX_TRANSFER(u32 pageNo);
u32 hynix_pairpage_mapping(u32 page, bool high_to_low);
u32 micron_pairpage_mapping(u32 page, bool high_to_low);
u32 sandisk_pairpage_mapping(u32 page, bool high_to_low);

typedef u32 (*GetLowPageNumber)(u32 pageNo);
typedef u32 (*TransferPageNumber)(u32 pageNo, bool high_to_low);

GetLowPageNumber functArray[]=
{
	MICRON_TRANSFER,
	HYNIX_TRANSFER,
	SANDISK_TRANSFER,
};

TransferPageNumber fsFuncArray[]=
{
	micron_pairpage_mapping,
	hynix_pairpage_mapping,
	sandisk_pairpage_mapping,
};

u32 SANDISK_TRANSFER(u32 pageNo)
{
	if(0 == pageNo)
	{
		return pageNo;
	}
	else
	{
		return pageNo+pageNo-1;
	}
}

u32 HYNIX_TRANSFER(u32 pageNo)
{
	u32 temp;
	if(pageNo < 4)
		return pageNo;
	temp = pageNo+(pageNo&0xFFFFFFFE)-2;
	return temp;
}


u32 MICRON_TRANSFER(u32 pageNo)
{
	u32 temp;
	if(pageNo < 4)
		return pageNo;
	temp = (pageNo - 4) & 0xFFFFFFFE;
	if(pageNo<=130)
		return (pageNo+temp);
	else
		return (pageNo+temp-2);
}

u32 sandisk_pairpage_mapping(u32 page, bool high_to_low)
{
	int offset;
	if(TRUE == high_to_low)
	{
		if(page == 255)
			return page-2;
		if((page == 0) || (1 == (page%2)))
			return page;
		else
		{
			if(page == 2)
				return 0;
			else
				return (page-3);
		}
	}
	else
	{
		if((page != 0) && (0 == (page%2)))
			return page;
		else
		{
			if(page == 255)
				return page;
			if(page == 0 || page == 253)
				return page + 2;
			else
				return page+3;
		}
	}
}
u32 hynix_pairpage_mapping(u32 page, bool high_to_low)
{
	int offset;
	if(TRUE == high_to_low)
	{
		//Micron 256pages
		if(page<4)
		{
			return page;
		}

		offset=page%4;
		if(offset==2 || offset==3)
		{
			return page;
		}
		else
		{
			if(page == 4 || page == 5 || page == 254 || page == 255)
				return page-4;
			else
				return page-6;
		}
	}
	else
	{
		if(page>251)
		{
			return page;
		}
		if(page == 0 || page == 1)
			return page+4;
		offset=page%4;
		if(offset==0 || offset==1)
		{
			return page;
		}
		else
		{
			return page+6;
		}
	}
}
u32 micron_pairpage_mapping(u32 page, bool high_to_low)
{
	int offset;
	if(TRUE == high_to_low)
	{
		//Micron 256pages
		if((page<4)||(page>251))
		{
			return page;
		}

		offset=page%4;
		if(offset==0 || offset==1)
		{
			return page;
		}
		else
		{
			return page-6;
		}
	}
	else
	{
		if((page == 2) || (page == 3) ||(page>247))
		{
			return page;
		}
		offset=page%4;
		if(offset==0 || offset==1)
		{
			return page+6;
		}
		else
		{
			return page;
		}
	}
}

int mtk_nand_paired_page_transfer(u32 pageNo, bool high_to_low)
{
	if((devinfo.vendor != VEND_NONE) && 
       (devinfo.feature_set.ptbl_idx != PPTBL_NOT_SUPPORT))
	{
		return fsFuncArray[devinfo.feature_set.ptbl_idx](pageNo,high_to_low);
	}
	else
	{
		return 0xFFFFFFFF;
	}
}

#if CFG_FPGA_PLATFORM
void nand_enable_clock(void)
{

}

void nand_disable_clock(void)
{

}
#else
#define PWR_DOWN 0
#define PWR_ON   1
#if defined(CONFIG_CLKMGR_BRINGUP)
int clock_is_on(int var)
{
	return 1;
}
#endif
void nand_enable_clock(void)
{
    if(clock_is_on(MT_CG_PERI_NFI)==PWR_DOWN)
        enable_clock(MT_CG_PERI_NFI, "NFI");
    if(clock_is_on(MT_CG_PERI_NFI_ECC)==PWR_DOWN)
        enable_clock(MT_CG_PERI_NFI_ECC, "NFI");
    if(clock_is_on(MT_CG_PERI_NFIPAD)==PWR_DOWN)
	    enable_clock(MT_CG_PERI_NFIPAD, "NFI");
}

void nand_disable_clock(void)
{
#if !defined (CONFIG_NAND_BOOTLOADER)
    if(clock_is_on(MT_CG_PERI_NFIPAD)==PWR_ON)
        disable_clock(MT_CG_PERI_NFIPAD, "NFI");
    if(clock_is_on(MT_CG_PERI_NFI_ECC)==PWR_ON)
        disable_clock(MT_CG_PERI_NFI_ECC, "NFI");
    if(clock_is_on(MT_CG_PERI_NFI)==PWR_ON)
	    disable_clock(MT_CG_PERI_NFI, "NFI");
#endif
}
#endif

static struct nand_ecclayout nand_oob_16 = {
    .eccbytes = 8,
    .eccpos = {8, 9, 10, 11, 12, 13, 14, 15},
    .oobfree = {{1, 6}, {0, 0}}
};

struct nand_ecclayout nand_oob_64 = {
    .eccbytes = 32,
    .eccpos = {32, 33, 34, 35, 36, 37, 38, 39,
               40, 41, 42, 43, 44, 45, 46, 47,
               48, 49, 50, 51, 52, 53, 54, 55,
               56, 57, 58, 59, 60, 61, 62, 63},
    .oobfree = {{1, 7}, {9, 7}, {17, 7}, {25, 6}, {0, 0}}
};

struct nand_ecclayout nand_oob_128 = {
    .eccbytes = 64,
    .eccpos = {
               64, 65, 66, 67, 68, 69, 70, 71,
               72, 73, 74, 75, 76, 77, 78, 79,
               80, 81, 82, 83, 84, 85, 86, 86,
               88, 89, 90, 91, 92, 93, 94, 95,
               96, 97, 98, 99, 100, 101, 102, 103,
               104, 105, 106, 107, 108, 109, 110, 111,
               112, 113, 114, 115, 116, 117, 118, 119,
               120, 121, 122, 123, 124, 125, 126, 127},
    .oobfree = {{1, 7}, {9, 7}, {17, 7}, {25, 7}, {33, 7}, {41, 7}, {49, 7}, {57, 6}}
};

/**************************************************************************
*  Randomizer
**************************************************************************/
#define SS_SEED_NUM 128
#define EFUSE_RANDOM_CFG	((volatile u32 *)(0x102061c0))
#define EFUSE_RANDOM_ENABLE 0x00000004
static bool use_randomizer = FALSE;
static bool pre_randomizer = FALSE;

static U16 SS_RANDOM_SEED[SS_SEED_NUM] =
{
    //for page 0~127
    0x576A, 0x05E8, 0x629D, 0x45A3, 0x649C, 0x4BF0, 0x2342, 0x272E,
    0x7358, 0x4FF3, 0x73EC, 0x5F70, 0x7A60, 0x1AD8, 0x3472, 0x3612,
    0x224F, 0x0454, 0x030E, 0x70A5, 0x7809, 0x2521, 0x484F, 0x5A2D,
    0x492A, 0x043D, 0x7F61, 0x3969, 0x517A, 0x3B42, 0x769D, 0x0647,
    0x7E2A, 0x1383, 0x49D9, 0x07B8, 0x2578, 0x4EEC, 0x4423, 0x352F,
    0x5B22, 0x72B9, 0x367B, 0x24B6, 0x7E8E, 0x2318, 0x6BD0, 0x5519,
    0x1783, 0x18A7, 0x7B6E, 0x7602, 0x4B7F, 0x3648, 0x2C53, 0x6B99,
    0x0C23, 0x67CF, 0x7E0E, 0x4D8C, 0x5079, 0x209D, 0x244A, 0x747B,
    0x350B, 0x0E4D, 0x7004, 0x6AC3, 0x7F3E, 0x21F5, 0x7A15, 0x2379,
    0x1517, 0x1ABA, 0x4E77, 0x15A1, 0x04FA, 0x2D61, 0x253A, 0x1302,
    0x1F63, 0x5AB3, 0x049A, 0x5AE8, 0x1CD7, 0x4A00, 0x30C8, 0x3247,
    0x729C, 0x5034, 0x2B0E, 0x57F2, 0x00E4, 0x575B, 0x6192, 0x38F8,
    0x2F6A, 0x0C14, 0x45FC, 0x41DF, 0x38DA, 0x7AE1, 0x7322, 0x62DF,
    0x5E39, 0x0E64, 0x6D85, 0x5951, 0x5937, 0x6281, 0x33A1, 0x6A32,
    0x3A5A, 0x2BAC, 0x743A, 0x5E74, 0x3B2E, 0x7EC7, 0x4FD2, 0x5D28,
    0x751F, 0x3EF8, 0x39B1, 0x4E49, 0x746B, 0x6EF6, 0x44BE, 0x6DB7      
};


#if CFG_PERFLOG_DEBUG
static suseconds_t Cal_timediff(struct timeval *end_time,struct timeval *start_time )
{
  struct timeval difference;

  difference.tv_sec =end_time->tv_sec -start_time->tv_sec ;
  difference.tv_usec=end_time->tv_usec-start_time->tv_usec;

  /* Using while instead of if below makes the code slightly more robust. */

  while(difference.tv_usec<0)
  {
    difference.tv_usec+=1000000;
    difference.tv_sec -=1;
  }

  return 1000000LL*difference.tv_sec+
                   difference.tv_usec;

} /* timeval_diff() */
#endif
#if CFG_PERFLOG_DEBUG

void dump_nand_rwcount(void)
{
    struct timeval now_time;
    do_gettimeofday(&now_time);
    if(Cal_timediff(&now_time,&g_NandLogTimer)>(500*1000))  // Dump per 100ms
    {
        MSG(INIT, " RPageCnt: %d (%lu us) RSubCnt: %d (%lu us) WPageCnt: %d (%lu us) ECnt: %d mtd(0/512/1K/2K/3K/4K): %d %d %d %d %d %d\n ",
                   g_NandPerfLog.ReadPageCount,
                   g_NandPerfLog.ReadPageCount ? (g_NandPerfLog.ReadPageTotalTime/g_NandPerfLog.ReadPageCount): 0,                    
                   g_NandPerfLog.ReadSubPageCount,
                   g_NandPerfLog.ReadSubPageCount? (g_NandPerfLog.ReadSubPageTotalTime/g_NandPerfLog.ReadSubPageCount): 0,
                   g_NandPerfLog.WritePageCount,
                   g_NandPerfLog.WritePageCount? (g_NandPerfLog.WritePageTotalTime/g_NandPerfLog.WritePageCount): 0,
                   g_NandPerfLog.EraseBlockCount,
                   g_MtdPerfLog.read_size_0_512,
                   g_MtdPerfLog.read_size_512_1K,    
                   g_MtdPerfLog.read_size_1K_2K,    
                   g_MtdPerfLog.read_size_2K_3K,     
                   g_MtdPerfLog.read_size_3K_4K,     
                   g_MtdPerfLog.read_size_Above_4K
                   );
       
        memset(&g_NandPerfLog,0x00,sizeof(g_NandPerfLog));            
        memset(&g_MtdPerfLog,0x00,sizeof(g_MtdPerfLog));            
        do_gettimeofday(&g_NandLogTimer);
        
    }    
}
#endif
void dump_nfi(void)
{
#if __DEBUG_NAND
    printk("~~~~Dump NFI Register in Kernel~~~~\n");
    printk("NFI_CNFG_REG16: 0x%x\n", DRV_Reg16(NFI_CNFG_REG16));
    printk("NFI_PAGEFMT_REG16: 0x%x\n", DRV_Reg16(NFI_PAGEFMT_REG16));
    printk("NFI_CON_REG16: 0x%x\n", DRV_Reg16(NFI_CON_REG16));
    printk("NFI_ACCCON_REG32: 0x%x\n", DRV_Reg32(NFI_ACCCON_REG32));
    printk("NFI_INTR_EN_REG16: 0x%x\n", DRV_Reg16(NFI_INTR_EN_REG16));
    printk("NFI_INTR_REG16: 0x%x\n", DRV_Reg16(NFI_INTR_REG16));
    printk("NFI_CMD_REG16: 0x%x\n", DRV_Reg16(NFI_CMD_REG16));
    printk("NFI_ADDRNOB_REG16: 0x%x\n", DRV_Reg16(NFI_ADDRNOB_REG16));
    printk("NFI_COLADDR_REG32: 0x%x\n", DRV_Reg32(NFI_COLADDR_REG32));
    printk("NFI_ROWADDR_REG32: 0x%x\n", DRV_Reg32(NFI_ROWADDR_REG32));
    printk("NFI_STRDATA_REG16: 0x%x\n", DRV_Reg16(NFI_STRDATA_REG16));
    printk("NFI_DATAW_REG32: 0x%x\n", DRV_Reg32(NFI_DATAW_REG32));
    printk("NFI_DATAR_REG32: 0x%x\n", DRV_Reg32(NFI_DATAR_REG32));
    printk("NFI_PIO_DIRDY_REG16: 0x%x\n", DRV_Reg16(NFI_PIO_DIRDY_REG16));
    printk("NFI_STA_REG32: 0x%x\n", DRV_Reg32(NFI_STA_REG32));
    printk("NFI_FIFOSTA_REG16: 0x%x\n", DRV_Reg16(NFI_FIFOSTA_REG16));
//    printk("NFI_LOCKSTA_REG16: 0x%x\n", DRV_Reg16(NFI_LOCKSTA_REG16));
    printk("NFI_ADDRCNTR_REG16: 0x%x\n", DRV_Reg16(NFI_ADDRCNTR_REG16));
    printk("NFI_STRADDR_REG32: 0x%x\n", DRV_Reg32(NFI_STRADDR_REG32));
    printk("NFI_BYTELEN_REG16: 0x%x\n", DRV_Reg16(NFI_BYTELEN_REG16));
    printk("NFI_CSEL_REG16: 0x%x\n", DRV_Reg16(NFI_CSEL_REG16));
    printk("NFI_IOCON_REG16: 0x%x\n", DRV_Reg16(NFI_IOCON_REG16));
    printk("NFI_FDM0L_REG32: 0x%x\n", DRV_Reg32(NFI_FDM0L_REG32));
    printk("NFI_FDM0M_REG32: 0x%x\n", DRV_Reg32(NFI_FDM0M_REG32));
    printk("NFI_LOCK_REG16: 0x%x\n", DRV_Reg16(NFI_LOCK_REG16));
    printk("NFI_LOCKCON_REG32: 0x%x\n", DRV_Reg32(NFI_LOCKCON_REG32));
    printk("NFI_LOCKANOB_REG16: 0x%x\n", DRV_Reg16(NFI_LOCKANOB_REG16));
    printk("NFI_FIFODATA0_REG32: 0x%x\n", DRV_Reg32(NFI_FIFODATA0_REG32));
    printk("NFI_FIFODATA1_REG32: 0x%x\n", DRV_Reg32(NFI_FIFODATA1_REG32));
    printk("NFI_FIFODATA2_REG32: 0x%x\n", DRV_Reg32(NFI_FIFODATA2_REG32));
    printk("NFI_FIFODATA3_REG32: 0x%x\n", DRV_Reg32(NFI_FIFODATA3_REG32));
    printk("NFI_MASTERSTA_REG16: 0x%x\n", DRV_Reg16(NFI_MASTERSTA_REG16));
    printk("NFI_DEBUG_CON1_REG16: 0x%x\n", DRV_Reg16(NFI_DEBUG_CON1_REG16));
    printk("ECC_ENCCON_REG16	  :%x\n",*ECC_ENCCON_REG16	    );
    printk("ECC_ENCCNFG_REG32  	:%x\n",*ECC_ENCCNFG_REG32  	);
    printk("ECC_ENCDIADDR_REG32	:%x\n",*ECC_ENCDIADDR_REG32	);
    printk("ECC_ENCIDLE_REG32  	:%x\n",*ECC_ENCIDLE_REG32  	);
    printk("ECC_ENCPAR0_REG32   :%x\n",*ECC_ENCPAR0_REG32    );
    printk("ECC_ENCPAR1_REG32   :%x\n",*ECC_ENCPAR1_REG32    );
    printk("ECC_ENCPAR2_REG32   :%x\n",*ECC_ENCPAR2_REG32    );
    printk("ECC_ENCPAR3_REG32   :%x\n",*ECC_ENCPAR3_REG32    );
    printk("ECC_ENCPAR4_REG32   :%x\n",*ECC_ENCPAR4_REG32    );
    printk("ECC_ENCPAR5_REG32   :%x\n",*ECC_ENCPAR5_REG32    );
    printk("ECC_ENCPAR6_REG32   :%x\n",*ECC_ENCPAR6_REG32    );
    printk("ECC_ENCSTA_REG32    :%x\n",*ECC_ENCSTA_REG32     );
    printk("ECC_ENCIRQEN_REG16  :%x\n",*ECC_ENCIRQEN_REG16   );
    printk("ECC_ENCIRQSTA_REG16 :%x\n",*ECC_ENCIRQSTA_REG16  ); 
    printk("ECC_DECCON_REG16    :%x\n",*ECC_DECCON_REG16     );
    printk("ECC_DECCNFG_REG32   :%x\n",*ECC_DECCNFG_REG32    );
    printk("ECC_DECDIADDR_REG32 :%x\n",*ECC_DECDIADDR_REG32  );
    printk("ECC_DECIDLE_REG16   :%x\n",*ECC_DECIDLE_REG16    );
    printk("ECC_DECFER_REG16    :%x\n",*ECC_DECFER_REG16     );
    printk("ECC_DECENUM0_REG32  :%x\n",*ECC_DECENUM0_REG32   );
    printk("ECC_DECENUM1_REG32  :%x\n",*ECC_DECENUM1_REG32   );
    printk("ECC_DECDONE_REG16   :%x\n",*ECC_DECDONE_REG16    );
    printk("ECC_DECEL0_REG32    :%x\n",*ECC_DECEL0_REG32     );
    printk("ECC_DECEL1_REG32    :%x\n",*ECC_DECEL1_REG32     );
    printk("ECC_DECEL2_REG32    :%x\n",*ECC_DECEL2_REG32     );
    printk("ECC_DECEL3_REG32    :%x\n",*ECC_DECEL3_REG32     );
    printk("ECC_DECEL4_REG32    :%x\n",*ECC_DECEL4_REG32     );
    printk("ECC_DECEL5_REG32    :%x\n",*ECC_DECEL5_REG32     );
    printk("ECC_DECEL6_REG32    :%x\n",*ECC_DECEL6_REG32     );
    printk("ECC_DECEL7_REG32    :%x\n",*ECC_DECEL7_REG32     );
    printk("ECC_DECIRQEN_REG16  :%x\n",*ECC_DECIRQEN_REG16   );
    printk("ECC_DECIRQSTA_REG16 :%x\n",*ECC_DECIRQSTA_REG16  );
    printk("ECC_DECFSM_REG32    :%x\n",*ECC_DECFSM_REG32     );
    printk("ECC_BYPASS_REG32    :%x\n",*ECC_BYPASS_REG32     );
    printk("NFI clock : %s\n", (DRV_Reg32((volatile u32 *)(PERICFG_BASE+0x18)) & (0x1)) ? "Clock Disabled" : "Clock Enabled");
    printk("NFI clock SEL (MT8127):0x%x: %s\n",(PERICFG_BASE+0x5C), (DRV_Reg32((volatile u32 *)(PERICFG_BASE+0x5C)) & (0x1)) ? "Half clock" : "Quarter clock");  	
#endif
}
void dump_data(const char* ptr, int len)
{
	int i;
	printk("------------------[dump_data]-----------------------\n");
	for (i=0; i < len ; i++)
	{
		printk("%x ", ptr[i]);
		if ((i%16)==15)
			printk("\n");
	}	
	printk("-------------------------------------------------\n");	
}	
u8 NFI_DMA_status(void)
{
    return g_running_dma;
}

u32 NFI_DMA_address(void)
{
    return DRV_Reg32(NFI_STRADDR_REG32);
}

EXPORT_SYMBOL(NFI_DMA_status);
EXPORT_SYMBOL(NFI_DMA_address);
#if __INTERNAL_USE_AHB_MODE__
#if !defined (CONFIG_NAND_BOOTLOADER)
u32 nand_virt_to_phys_add(u32 va)
{
    u32 pageOffset = (va & (PAGE_SIZE - 1));
    pgd_t *pgd;
    pmd_t *pmd;
    pte_t *pte;
    u32 pa;

    if (virt_addr_valid(va))
    {
        return __virt_to_phys(va);
    }

    if (NULL == current)
    {
        printk(KERN_ERR "[nand_virt_to_phys_add] ERROR ,current is NULL! \n");
        return 0;
    }

    if (NULL == current->mm)
    {
        printk(KERN_ERR "[nand_virt_to_phys_add] ERROR current->mm is NULL! tgid=0x%x, name=%s \n", current->tgid, current->comm);
        return 0;
    }

    pgd = pgd_offset(current->mm, va);  /* what is tsk->mm */
    if (pgd_none(*pgd) || pgd_bad(*pgd))
    {
        printk(KERN_ERR "[nand_virt_to_phys_add] ERROR, va=0x%x, pgd invalid! \n", va);
        return 0;
    }

    pmd = pmd_offset((pud_t *)pgd, va);
    if (pmd_none(*pmd) || pmd_bad(*pmd))
    {
        printk(KERN_ERR "[nand_virt_to_phys_add] ERROR, va=0x%x, pmd invalid! \n", va);
        return 0;
    }

    pte = pte_offset_map(pmd, va);
    if (pte_present(*pte))
    {
        pa = (pte_val(*pte) & (PAGE_MASK)) | pageOffset;
        return pa;
    }

    printk(KERN_ERR "[nand_virt_to_phys_add] ERROR va=0x%x, pte invalid! \n", va);
    return 0;
}
EXPORT_SYMBOL(nand_virt_to_phys_add);
#endif
#endif

bool get_device_info(u8*id, flashdev_info *devinfo)
{
    u32 i,m,n,mismatch;
    int target=-1;
    u8 target_id_len=0;
		MSG(INIT, "flash_number: 0x%x ,then print the flashtable \n",flash_number);
    for (i = 0; i<flash_number; i++){
		//MSG(INIT, "gen_FlashTable_p[%d].devciename: %s \n",i,gen_FlashTable_p[i].devciename);
		mismatch=0;
		for(m=0;m<gen_FlashTable_p[i].id_length;m++){
			if(id[m]!=gen_FlashTable_p[i].id[m]){
				mismatch=1;
				break;
			}
		}
		if(mismatch == 0 && gen_FlashTable_p[i].id_length > target_id_len){
				target=i;
				target_id_len=gen_FlashTable_p[i].id_length;
		}
    }

    if(target != -1){
		MSG(INIT, "Recognize NAND: ID [");
		for(n=0;n<gen_FlashTable_p[target].id_length;n++){
			devinfo->id[n] = gen_FlashTable_p[target].id[n];
			MSG(INIT, "%x ",devinfo->id[n]);
		}
		MSG(INIT, "], Device Name [%s], Page Size [%d]B Spare Size [%d]B Total Size [%d]MB\n",gen_FlashTable_p[target].devciename,gen_FlashTable_p[target].pagesize,gen_FlashTable_p[target].sparesize,gen_FlashTable_p[target].totalsize);
		devinfo->id_length=gen_FlashTable_p[target].id_length;
		devinfo->blocksize = gen_FlashTable_p[target].blocksize;
		devinfo->addr_cycle = gen_FlashTable_p[target].addr_cycle;
		devinfo->iowidth = gen_FlashTable_p[target].iowidth;
		devinfo->timmingsetting = gen_FlashTable_p[target].timmingsetting;
		devinfo->advancedmode = gen_FlashTable_p[target].advancedmode;
		devinfo->pagesize = gen_FlashTable_p[target].pagesize;
		devinfo->sparesize = gen_FlashTable_p[target].sparesize;
		devinfo->totalsize = gen_FlashTable_p[target].totalsize;
		devinfo->sectorsize = gen_FlashTable_p[target].sectorsize;
		devinfo->s_acccon= gen_FlashTable_p[target].s_acccon;
		devinfo->s_acccon1= gen_FlashTable_p[target].s_acccon1;
		devinfo->freq= gen_FlashTable_p[target].freq;
		devinfo->vendor = gen_FlashTable_p[target].vendor;
		//devinfo->ttarget = gen_FlashTable[target].ttarget;
		memcpy((u8*)&devinfo->feature_set, (u8*)&gen_FlashTable_p[target].feature_set, sizeof(struct MLC_feature_set));
		memcpy(devinfo->devciename, gen_FlashTable_p[target].devciename, sizeof(devinfo->devciename));
    	return true;
	}else{
	    MSG(INIT, "Not Found NAND: ID [");
		for(n=0;n<NAND_MAX_ID;n++){
			MSG(INIT, "%x ",id[n]);
		}
		MSG(INIT, "]\n");
        return false;
	}
}
#ifdef DUMP_NATIVE_BACKTRACE
#define NFI_NATIVE_LOG_SD    "/sdcard/NFI_native_log_%s-%02d-%02d-%02d_%02d-%02d-%02d.log"
#define NFI_NATIVE_LOG_DATA "/data/NFI_native_log_%s-%02d-%02d-%02d_%02d-%02d-%02d.log"
static int nfi_flush_log(char *s)
{
    mm_segment_t old_fs;
    struct rtc_time tm;
    struct timeval tv = { 0 };
    struct file *filp = NULL;
    char name[256];
    unsigned int re = 0;
    int data_write = 0;

    do_gettimeofday(&tv);
    rtc_time_to_tm(tv.tv_sec, &tm);
    memset(name, 0, sizeof(name));
    sprintf(name, NFI_NATIVE_LOG_DATA, s, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    old_fs = get_fs();
    set_fs(KERNEL_DS);
    filp = filp_open(name, O_WRONLY | O_CREAT, 0777);
    if (IS_ERR(filp))
    {
        printk("[NFI_flush_log]error create file in %s, IS_ERR:%ld, PTR_ERR:%ld\n", name, IS_ERR(filp), PTR_ERR(filp));
        memset(name, 0, sizeof(name));
        sprintf(name, NFI_NATIVE_LOG_SD, s, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        filp = filp_open(name, O_WRONLY | O_CREAT, 0777);
        if (IS_ERR(filp))
        {
            printk("[NFI_flush_log]error create file in %s, IS_ERR:%ld, PTR_ERR:%ld\n", name, IS_ERR(filp), PTR_ERR(filp));
            set_fs(old_fs);
            return -1;
        }
    }
    printk("[NFI_flush_log]log file:%s\n", name);
    set_fs(old_fs);

    if (!(filp->f_op) || !(filp->f_op->write))
    {
        printk("[NFI_flush_log] No operation\n");
        re = -1;
        goto ClOSE_FILE;
    }

    DumpNativeInfo();
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    data_write = vfs_write(filp, (char __user *)NativeInfo, strlen(NativeInfo), &filp->f_pos);
    if (!data_write)
    {
        printk("[nfi_flush_log] write fail\n");
        re = -1;
    }
    set_fs(old_fs);

  ClOSE_FILE:
    if (filp)
    {
        filp_close(filp, current->files);
        filp = NULL;
    }
    return re;
}
#endif

static bool mtk_nand_reset(void);
extern u64 part_get_startaddress(u64 byte_address,u32* idx);
extern bool raw_partition(u32 index);
u32 mtk_nand_page_transform(struct mtd_info *mtd, struct nand_chip *chip, u32 page, u32* blk, u32* map_blk)
{
	u32 block_size = 1 <<(chip->phys_erase_shift);
	u32 page_size = (1<<chip->page_shift);
	loff_t start_address;
	u32 idx;
    u32 block;
    u32 page_in_block;
    u32 mapped_block;
	bool translate = FALSE;

	loff_t logical_address = (loff_t)page*(1<<chip->page_shift);
	//MSG(INIT , "[BEAN]%d, %lld\n",page,logical_address);

	if (MLC_DEVICE==FALSE)
	{	
			block = page/(block_size/page_size);
		mapped_block = get_mapping_block_index(block);
		page_in_block = page % (block_size/page_size);
		//MSG(INIT , "[FULL]0x%x, 0x%x 0x%x 0x%x\n",block,page_in_block,mapped_block, page_in_block+mapped_block*(block_size/page_size));
		*blk = block;
		*map_blk = mapped_block;
		return page_in_block; 
	}
	
	{
		start_address = part_get_startaddress(logical_address,&idx);
		//MSG(INIT , "[start_address]page = 0x%x, start_address=0x%llx\n",page,start_address);
#if defined (CONFIG_NAND_BOOTLOADER)
		if (idx == (PART_MAX_COUNT -1))
		{	
			bBCHRetry = FALSE;
			translate = FALSE;
		}
		else if ((idx >= (part_num-1)) && (part_num > 0))
		{
			bBCHRetry = FALSE;	
			translate = FALSE;//TRUE;
		}
		else
		{		
#endif
			if(raw_partition(idx))
				translate = TRUE;
			else
				translate = FALSE;
#if defined (CONFIG_NAND_BOOTLOADER)
		}
#endif
		//MSG(INIT , "[start_address]page = 0x%x, start_address=0x%llx idx=%d translate=%d\n",page,start_address,idx,translate);		
	}
	if(translate == TRUE)
	{	
		block = (u32)((u32)(start_address >> chip->phys_erase_shift) + (u32)((logical_address-start_address) >> (chip->phys_erase_shift-1)));
		page_in_block = ((u32)((logical_address-start_address) >> chip->page_shift) % ((mtd->erasesize/page_size)/2));
		//MSG(INIT , "[LOW]%d, %d\n",block,page_in_block);
		if((devinfo.vendor != VEND_NONE) && 
           (devinfo.feature_set.ptbl_idx != PPTBL_NOT_SUPPORT))
		{
			//page_in_block = devinfo.feature_set.PairPage[page_in_block];
			page_in_block = functArray[devinfo.feature_set.ptbl_idx](page_in_block);
		}
		
	    mapped_block = get_mapping_block_index(block);
		
		//MSG(INIT , "[page_in_block]block=%d mapped_block=%d, page_in_block=%d (mapped_page=%x)\n",block, mapped_block,page_in_block,page_in_block + mapped_block*(mtd->erasesize/page_size));
		*blk = block;
		*map_blk = mapped_block;
		return page_in_block;
	}
	else		
	{
		block = page/(block_size/page_size);
		mapped_block = get_mapping_block_index(block);
		page_in_block = page % (block_size/page_size);
		//MSG(INIT , "[FULL]0x%x, 0x%x 0x%x 0x%x block_size=%x\n",block,page_in_block,mapped_block, page_in_block+mapped_block*(block_size/page_size),block_size);
		*blk = block;
		*map_blk = mapped_block;
		return page_in_block;
	}
}

bool mtk_nand_IsRawPartition(loff_t logical_address)
{
    u32 idx;
    part_get_startaddress(logical_address,&idx);	
	if(raw_partition(idx))
	{
	    return true;
	}
	else
	{
	    return false;
    }        
}

static int mtk_nand_interface_config(struct mtd_info *mtd)
{
	u32 timeout;
	u32 val;
	u32 acccon1;
	struct gFeatureSet *feature_set = &(devinfo.feature_set.FeatureSet);
	//int clksrc = ARMPLL;
	if(devinfo.iowidth == IO_ONFI || devinfo.iowidth ==IO_TOGGLEDDR || devinfo.iowidth ==IO_TOGGLESDR)
	{
		nand_enable_clock();
		//0:26M   1:182M  2:156M  3:124.8M  4:91M  5:62.4M   6:39M   7:26M
		if(devinfo.freq == 80) // mode 4
		{
				g_iNFI2X_CLKSRC = MSDCPLL; // 156M
		}else if(devinfo.freq == 100) // mode 5
		{
				g_iNFI2X_CLKSRC = MAINPLL; //182M
		}
//reset 
        //printk("[Bean]mode:%d\n", g_iNFI2X_CLKSRC);
		NFI_ISSUE_COMMAND (NAND_CMD_RESET, 0, 0, 0, 0);
        timeout = TIMEOUT_4;
        while (timeout)
          timeout--;
        mtk_nand_reset();
//set feature 	
        //printk("[Interface Config]cmd:0x%X addr:0x%x feature:0x%x\n",
        //feature_set->sfeatureCmd, feature_set->Interface.address, feature_set->Interface.feature);

        //mtk_nand_GetFeature(mtd, feature_set->gfeatureCmd, \
		//feature_set->Interface.address, &val,4);
        //printk("[Interface]0x%X\n", val);
		mtk_nand_SetFeature(mtd, (u16) feature_set->sfeatureCmd, \
		feature_set->Interface.address, (u8 *)&feature_set->Interface.feature,\
		sizeof(feature_set->Interface.feature));
        mb();
		NFI_CLN_REG32(NFI_DEBUG_CON1_REG16,HWDCM_SWCON_ON);

//setup register
        mb();
		NFI_CLN_REG32(NFI_DEBUG_CON1_REG16,NFI_BYPASS);
		//clear bypass of ecc
		mb();
		NFI_CLN_REG32(ECC_BYPASS_REG32,ECC_BYPASS);
		mb();
		DRV_WriteReg32(PERICFG_BASE+0x5C, 0x0); // setting default AHB clock
	  //MSG(INIT, "AHB Clock(0x%x)\n",DRV_Reg32(PERICFG_BASE+0x5C));
		mb();	
		NFI_SET_REG32(PERI_NFI_CLK_SOURCE_SEL, NFI_PAD_1X_CLOCK);
		mb();
#if !defined (CONFIG_CLKMGR_BRINGUP) 
		clkmux_sel(MT_MUX_NFI2X,g_iNFI2X_CLKSRC,"NFI");
		mb();
#endif		
		DRV_WriteReg32(NFI_DLYCTRL_REG32, 0x4001);
		DRV_WriteReg32(PERI_NFI_MAC_CTRL, 0x10006);
		while(0 == (DRV_Reg32(NFI_STA_REG32) && STA_FLASH_MACRO_IDLE));
		if(devinfo.iowidth == IO_ONFI)
			DRV_WriteReg16(NFI_NAND_TYPE_CNFG_REG32, 2); //ONFI
		else
			DRV_WriteReg16(NFI_NAND_TYPE_CNFG_REG32, 1); //Toggle
		//printk("[Timing]0x%x 0x%x\n", devinfo.s_acccon, devinfo.s_acccon1);
		acccon1 = DRV_Reg32(NFI_ACCCON1_REG3);
		DRV_WriteReg32(NFI_ACCCON1_REG3,devinfo.s_acccon1);
		DRV_WriteReg32(NFI_ACCCON_REG32,devinfo.s_acccon);
//read back confirm
		mtk_nand_GetFeature(mtd, feature_set->gfeatureCmd, \
		feature_set->Interface.address, (u8 *)&val,4);
        //printk("[Bean]feature is %x\n", val);
		if((val&0xFF) != (feature_set->Interface.feature & 0xFF))
		{
			MSG(INIT, "[%s] fail 0x%X\n",__FUNCTION__,val);
			NFI_ISSUE_COMMAND (NAND_CMD_RESET, 0, 0, 0, 0); //ASYNC
	        timeout = TIMEOUT_4;
	        while (timeout)
	          timeout--;
			mtk_nand_reset();
#if !defined (CONFIG_CLKMGR_BRINGUP)	
			clkmux_sel(MT_MUX_NFI2X, MAINPLL, "NFI"); // 182M
#endif			
			NFI_SET_REG32(NFI_DEBUG_CON1_REG16,NFI_BYPASS);
			NFI_SET_REG32(ECC_BYPASS_REG32,ECC_BYPASS);
			NFI_CLN_REG32(PERI_NFI_CLK_SOURCE_SEL, NFI_PAD_1X_CLOCK);
			DRV_WriteReg32(PERICFG_BASE+0x5C, 0x1); // setting AHB clock
	        //MSG(INIT, "AHB Clock(0x%x)\n",DRV_Reg32(PERICFG_BASE+0x5C));
			DRV_WriteReg32(NFI_ACCCON1_REG3,acccon1);
			DRV_WriteReg32(NFI_ACCCON_REG32,devinfo.timmingsetting);
			DRV_WriteReg16(NFI_NAND_TYPE_CNFG_REG32, 0); //Legacy
			g_bSyncOrToggle = false;
			return 0;
		}
		g_bSyncOrToggle = true;
		
		MSG(INIT, "[%s] success 0x%X\n",__FUNCTION__, devinfo.iowidth);
		//extern void log_boot(char *str);
		//log_boot("[Bean]sync mode success!");	
	}
	else
	{
		g_bSyncOrToggle = false;
		MSG(INIT, "[%s] legacy interface \n",__FUNCTION__);
		return 0;
	}
	
	return 1;
}

#if CFG_RANDOMIZER 
static int mtk_nand_turn_on_randomizer(u32 page, int type, int fgPage)
{
    u32 u4NFI_CFG = 0;
	u32 u4NFI_RAN_CFG = 0;
	u4NFI_CFG = DRV_Reg32(NFI_CNFG_REG16);

	DRV_WriteReg32(NFI_ENMPTY_THRESH_REG32, 40); // empty threshold 40

	if (type) //encode
	{
    	DRV_WriteReg32(NFI_RANDOM_ENSEED01_TS_REG32, 0);
    	DRV_WriteReg32(NFI_RANDOM_ENSEED02_TS_REG32, 0);
    	DRV_WriteReg32(NFI_RANDOM_ENSEED03_TS_REG32, 0);
    	DRV_WriteReg32(NFI_RANDOM_ENSEED04_TS_REG32, 0);
    	DRV_WriteReg32(NFI_RANDOM_ENSEED05_TS_REG32, 0);
    	DRV_WriteReg32(NFI_RANDOM_ENSEED06_TS_REG32, 0);
    }
    else
    {
    	DRV_WriteReg32(NFI_RANDOM_DESEED01_TS_REG32, 0);
    	DRV_WriteReg32(NFI_RANDOM_DESEED02_TS_REG32, 0);
    	DRV_WriteReg32(NFI_RANDOM_DESEED03_TS_REG32, 0);
    	DRV_WriteReg32(NFI_RANDOM_DESEED04_TS_REG32, 0);
    	DRV_WriteReg32(NFI_RANDOM_DESEED05_TS_REG32, 0);
    	DRV_WriteReg32(NFI_RANDOM_DESEED06_TS_REG32, 0);
	}
	u4NFI_CFG |= CNFG_RAN_SEL;
	if(PAGES_PER_BLOCK <= SS_SEED_NUM)
	{
	    if (type)
	    {
		    u4NFI_RAN_CFG |= RAN_CNFG_ENCODE_SEED(SS_RANDOM_SEED[page & (PAGES_PER_BLOCK-1)]) | RAN_CNFG_ENCODE_EN;
		}
		else
		{
		    u4NFI_RAN_CFG |= RAN_CNFG_DECODE_SEED(SS_RANDOM_SEED[page & (PAGES_PER_BLOCK-1)]) | RAN_CNFG_DECODE_EN;
		}
	}
	else
	{
	    if (type)
	    {
		    u4NFI_RAN_CFG |= RAN_CNFG_ENCODE_SEED(SS_RANDOM_SEED[page & (SS_SEED_NUM-1)]) | RAN_CNFG_ENCODE_EN;
        }
        else
        {
		    u4NFI_RAN_CFG |= RAN_CNFG_DECODE_SEED(SS_RANDOM_SEED[page & (SS_SEED_NUM-1)]) | RAN_CNFG_DECODE_EN;
		}
	}
		

	if(fgPage) //reload seed for each page
		u4NFI_CFG &= ~CNFG_RAN_SEC;
	else //reload seed for each sector
		u4NFI_CFG |= CNFG_RAN_SEC;

	DRV_WriteReg32(NFI_CNFG_REG16, u4NFI_CFG);
	DRV_WriteReg32(NFI_RANDOM_CNFG_REG32, u4NFI_RAN_CFG);
    //MSG(INIT, "[K]ran turn on type:%d 0x%x 0x%x\n", type, DRV_Reg32(NFI_RANDOM_CNFG_REG32), page);
	return 0;
}

static bool mtk_nand_israndomizeron(void)
{
	u32   nfi_ran_cnfg = 0;
	nfi_ran_cnfg = DRV_Reg32(NFI_RANDOM_CNFG_REG32);
	if(nfi_ran_cnfg&(RAN_CNFG_ENCODE_EN | RAN_CNFG_DECODE_EN))
		return TRUE;

	return FALSE;
}

static void mtk_nand_turn_off_randomizer(void)
{
	u32 u4NFI_CFG = DRV_Reg32(NFI_CNFG_REG16);
    u4NFI_CFG &= ~CNFG_RAN_SEL;
    u4NFI_CFG &= ~CNFG_RAN_SEC;
	DRV_WriteReg32(NFI_RANDOM_CNFG_REG32, 0);
	DRV_WriteReg32(NFI_CNFG_REG16, u4NFI_CFG);
	//MSG(INIT, "[K]ran turn off\n");
}
#else
#define mtk_nand_israndomizeron() (FALSE)
#define mtk_nand_turn_on_randomizer(page, type, fgPage)
#define mtk_nand_turn_off_randomizer()
#endif


/******************************************************************************
 * mtk_nand_irq_handler
 *
 * DESCRIPTION:
 *   NAND interrupt handler!
 *
 * PARAMETERS:
 *   int irq
 *   void *dev_id
 *
 * RETURNS:
 *   IRQ_HANDLED : Successfully handle the IRQ
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
/* Modified for TCM used */
#if defined (CONFIG_NAND_BOOTLOADER)
#else
static irqreturn_t mtk_nand_irq_handler(int irqno, void *dev_id)
{
    u16 u16IntStatus = DRV_Reg16(NFI_INTR_REG16);
    (void)irqno;

    if (u16IntStatus & (u16) INTR_AHB_DONE_EN)
    {
        complete(&g_comp_AHB_Done);
    }
    return IRQ_HANDLED;
}
#endif
/******************************************************************************
 * ECC_Config
 *
 * DESCRIPTION:
 *   Configure HW ECC!
 *
 * PARAMETERS:
 *   struct mtk_nand_host_hw *hw
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void ECC_Config(struct mtk_nand_host_hw *hw,u32 ecc_bit)
{
    u32 u4ENCODESize;
    u32 u4DECODESize;
    u32 ecc_bit_cfg = ECC_CNFG_ECC4;

    switch(ecc_bit){
//#ifndef MTK_COMBO_NAND_SUPPORT
#if 1
	case 4:
  		ecc_bit_cfg = ECC_CNFG_ECC4;
  		break;
  	case 8:
  		ecc_bit_cfg = ECC_CNFG_ECC8;
  		break;
  	case 10:
  		ecc_bit_cfg = ECC_CNFG_ECC10;
  		break;
  	case 12:
  		ecc_bit_cfg = ECC_CNFG_ECC12;
  		break;
	case 14:
  		ecc_bit_cfg = ECC_CNFG_ECC14;
  		break;
	case 16:
  		ecc_bit_cfg = ECC_CNFG_ECC16;
  		break;
	case 18:
  		ecc_bit_cfg = ECC_CNFG_ECC18;
  		break;
	case 20:
  		ecc_bit_cfg = ECC_CNFG_ECC20;
  		break;
	case 22:
  		ecc_bit_cfg = ECC_CNFG_ECC22;
  		break;
	case 24:
  		ecc_bit_cfg = ECC_CNFG_ECC24;
  		break;
 #endif
	case 28:
  		ecc_bit_cfg = ECC_CNFG_ECC28;
  		break;
	case 32:
  		ecc_bit_cfg = ECC_CNFG_ECC32;
  		break;
	case 36:
  		ecc_bit_cfg = ECC_CNFG_ECC36;
  		break;
	case 40:
  		ecc_bit_cfg = ECC_CNFG_ECC40;
  		break;
	case 44:
  		ecc_bit_cfg = ECC_CNFG_ECC44;
  		break;
	case 48:
  		ecc_bit_cfg = ECC_CNFG_ECC48;
  		break;
	case 52:
  		ecc_bit_cfg = ECC_CNFG_ECC52;
  		break;
	case 56:
  		ecc_bit_cfg = ECC_CNFG_ECC56;
  		break;
	case 60:
  		ecc_bit_cfg = ECC_CNFG_ECC60;
  		break;
        default:
  			break;

    }
    DRV_WriteReg16(ECC_DECCON_REG16, DEC_DE);
    do
    {;
    }
    while (!DRV_Reg16(ECC_DECIDLE_REG16));

    DRV_WriteReg16(ECC_ENCCON_REG16, ENC_DE);
    do
    {;
    }
    while (!DRV_Reg32(ECC_ENCIDLE_REG32));

    /* setup FDM register base */
//    DRV_WriteReg32(ECC_FDMADDR_REG32, NFI_FDM0L_REG32);

    /* Sector + FDM */
    u4ENCODESize = (hw->nand_sec_size + 8) << 3;
    /* Sector + FDM + YAFFS2 meta data bits */
    u4DECODESize = ((hw->nand_sec_size + 8) << 3) + ecc_bit * ECC_PARITY_BIT;

    /* configure ECC decoder && encoder */
    DRV_WriteReg32(ECC_DECCNFG_REG32, ecc_bit_cfg | DEC_CNFG_NFI | DEC_CNFG_EMPTY_EN | (u4DECODESize << DEC_CNFG_CODE_SHIFT));

    DRV_WriteReg32(ECC_ENCCNFG_REG32, ecc_bit_cfg | ENC_CNFG_NFI | (u4ENCODESize << ENC_CNFG_MSG_SHIFT));
#ifndef MANUAL_CORRECT
    NFI_SET_REG32(ECC_DECCNFG_REG32, DEC_CNFG_CORRECT);
#else
    NFI_SET_REG32(ECC_DECCNFG_REG32, DEC_CNFG_EL);
#endif
}

/******************************************************************************
 * ECC_Decode_Start
 *
 * DESCRIPTION:
 *   HW ECC Decode Start !
 *
 * PARAMETERS:
 *   None
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void ECC_Decode_Start(void)
{
    /* wait for device returning idle */
    while (!(DRV_Reg16(ECC_DECIDLE_REG16) & DEC_IDLE)) ;
    DRV_WriteReg16(ECC_DECCON_REG16, DEC_EN);
}

/******************************************************************************
 * ECC_Decode_End
 *
 * DESCRIPTION:
 *   HW ECC Decode End !
 *
 * PARAMETERS:
 *   None
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void ECC_Decode_End(void)
{
    /* wait for device returning idle */
    while (!(DRV_Reg16(ECC_DECIDLE_REG16) & DEC_IDLE)) ;
    DRV_WriteReg16(ECC_DECCON_REG16, DEC_DE);
}

/******************************************************************************
 * ECC_Encode_Start
 *
 * DESCRIPTION:
 *   HW ECC Encode Start !
 *
 * PARAMETERS:
 *   None
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void ECC_Encode_Start(void)
{
    /* wait for device returning idle */
    while (!(DRV_Reg32(ECC_ENCIDLE_REG32) & ENC_IDLE)) ;
    mb();
    DRV_WriteReg16(ECC_ENCCON_REG16, ENC_EN);
}

/******************************************************************************
 * ECC_Encode_End
 *
 * DESCRIPTION:
 *   HW ECC Encode End !
 *
 * PARAMETERS:
 *   None
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void ECC_Encode_End(void)
{
    /* wait for device returning idle */
    while (!(DRV_Reg32(ECC_ENCIDLE_REG32) & ENC_IDLE)) ;
    mb();
    DRV_WriteReg16(ECC_ENCCON_REG16, ENC_DE);
}
#if 0
static bool is_empty_page(u8 * spare_buf, u32 sec_num){
	u32 i=0;
	bool is_empty=true;
#if 0
	for(i=0;i<sec_num*8;i++){
		if(spare_buf[i]!=0xFF){
			is_empty=false;
			break;
		}
	}
	printk("\n");
#else
	for(i=0;i<OOB_INDEX_SIZE;i++){
		//xlog_printk(ANDROID_LOG_INFO,"NFI", "flag byte: %x ",spare_buf[OOB_INDEX_OFFSET+i] );
		switch(g_page_size)
		{
    	case 2048:
            if(spare_buf[13+i] !=0xFF){
    			is_empty=false;
    			//break;
    		}
    		break;
		default:
    		if(spare_buf[OOB_INDEX_OFFSET+i] !=0xFF){
    			is_empty=false;
    			//break;
    		}
		}
		if(!is_empty)
		    break;
	}
#endif
	xlog_printk(ANDROID_LOG_INFO,"NFI", "This page is %s!\n",is_empty?"empty":"occupied");
	return is_empty;
}
static bool return_fake_buf(u8 * data_buf, u32 page_size, u32 sec_num,u32 u4PageAddr){
	u32 i=0,j=0;
	u32 sec_zero_count=0;
	u8 t=0;
	u8 *p=data_buf;
	bool ret=true;
	for(j=0;j<sec_num;j++){
		p=data_buf+j*host->hw->nand_sec_size;
		sec_zero_count=0;
		for(i=0;i<host->hw->nand_sec_size;i++){
			t=p[i];
			t=~t;
			t=((t&0xaa)>>1) + (t&0x55);
			t=((t&0xcc)>>2)+(t&0x33);
			t=((t&0xf0f0)>>4)+(t&0x0f0f);
			sec_zero_count+=t;
			if(t>0){
				xlog_printk(ANDROID_LOG_INFO,"NFI", "there is %d bit filp at sector(%d): %d in empty page \n ",t,j,i);
			}
		}
		if(sec_zero_count > 2){
			xlog_printk(ANDROID_LOG_ERROR,"NFI","too many bit filp=%d @ page addr=0x%x, we can not return fake buf\n",sec_zero_count,u4PageAddr);
			ret=false;
		}
	}
	return ret;
}
#endif
/******************************************************************************
 * mtk_nand_check_bch_error
 *
 * DESCRIPTION:
 *   Check BCH error or not !
 *
 * PARAMETERS:
 *   struct mtd_info *mtd
 *	 u8* pDataBuf
 *	 u32 u4SecIndex
 *	 u32 u4PageAddr
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static bool mtk_nand_check_bch_error(struct mtd_info *mtd, u8 * pDataBuf,u8 * spareBuf,u32 u4SecIndex, u32 u4PageAddr, u32* bitmap)
{
    bool ret = true;
    u16 u2SectorDoneMask = 1 << u4SecIndex;
    u32 u4ErrorNumDebug0,u4ErrorNumDebug1, i, u4ErrNum;
    u32 timeout = 0xFFFF;
    u32 correct_count = 0;
    u32 page_size=(u4SecIndex+1)*host->hw->nand_sec_size;
    u32 sec_num=u4SecIndex+1;
    //u32 bitflips = sec_num * 39;
	u16 failed_sec=0;
	u32 maxSectorBitErr = 0;

#ifdef MANUAL_CORRECT
    u32 au4ErrBitLoc[6];
    u32 u4ErrByteLoc, u4BitOffset;
    u32 u4ErrBitLoc1th, u4ErrBitLoc2nd;
#endif
    while (0 == (u2SectorDoneMask & DRV_Reg16(ECC_DECDONE_REG16)))
    {
        timeout--;
        if (0 == timeout)
        {
            return false;
        }
    }
#ifndef MANUAL_CORRECT
    if(0 == (DRV_Reg32(NFI_STA_REG32) & STA_READ_EMPTY))
    {
        u4ErrorNumDebug0 = DRV_Reg32(ECC_DECENUM0_REG32);
        u4ErrorNumDebug1 = DRV_Reg32(ECC_DECENUM1_REG32);
	if (0 != (u4ErrorNumDebug0 & 0xFFFFFFFF) || 0 != (u4ErrorNumDebug1 & 0xFFFFFFFF))
        {
            for (i = 0; i <= u4SecIndex; ++i)
            {
#if 1
				u4ErrNum = (DRV_Reg32((ECC_DECENUM0_REG32+(i/4)))>>((i%4)*8))& ERR_NUM0;
#else
                if (i < 4)
                {
	                u4ErrNum = DRV_Reg32(ECC_DECENUM0_REG32) >> (i * 8);
                } else
                {
	                u4ErrNum = DRV_Reg32(ECC_DECENUM1_REG32) >> ((i - 4) * 8);
                }
	        	u4ErrNum &= ERR_NUM0;	      	
#endif
				if (ERR_NUM0 == u4ErrNum)
                {
	            	failed_sec++;
                    ret = false;
                    printf("UnCorrectable ECC errors at PageAddr=%d, Sector=%d\n", u4PageAddr, i);
                } else
                {

                	*bitmap |= 1 << u4SecIndex;
		    		if (u4ErrNum)
                    {
                    		if(maxSectorBitErr < u4ErrNum)
								maxSectorBitErr = u4ErrNum;
			                correct_count += u4ErrNum;
                        printf("In kernel Correct %d ECC error(s) at PageAddr=%d, Sector=%d\n", u4ErrNum, u4PageAddr, i);
		    }
                }
            }
            if(ret == false){
				if(failed_sec == (u4SecIndex+1))
				{

    				if(0 != (DRV_Reg32(NFI_STA_REG32) & STA_READ_EMPTY))
    				{
    					ret=true;
    					MSG(INIT,"NFI empty page have few filped bit(s) , fake buffer returned\n");
    					memset(pDataBuf,0xff,page_size);
    					memset(spareBuf,0xff,sec_num*8);
    					failed_sec=0;
    					maxSectorBitErr = 0;
    				}
	    	    }
	    	}
	    	mtd->ecc_stats.failed+=failed_sec;
            if (maxSectorBitErr > ecc_threshold)
            {
            	MSG(INIT,"ECC bit flips (0x%x) exceed eccthreshold (0x%x)\n",maxSectorBitErr,ecc_threshold);
                mtd->ecc_stats.corrected++;
            } else
            {
            	printf("Less than %d bit error, ignore\n",ecc_threshold);
                //xlog_printk(ANDROID_LOG_WARN,"NFI",  "Less than 39 bit error, ignore\n");
            }
        }
    }

#else
    /* We will manually correct the error bits in the last sector, not all the sectors of the page! */
    memset(au4ErrBitLoc, 0x0, sizeof(au4ErrBitLoc));
    u4ErrorNumDebug0 = DRV_Reg32(ECC_DECENUM0_REG32);
    u4ErrorNumDebug1 = DRV_Reg32(ECC_DECENUM1_REG32);
	u4ErrNum = (DRV_Reg32((ECC_DECENUM0_REG32+(u4SecIndex/4)))>>((u4SecIndex%4)*8))& ERR_NUM0;

    if (u4ErrNum)
    {
        if (ERR_NUM0 == u4ErrNum)
        {
            mtd->ecc_stats.failed++;
            ret = false;
            printk(KERN_ERR"UnCorrectable at PageAddr=%d\n", u4PageAddr);
        } else
        {
            for (i = 0; i < ((u4ErrNum + 1) >> 1); ++i)
            {
                au4ErrBitLoc[i] = DRV_Reg32(ECC_DECEL0_REG32 + i);
                u4ErrBitLoc1th = au4ErrBitLoc[i] & 0x3FFF;

                if (u4ErrBitLoc1th < 0x1000)
                {
                    u4ErrByteLoc = u4ErrBitLoc1th / 8;
                    u4BitOffset = u4ErrBitLoc1th % 8;
                    pDataBuf[u4ErrByteLoc] = pDataBuf[u4ErrByteLoc] ^ (1 << u4BitOffset);
                    mtd->ecc_stats.corrected++;
                } else
                {
                    mtd->ecc_stats.failed++;
                    printk(KERN_ERR"UnCorrectable ErrLoc=%d\n", au4ErrBitLoc[i]);
                }
                u4ErrBitLoc2nd = (au4ErrBitLoc[i] >> 16) & 0x3FFF;
                if (0 != u4ErrBitLoc2nd)
                {
                    if (u4ErrBitLoc2nd < 0x1000)
                    {
                        u4ErrByteLoc = u4ErrBitLoc2nd / 8;
                        u4BitOffset = u4ErrBitLoc2nd % 8;
                        pDataBuf[u4ErrByteLoc] = pDataBuf[u4ErrByteLoc] ^ (1 << u4BitOffset);
                        mtd->ecc_stats.corrected++;
                    } else
                    {
                        mtd->ecc_stats.failed++;
                        printk(KERN_ERR"UnCorrectable High ErrLoc=%d\n", au4ErrBitLoc[i]);
                    }
                }
            }
        }
        if (0 == (DRV_Reg16(ECC_DECFER_REG16) & (1 << u4SecIndex)))
        {
            ret = false;
        }
    }
#endif
    return ret;
}

/******************************************************************************
 * mtk_nand_RFIFOValidSize
 *
 * DESCRIPTION:
 *   Check the Read FIFO data bytes !
 *
 * PARAMETERS:
 *   u16 u2Size
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static bool mtk_nand_RFIFOValidSize(u16 u2Size)
{
    u32 timeout = 0xFFFF;
    while (FIFO_RD_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) < u2Size)
    {
        timeout--;
        if (0 == timeout)
        {
            return false;
        }
    }
    return true;
}

/******************************************************************************
 * mtk_nand_WFIFOValidSize
 *
 * DESCRIPTION:
 *   Check the Write FIFO data bytes !
 *
 * PARAMETERS:
 *   u16 u2Size
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static bool mtk_nand_WFIFOValidSize(u16 u2Size)
{
    u32 timeout = 0xFFFF;
    while (FIFO_WR_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) > u2Size)
    {
        timeout--;
        if (0 == timeout)
        {
            return false;
        }
    }
    return true;
}

/******************************************************************************
 * mtk_nand_status_ready
 *
 * DESCRIPTION:
 *   Indicate the NAND device is ready or not !
 *
 * PARAMETERS:
 *   u32 u4Status
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static bool mtk_nand_status_ready(u32 u4Status)
{
    u32 timeout = 0xFFFF;
    while ((DRV_Reg32(NFI_STA_REG32) & u4Status) != 0)
    {
        timeout--;
        if (0 == timeout)
        {
            return false;
        }
    }
    return true;
}

/******************************************************************************
 * mtk_nand_reset
 *
 * DESCRIPTION:
 *   Reset the NAND device hardware component !
 *
 * PARAMETERS:
 *   struct mtk_nand_host *host (Initial setting data)
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static bool mtk_nand_reset(void)
{
    // HW recommended reset flow
    int timeout = 0xFFFF;
    if (DRV_Reg16(NFI_MASTERSTA_REG16) & 0xFFF) // master is busy
    {
        mb();
        DRV_WriteReg32(NFI_CON_REG16, CON_FIFO_FLUSH | CON_NFI_RST);
        while (DRV_Reg16(NFI_MASTERSTA_REG16) & 0xFFF)
        {
            timeout--;
            if (!timeout)
            {
                MSG(INIT, "Wait for NFI_MASTERSTA timeout\n");
            }
        }
    }
    /* issue reset operation */
    mb();
    DRV_WriteReg32(NFI_CON_REG16, CON_FIFO_FLUSH | CON_NFI_RST);

    return mtk_nand_status_ready(STA_NFI_FSM_MASK | STA_NAND_BUSY) && mtk_nand_RFIFOValidSize(0) && mtk_nand_WFIFOValidSize(0);
}

/******************************************************************************
 * mtk_nand_set_mode
 *
 * DESCRIPTION:
 *    Set the oepration mode !
 *
 * PARAMETERS:
 *   u16 u2OpMode (read/write)
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void mtk_nand_set_mode(u16 u2OpMode)
{
    u16 u2Mode = DRV_Reg16(NFI_CNFG_REG16);
    u2Mode &= ~CNFG_OP_MODE_MASK;
    u2Mode |= u2OpMode;
    DRV_WriteReg16(NFI_CNFG_REG16, u2Mode);
}

/******************************************************************************
 * mtk_nand_set_autoformat
 *
 * DESCRIPTION:
 *    Enable/Disable hardware autoformat !
 *
 * PARAMETERS:
 *   bool bEnable (Enable/Disable)
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void mtk_nand_set_autoformat(bool bEnable)
{
    if (bEnable)
    {
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AUTO_FMT_EN);
    } else
    {
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AUTO_FMT_EN);
    }
}

/******************************************************************************
 * mtk_nand_configure_fdm
 *
 * DESCRIPTION:
 *   Configure the FDM data size !
 *
 * PARAMETERS:
 *   u16 u2FDMSize
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void mtk_nand_configure_fdm(u16 u2FDMSize)
{
    NFI_CLN_REG16(NFI_PAGEFMT_REG16, PAGEFMT_FDM_MASK | PAGEFMT_FDM_ECC_MASK);
    NFI_SET_REG16(NFI_PAGEFMT_REG16, u2FDMSize << PAGEFMT_FDM_SHIFT);
    NFI_SET_REG16(NFI_PAGEFMT_REG16, u2FDMSize << PAGEFMT_FDM_ECC_SHIFT);
}


static bool mtk_nand_pio_ready(void)
{
    int count = 0;
    while (!(DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1))
    {
        count++;
        if (count > 0xffff)
        {
            printk("PIO_DIRDY timeout\n");
            return false;
        }
    }

    return true;
}

/******************************************************************************
 * mtk_nand_set_command
 *
 * DESCRIPTION:
 *    Send hardware commands to NAND devices !
 *
 * PARAMETERS:
 *   u16 command
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static bool mtk_nand_set_command(u16 command)
{
    /* Write command to device */
    mb();
    DRV_WriteReg16(NFI_CMD_REG16, command);
    return mtk_nand_status_ready(STA_CMD_STATE);
}

/******************************************************************************
 * mtk_nand_set_address
 *
 * DESCRIPTION:
 *    Set the hardware address register !
 *
 * PARAMETERS:
 *   struct nand_chip *nand, u32 u4RowAddr
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static bool mtk_nand_set_address(u32 u4ColAddr, u32 u4RowAddr, u16 u2ColNOB, u16 u2RowNOB)
{
    /* fill cycle addr */
    mb();
    DRV_WriteReg32(NFI_COLADDR_REG32, u4ColAddr);
    DRV_WriteReg32(NFI_ROWADDR_REG32, u4RowAddr);
    DRV_WriteReg16(NFI_ADDRNOB_REG16, u2ColNOB | (u2RowNOB << ADDR_ROW_NOB_SHIFT));
    //if (part_num > 0)
    //	printk("COL=%X,ROW=%X\n",u4ColAddr,u4RowAddr);
    return mtk_nand_status_ready(STA_ADDR_STATE);
}

//-------------------------------------------------------------------------------
static bool mtk_nand_device_reset(void)
{
	u32 timeout = 0xFFFF;

	mtk_nand_reset();
	
	DRV_WriteReg(NFI_CNFG_REG16, CNFG_OP_RESET);
	
	mtk_nand_set_command(NAND_CMD_RESET);
	
	while(!(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY_RETURN) && (timeout--));

	if(!timeout)
		return FALSE;
	else
		return TRUE;
}
//-------------------------------------------------------------------------------

/******************************************************************************
 * mtk_nand_check_RW_count
 *
 * DESCRIPTION:
 *    Check the RW how many sectors !
 *
 * PARAMETERS:
 *   u16 u2WriteSize
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static bool mtk_nand_check_RW_count(u16 u2WriteSize)
{
    u32 timeout = 0xFFFF;
    u16 u2SecNum = u2WriteSize >> host->hw->nand_sec_shift;

    while (ADDRCNTR_CNTR(DRV_Reg32(NFI_ADDRCNTR_REG16)) < u2SecNum)
    {
        timeout--;
        if (0 == timeout)
        {
            printk(KERN_INFO "[%s] timeout\n", __FUNCTION__);
            return false;
        }
    }
    return true;
}

/******************************************************************************
 * mtk_nand_ready_for_read
 *
 * DESCRIPTION:
 *    Prepare hardware environment for read !
 *
 * PARAMETERS:
 *   struct nand_chip *nand, u32 u4RowAddr
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static bool mtk_nand_ready_for_read(struct nand_chip *nand, u32 u4RowAddr, u32 u4ColAddr, u16 sec_num, bool full, u8 * buf)
{
    /* Reset NFI HW internal state machine and flush NFI in/out FIFO */
    bool bRet = false;
    //u16 sec_num = 1 << (nand->page_shift - host->hw->nand_sec_shift);
    u32 col_addr = u4ColAddr;
    u32 colnob = 2, rownob = devinfo.addr_cycle - 2;
	//u32 reg_val = DRV_Reg32(NFI_MASTERRST_REG32);
#if __INTERNAL_USE_AHB_MODE__
    u32 phys = 0;
#endif
#if CFG_PERFLOG_DEBUG
    struct timeval stimer,etimer;
    do_gettimeofday(&stimer);
#endif
	if(DRV_Reg32(NFI_NAND_TYPE_CNFG_REG32)&0x3)
	{
		NFI_SET_REG16(NFI_MASTERRST_REG32, PAD_MACRO_RST);//reset
		NFI_CLN_REG16(NFI_MASTERRST_REG32, PAD_MACRO_RST);//dereset
	}

    if (nand->options & NAND_BUSWIDTH_16)
        col_addr /= 2;

    if (!mtk_nand_reset())
    {
        goto cleanup;
    }
    if (g_bHwEcc)
    {
        /* Enable HW ECC */
        NFI_SET_REG32(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
    } else
    {
        NFI_CLN_REG32(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
    }

    mtk_nand_set_mode(CNFG_OP_READ);
    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
    DRV_WriteReg32(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

    if (full)
    {
#if __INTERNAL_USE_AHB_MODE__
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
#if defined (CONFIG_NAND_BOOTLOADER)
				phys = buf;//((~(0x1<<31))&(u32)buf);
#else        
        phys = nand_virt_to_phys_add((u32) buf);
#endif
        if (!phys)
        {
            printk(KERN_ERR "[mtk_nand_ready_for_read]convert virt addr (%x) to phys add (%x)fail!!!", (u32) buf, phys);
            return false;
        } else
        {
            DRV_WriteReg32(NFI_STRADDR_REG32, phys);
        }
#else
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
#endif

        if (g_bHwEcc)
        {
            NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
        } else
        {
            NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
        }

    } else
    {
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
    }

    mtk_nand_set_autoformat(full);
    if (full)
    {
        if (g_bHwEcc)
        {
            ECC_Decode_Start();
        }
    }
    if (!mtk_nand_set_command(NAND_CMD_READ0))
    {
        goto cleanup;
    }
    if (!mtk_nand_set_address(col_addr, u4RowAddr, colnob, rownob))
    {
        goto cleanup;
    }

    if (!mtk_nand_set_command(NAND_CMD_READSTART))
    {
        goto cleanup;
    }

    if (!mtk_nand_status_ready(STA_NAND_BUSY))
    {
        goto cleanup;
    }

    bRet = true;

  cleanup:
 #if CFG_PERFLOG_DEBUG
      do_gettimeofday(&etimer);
      g_NandPerfLog.ReadBusyTotalTime+= Cal_timediff(&etimer,&stimer);
      g_NandPerfLog.ReadBusyCount++;
 #endif
    return bRet;
}

/******************************************************************************
 * mtk_nand_ready_for_write
 *
 * DESCRIPTION:
 *    Prepare hardware environment for write !
 *
 * PARAMETERS:
 *   struct nand_chip *nand, u32 u4RowAddr
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static bool mtk_nand_ready_for_write(struct nand_chip *nand, u32 u4RowAddr, u32 col_addr, bool full, u8 * buf)
{
    bool bRet = false;
    u32 sec_num = 1 << (nand->page_shift - host->hw->nand_sec_shift);
    u32 colnob = 2, rownob = devinfo.addr_cycle - 2;
#if __INTERNAL_USE_AHB_MODE__
    u32 phys = 0;
    //u32 T_phys=0;
#endif
    if (nand->options & NAND_BUSWIDTH_16)
        col_addr /= 2;

    /* Reset NFI HW internal state machine and flush NFI in/out FIFO */
    if (!mtk_nand_reset())
    {
        printk("[Bean]mtk_nand_ready_for_write (mtk_nand_reset) fail!\n");
        return false;
    }

    mtk_nand_set_mode(CNFG_OP_PRGM);

    NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_READ_EN);

    DRV_WriteReg32(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

    if (full)
    {
#if __INTERNAL_USE_AHB_MODE__
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
#if defined (CONFIG_NAND_BOOTLOADER)
		phys = buf;//((~(0x1<<31))&(u32)buf);
#else        
        phys = nand_virt_to_phys_add((u32) buf);
#endif
        //T_phys=__virt_to_phys(buf);
        if (!phys)
        {
            printk(KERN_ERR "[mt65xx_nand_ready_for_write]convert virt addr (%x) to phys add fail!!!", (u32) buf);
            return false;
        } else
        {
            DRV_WriteReg32(NFI_STRADDR_REG32, phys);
        }
#if 0
        if ((T_phys > 0x700000 && T_phys < 0x800000) || (phys > 0x700000 && phys < 0x800000))
        {
            {
                printk("[NFI_WRITE]ERROR: Forbidden AHB address wrong phys address =0x%x , right phys address=0x%x, virt  address= 0x%x (count = %d)\n", T_phys, phys, (u32) buf, g_dump_count++);
                show_stack(NULL, NULL);
            }
            BUG_ON(1);
        }
#endif
#else
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
#endif
        if (g_bHwEcc)
        {
            NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
        } else
        {
            NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
        }
    } else
    {
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
    }

    mtk_nand_set_autoformat(full);

    if (full)
    {
        if (g_bHwEcc)
        {
            ECC_Encode_Start();
        }
    }

    if (!mtk_nand_set_command(NAND_CMD_SEQIN))
    {
        printk("[Bean]mtk_nand_ready_for_write (mtk_nand_set_command) fail!\n");
        goto cleanup;
    }
    //1 FIXED ME: For Any Kind of AddrCycle
    if (!mtk_nand_set_address(col_addr, u4RowAddr, colnob, rownob))
    {
        printk("[Bean]mtk_nand_ready_for_write (mtk_nand_set_address) fail!\n");
        goto cleanup;
    }

    if (!mtk_nand_status_ready(STA_NAND_BUSY))
    {
        printk("[Bean]mtk_nand_ready_for_write (mtk_nand_status_ready) fail!\n");
        goto cleanup;
    }

    bRet = true;
  cleanup:

    return bRet;
}

static bool mtk_nand_check_dececc_done(u32 u4SecNum)
{  
    u32 dec_mask;
#if CFG_PERFLOG_DEBUG
    struct timeval timer_timeout, timer_cur;
	do_gettimeofday(&timer_timeout);
	timer_timeout.tv_usec += 800 * 1000;   // 500ms
    if (timer_timeout.tv_usec >= 1000000)     // 1 second
    {
        timer_timeout.tv_usec -= 1000000;
        timer_timeout.tv_sec += 1;
    }
#else
		unsigned int nTry = 5;        
#endif        
    dec_mask = (1 << (u4SecNum - 1));
	udelay(10);
    while (dec_mask != (DRV_Reg(ECC_DECDONE_REG16) & dec_mask))
    {
#if CFG_PERFLOG_DEBUG    	
        do_gettimeofday(&timer_cur);
 		if (timeval_compare(&timer_cur, &timer_timeout) >= 0)
	    {
	        MSG(INIT, "ECC_DECDONE: timeout 0x%x %d\n",DRV_Reg(ECC_DECDONE_REG16),u4SecNum);
			dump_nfi();
        	return false;
	    }
#else
		mdelay(100);
		nTry--;
		if (nTry==0)
		{
			MSG(INIT, "ECC_DECDONE: timeout1 0x%x %d\n",DRV_Reg(ECC_DECDONE_REG16),u4SecNum);
			dump_nfi();
			return false;
		}	
#endif	              
	}
	nTry = 5;
	while ((DRV_Reg32(ECC_DECFSM_REG32)&(0x7f0f0f0f)) != ECC_DECFSM_IDLE)
	{
#if CFG_PERFLOG_DEBUG    	
        do_gettimeofday(&timer_cur);
 		if (timeval_compare(&timer_cur, &timer_timeout) >= 0)
	    {
	        MSG(INIT, "ECC_DECDONE: timeout2 0x%x %d\n",DRV_Reg(ECC_DECDONE_REG16),u4SecNum);
			dump_nfi();
        	return false;
	    }
#else
		mdelay(100);
		nTry--;
		if (nTry==0)
		{
			MSG(INIT, "ECC_DECDONE: timeout2 0x%x %d\n",DRV_Reg(ECC_DECDONE_REG16),u4SecNum);
			dump_nfi();
			return false;
		}		
#endif	              
	}
    return true;
}

/******************************************************************************
 * mtk_nand_read_page_data
 *
 * DESCRIPTION:
 *   Fill the page data into buffer !
 *
 * PARAMETERS:
 *   u8* pDataBuf, u32 u4Size
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
#if (__INTERNAL_USE_AHB_MODE__)
static bool mtk_nand_dma_read_data(struct mtd_info *mtd, u8 * buf, u32 length)
{
	int interrupt_en = g_i4Interrupt;
    int timeout = 0xfffff;
#if !defined (CONFIG_NAND_BOOTLOADER)    
    struct scatterlist sg;
    enum dma_data_direction dir = DMA_FROM_DEVICE;
#endif    
#if CFG_PERFLOG_DEBUG
    struct timeval stimer,etimer;
    do_gettimeofday(&stimer);
#endif
#if !defined (CONFIG_NAND_BOOTLOADER)
    sg_init_one(&sg, buf, length);
    dma_map_sg(&(mtd->dev), &sg, 1, dir);
#endif
    NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
    //DRV_WriteReg32(NFI_STRADDR_REG32, ((~(0x1<<31))&((u32)buf)));

    if ((unsigned int)buf % 16) // TODO: can not use AHB mode here
    {
        printk(KERN_INFO "Un-16-aligned address\n");
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
    } else
    {
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
    }

	DRV_Reg16(NFI_INTR_REG16);
	DRV_WriteReg16(NFI_INTR_EN_REG16, INTR_AHB_DONE_EN);
#if !defined (CONFIG_NAND_BOOTLOADER)
    if (interrupt_en)
    {
        init_completion(&g_comp_AHB_Done);
    }
#endif    
    //dmac_inv_range(pDataBuf, pDataBuf + u4Size);
    mb();
    NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BRD);
    g_running_dma = 1;
#if !defined (CONFIG_NAND_BOOTLOADER)
    if (interrupt_en)
    {
        // Wait 10ms for AHB done
        if (!wait_for_completion_timeout(&g_comp_AHB_Done, 50))
        {
            MSG(INIT, "wait for completion timeout happened @ [%s]: %d\n", __FUNCTION__, __LINE__);
            dump_nfi();
            g_running_dma = 0;
            return false;
        }
        g_running_dma = 0;
        while ((length >> host->hw->nand_sec_shift) > ((DRV_Reg32(NFI_BYTELEN_REG16) & 0x1f000) >> 12))
        {
            timeout--;
            if (0 == timeout)
            {
                printk(KERN_ERR "[%s] poll BYTELEN error\n", __FUNCTION__);
                g_running_dma = 0;
                return false;   //4  // AHB Mode Time Out!
            }
        }
    } else
#endif    	
    {
#if defined (CONFIG_NAND_BOOTLOADER)
    	unsigned long reg;

    	timeout = 0xffff;
        while (1)
        {
			udelay(500);
        	reg = DRV_Reg16(NFI_INTR_REG16);
    		if (reg & (u16) INTR_AHB_DONE)
    		{
    			break;
    		}	
            timeout--;
            if (0 == timeout)
            {
                printk(KERN_ERR "[%s] poll nfi_intr error\n", __FUNCTION__);
                dump_nfi();
                g_running_dma = 0;
                return false;   //4  // AHB Mode Time Out!
            }
        }
        timeout = 0xffff;
#endif
        g_running_dma = 0;
        while ((length >> host->hw->nand_sec_shift) > ((DRV_Reg32(NFI_BYTELEN_REG16) & 0x1f000) >> 12))
        {
            timeout--;
            if (0 == timeout)
            {
                printk(KERN_ERR "[%s] poll BYTELEN=%d error\n", __FUNCTION__,DRV_Reg32(NFI_BYTELEN_REG16));
                dump_nfi();
                g_running_dma = 0;
                return false;   //4  // AHB Mode Time Out!
            }
        }
    }
#if !defined (CONFIG_NAND_BOOTLOADER)
    dma_unmap_sg(&(mtd->dev), &sg, 1, dir);
#endif    
#if CFG_PERFLOG_DEBUG
    do_gettimeofday(&etimer);
    g_NandPerfLog.ReadDMATotalTime+= Cal_timediff(&etimer,&stimer);
    g_NandPerfLog.ReadDMACount++;
#endif
    return true;
}
#endif
static bool mtk_nand_mcu_read_data(u8 * buf, u32 length)
{
    int timeout = 0xffff;
    u32 i;
    u32 *buf32 = (u32 *) buf;
#ifdef TESTTIME
    unsigned long long time1, time2;
    time1 = sched_clock();
#endif
    if ((u32) buf % 4 || length % 4)
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
    else
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);

    //DRV_WriteReg32(NFI_STRADDR_REG32, 0);
    mb();
    NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BRD);
		
    if ((u32) buf % 4 || length % 4)
    {
        for (i = 0; (i < (length)) && (timeout > 0);)
        {
            //if (FIFO_RD_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) >= 4)
            if (DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1)
            {
                *buf++ = (u8) DRV_Reg32(NFI_DATAR_REG32);
                i++;
            } else
            {
                timeout--;
            }
            if (0 == timeout)
            {
                printk(KERN_ERR "[%s] timeout\n", __FUNCTION__);
                dump_nfi();
                return false;
            }
        }
    } else
    {
        for (i = 0; (i < (length >> 2)) && (timeout > 0);)
        {
            //if (FIFO_RD_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) >= 4)
            if (DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1)
            {
                *buf32++ = DRV_Reg32(NFI_DATAR_REG32);
                i++;
            } else
            {
                timeout--;
            }
            if (0 == timeout)
            {
                printk(KERN_ERR "[%s] timeout\n", __FUNCTION__);
                dump_nfi();
                return false;
            }
        }
    }
#ifdef TESTTIME
    time2 = sched_clock() - time1;
    if (!readdatatime)
    {
        readdatatime = (time2);
    }
#endif
    return true;
}

static bool mtk_nand_read_page_data(struct mtd_info *mtd, u8 * pDataBuf, u32 u4Size)
{
#if (__INTERNAL_USE_AHB_MODE__)
    return mtk_nand_dma_read_data(mtd, pDataBuf, u4Size);
#else
#if defined (CONFIG_NAND_BOOTLOADER) 
    return mtk_nand_mcu_read_data(pDataBuf, u4Size);
#else
return mtk_nand_mcu_read_data(mtd, pDataBuf, u4Size);
#endif
#endif
}

/******************************************************************************
 * mtk_nand_write_page_data
 *
 * DESCRIPTION:
 *   Fill the page data into buffer !
 *
 * PARAMETERS:
 *   u8* pDataBuf, u32 u4Size
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
#if (__INTERNAL_USE_AHB_MODE__)
static bool mtk_nand_dma_write_data(struct mtd_info *mtd, u8 * pDataBuf, u32 u4Size)
{
    int i4Interrupt = g_i4Interrupt;
    u32 timeout = 0xFFFF;
#if !defined (CONFIG_NAND_BOOTLOADER)    
    struct scatterlist sg;
    enum dma_data_direction dir = DMA_TO_DEVICE;
#endif    
#if CFG_PERFLOG_DEBUG
    struct timeval stimer,etimer;
    do_gettimeofday(&stimer);
#endif
#if !defined (CONFIG_NAND_BOOTLOADER)    
    sg_init_one(&sg, pDataBuf, u4Size);
    dma_map_sg(&(mtd->dev), &sg, 1, dir);
#endif
    NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
    DRV_Reg16(NFI_INTR_REG16);
    DRV_WriteReg16(NFI_INTR_EN_REG16, 0);
    // DRV_WriteReg32(NFI_STRADDR_REG32, (u32*)virt_to_phys(pDataBuf));

    if ((unsigned int)pDataBuf % 16)    // TODO: can not use AHB mode here
    {
        printk(KERN_INFO "Un-16-aligned address\n");
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
    } else
    {
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
    }

    if (i4Interrupt)
    {
#if !defined (CONFIG_NAND_BOOTLOADER)    	
        init_completion(&g_comp_AHB_Done);
#endif
        DRV_Reg16(NFI_INTR_REG16);
        DRV_WriteReg16(NFI_INTR_EN_REG16, INTR_AHB_DONE_EN);
    }
    //dmac_clean_range(pDataBuf, pDataBuf + u4Size);
    mb();
    NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BWR);
    g_running_dma = 3;
#if !defined (CONFIG_NAND_BOOTLOADER)    
    if (i4Interrupt)
    {
        // Wait 10ms for AHB done
        if (!wait_for_completion_timeout(&g_comp_AHB_Done, 10))
        {
            MSG(READ, "wait for completion timeout happened @ [%s]: %d\n", __FUNCTION__, __LINE__);
            dump_nfi();
            g_running_dma = 0;
            return false;
        }
        g_running_dma = 0;
        // wait_for_completion(&g_comp_AHB_Done);
    } else
#endif    	
    {
#if defined (CONFIG_NAND_BOOTLOADER)
        unsigned long reg;
    	timeout = 0xffff;
        while (1)
        {
			udelay(200);
        	reg = DRV_Reg16(NFI_INTR_REG16);
    		if (reg & (u16) INTR_AHB_DONE)
    		{
    			break;
    		}	
    
        	timeout--;
            if (0 == timeout)
            {
                printk(KERN_ERR "[%s] poll nfi_intr error\n", __FUNCTION__);
                dump_nfi();
                g_running_dma = 0;
                return false;   //4  // AHB Mode Time Out!
            }

        }

        timeout = 0xffff;
        
        g_running_dma = 0;
#endif
        while ((u4Size >> host->hw->nand_sec_shift) > ((DRV_Reg32(NFI_BYTELEN_REG16) & 0x1f000) >> 12))
        {
            timeout--;
            if (0 == timeout)
            {
                printk(KERN_ERR "[%s] poll BYTELEN error\n", __FUNCTION__);
                g_running_dma = 0;
                return false;   //4  // AHB Mode Time Out!
            }
        }
        g_running_dma = 0;
    }
#if !defined (CONFIG_NAND_BOOTLOADER)   
    dma_unmap_sg(&(mtd->dev), &sg, 1, dir);
#endif    
#if CFG_PERFLOG_DEBUG
    do_gettimeofday(&etimer);
    g_NandPerfLog.WriteDMATotalTime+= Cal_timediff(&etimer,&stimer);
    g_NandPerfLog.WriteDMACount++;
#endif
    return true;
}
#endif
static bool mtk_nand_mcu_write_data(struct mtd_info *mtd, const u8 * buf, u32 length)
{
    u32 timeout = 0xFFFF;
    u32 i;
    u32 *pBuf32;
    NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
    mb();
    NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BWR);
    pBuf32 = (u32 *) buf;

    if ((u32) buf % 4 || length % 4)
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
    else
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);

    if ((u32) buf % 4 || length % 4)
    {
        for (i = 0; (i < (length)) && (timeout > 0);)
        {
            if (DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1)
            {
                DRV_WriteReg32(NFI_DATAW_REG32, *buf++);
                i++;
            } else
            {
                timeout--;
            }
            if (0 == timeout)
            {
                printk(KERN_ERR "[%s] timeout\n", __FUNCTION__);
                dump_nfi();
                return false;
            }
        }
    } else
    {
        for (i = 0; (i < (length >> 2)) && (timeout > 0);)
        {
            // if (FIFO_WR_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) <= 12)
            if (DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1)
            {
                DRV_WriteReg32(NFI_DATAW_REG32, *pBuf32++);
                i++;
            } else
            {
                timeout--;
            }
            if (0 == timeout)
            {
                printk(KERN_ERR "[%s] timeout\n", __FUNCTION__);
                dump_nfi();
                return false;
            }
        }
    }

    return true;
}

static bool mtk_nand_write_page_data(struct mtd_info *mtd, u8 * buf, u32 size)
{
#if (__INTERNAL_USE_AHB_MODE__)
    return mtk_nand_dma_write_data(mtd, buf, size);
#else
    return mtk_nand_mcu_write_data(mtd, buf, size);
#endif
}

/******************************************************************************
 * mtk_nand_read_fdm_data
 *
 * DESCRIPTION:
 *   Read a fdm data !
 *
 * PARAMETERS:
 *   u8* pDataBuf, u32 u4SecNum
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void mtk_nand_read_fdm_data(u8 * pDataBuf, u32 u4SecNum)
{
    u32 i;
    u32 *pBuf32 = (u32 *) pDataBuf;

    if (pBuf32)
    {
        for (i = 0; i < u4SecNum; ++i)
        {
            *pBuf32++ = DRV_Reg32(NFI_FDM0L_REG32 + (i << 1));
            *pBuf32++ = DRV_Reg32(NFI_FDM0M_REG32 + (i << 1));
            //*pBuf32++ = DRV_Reg32((u32)NFI_FDM0L_REG32 + (i<<3));
            //*pBuf32++ = DRV_Reg32((u32)NFI_FDM0M_REG32 + (i<<3));
        }
    }
}

/******************************************************************************
 * mtk_nand_write_fdm_data
 *
 * DESCRIPTION:
 *   Write a fdm data !
 *
 * PARAMETERS:
 *   u8* pDataBuf, u32 u4SecNum
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static __attribute__((aligned(64))) u8 fdm_buf[128];
static void mtk_nand_write_fdm_data(struct nand_chip *chip, u8 * pDataBuf, u32 u4SecNum)
{
    u32 i, j;
    u8 checksum = 0;
    bool empty = true;
    struct nand_oobfree *free_entry;
    u32 *pBuf32;

    memcpy(fdm_buf, pDataBuf, u4SecNum * 8);

    free_entry = chip->ecc.layout->oobfree;
    for (i = 0; i < MTD_MAX_OOBFREE_ENTRIES && free_entry[i].length; i++)
    {
        for (j = 0; j < free_entry[i].length; j++)
        {
            if (pDataBuf[free_entry[i].offset + j] != 0xFF)
                empty = false;
            checksum ^= pDataBuf[free_entry[i].offset + j];
        }
    }

    if (!empty)
    {
        fdm_buf[free_entry[i - 1].offset + free_entry[i - 1].length] = checksum;
    }

    pBuf32 = (u32 *) fdm_buf;
    for (i = 0; i < u4SecNum; ++i)
    {
        DRV_WriteReg32(NFI_FDM0L_REG32 + (i << 1), *pBuf32++);
        DRV_WriteReg32(NFI_FDM0M_REG32 + (i << 1), *pBuf32++);
        //DRV_WriteReg32((u32)NFI_FDM0L_REG32 + (i<<3), *pBuf32++);
        //DRV_WriteReg32((u32)NFI_FDM0M_REG32 + (i<<3), *pBuf32++);
    }
}

/******************************************************************************
 * mtk_nand_stop_read
 *
 * DESCRIPTION:
 *   Stop read operation !
 *
 * PARAMETERS:
 *   None
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void mtk_nand_stop_read(void)
{
    NFI_CLN_REG32(NFI_CON_REG16, CON_NFI_BRD);
    mtk_nand_reset();
    if (g_bHwEcc)
    {
        ECC_Decode_End();
    }
    DRV_WriteReg16(NFI_INTR_EN_REG16, 0);
}

/******************************************************************************
 * mtk_nand_stop_write
 *
 * DESCRIPTION:
 *   Stop write operation !
 *
 * PARAMETERS:
 *   None
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void mtk_nand_stop_write(void)
{
    NFI_CLN_REG32(NFI_CON_REG16, CON_NFI_BWR);
    if (g_bHwEcc)
    {
        ECC_Encode_End();
    }
    DRV_WriteReg16(NFI_INTR_EN_REG16, 0);
}

//---------------------------------------------------------------------------
#define STATUS_READY			(0x40)
#define STATUS_FAIL				(0x01)
#define STATUS_WR_ALLOW			(0x80)

static bool mtk_nand_read_status(void)
{
    int status = 0;//, i;
    unsigned int timeout;

    mtk_nand_reset();

    /* Disable HW ECC */
    NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);

    /* Disable 16-bit I/O */
    NFI_CLN_REG16(NFI_PAGEFMT_REG16, PAGEFMT_DBYTE_EN);
    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_OP_SRD | CNFG_READ_EN | CNFG_BYTE_RW);

    DRV_WriteReg32(NFI_CON_REG16, CON_NFI_SRD | (1 << CON_NFI_NOB_SHIFT));

    DRV_WriteReg32(NFI_CON_REG16, 0x3);
    mtk_nand_set_mode(CNFG_OP_SRD);
    DRV_WriteReg16(NFI_CNFG_REG16, 0x2042);
   	mtk_nand_set_command(NAND_CMD_STATUS);
    DRV_WriteReg32(NFI_CON_REG16, 0x90);

    timeout = TIMEOUT_4;
    WAIT_NFI_PIO_READY(timeout);

    if (timeout)
    {
        status = (DRV_Reg16(NFI_DATAR_REG32));
    }
    //~  clear NOB
    DRV_WriteReg32(NFI_CON_REG16, 0);

    if (devinfo.iowidth == 16)
    {
        NFI_SET_REG16(NFI_PAGEFMT_REG16, PAGEFMT_DBYTE_EN);
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
    }
    // check READY/BUSY status first
    if (!(STATUS_READY & status))
    {
        //MSG(ERR, "status is not ready\n");
    }
    // flash is ready now, check status code
    if (STATUS_FAIL & status)
    {
        if (!(STATUS_WR_ALLOW & status))
        {
            //MSG(INIT, "status locked\n");
            return FALSE;
        } else
        {
            //MSG(INIT, "status unknown\n");
            return FALSE;
        }
    } else
    {
        return TRUE;
    }
}

bool mtk_nand_SetFeature(struct mtd_info *mtd, u16 cmd, u32 addr, u8 *value,  u8 bytes)
{
	u16           reg_val     	 = 0;
	u8            write_count     = 0;
	u32           reg = 0;
	u32           timeout=TIMEOUT_3;//0xffff;
//	u32           status;
//	struct nand_chip *chip = (struct nand_chip *)mtd->priv;

	mtk_nand_reset();

    reg = DRV_Reg32(NFI_NAND_TYPE_CNFG_REG32);
    if (!(reg&TYPE_SLC))
        bytes <<= 1;

	reg_val |= (CNFG_OP_CUST | CNFG_BYTE_RW);
	DRV_WriteReg(NFI_CNFG_REG16, reg_val);

	mtk_nand_set_command(cmd);
	mtk_nand_set_address(addr, 0, 1, 0);

	mtk_nand_status_ready(STA_NFI_OP_MASK);

	DRV_WriteReg32(NFI_CON_REG16, 1 << CON_NFI_SEC_SHIFT);
	NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BWR);
	DRV_WriteReg(NFI_STRDATA_REG16, 0x1);
	//printk("Bytes=%d\n", bytes);
	while ( (write_count < bytes) && timeout )
    {
    	WAIT_NFI_PIO_READY(timeout)
        if(timeout == 0)
        {
            break;
        }
        if (reg&TYPE_SLC)
        {
            //printk("VALUE1:0x%2X\n", *value);
            DRV_WriteReg8(NFI_DATAW_REG32, *value++);
        }else if(write_count % 2)
        {
            //printk("VALUE2:0x%2X\n", *value);
            DRV_WriteReg8(NFI_DATAW_REG32, *value++);
        }
        else
        {
            //printk("VALUE3:0x%2X\n", *value);
            DRV_WriteReg8(NFI_DATAW_REG32, *value);
        }
        write_count++;
        timeout = TIMEOUT_3;
    }
	*NFI_CNRNB_REG16 = 0x81;
	if (!mtk_nand_status_ready(STA_NAND_BUSY_RETURN))
    {
        return FALSE;
    }
	
	//mtk_nand_read_status();
	//if(status& 0x1)
	//	return FALSE;
	return TRUE;
}

bool mtk_nand_GetFeature(struct mtd_info *mtd, u16 cmd, u32 addr, u8 *value,  u8 bytes)
{
	u16           reg_val     	 = 0;
	u8            read_count     = 0;
	u32           timeout=TIMEOUT_3;//0xffff;
//	struct nand_chip *chip = (struct nand_chip *)mtd->priv;

	mtk_nand_reset();

	reg_val |= (CNFG_OP_CUST | CNFG_BYTE_RW | CNFG_READ_EN);
	DRV_WriteReg(NFI_CNFG_REG16, reg_val);

	mtk_nand_set_command(cmd);
	mtk_nand_set_address(addr, 0, 1, 0);
	mtk_nand_status_ready(STA_NFI_OP_MASK);
	*NFI_CNRNB_REG16 = 0x81;
	mtk_nand_status_ready(STA_NAND_BUSY_RETURN);

	//DRV_WriteReg32(NFI_CON_REG16, 0 << CON_NFI_SEC_SHIFT);
	reg_val = DRV_Reg32(NFI_CON_REG16);
    reg_val &= ~CON_NFI_NOB_MASK;
    reg_val |= ((4 << CON_NFI_NOB_SHIFT)|CON_NFI_SRD);
    DRV_WriteReg32(NFI_CON_REG16, reg_val);
	DRV_WriteReg(NFI_STRDATA_REG16, 0x1);
	//bytes = 20;
	while ( (read_count < bytes) && timeout )
    {
    	WAIT_NFI_PIO_READY(timeout)
        if(timeout == 0)
        {
            break;
        }
        *value++ = DRV_Reg8(NFI_DATAR_REG32);
		//printk("Value[0x%02X]\n", DRV_Reg8(NFI_DATAR_REG32));
        read_count++;
        timeout = TIMEOUT_3;
    }
//	chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);
	//mtk_nand_read_status();
	if(timeout != 0)
		return TRUE;
	else
		return FALSE;

}

#if 1
const u8 addr_tbl[8][5] =
{
	{0x04, 0x05, 0x06, 0x07, 0x0D},
	{0x04, 0x05, 0x06, 0x07, 0x0D},
	{0x04, 0x05, 0x06, 0x07, 0x0D},
	{0x04, 0x05, 0x06, 0x07, 0x0D},
	{0x04, 0x05, 0x06, 0x07, 0x0D},
	{0x04, 0x05, 0x06, 0x07, 0x0D},
	{0x04, 0x05, 0x06, 0x07, 0x0D},
	{0x04, 0x05, 0x06, 0x07, 0x0D}
};

const u8 data_tbl[8][5] =
{
	{0x04, 0x04, 0x7C, 0x7E, 0x00},
	{0x00, 0x7C, 0x78, 0x78, 0x00},
	{0x7C, 0x76, 0x74, 0x72, 0x00},
	{0x08, 0x08, 0x00, 0x00, 0x00},
	{0x0B, 0x7E, 0x76, 0x74, 0x00},
	{0x10, 0x76, 0x72, 0x70, 0x00},
	{0x02, 0x7C, 0x7E, 0x70, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00}
};	

static void mtk_nand_modeentry_rrtry(void)
{
	mtk_nand_reset();

	mtk_nand_set_mode(CNFG_OP_CUST);
	
	mtk_nand_set_command(0x5C);
	mtk_nand_set_command(0xC5);

	mtk_nand_status_ready(STA_NFI_OP_MASK);
}

static void mtk_nand_rren_rrtry(bool needB3)
{
	mtk_nand_reset();

	mtk_nand_set_mode(CNFG_OP_CUST);

	if(needB3)
		mtk_nand_set_command(0xB3);	
	mtk_nand_set_command(0x26);
	mtk_nand_set_command(0x5D);

	mtk_nand_status_ready(STA_NFI_OP_MASK);
}

static void mtk_nand_sprmset_rrtry(u32 addr, u32 data) //single parameter setting
{
	u16           reg_val     	 = 0;
	u8            write_count     = 0;
	u32           reg = 0;
	u32           timeout=TIMEOUT_3;//0xffff;

	mtk_nand_reset();

	reg_val |= (CNFG_OP_CUST | CNFG_BYTE_RW);
	DRV_WriteReg(NFI_CNFG_REG16, reg_val);

	mtk_nand_set_command(0x55);
	mtk_nand_set_address(addr, 0, 1, 0);

	mtk_nand_status_ready(STA_NFI_OP_MASK);

	DRV_WriteReg32(NFI_CON_REG16, 1 << CON_NFI_SEC_SHIFT);
	NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BWR);
	DRV_WriteReg(NFI_STRDATA_REG16, 0x1);
	
	
	WAIT_NFI_PIO_READY(timeout);
    
    DRV_WriteReg8(NFI_DATAW_REG32, data);

	while(!(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY_RETURN) && (timeout--));    	
}

static void mtk_nand_toshiba_rrtry(struct mtd_info *mtd,flashdev_info deviceinfo, u32 retryCount, bool defValue)
{
	u32 acccon;

	acccon = DRV_Reg32(NFI_ACCCON_REG32);
	DRV_WriteReg32(NFI_ACCCON_REG32, 0x31C083F9); //to fit read retry timing
	
	if(0 == retryCount)
		mtk_nand_modeentry_rrtry();

	mtk_nand_sprmset_rrtry(addr_tbl[retryCount][0], data_tbl[retryCount][0]);
	mtk_nand_sprmset_rrtry(addr_tbl[retryCount][1], data_tbl[retryCount][1]);
	mtk_nand_sprmset_rrtry(addr_tbl[retryCount][2], data_tbl[retryCount][2]);
	mtk_nand_sprmset_rrtry(addr_tbl[retryCount][3], data_tbl[retryCount][3]);
	mtk_nand_sprmset_rrtry(addr_tbl[retryCount][4], data_tbl[retryCount][4]);

	if(3 == retryCount)
		mtk_nand_rren_rrtry(TRUE);
	else if(6 > retryCount)
		mtk_nand_rren_rrtry(FALSE);

	if(7 == retryCount) // to exit
	{
		mtk_nand_set_mode(CNFG_OP_RESET);
		NFI_ISSUE_COMMAND (NAND_CMD_RESET, 0, 0, 0, 0);
		mtk_nand_reset();
		//should do NAND DEVICE interface change under sync mode  xiaolei 
	}

	DRV_WriteReg32(NFI_ACCCON_REG32, acccon);
}

#endif
static void mtk_nand_micron_rrtry(struct mtd_info *mtd,flashdev_info deviceinfo, u32 feature, bool defValue)
{
	//u32 feature = deviceinfo.feature_set.FeatureSet.readRetryStart+retryCount;
	mtk_nand_SetFeature(mtd, deviceinfo.feature_set.FeatureSet.sfeatureCmd,\
								deviceinfo.feature_set.FeatureSet.readRetryAddress,\
								(u8 *)&feature,4);
}

static void mtk_nand_sandisk_rrtry(struct mtd_info *mtd,flashdev_info deviceinfo, u32 feature, bool defValue)
{
	//u32 feature = deviceinfo.feature_set.FeatureSet.readRetryStart+retryCount;
	if(FALSE == defValue)
	{
	mtk_nand_reset();
	}
	else
	{
		mtk_nand_set_mode(CNFG_OP_RESET);
		NFI_ISSUE_COMMAND (NAND_CMD_RESET, 0, 0, 0, 0);
		mtk_nand_reset();
		//should do NAND DEVICE interface change under sync mode  xiaolei
	}
	
	mtk_nand_SetFeature(mtd, deviceinfo.feature_set.FeatureSet.sfeatureCmd,\
								deviceinfo.feature_set.FeatureSet.readRetryAddress,\
								(u8 *)&feature,4);
	if(FALSE == defValue)
		mtk_nand_set_command(deviceinfo.feature_set.FeatureSet.readRetryPreCmd);
}

#if 1 //sandisk 19nm read retry
u16 sandisk_19nm_rr_table[17] =
{
	0x0000, 
	0xFF0F, 0xEEFE, 0xDDFD, 0x11EE, //04h[7:4] | 07h[7:4] | 04h[3:0] | 05h[7:4]
	0x22ED, 0x33DF, 0xCDDE, 0x01DD,
	0x0211, 0x1222, 0xBD21, 0xAD32,
	0x9DF0, 0xBCEF, 0xACDC, 0x9CFF
};

static void sandisk_19nm_rr_init(void)
{
	u32 		  reg_val		 = 0;
	u32 		   count	  = 0;
	u32 		  timeout = 0xffff;
	u32 u4RandomSetting;
	u32 acccon;

	acccon = DRV_Reg32(NFI_ACCCON_REG32);
	DRV_WriteReg32(NFI_ACCCON_REG32, 0x31C083F9); //to fit read retry timing
	
	mtk_nand_reset();

	if(use_randomizer)
	{
		u4RandomSetting = DRV_Reg32(NFI_RANDOM_CNFG_REG32);
		DRV_WriteReg32(NFI_RANDOM_CNFG_REG32, (u4RandomSetting & (~(RAN_CNFG_ENCODE_EN | RAN_CNFG_DECODE_EN))));
	}
	
	reg_val = (CNFG_OP_CUST | CNFG_BYTE_RW);
	DRV_WriteReg(NFI_CNFG_REG16, reg_val);

	DRV_WriteReg(NFI_CMD_REG16,0x3B);
	  while (DRV_Reg32(NFI_STA_REG32) & STA_CMD_STATE);
	DRV_WriteReg(NFI_CMD_REG16,0xB9);
	  while (DRV_Reg32(NFI_STA_REG32) & STA_CMD_STATE);

	for(count = 0; count < 9; count++)
	{
		DRV_WriteReg(NFI_CMD_REG16,0x53);
		  while (DRV_Reg32(NFI_STA_REG32) & STA_CMD_STATE);	  
		  DRV_WriteReg32(NFI_COLADDR_REG32, (0x04 + count));
		  DRV_WriteReg32(NFI_ROWADDR_REG32, 0);
		  DRV_WriteReg(NFI_ADDRNOB_REG16, 0x1);
		  while (DRV_Reg32(NFI_STA_REG32) & STA_ADDR_STATE);
		DRV_WriteReg(NFI_CON_REG16, (CON_NFI_BWR | (1 << CON_NFI_SEC_SHIFT)));
		DRV_WriteReg(NFI_STRDATA_REG16, 1);
		WAIT_NFI_PIO_READY(timeout);
			DRV_WriteReg32(NFI_DATAW_REG32, 0x00);

		mtk_nand_reset();
	}
	
	DRV_WriteReg32(NFI_ACCCON_REG32, acccon);	
}

static void sandisk_19nm_rr_loading(u32 retryCount, bool defValue)
{
	u32 		  reg_val		 = 0;
	u32 		  timeout = 0xffff;
	u32 u4RandomSetting;
	u32 acccon;

	acccon = DRV_Reg32(NFI_ACCCON_REG32);
	DRV_WriteReg32(NFI_ACCCON_REG32, 0x31C083F9); //to fit read retry timing
	
	mtk_nand_reset();

	if(use_randomizer)
	{
		u4RandomSetting = DRV_Reg32(NFI_RANDOM_CNFG_REG32);
		DRV_WriteReg32(NFI_RANDOM_CNFG_REG32, (u4RandomSetting & (~(RAN_CNFG_ENCODE_EN | RAN_CNFG_DECODE_EN))));
	}
	
	reg_val = (CNFG_OP_CUST | CNFG_BYTE_RW);
	DRV_WriteReg(NFI_CNFG_REG16, reg_val);

	if((0 != retryCount) || defValue)
	{
		DRV_WriteReg(NFI_CMD_REG16,0xD6); //disable rr
	 	while (DRV_Reg32(NFI_STA_REG32) & STA_CMD_STATE);
	}

	DRV_WriteReg(NFI_CMD_REG16,0x3B);
	  while (DRV_Reg32(NFI_STA_REG32) & STA_CMD_STATE);
	DRV_WriteReg(NFI_CMD_REG16,0xB9);
	  while (DRV_Reg32(NFI_STA_REG32) & STA_CMD_STATE);

	DRV_WriteReg(NFI_CMD_REG16,0x53);
	  while (DRV_Reg32(NFI_STA_REG32) & STA_CMD_STATE);	  
	  DRV_WriteReg32(NFI_COLADDR_REG32, 0x04);
	  DRV_WriteReg32(NFI_ROWADDR_REG32, 0);
	  DRV_WriteReg(NFI_ADDRNOB_REG16, 0x1);
	  while (DRV_Reg32(NFI_STA_REG32) & STA_ADDR_STATE);
	DRV_WriteReg(NFI_CON_REG16, (CON_NFI_BWR | (1 << CON_NFI_SEC_SHIFT)));
	DRV_WriteReg(NFI_STRDATA_REG16, 1);
	WAIT_NFI_PIO_READY(timeout);
		DRV_WriteReg32(NFI_DATAW_REG32, (((sandisk_19nm_rr_table[retryCount] & 0xF000) >> 8) | ((sandisk_19nm_rr_table[retryCount] & 0x00F0) >> 4)));

	mtk_nand_reset();

	DRV_WriteReg(NFI_CMD_REG16,0x53);
	  while (DRV_Reg32(NFI_STA_REG32) & STA_CMD_STATE);	  
	  DRV_WriteReg32(NFI_COLADDR_REG32, 0x05);
	  DRV_WriteReg32(NFI_ROWADDR_REG32, 0);
	  DRV_WriteReg(NFI_ADDRNOB_REG16, 0x1);
	  while (DRV_Reg32(NFI_STA_REG32) & STA_ADDR_STATE);
	DRV_WriteReg(NFI_CON_REG16, (CON_NFI_BWR | (1 << CON_NFI_SEC_SHIFT)));
	DRV_WriteReg(NFI_STRDATA_REG16, 1);
	WAIT_NFI_PIO_READY(timeout);
		DRV_WriteReg32(NFI_DATAW_REG32, ((sandisk_19nm_rr_table[retryCount] & 0x000F) << 4));

	mtk_nand_reset();

	DRV_WriteReg(NFI_CMD_REG16,0x53);
	  while (DRV_Reg32(NFI_STA_REG32) & STA_CMD_STATE);	  
	  DRV_WriteReg32(NFI_COLADDR_REG32, 0x07);
	  DRV_WriteReg32(NFI_ROWADDR_REG32, 0);
	  DRV_WriteReg(NFI_ADDRNOB_REG16, 0x1);
	  while (DRV_Reg32(NFI_STA_REG32) & STA_ADDR_STATE);
	DRV_WriteReg(NFI_CON_REG16, (CON_NFI_BWR | (1 << CON_NFI_SEC_SHIFT)));
	DRV_WriteReg(NFI_STRDATA_REG16, 1);
	WAIT_NFI_PIO_READY(timeout);
		DRV_WriteReg32(NFI_DATAW_REG32, ((sandisk_19nm_rr_table[retryCount] & 0x0F00) >> 4));

	if(!defValue)
	{
		DRV_WriteReg(NFI_CMD_REG16,0xB6); //enable rr
		while (DRV_Reg32(NFI_STA_REG32) & STA_CMD_STATE);	  
	}
	
	DRV_WriteReg32(NFI_ACCCON_REG32, acccon);	
}

static void mtk_nand_sandisk_19nm_rrtry(struct mtd_info *mtd,flashdev_info deviceinfo, u32 retryCount, bool defValue)
{
	if((retryCount == 0) && (!defValue))
		sandisk_19nm_rr_init();
	sandisk_19nm_rr_loading(retryCount, defValue);
}
#endif

#define HYNIX_RR_TABLE_SIZE  (1026)  //hynix read retry table size
#define SINGLE_RR_TABLE_SIZE (64)

#define HYNIX_16NM_RR_TABLE_SIZE  (528)  //hynix read retry table size
#define SINGLE_RR_TABLE_16NM_SIZE (32)

u8 nand_hynix_rr_table[(HYNIX_RR_TABLE_SIZE+16)/16*16]; //align as 16 byte

#define NAND_HYX_RR_TBL_BUF nand_hynix_rr_table

u8 real_hynix_rr_table_idx = 0;

static bool hynix_rr_table_select(u8 table_index, flashdev_info *deviceinfo)
{
	u32 i;
	u32 table_size = (deviceinfo->feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM)?SINGLE_RR_TABLE_16NM_SIZE : SINGLE_RR_TABLE_SIZE;
	
	for(i = 0; i < table_size; i++)
	{
	    u8 *temp_rr_table = (u8 *)NAND_HYX_RR_TBL_BUF+table_size*table_index*2+2;
	    u8 *temp_inversed_rr_table = (u8 *)NAND_HYX_RR_TBL_BUF+table_size*table_index*2+table_size+2;
	    if(deviceinfo->feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM)
	    {
            temp_rr_table += 14;
            temp_inversed_rr_table += 14;
	    }
		if(0xFF != (temp_rr_table[i] ^ temp_inversed_rr_table[i]))
			return FALSE; // error table
	}

	return TRUE; // correct table
}

static void HYNIX_RR_TABLE_READ(flashdev_info *deviceinfo)
{
	u32 reg_val = 0;
	u32 read_count = 0, max_count = HYNIX_RR_TABLE_SIZE;
	u32 timeout = 0xffff;
	u8*  rr_table = (u8*)(NAND_HYX_RR_TBL_BUF);
	u8 table_index = 0;
	u8 add_reg1[3] = {0xFF, 0xCC};
	u8 data_reg1[3] = {0x40, 0x4D};
	u8 cmd_reg[6] = {0x16, 0x17, 0x04, 0x19, 0x00};
	u8 add_reg2[6] = {0x00, 0x00, 0x00, 0x02, 0x00};
	bool RR_TABLE_EXIST = TRUE;
	if(deviceinfo->feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM)
	{
        read_count = 1;
        add_reg1[1]= 0x38;
        data_reg1[1] = 0x52;
        max_count = HYNIX_16NM_RR_TABLE_SIZE;
	}
	mtk_nand_device_reset();
	// take care under sync mode. need change nand device inferface xiaolei
		
	mtk_nand_reset();	
	
	DRV_WriteReg(NFI_CNFG_REG16, (CNFG_OP_CUST | CNFG_BYTE_RW));

    mtk_nand_set_command(0x36);

    for(; read_count < 2; read_count++)
    {
        mtk_nand_set_address(add_reg1[read_count],0,1,0);
        DRV_WriteReg(NFI_CON_REG16, (CON_NFI_BWR | (1 << CON_NFI_SEC_SHIFT)));
    	DRV_WriteReg(NFI_STRDATA_REG16, 1);
    	timeout = 0xffff;
    	WAIT_NFI_PIO_READY(timeout);
        DRV_WriteReg32(NFI_DATAW_REG32, data_reg1[read_count]);
    	mtk_nand_reset();
    }

	for(read_count = 0; read_count < 5; read_count++)
	{		
		mtk_nand_set_command(cmd_reg[read_count]);
	}
	for(read_count = 0; read_count < 5; read_count++)
	{
        mtk_nand_set_address(add_reg2[read_count],0,1,0);
	}
	mtk_nand_set_command(0x30);
	DRV_WriteReg(NFI_CNRNB_REG16, 0xF1);
	timeout = 0xffff;
	while(!(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY_RETURN) && (timeout--));

    reg_val = (CNFG_OP_CUST | CNFG_BYTE_RW | CNFG_READ_EN);
    DRV_WriteReg(NFI_CNFG_REG16, reg_val);	  	
    DRV_WriteReg(NFI_CON_REG16, (CON_NFI_BRD | (2<< CON_NFI_SEC_SHIFT)));
    DRV_WriteReg(NFI_STRDATA_REG16, 0x1);
	timeout = 0xffff;
	read_count = 0; // how????
	while ((read_count < max_count) && timeout )
    {
		WAIT_NFI_PIO_READY(timeout);        
		*rr_table++ = (U8)DRV_Reg32(NFI_DATAR_REG32);
        read_count++;
        timeout = 0xFFFF;
    }
	
	mtk_nand_device_reset();
	// take care under sync mode. need change nand device inferface xiaolei

	reg_val = (CNFG_OP_CUST | CNFG_BYTE_RW);
	if(deviceinfo->feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM)
	{
	    DRV_WriteReg(NFI_CNFG_REG16, reg_val);
	    mtk_nand_set_command(0x36);
	    mtk_nand_set_address(0x38,0,1,0);
	    DRV_WriteReg(NFI_CON_REG16, (CON_NFI_BWR | (1 << CON_NFI_SEC_SHIFT)));
	    DRV_WriteReg(NFI_STRDATA_REG16, 1);
	    WAIT_NFI_PIO_READY(timeout);
        DRV_WriteReg32(NFI_DATAW_REG32, 0x00);
        mtk_nand_reset();
        mtk_nand_set_command(0x16);
        mtk_nand_set_command(0x00);
        mtk_nand_set_address(0x00,0,1,0);//dummy read, add don't care
        mtk_nand_set_command(0x30);
	}else
	{
	    DRV_WriteReg(NFI_CNFG_REG16, reg_val);
	    mtk_nand_set_command(0x38);
	}
	timeout = 0xffff;
	while(!(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY_RETURN) && (timeout--));
	rr_table = (u8*)(NAND_HYX_RR_TBL_BUF);
	if(deviceinfo->feature_set.FeatureSet.rtype == RTYPE_HYNIX)
	{
        if((rr_table[0] != 8) || (rr_table[1] != 8))
        {
            RR_TABLE_EXIST = FALSE;
    	    ASSERT(0);
        }
	}
	else if(deviceinfo->feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM)
	{
    	for(read_count=0;read_count<8;read_count++)
    	{
        	if((rr_table[read_count] != 8) || (rr_table[read_count+8] != 4))
        	{
        		RR_TABLE_EXIST = FALSE;
    			break;
        	}
    	}
	}
	if(RR_TABLE_EXIST)
	{
		for(table_index = 0 ;table_index < 8; table_index++)
		{
			if(hynix_rr_table_select(table_index, deviceinfo))
			{
				real_hynix_rr_table_idx = table_index;
				MSG(INIT, "Hynix rr_tbl_id %d\n",real_hynix_rr_table_idx);
				break;
			}
		}
		if(table_index == 8)
		{
			ASSERT(0);
		}
	}
	else
	{
        MSG(INIT, "Hynix RR table index error!\n");
	}
}

static void HYNIX_Set_RR_Para(u32 rr_index, flashdev_info *deviceinfo)
{
	u32           reg_val     	 = 0;
	u32           timeout=0xffff;
	u8 count, max_count = 8;
	u8 add_reg[9] = {0xCC, 0xBF, 0xAA, 0xAB, 0xCD, 0xAD, 0xAE, 0xAF};
	u8 *hynix_rr_table = (u8 *)NAND_HYX_RR_TBL_BUF+SINGLE_RR_TABLE_SIZE*real_hynix_rr_table_idx*2+2;
	if(deviceinfo->feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM)
	{
	    add_reg[0] = 0x38; //0x38, 0x39, 0x3A, 0x3B
	    for(count =1; count < 4; count++)
	    {
            add_reg[count] = add_reg[0] + count; 
	    }
        hynix_rr_table += 14;
		max_count = 4;
	}
	mtk_nand_reset();	
	
	DRV_WriteReg(NFI_CNFG_REG16, (CNFG_OP_CUST | CNFG_BYTE_RW));
    mtk_nand_set_command(0x36);
    
	for(count = 0; count < max_count; count++)
	{
	    mtk_nand_set_address(add_reg[count], 0, 1, 0);
	    DRV_WriteReg(NFI_CON_REG16, (CON_NFI_BWR | (1 << CON_NFI_SEC_SHIFT)));
		DRV_WriteReg(NFI_STRDATA_REG16, 1);
		timeout = 0xffff;
		WAIT_NFI_PIO_READY(timeout);
        DRV_WriteReg32(NFI_DATAW_REG32, hynix_rr_table[rr_index*max_count + count]);
		mtk_nand_reset();
	}
    mtk_nand_set_command(0x16);
}

static void mtk_nand_hynix_rrtry(flashdev_info deviceinfo, u32 retryCount, bool defValue)
{
	HYNIX_Set_RR_Para(retryCount, &deviceinfo);
}	

static void mtk_nand_hynix_16nm_rrtry(flashdev_info deviceinfo, u32 retryCount, bool defValue)
{
	HYNIX_Set_RR_Para(retryCount, &deviceinfo);
}	

u32 special_rrtry_setting[32]= 
{
0x00000000,0x7C00007C,0x04007C78,0x78007874,
0x087C007C,0x007C7C78,0x7C7C7874,0x007C7470,
0x0078007C,0x00787C78,0x00787874,0x00787470,
0x0078706C,0x00040400,0x0004007C,0x0C047C78,
0x0C047874,0x1008007C,0x10080400,0x78747874,
0x78747470,0x7874706C,0x78746C68,0x78707874,
0x78707470,0x78706C68,0x7870706C,0x786C706C,
0x786C6C68,0x786C6864,0x74686C68,0x74686864,
};

static u32 mtk_nand_rrtry_setting(flashdev_info deviceinfo, enum readRetryType type, u32 retryStart, u32 loopNo)
{
	u32 value;
	//if(RTYPE_MICRON == type || RTYPE_SANDISK== type || RTYPE_TOSHIBA== type || RTYPE_HYNIX== type)
	{
		if(retryStart != 0xFFFFFFFF)
		{
			value = retryStart+loopNo;
		}
		else
		{
			value = special_rrtry_setting[loopNo];
		}
	}
	
	return value;
}

typedef u32 (*rrtryFunctionType)(struct mtd_info *mtd,flashdev_info deviceinfo, u32 feature, bool defValue);

static rrtryFunctionType rtyFuncArray[]=
{
	mtk_nand_micron_rrtry,
	mtk_nand_sandisk_rrtry,
	mtk_nand_sandisk_19nm_rrtry,
	mtk_nand_toshiba_rrtry,
	mtk_nand_hynix_rrtry,
	mtk_nand_hynix_16nm_rrtry
};


static void mtk_nand_rrtry_func(struct mtd_info *mtd,flashdev_info deviceinfo, u32 feature, bool defValue)
{
	rtyFuncArray[deviceinfo.feature_set.FeatureSet.rtype](mtd,deviceinfo, feature,defValue);
}

/******************************************************************************
 * mtk_nand_exec_read_page
 *
 * DESCRIPTION:
 *   Read a page data !
 *
 * PARAMETERS:
 *   struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize,
 *   u8* pPageBuf, u8* pFDMBuf
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
int mtk_nand_exec_read_page(struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize, u8 * pPageBuf, u8 * pFDMBuf)
{
    u8 *buf;
    int bRet = ERR_RTN_SUCCESS;
    struct nand_chip *nand = mtd->priv;
    u32 u4SecNum = u4PageSize >> host->hw->nand_sec_shift;
	u32 u4SecSize = host->hw->nand_sec_size;
    u32 backup_corrected, backup_failed;
	bool readRetry = FALSE;
	int retryCount = 0;
	u32 val;
	u32 tempBitMap, bitMap, i;
	u32 feature ;
#ifdef NAND_PFM
    struct timeval pfm_time_read;
#endif
#if 0
    unsigned short PageFmt_Reg = 0;
    unsigned int NAND_ECC_Enc_Reg = 0;
    unsigned int NAND_ECC_Dec_Reg = 0;
#endif
	//MSG(INIT, "mtk_nand_exec_read_page, host->hw->nand_sec_shift: %d\n", host->hw->nand_sec_shift);
	//MSG(INIT, "mtk_nand_exec_read_page,u4RowAddr: 0x%x\n", u4RowAddr);
    PFM_BEGIN(pfm_time_read);
	tempBitMap = 0;
	bitMap = 0;

    if (((u32) pPageBuf % 16) && local_buffer_16_align)
    {
        buf = local_buffer_16_align;
    } else
        buf = pPageBuf;
    backup_corrected = mtd->ecc_stats.corrected;
    backup_failed = mtd->ecc_stats.failed;
#if 0
	{
		val = devinfo.feature_set.FeatureSet.readRetryDefault;
		mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,\
									devinfo.feature_set.FeatureSet.readRetryAddress,\
									(u8 *)&val,4);
		mtk_nand_GetFeature(mtd, devinfo.feature_set.FeatureSet.gfeatureCmd,\
			devinfo.feature_set.FeatureSet.readRetryAddress,\
			(u8 *)&val,4);
		if((val&0xFF) != (devinfo.feature_set.FeatureSet.readRetryDefault&0xFF))
		{
			MSG(INIT, "mtk_nand_exec_read_page check read retry defalut value fail 0x%x\n",val);
		}
    }
#endif

#if CFG_2CS_NAND
    if (g_bTricky_CS)
    {
        u4RowAddr = mtk_nand_cs_on(nand, NFI_TRICKY_CS, u4RowAddr);
    }
#endif

	do{
		if(use_randomizer && u4RowAddr >=  RAND_START_ADDR)
		{	mtk_nand_turn_on_randomizer(u4RowAddr, 0, 0);}
		else if(pre_randomizer && u4RowAddr <  RAND_START_ADDR)
		{	mtk_nand_turn_on_randomizer(u4RowAddr, 0, 0);}
    if (mtk_nand_ready_for_read(nand, u4RowAddr, 0, u4SecNum, true, buf))
    {
        if (!mtk_nand_read_page_data(mtd, buf, u4PageSize))
        {
        	MSG(INIT, "mtk_nand_read_page_data fail\n");
            bRet = ERR_RTN_FAIL;
        }

        if (!mtk_nand_status_ready(STA_NAND_BUSY))
        {
        	MSG(INIT, "mtk_nand_status_ready fail\n");
            bRet = ERR_RTN_FAIL;
        }
        if (g_bHwEcc)
        {
            if (!mtk_nand_check_dececc_done(u4SecNum))
            {
            	MSG(INIT, "mtk_nand_check_dececc_done fail\n");
                bRet = ERR_RTN_FAIL;
            }
        }
        mtk_nand_read_fdm_data(pFDMBuf, u4SecNum);
        if (g_bHwEcc)
        {
            if (!mtk_nand_check_bch_error(mtd, buf, pFDMBuf,u4SecNum - 1, u4RowAddr, &tempBitMap))
            {
				if(devinfo.vendor != VEND_NONE){
					readRetry = TRUE;
				}
            	MSG(INIT, "mtk_nand_check_bch_error fail, retryCount:%d\n",retryCount);
                bRet = ERR_RTN_BCH_FAIL;
#if defined (CONFIG_NAND_BOOTLOADER)
              if (bBCHRetry==false)
								break;
#endif
            }
			else
			{
				if(0 != (DRV_Reg32(NFI_STA_REG32) & STA_READ_EMPTY)) // if empty
				{
					if(retryCount != 0)
					{
						MSG(INIT,"NFI read retry read empty page, return as uncorrectable\n");
						mtd->ecc_stats.failed+=u4SecNum;
						bRet = ERR_RTN_BCH_FAIL;
				    }else
				    {
    				    memset(buf,0xff,u4PageSize);
    		            memset(pFDMBuf,0xff,u4SecNum*8);
    		            readRetry = FALSE;
    		            bRet = ERR_RTN_SUCCESS;
		            }
			    }
            }
        }
        mtk_nand_stop_read();
    }
		if(use_randomizer && u4RowAddr >= RAND_START_ADDR)
		{	mtk_nand_turn_off_randomizer();}
		else if(pre_randomizer && u4RowAddr < RAND_START_ADDR)
		{	mtk_nand_turn_off_randomizer();}
		if (bRet == ERR_RTN_BCH_FAIL)
		{
			tempBitMap -= (tempBitMap&bitMap);
			if(tempBitMap != 0)
			{				
				MSG(INIT, "read retry has partial data correct 0x%x\n",tempBitMap);
				for(i = 0; i < u4SecNum; i++)
				{
					if((tempBitMap & (1 << i)) != 0)
					{	
						memcpy((temp_buffer_16_align+(u4SecSize*i)),(buf+(u4SecSize*i)),u4SecSize);
						memcpy((temp_buffer_16_align+mtd->writesize+(8*i)),(pFDMBuf+(8*i)),8);
					}
				}
				bitMap |= tempBitMap;
			}
			if(bitMap == ((1 << u4SecNum) - 1))
			{
				MSG(INIT, "read retry has reformat the page data correctly @ page 0x%x\n",u4RowAddr);
				memcpy(buf,temp_buffer_16_align,mtd->writesize);
				memcpy(pFDMBuf,(temp_buffer_16_align+mtd->writesize),8*u4SecNum);
				mtd->ecc_stats.corrected++;
				mtd->ecc_stats.failed = backup_failed;
				bRet = ERR_RTN_SUCCESS;
			}
		}
		if (bRet == ERR_RTN_BCH_FAIL)
		{
		    tempBitMap = 0;
			//feature= devinfo.feature_set.FeatureSet.readRetryStart+retryCount;
			feature = mtk_nand_rrtry_setting(devinfo, devinfo.feature_set.FeatureSet.rtype,devinfo.feature_set.FeatureSet.readRetryStart,retryCount);
			if(retryCount < devinfo.feature_set.FeatureSet.readRetryCnt)
			{
				mtd->ecc_stats.corrected = backup_corrected;
    			mtd->ecc_stats.failed = backup_failed;
			  if(MLC_DEVICE)    			
			  {
			    //printk("[Bean]feature=%x\n", feature);
			  	mtk_nand_rrtry_func(mtd,devinfo,feature,FALSE);
				//mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,\
				//				devinfo.feature_set.FeatureSet.readRetryAddress,\
				//				(u8*)&feature,4);
				}
				retryCount++;
			}
			else
			{
				if(MLC_DEVICE)
				{										
				feature = devinfo.feature_set.FeatureSet.readRetryDefault;
				mtk_nand_rrtry_func(mtd,devinfo,feature,TRUE);
				//mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,\
				//				devinfo.feature_set.FeatureSet.readRetryAddress,\
				//				(u8*)&feature,4);
				}
				readRetry = FALSE;
			}		
		}
		else
		{
			if((retryCount != 0) && MLC_DEVICE)
			{
				feature = devinfo.feature_set.FeatureSet.readRetryDefault;
				mtk_nand_rrtry_func(mtd,devinfo,feature,TRUE);
				//mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,\
				//				devinfo.feature_set.FeatureSet.readRetryAddress,\
				//				(u8*)&feature,4);
			}
			readRetry = FALSE;
		}
		if(TRUE == readRetry)
			bRet = ERR_RTN_SUCCESS;
	}while(readRetry);
	if(retryCount != 0)
	{
		feature = devinfo.feature_set.FeatureSet.readRetryDefault;
	    if(bRet == ERR_RTN_SUCCESS)
	    {
	        MSG(INIT, "u4RowAddr:0x%x read retry pass, retrycnt:%d ENUM0:%x,ENUM1:%x,mtd_ecc(A):%x,mtd_ecc(B):%x \n",u4RowAddr,retryCount,DRV_Reg32(ECC_DECENUM1_REG32),DRV_Reg32(ECC_DECENUM0_REG32),mtd->ecc_stats.failed,backup_failed);
	    }
	    else
	    {
    		MSG(INIT, "u4RowAddr:0x%x read retry fail, mtd_ecc(A):%x ,fail, mtd_ecc(B):%x\n",u4RowAddr,mtd->ecc_stats.failed,backup_failed);	    	
	    }
	    if(MLC_DEVICE)
	    {
				mtk_nand_rrtry_func(mtd,devinfo,feature,TRUE);
				//mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,\
				//				devinfo.feature_set.FeatureSet.readRetryAddress,\
				//				(u8*)&feature,4);
		}
	}
	
    if (buf == local_buffer_16_align)
	{
        memcpy(pPageBuf, buf, u4PageSize);
	}
	if(bRet != ERR_RTN_SUCCESS)
	{
		MSG(INIT,"NFI ECC uncorrectable , fake buffer returned\n");
		memset(pPageBuf,0xff,u4PageSize);
		memset(pFDMBuf,0xff,u4SecNum*8);
	}

    PFM_END_R(pfm_time_read, u4PageSize + 32);
	
    return bRet;
}

bool mtk_nand_exec_read_sector(struct mtd_info *mtd, u32 u4RowAddr,  u32 u4ColAddr, u32 u4PageSize, u8 * pPageBuf, u8 * pFDMBuf, int subpageno)
{
    u8 *buf;
    int bRet = ERR_RTN_SUCCESS;
    struct nand_chip *nand = mtd->priv;
    u32 u4SecNum = subpageno;
    u32 backup_corrected, backup_failed;
	bool readRetry = FALSE;
	int retryCount = 0;
	u32 tempBitMap;
#ifdef NAND_PFM
    struct timeval pfm_time_read;
#endif
#if 0
    unsigned short PageFmt_Reg = 0;
    unsigned int NAND_ECC_Enc_Reg = 0;
    unsigned int NAND_ECC_Dec_Reg = 0;
#endif
	//MSG(INIT, "mtk_nand_exec_read_page, host->hw->nand_sec_shift: %d\n", host->hw->nand_sec_shift);

    PFM_BEGIN(pfm_time_read);

    if (((u32) pPageBuf % 16) && local_buffer_16_align)
    {
        buf = local_buffer_16_align;
    } else
        buf = pPageBuf;
  backup_corrected = mtd->ecc_stats.corrected;
  backup_failed = mtd->ecc_stats.failed;
#if CFG_2CS_NAND
    if (g_bTricky_CS)
    {
        u4RowAddr = mtk_nand_cs_on(nand, NFI_TRICKY_CS, u4RowAddr);
    }
#endif
	do{
		if(use_randomizer && u4RowAddr >= RAND_START_ADDR)
		{	mtk_nand_turn_on_randomizer(u4RowAddr, 0, 0);}
		else if(pre_randomizer && u4RowAddr < RAND_START_ADDR)
		{	mtk_nand_turn_on_randomizer(u4RowAddr, 0, 0);}
    if (mtk_nand_ready_for_read(nand, u4RowAddr, u4ColAddr, u4SecNum, true, buf))
    {	
        if (!mtk_nand_read_page_data(mtd, buf, u4PageSize))
        {
        	MSG(INIT, "mtk_nand_read_page_data fail\n");
            bRet = ERR_RTN_FAIL;
        }

        if (!mtk_nand_status_ready(STA_NAND_BUSY))
        {
        	MSG(INIT, "mtk_nand_status_ready fail\n");
            bRet = ERR_RTN_FAIL;
        }
        if (g_bHwEcc)
        {
            if (!mtk_nand_check_dececc_done(u4SecNum))
            {
            	MSG(INIT, "mtk_nand_check_dececc_done fail\n");
                bRet = ERR_RTN_FAIL;
            }
        }
        mtk_nand_read_fdm_data(pFDMBuf, u4SecNum);
        if (g_bHwEcc)
        {
            if (!mtk_nand_check_bch_error(mtd, buf, pFDMBuf,u4SecNum - 1, u4RowAddr, &tempBitMap))
            {
								if(devinfo.vendor != VEND_NONE){
									readRetry = TRUE;
								}
            		MSG(INIT, "mtk_nand_check_bch_error fail sector, retryCount:%d\n",retryCount);
                bRet = ERR_RTN_BCH_FAIL;
#if defined (CONFIG_NAND_BOOTLOADER)
                if (bBCHRetry==false)
									break;  
#endif
            }
						else
						{
				if(0 != (DRV_Reg32(NFI_STA_REG32) & STA_READ_EMPTY)) // if empty
				{
					if(retryCount != 0)
					{
						MSG(INIT,"NFI read retry read empty page, return as uncorrectable\n");
						mtd->ecc_stats.failed+=u4SecNum;
						bRet = ERR_RTN_BCH_FAIL;
				    }else
				    {
    				    memset(buf,0xff,u4PageSize);
    		            memset(pFDMBuf,0xff,u4SecNum*8);
    		            readRetry = FALSE;
    		            bRet = ERR_RTN_SUCCESS;
		            }
			    }
        }
        }
        mtk_nand_stop_read();
    }
		if(use_randomizer && u4RowAddr >= RAND_START_ADDR)
		{	mtk_nand_turn_off_randomizer();}
		else if(pre_randomizer && u4RowAddr < RAND_START_ADDR)
		{	mtk_nand_turn_off_randomizer();}
		if (bRet == ERR_RTN_BCH_FAIL)
		{
			//u32 feature = devinfo.feature_set.FeatureSet.readRetryStart+retryCount;
			u32 feature = mtk_nand_rrtry_setting(devinfo, devinfo.feature_set.FeatureSet.rtype,devinfo.feature_set.FeatureSet.readRetryStart,retryCount);
			if(retryCount < devinfo.feature_set.FeatureSet.readRetryCnt)
			{
				mtd->ecc_stats.corrected = backup_corrected;
    			mtd->ecc_stats.failed = backup_failed;
				if(MLC_DEVICE)
				{
					mtk_nand_rrtry_func(mtd,devinfo,feature,FALSE);
					//mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,\
					//			devinfo.feature_set.FeatureSet.readRetryAddress,\
					//			(u8*)&feature,4);
				}
				retryCount++;
			}
			else
			{
				if(MLC_DEVICE)
				{
					feature = devinfo.feature_set.FeatureSet.readRetryDefault;
					mtk_nand_rrtry_func(mtd,devinfo,feature,TRUE);
					//mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,\
					//			devinfo.feature_set.FeatureSet.readRetryAddress,\
					//			(u8*)&feature,4);
				}
				readRetry = FALSE;
			}		
		}
		else
		{
			if((retryCount != 0) && MLC_DEVICE)
			{
				u32 feature = devinfo.feature_set.FeatureSet.readRetryDefault;
				mtk_nand_rrtry_func(mtd,devinfo,feature,TRUE);
				//mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,\
				//				devinfo.feature_set.FeatureSet.readRetryAddress,\
				//				(u8*)&feature,4);
			}
			readRetry = FALSE;
		}
		if(TRUE == readRetry)
			bRet = ERR_RTN_SUCCESS;
	}while(readRetry);
	if(retryCount != 0)
	{
		u32 feature = devinfo.feature_set.FeatureSet.readRetryDefault;
	    if(bRet == ERR_RTN_SUCCESS)
	    {
	        MSG(INIT, "u4RowAddr:0x%x read retry pass, retrycnt:%d ENUM0:%x,ENUM1:%x,\n",u4RowAddr,retryCount,DRV_Reg32(ECC_DECENUM1_REG32),DRV_Reg32(ECC_DECENUM0_REG32));
	    }
		if(MLC_DEVICE)
		{
		mtk_nand_rrtry_func(mtd,devinfo,feature,TRUE);
		//mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,\
		//				devinfo.feature_set.FeatureSet.readRetryAddress,\
		//				(u8*)&feature,4);
		}
	}	
    if (buf == local_buffer_16_align)
        memcpy(pPageBuf, buf, u4PageSize);

    PFM_END_R(pfm_time_read, u4PageSize + 32);
	//if(use_randomizer /*&& u4RowAddr >= RAND_START_ADDR*/)
	//{	mtk_nand_turn_off_randomizer();}
	//else if(pre_randomizer && u4RowAddr < RAND_START_ADDR)
	//{	mtk_nand_turn_off_randomizer();}
    return bRet;
}

/******************************************************************************
 * mtk_nand_exec_write_page
 *
 * DESCRIPTION:
 *   Write a page data !
 *
 * PARAMETERS:
 *   struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize,
 *   u8* pPageBuf, u8* pFDMBuf
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
int mtk_nand_exec_write_page(struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize, u8 * pPageBuf, u8 * pFDMBuf)
{
    struct nand_chip *chip = mtd->priv;
    u32 u4SecNum = u4PageSize >> host->hw->nand_sec_shift;
    u8 *buf;
    u8 status;
#ifdef PWR_LOSS_SPOH
    u32 time;
    struct timeval pl_time_write;
    suseconds_t duration;
#endif
#if 0
	{
		val = devinfo.feature_set.FeatureSet.readRetryDefault;
		mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,\
									devinfo.feature_set.FeatureSet.readRetryAddress,\
									(u8 *)&val,4);
		mtk_nand_GetFeature(mtd, devinfo.feature_set.FeatureSet.gfeatureCmd,\
			devinfo.feature_set.FeatureSet.readRetryAddress,\
			(u8 *)&val,4);
		if((val&0xFF) != (devinfo.feature_set.FeatureSet.readRetryDefault&0xFF))
		{
			MSG(INIT, "mtk_nand_exec_write_page check read retry defalut value fail 0x%x\n",val);
		}
	}
#endif
    //MSG(INIT, "mtk_nand_exec_write_page, page: 0x%x\n", u4RowAddr);
#if CFG_2CS_NAND
    if (g_bTricky_CS)
    {
        u4RowAddr = mtk_nand_cs_on(chip, NFI_TRICKY_CS, u4RowAddr);
    }
#endif

	if(use_randomizer && u4RowAddr >= RAND_START_ADDR)
	{	mtk_nand_turn_on_randomizer(u4RowAddr, 1, 0);}
	else if(pre_randomizer && u4RowAddr < RAND_START_ADDR)
	{	mtk_nand_turn_on_randomizer(u4RowAddr, 1, 0);}

#ifdef _MTK_NAND_DUMMY_DRIVER_
    if (dummy_driver_debug)
    {
        unsigned long long time = sched_clock();
        if (!((time * 123 + 59) % 32768))
        {
            printk(KERN_INFO "[NAND_DUMMY_DRIVER] Simulate write error at page: 0x%x\n", u4RowAddr);
            return -EIO;
        }
    }
#endif

#ifdef NAND_PFM
    struct timeval pfm_time_write;
#endif
    PFM_BEGIN(pfm_time_write);
    if (((u32) pPageBuf % 16) && local_buffer_16_align)
    {
        printk(KERN_INFO "Data buffer not 16 bytes aligned: %p\n", pPageBuf);
        memcpy(local_buffer_16_align, pPageBuf, mtd->writesize);
        buf = local_buffer_16_align;
    } else
        buf = pPageBuf;

    if (mtk_nand_ready_for_write(chip, u4RowAddr, 0, true, buf))
    {
        mtk_nand_write_fdm_data(chip, pFDMBuf, u4SecNum);
        (void)mtk_nand_write_page_data(mtd, buf, u4PageSize);
        (void)mtk_nand_check_RW_count(u4PageSize);
        mtk_nand_stop_write();
        PL_NAND_BEGIN(pl_time_write);
        PL_TIME_RAND_PROG(chip, u4RowAddr, time);
        (void)mtk_nand_set_command(NAND_CMD_PAGEPROG);
        PL_NAND_RESET(time);
        {
        #if CFG_PERFLOG_DEBUG
            struct timeval stimer,etimer;
            do_gettimeofday(&stimer);
        #endif
            while (DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY) ;
        #if CFG_PERFLOG_DEBUG
            do_gettimeofday(&etimer);
            //printk("[Bean]Cal_timediff(&etimer,&stimer):0x%x\n", Cal_timediff(&etimer,&stimer));
            g_NandPerfLog.WriteBusyTotalTime+= Cal_timediff(&etimer,&stimer);
            g_NandPerfLog.WriteBusyCount++;
        #endif
        }
    }
    else
    {
        printk("[Bean]mtk_nand_ready_for_write fail!\n");
    }
    PL_NAND_END(pl_time_write, duration);
    PL_TIME_PROG(duration);
    PFM_END_W(pfm_time_write, u4PageSize + 32);

	if(use_randomizer && u4RowAddr >= RAND_START_ADDR)
	{	mtk_nand_turn_off_randomizer();}
	else if(pre_randomizer && u4RowAddr < RAND_START_ADDR)
	{	mtk_nand_turn_off_randomizer();}
    status = chip->waitfunc(mtd, chip);
    //printk("[Bean]status:%d\n", status);
    if (status & NAND_STATUS_FAIL)
        return -EIO;
    else
        return 0;
}

/******************************************************************************
 *
 * Write a page to a logical address
 *
 *****************************************************************************/
#if defined (CONFIG_NAND_BOOTLOADER)
static int mtk_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip, const u8 * buf, int oob_required, int page, int cached, int raw)
#else
static int mtk_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip, int column, int bytes, const u8 * buf, \
		int oob_required, int page, int cached, int raw)
#endif
{
//	int block_size = 1 << (chip->phys_erase_shift);
    int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
    u32 block;
    u32 page_in_block;
    u32 mapped_block;
#if CFG_PERFLOG_DEBUG
    struct timeval stimer,etimer;
    do_gettimeofday(&stimer);
#endif
	page_in_block = mtk_nand_page_transform(mtd,chip,page,&block,&mapped_block);
	//MSG(INIT,"[WRITE] %d, %d, %d %d\n",mapped_block, block, page_in_block, page_per_block);
    // write bad index into oob
    if (mapped_block != block)
    {
        set_bad_index_to_oob(chip->oob_poi, block);
    } else
    {
        set_bad_index_to_oob(chip->oob_poi, FAKE_INDEX);
    }

    if (mtk_nand_exec_write_page(mtd, page_in_block + mapped_block * page_per_block, mtd->writesize, (u8 *) buf, chip->oob_poi))
    {
        MSG(INIT, "write fail at block: 0x%x, page: 0x%x\n", mapped_block, page_in_block);
        if (update_bmt((u64)((u64)page_in_block + (u64)mapped_block * page_per_block) << chip->page_shift, UPDATE_WRITE_FAIL, (u8 *) buf, chip->oob_poi))
        {
            MSG(INIT, "Update BMT success\n");
            return 0;
        } else
        {
            MSG(INIT, "Update BMT fail\n");
            return -EIO;
        }
    }
#if CFG_PERFLOG_DEBUG
    do_gettimeofday(&etimer);
    g_NandPerfLog.WritePageTotalTime+= Cal_timediff(&etimer,&stimer);
    g_NandPerfLog.WritePageCount++;
    dump_nand_rwcount();    
#endif
    return 0;
}

//-------------------------------------------------------------------------------
/*
static void mtk_nand_command_sp(
	struct mtd_info *mtd, unsigned int command, int column, int page_addr)
{
	g_u4ColAddr	= column;
	g_u4RowAddr	= page_addr;

	switch(command)
	{
	case NAND_CMD_STATUS:
		break;

	case NAND_CMD_READID:
		break;

	case NAND_CMD_RESET:
		break;

	case NAND_CMD_RNDOUT:
	case NAND_CMD_RNDOUTSTART:
	case NAND_CMD_RNDIN:
	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_STATUS_MULTI:
	default:
		break;
	}

}
*/


/******************************************************************************
 * mtk_nand_command_bp
 *
 * DESCRIPTION:
 *   Handle the commands from MTD !
 *
 * PARAMETERS:
 *   struct mtd_info *mtd, unsigned int command, int column, int page_addr
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void mtk_nand_command_bp(struct mtd_info *mtd, unsigned int command, int column, int page_addr)
{
    struct nand_chip *nand = mtd->priv;
#ifdef NAND_PFM
    struct timeval pfm_time_erase;
#endif
#if 0
//	int block_size = 1 << (nand->phys_erase_shift);
//	int page_per_block = 1 << (nand->phys_erase_shift - nand->page_shift);
//	u32 block;
//	u16 page_in_block;
//	u32 mapped_block;
//	bool rand= FALSE;
	page_addr = mtk_nand_page_transform(mtd,nand,&block,&mapped_block);
	page_addr = mapped_block*page_per_block + page_addr;
#endif
    switch (command)
    {
      case NAND_CMD_SEQIN:
          memset(g_kCMD.au1OOB, 0xFF, sizeof(g_kCMD.au1OOB));
          g_kCMD.pDataBuf = NULL;
          //}
          g_kCMD.u4RowAddr = page_addr;
          g_kCMD.u4ColAddr = column;
          break;

      case NAND_CMD_PAGEPROG:
          if (g_kCMD.pDataBuf || (0xFF != g_kCMD.au1OOB[0]))
          {
              u8 *pDataBuf = g_kCMD.pDataBuf ? g_kCMD.pDataBuf : nand->buffers->databuf;
              mtk_nand_exec_write_page(mtd, g_kCMD.u4RowAddr, mtd->writesize, pDataBuf, g_kCMD.au1OOB);
              g_kCMD.u4RowAddr = (u32) - 1;
              g_kCMD.u4OOBRowAddr = (u32) - 1;
          }
          break;

      case NAND_CMD_READOOB:
          g_kCMD.u4RowAddr = page_addr;
          g_kCMD.u4ColAddr = column + mtd->writesize;
#ifdef NAND_PFM
          g_kCMD.pureReadOOB = 1;
          g_kCMD.pureReadOOBNum += 1;
#endif
          break;

      case NAND_CMD_READ0:
          g_kCMD.u4RowAddr = page_addr;
          g_kCMD.u4ColAddr = column;
#ifdef NAND_PFM
          g_kCMD.pureReadOOB = 0;
#endif
          break;

      case NAND_CMD_ERASE1:
          PFM_BEGIN(pfm_time_erase);
          (void)mtk_nand_reset();
          mtk_nand_set_mode(CNFG_OP_ERASE);
          (void)mtk_nand_set_command(NAND_CMD_ERASE1);
          (void)mtk_nand_set_address(0, page_addr, 0, devinfo.addr_cycle - 2);
          break;

      case NAND_CMD_ERASE2:
          (void)mtk_nand_set_command(NAND_CMD_ERASE2);
          while (DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY) ;
          PFM_END_E(pfm_time_erase);
          break;

      case NAND_CMD_STATUS:
          (void)mtk_nand_reset();
		  if(mtk_nand_israndomizeron())
		  {
		  	//g_brandstatus = TRUE;
		  	mtk_nand_turn_off_randomizer();
		  }
          NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
          mtk_nand_set_mode(CNFG_OP_SRD);
          mtk_nand_set_mode(CNFG_READ_EN);
          NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
          NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
          (void)mtk_nand_set_command(NAND_CMD_STATUS);
          NFI_CLN_REG32(NFI_CON_REG16, CON_NFI_NOB_MASK);
          mb();
          DRV_WriteReg32(NFI_CON_REG16, CON_NFI_SRD | (1 << CON_NFI_NOB_SHIFT));
          g_bcmdstatus = true;
          break;

      case NAND_CMD_RESET:
          (void)mtk_nand_reset();
          break;

      case NAND_CMD_READID:
          /* Issue NAND chip reset command */
          //NFI_ISSUE_COMMAND (NAND_CMD_RESET, 0, 0, 0, 0);

          //timeout = TIMEOUT_4;

          //while (timeout)
          //timeout--;

          mtk_nand_reset();
          /* Disable HW ECC */
          NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
          NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);

          /* Disable 16-bit I/O */
          //NFI_CLN_REG16(NFI_PAGEFMT_REG16, PAGEFMT_DBYTE_EN);

          NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN | CNFG_BYTE_RW);
          (void)mtk_nand_reset();
          mb();
          mtk_nand_set_mode(CNFG_OP_SRD);
          (void)mtk_nand_set_command(NAND_CMD_READID);
          (void)mtk_nand_set_address(0, 0, 1, 0);
          DRV_WriteReg32(NFI_CON_REG16, CON_NFI_SRD);
          while (DRV_Reg32(NFI_STA_REG32) & STA_DATAR_STATE) ;
          break;

      default:
          BUG();
          break;
    }
}

/******************************************************************************
 * mtk_nand_select_chip
 *
 * DESCRIPTION:
 *   Select a chip !
 *
 * PARAMETERS:
 *   struct mtd_info *mtd, int chip
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void mtk_nand_select_chip(struct mtd_info *mtd, int chip)
{
    if (chip == -1 && false == g_bInitDone)
    {
        struct nand_chip *nand = mtd->priv;
#if defined (CONFIG_NAND_BOOTLOADER)
	struct mtk_nand_host *host_ptr = host;
#else
	struct mtk_nand_host *host_ptr = nand->priv;
#endif	
	struct mtk_nand_host_hw *hw = host->hw;
	u32 spare_per_sector = mtd->oobsize/( mtd->writesize/hw->nand_sec_size);
	u32 ecc_bit = 4;
	u32 spare_bit = PAGEFMT_SPARE_16;	
	switch(spare_per_sector)
    {
//#ifndef MTK_COMBO_NAND_SUPPORT
#if 1
		case 16:
            spare_bit = PAGEFMT_SPARE_16;
    		ecc_bit = 4;
			spare_per_sector = 16;
            break;
        case 26:
        case 27:
		case 28:
  		spare_bit = PAGEFMT_SPARE_26;
    		ecc_bit = 10;
		spare_per_sector = 26;
            break;
		case 32:
            ecc_bit = 12;
			//if(MLC_DEVICE == TRUE)
			if(hw->nand_sec_size == 1024)
            	spare_bit = PAGEFMT_SPARE_32_1KS;
			else
				spare_bit = PAGEFMT_SPARE_32;
			spare_per_sector = 32;
            break;
		case 40:
            ecc_bit = 18;
            spare_bit = PAGEFMT_SPARE_40;
			spare_per_sector = 40;
            break;
		case 44:
            ecc_bit = 20;
            spare_bit = PAGEFMT_SPARE_44;
			spare_per_sector = 44;
            break;
		case 48:
		case 49:
            ecc_bit = 22;
            spare_bit = PAGEFMT_SPARE_48;
			spare_per_sector = 48;
            break;
		case 50:
		case 51:
            ecc_bit = 24;
            spare_bit = PAGEFMT_SPARE_50;
			spare_per_sector = 50;
            break;
		case 52:
		case 54:
		case 56:
            ecc_bit = 24;
			//if(MLC_DEVICE == TRUE)
			if(hw->nand_sec_size == 1024)
            	spare_bit = PAGEFMT_SPARE_52_1KS;
			else
				spare_bit = PAGEFMT_SPARE_52;
			spare_per_sector = 32;
            break;
#endif
		case 62:
		case 63:
            ecc_bit = 28;
            spare_bit = PAGEFMT_SPARE_62;
			spare_per_sector = 62;
            break;
		case 64:
            ecc_bit = 32;
			//if(MLC_DEVICE == TRUE)
			if(hw->nand_sec_size == 1024)
            	spare_bit = PAGEFMT_SPARE_64_1KS;
			else
				spare_bit = PAGEFMT_SPARE_64;
			spare_per_sector = 64;
            break;
		case 72:
			ecc_bit = 36;
			//if(MLC_DEVICE == TRUE)
			if(hw->nand_sec_size == 1024)
            	spare_bit = PAGEFMT_SPARE_72_1KS;
			spare_per_sector = 72;
            break;
		case 80:
			ecc_bit = 40;
			//if(MLC_DEVICE == TRUE)
			if(hw->nand_sec_size == 1024)
            	spare_bit = PAGEFMT_SPARE_80_1KS;
			spare_per_sector = 80;
            break;
		case 88:
			ecc_bit = 44;
			//if(MLC_DEVICE == TRUE)
			if(hw->nand_sec_size == 1024)
            	spare_bit = PAGEFMT_SPARE_88_1KS;
			spare_per_sector = 88;
            break;
		case 96:
		case 98:
			ecc_bit = 48;
			//if(MLC_DEVICE == TRUE)
			if(hw->nand_sec_size == 1024)
            	spare_bit = PAGEFMT_SPARE_96_1KS;
			spare_per_sector = 96;
            break;
		case 100:
		case 102:
		case 104:
			ecc_bit = 52;
			//if(MLC_DEVICE == TRUE)
			if(hw->nand_sec_size == 1024)
            	spare_bit = PAGEFMT_SPARE_100_1KS;
			spare_per_sector = 100;
            break;
		case 124:
		case 126:
		case 128:
			ecc_bit = 60;
			//if(MLC_DEVICE == TRUE)
			if(hw->nand_sec_size == 1024)
            	spare_bit = PAGEFMT_SPARE_124_1KS;
			spare_per_sector = 124;
            break;
		default:
  		    MSG(INIT, "[NAND]: NFI not support oobsize: %d\n", spare_per_sector);
    		ASSERT(0);
  	}

	 mtd->oobsize = spare_per_sector*(mtd->writesize/hw->nand_sec_size);
	 printk("[NAND]select ecc bit:%d, sparesize :%d\n",ecc_bit,mtd->oobsize);
/* Setup PageFormat */

	if (16384 == mtd->writesize)
    {
        NFI_SET_REG16(NFI_PAGEFMT_REG16, (spare_bit << PAGEFMT_SPARE_SHIFT) | PAGEFMT_16K_1KS);
        nand->cmdfunc = mtk_nand_command_bp;
    } else if (8192 == mtd->writesize)
    {
        NFI_SET_REG16(NFI_PAGEFMT_REG16, (spare_bit << PAGEFMT_SPARE_SHIFT) | PAGEFMT_8K_1KS);
        nand->cmdfunc = mtk_nand_command_bp;
    } else if (4096 == mtd->writesize)
    {
    	if(hw->nand_sec_size != 1024)
        NFI_SET_REG16(NFI_PAGEFMT_REG16, (spare_bit << PAGEFMT_SPARE_SHIFT) | PAGEFMT_4K);
		else
			NFI_SET_REG16(NFI_PAGEFMT_REG16, (spare_bit << PAGEFMT_SPARE_SHIFT) | PAGEFMT_4K_1KS);
        nand->cmdfunc = mtk_nand_command_bp;
    } else if (2048 == mtd->writesize)
    {
    	if(hw->nand_sec_size != 1024)
        	NFI_SET_REG16(NFI_PAGEFMT_REG16, (spare_bit << PAGEFMT_SPARE_SHIFT) | PAGEFMT_2K);
		else
			NFI_SET_REG16(NFI_PAGEFMT_REG16, (spare_bit << PAGEFMT_SPARE_SHIFT) | PAGEFMT_2K_1KS);
        nand->cmdfunc = mtk_nand_command_bp;
	}                       
	 ecc_threshold = ecc_bit*4/5;
	 ECC_Config(hw,ecc_bit);
        g_bInitDone = true;

	 //xiaolei for kernel3.10
	 nand->ecc.strength = ecc_bit;
	 mtd->bitflip_threshold = nand->ecc.strength;
    }
    switch (chip)
    {
      case -1:
          break;
      case 0:
#ifdef CFG_FPGA_PLATFORM		// FPGA NAND is placed at CS1 not CS0
			DRV_WriteReg16(NFI_CSEL_REG16, 0);
			break;
#endif
      case 1:
          DRV_WriteReg16(NFI_CSEL_REG16, chip);
          break;
    }
}

/******************************************************************************
 * mtk_nand_read_byte
 *
 * DESCRIPTION:
 *   Read a byte of data !
 *
 * PARAMETERS:
 *   struct mtd_info *mtd
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static uint8_t mtk_nand_read_byte(struct mtd_info *mtd)
{
#if 0
    //while(0 == FIFO_RD_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)));
    /* Check the PIO bit is ready or not */
    u32 timeout = TIMEOUT_4;
    uint8_t retval = 0;
    WAIT_NFI_PIO_READY(timeout);

    retval = DRV_Reg8(NFI_DATAR_REG32);
    MSG(INIT, "mtk_nand_read_byte (0x%x)\n", retval);

    if (g_bcmdstatus)
    {
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
        g_bcmdstatus = false;
    }

    return retval;
#endif
    uint8_t retval = 0;

    if (!mtk_nand_pio_ready())
    {
        printk("pio ready timeout\n");
        retval = false;
    }

    if (g_bcmdstatus)
    {
        retval = DRV_Reg8(NFI_DATAR_REG32);
        NFI_CLN_REG32(NFI_CON_REG16, CON_NFI_NOB_MASK);
        mtk_nand_reset();
#if (__INTERNAL_USE_AHB_MODE__)
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
#endif
        if (g_bHwEcc)
        {
            NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
        } else
        {
            NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
        }
        g_bcmdstatus = false;
    } else
        retval = DRV_Reg8(NFI_DATAR_REG32);
    
	/*if(g_brandstatus)
	{
	 	g_brandstatus = FALSE;
	  	mtk_nand_turn_on_randomizer(g_kCMD.u4RowAddr, g_kCMD.u4ColAddr / devinfo.sectorsize, FALSE);
	}*/

    return retval;
}

/******************************************************************************
 * mtk_nand_read_buf
 *
 * DESCRIPTION:
 *   Read NAND data !
 *
 * PARAMETERS:
 *   struct mtd_info *mtd, uint8_t *buf, int len
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void mtk_nand_read_buf(struct mtd_info *mtd, uint8_t * buf, int len)
{
    struct nand_chip *nand = (struct nand_chip *)mtd->priv;
    struct NAND_CMD *pkCMD = &g_kCMD;
    u32 u4ColAddr = pkCMD->u4ColAddr;
    u32 u4PageSize = mtd->writesize;
#if defined (CONFIG_NAND_BOOTLOADER)
		int ret;

		if (nand->oob_poi==buf)
		{
			mtk_nand_read_oob(mtd, nand, pkCMD->u4RowAddr, 0);
			return;
		}
#endif	
    if (u4ColAddr < u4PageSize)
    {
        if ((u4ColAddr == 0) && (len > u4PageSize))
        {
#if defined (CONFIG_NAND_BOOTLOADER)
        mtk_nand_read_page(mtd, nand, nand->buffers->databuf, pkCMD->u4RowAddr);
#else
            mtk_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize, buf, pkCMD->au1OOB);
            #endif
if (len > u4PageSize)
            {
                u32 u4Size = min(len - u4PageSize, sizeof(pkCMD->au1OOB));
                memcpy(buf + u4PageSize, pkCMD->au1OOB, u4Size);
            }
        } else
        {
#if defined (CONFIG_NAND_BOOTLOADER)
char* ptr = nand->buffers->databuf;
mtk_nand_read_page(mtd, nand, nand->buffers->databuf, pkCMD->u4RowAddr);
#else
           mtk_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize, nand->buffers->databuf, pkCMD->au1OOB);
#endif
            memcpy(buf, nand->buffers->databuf + u4ColAddr, len);
        }
        pkCMD->u4OOBRowAddr = pkCMD->u4RowAddr;
    } else
    {
        u32 u4Offset = u4ColAddr - u4PageSize;
        u32 u4Size = min(len - u4Offset, sizeof(pkCMD->au1OOB));
        if (pkCMD->u4OOBRowAddr != pkCMD->u4RowAddr)
        {
            mtk_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize, nand->buffers->databuf, pkCMD->au1OOB);
            pkCMD->u4OOBRowAddr = pkCMD->u4RowAddr;
        }
        memcpy(buf, pkCMD->au1OOB + u4Offset, u4Size);
    }
    pkCMD->u4ColAddr += len;
}

/******************************************************************************
 * mtk_nand_write_buf
 *
 * DESCRIPTION:
 *   Write NAND data !
 *
 * PARAMETERS:
 *   struct mtd_info *mtd, const uint8_t *buf, int len
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void mtk_nand_write_buf(struct mtd_info *mtd, const uint8_t * buf, int len)
{
    struct NAND_CMD *pkCMD = &g_kCMD;
    u32 u4ColAddr = pkCMD->u4ColAddr;
    u32 u4PageSize = mtd->writesize;
    int i4Size, i;

    if (u4ColAddr >= u4PageSize)
    {
        u32 u4Offset = u4ColAddr - u4PageSize;
        u8 *pOOB = pkCMD->au1OOB + u4Offset;
        i4Size = min(len, (int)(sizeof(pkCMD->au1OOB) - u4Offset));

        for (i = 0; i < i4Size; i++)
        {
            pOOB[i] &= buf[i];
        }
    } else
    {
        pkCMD->pDataBuf = (u8 *) buf;
    }

    pkCMD->u4ColAddr += len;
}

/******************************************************************************
 * mtk_nand_write_page_hwecc
 *
 * DESCRIPTION:
 *   Write NAND data with hardware ecc !
 *
 * PARAMETERS:
 *   struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void mtk_nand_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t * buf)
{
    mtk_nand_write_buf(mtd, buf, mtd->writesize);
    mtk_nand_write_buf(mtd, chip->oob_poi, mtd->oobsize);
}

/******************************************************************************
 * mtk_nand_read_page_hwecc
 *
 * DESCRIPTION:
 *   Read NAND data with hardware ecc !
 *
 * PARAMETERS:
 *   struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static int mtk_nand_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, uint8_t * buf, int oob_required, int page)
{
#if 0
    mtk_nand_read_buf(mtd, buf, mtd->writesize);
    mtk_nand_read_buf(mtd, chip->oob_poi, mtd->oobsize);
#else
    struct NAND_CMD *pkCMD = &g_kCMD;
    u32 u4ColAddr = pkCMD->u4ColAddr;
    u32 u4PageSize = mtd->writesize;

    if (u4ColAddr == 0)
    {
        mtk_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize, buf, chip->oob_poi);
        pkCMD->u4ColAddr += u4PageSize + mtd->oobsize;
    }
#endif
    return 0;
}

/******************************************************************************
 *
 * Read a page to a logical address
 *
 *****************************************************************************/
static int mtk_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip, u8 * buf, int page)
{
//	int block_size = 1 << (chip->phys_erase_shift);
    int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
//	int page_per_block1 = page_per_block;
    u32 block;
    u32 page_in_block;
    u32 mapped_block;
	int bRet = ERR_RTN_SUCCESS;
#if CFG_PERFLOG_DEBUG
    struct timeval stimer,etimer;
    do_gettimeofday(&stimer);
#endif
	page_in_block = mtk_nand_page_transform(mtd,chip,page,&block,&mapped_block);
	//MSG(INIT,"[READ] %d, %d, %d %d (mapped_page=%x)\n",mapped_block, block, page_in_block, page_per_block,page_in_block + mapped_block * page_per_block);

	bRet = mtk_nand_exec_read_page(mtd, page_in_block + mapped_block * page_per_block, mtd->writesize, buf, chip->oob_poi);
	if (bRet == ERR_RTN_SUCCESS)
    {
#if CFG_PERFLOG_DEBUG
        do_gettimeofday(&etimer);
        g_NandPerfLog.ReadPageTotalTime+= Cal_timediff(&etimer,&stimer);
        g_NandPerfLog.ReadPageCount++;
        dump_nand_rwcount(); 
#endif
        return 0;
    }
	
    /* else
       return -EIO; */
    return 0;
}

static int mtk_nand_read_subpage(struct mtd_info *mtd, struct nand_chip *chip, u8 * buf, int page, int subpage, int subpageno)
{
//	int block_size = 1 << (chip->phys_erase_shift);
    int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
//	int page_per_block1 = page_per_block;
    u32 block;
	int coladdr;
    u32 page_in_block;
    u32 mapped_block;
//	bool readRetry = FALSE;
//	int retryCount = 0;
	int bRet = ERR_RTN_SUCCESS;
	int sec_num = 1<<(chip->page_shift-host->hw->nand_sec_shift);
	int spare_per_sector = mtd->oobsize/sec_num;
#if CFG_PERFLOG_DEBUG
    struct timeval stimer,etimer;
    do_gettimeofday(&stimer);
#endif
	page_in_block = mtk_nand_page_transform(mtd,chip,page,&block,&mapped_block);
	coladdr = subpage*(devinfo.sectorsize+spare_per_sector);
	//coladdr = subpage*(devinfo.sectorsize);
	//MSG(INIT,"[Read Subpage] %d, %d, %d %d\n",mapped_block, block, page_in_block, page_per_block);

	bRet = mtk_nand_exec_read_sector(mtd, page_in_block + mapped_block * page_per_block, coladdr, devinfo.sectorsize*subpageno, buf, chip->oob_poi,subpageno);
	//memset(bean_buffer, 0xFF, LPAGE);
	//bRet = mtk_nand_exec_read_page(mtd, page, mtd->writesize, bean_buffer, chip->oob_poi);
	if (bRet == ERR_RTN_SUCCESS)
    {
#if CFG_PERFLOG_DEBUG
        do_gettimeofday(&etimer);
        g_NandPerfLog.ReadSubPageTotalTime+= Cal_timediff(&etimer,&stimer);
        g_NandPerfLog.ReadSubPageCount++;
        dump_nand_rwcount();
#endif
        return 0;
    }
	//memcpy(buf, bean_buffer+coladdr, mtd->writesize);
    /* else
       return -EIO; */
    return 0;
}


/******************************************************************************
 *
 * Erase a block at a logical address
 *
 *****************************************************************************/
int mtk_nand_erase_hw(struct mtd_info *mtd, int page)
{
#ifdef PWR_LOSS_SPOH
    struct timeval pl_time_write;
    suseconds_t duration;
    u32 time;
#endif
    int result;
    struct nand_chip *chip = (struct nand_chip *)mtd->priv;
#ifdef _MTK_NAND_DUMMY_DRIVER_
    if (dummy_driver_debug)
    {
        unsigned long long time = sched_clock();
        if (!((time * 123 + 59) % 1024))
        {
            printk(KERN_INFO "[NAND_DUMMY_DRIVER] Simulate erase error at page: 0x%x\n", page);
            return NAND_STATUS_FAIL;
        }
    }
#endif
#if CFG_2CS_NAND
    if (g_bTricky_CS)
    {
        page = mtk_nand_cs_on(chip, NFI_TRICKY_CS, page);
    }
#endif
    PL_NAND_BEGIN(pl_time_write);
    PL_TIME_RAND_ERASE(chip, page, time);
    chip->erase_cmd(mtd, page);
    PL_NAND_RESET(time);
    result=chip->waitfunc(mtd, chip);
    PL_NAND_END(pl_time_write, duration);
    PL_TIME_ERASE(duration);
    return result;
}

static int mtk_nand_erase(struct mtd_info *mtd, int page)
{
    int status;
    struct nand_chip *chip = mtd->priv;
//    int block_size = 1 << (chip->phys_erase_shift);
    int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
    u32 block;
    u32 page_in_block;
    u32 mapped_block;
#if CFG_PERFLOG_DEBUG
    struct timeval stimer,etimer;
    do_gettimeofday(&stimer);
#endif

	page_in_block = mtk_nand_page_transform(mtd,chip,page,&block,&mapped_block);
	//MSG(INIT, "[ERASE] 0x%x 0x%x (mapped_page=%x)\n", mapped_block, page, page_in_block + page_per_block * mapped_block);
    status = mtk_nand_erase_hw(mtd, page_in_block + page_per_block * mapped_block);

    if (status & NAND_STATUS_FAIL)
    {
        if (update_bmt((u64)((u64)page_in_block + (u64)mapped_block * page_per_block) << chip->page_shift, UPDATE_ERASE_FAIL, NULL, NULL))
        {
            MSG(INIT, "Erase fail at block: 0x%x, update BMT success\n", mapped_block);
            return 0;
        } else
        {
            MSG(INIT, "Erase fail at block: 0x%x, update BMT fail\n", mapped_block);
            return NAND_STATUS_FAIL;
        }
    }
#if CFG_PERFLOG_DEBUG
    do_gettimeofday(&etimer);
    g_NandPerfLog.EraseBlockTotalTime+= Cal_timediff(&etimer,&stimer);
    g_NandPerfLog.EraseBlockCount++;
    dump_nand_rwcount();    
#endif
    return 0;
}

/******************************************************************************
 * mtk_nand_read_multi_page_cache
 *
 * description:
 *   read multi page data using cache read
 *
 * parameters:
 *   struct mtd_info *mtd, struct nand_chip *chip, int page, struct mtd_oob_ops *ops
 *
 * returns:
 *   none
 *
 * notes:
 *   only available for nand flash support cache read.
 *   read main data only.
 *
 *****************************************************************************/
#if 0
static int mtk_nand_read_multi_page_cache(struct mtd_info *mtd, struct nand_chip *chip, int page, struct mtd_oob_ops *ops)
{
    int res = -EIO;
    int len = ops->len;
    struct mtd_ecc_stats stat = mtd->ecc_stats;
    uint8_t *buf = ops->datbuf;

    if (!mtk_nand_ready_for_read(chip, page, 0, true, buf))
        return -EIO;

    while (len > 0)
    {
        mtk_nand_set_mode(CNFG_OP_CUST);
        DRV_WriteReg32(NFI_CON_REG16, 8 << CON_NFI_SEC_SHIFT);

        if (len > mtd->writesize)   // remained more than one page
        {
            if (!mtk_nand_set_command(0x31)) // todo: add cache read command
                goto ret;
        } else
        {
            if (!mtk_nand_set_command(0x3f)) // last page remained
                goto ret;
        }

        mtk_nand_status_ready(STA_NAND_BUSY);

#ifdef __INTERNAL_USE_AHB_MODE__
        //if (!mtk_nand_dma_read_data(buf, mtd->writesize))
        if (!mtk_nand_read_page_data(mtd, buf, mtd->writesize))
            goto ret;
#else
        if (!mtk_nand_mcu_read_data(buf, mtd->writesize))
            goto ret;
#endif

        // get ecc error info
        mtk_nand_check_bch_error(mtd, buf, 3, page);
        ECC_Decode_End();

        page++;
        len -= mtd->writesize;
        buf += mtd->writesize;
        ops->retlen += mtd->writesize;

        if (len > 0)
        {
            ECC_Decode_Start();
            mtk_nand_reset();
        }

    }

    res = 0;

  ret:
    mtk_nand_stop_read();

    if (res)
        return res;

    if (mtd->ecc_stats.failed > stat.failed)
    {
        printk(KERN_INFO "ecc fail happened\n");
        return -EBADMSG;
    }

    return mtd->ecc_stats.corrected - stat.corrected ? -EUCLEAN : 0;
}
#endif

/******************************************************************************
 * mtk_nand_read_oob_raw
 *
 * DESCRIPTION:
 *   Read oob data
 *
 * PARAMETERS:
 *   struct mtd_info *mtd, const uint8_t *buf, int addr, int len
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   this function read raw oob data out of flash, so need to re-organise
 *   data format before using.
 *   len should be times of 8, call this after nand_get_device.
 *   Should notice, this function read data without ECC protection.
 *
 *****************************************************************************/
static int mtk_nand_read_oob_raw(struct mtd_info *mtd, uint8_t * buf, int page_addr, int len)
{
    struct nand_chip *chip = (struct nand_chip *)mtd->priv;
    u32 col_addr = 0;
    u32 sector = 0;
    int res = 0;
    u32 colnob = 2, rawnob = devinfo.addr_cycle - 2;
    int randomread = 0;
    int read_len = 0;
    int sec_num = 1<<(chip->page_shift-host->hw->nand_sec_shift);
    int spare_per_sector = mtd->oobsize/sec_num;
	u32 sector_size = NAND_SECTOR_SIZE;
	if(devinfo.sectorsize == 1024)
		sector_size = 1024;

    if (len >  NAND_MAX_OOBSIZE || len % OOB_AVAI_PER_SECTOR || !buf)
    {
        printk(KERN_WARNING "[%s] invalid parameter, len: %d, buf: %p\n", __FUNCTION__, len, buf);
        return -EINVAL;
    }
    if (len > spare_per_sector)
    {
        randomread = 1;
    }
    if (!randomread || !(devinfo.advancedmode & RAMDOM_READ))
    {
        while (len > 0)
        {
            read_len = min(len, spare_per_sector);
            col_addr = sector_size + sector * (sector_size + spare_per_sector); // TODO: Fix this hard-code 16
            if (!mtk_nand_ready_for_read(chip, page_addr, col_addr, sec_num, false, NULL))
            {
                printk(KERN_WARNING "mtk_nand_ready_for_read return failed\n");
                res = -EIO;
                goto error;
            }
            if (!mtk_nand_mcu_read_data(buf + spare_per_sector * sector, read_len))    // TODO: and this 8
            {
                printk(KERN_WARNING "mtk_nand_mcu_read_data return failed\n");
                res = -EIO;
                goto error;
            }
            mtk_nand_stop_read();
            //dump_data(buf + spare_per_sector * sector,spare_per_sector);
            sector++;
            len -= read_len;

        }
    } else                      //should be 64
    {
        col_addr = sector_size;
        if (chip->options & NAND_BUSWIDTH_16)
        {
            col_addr /= 2;
        }

        if (!mtk_nand_reset())
        {
            goto error;
        }

        mtk_nand_set_mode(0x6000);
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
        DRV_WriteReg32(NFI_CON_REG16, 4 << CON_NFI_SEC_SHIFT);

        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);

        mtk_nand_set_autoformat(false);

        if (!mtk_nand_set_command(NAND_CMD_READ0))
        {
            goto error;
        }
        //1 FIXED ME: For Any Kind of AddrCycle
        if (!mtk_nand_set_address(col_addr, page_addr, colnob, rawnob))
        {
            goto error;
        }

        if (!mtk_nand_set_command(NAND_CMD_READSTART))
        {
            goto error;
        }
        if (!mtk_nand_status_ready(STA_NAND_BUSY))
        {
            goto error;
        }

        read_len = min(len, spare_per_sector);
        if (!mtk_nand_mcu_read_data(buf + spare_per_sector * sector, read_len))    // TODO: and this 8
        {
            printk(KERN_WARNING "mtk_nand_mcu_read_data return failed first 16\n");
            res = -EIO;
            goto error;
        }
        sector++;
        len -= read_len;
        mtk_nand_stop_read();
        while (len > 0)
        {
            read_len = min(len,  spare_per_sector);
            if (!mtk_nand_set_command(0x05))
            {
                goto error;
            }

            col_addr = sector_size + sector * (sector_size + spare_per_sector); //:TODO_JP careful 16
            if (chip->options & NAND_BUSWIDTH_16)
            {
                col_addr /= 2;
            }
            DRV_WriteReg32(NFI_COLADDR_REG32, col_addr);
            DRV_WriteReg16(NFI_ADDRNOB_REG16, 2);
            DRV_WriteReg32(NFI_CON_REG16, 4 << CON_NFI_SEC_SHIFT);

            if (!mtk_nand_status_ready(STA_ADDR_STATE))
            {
                goto error;
            }

            if (!mtk_nand_set_command(0xE0))
            {
                goto error;
            }
            if (!mtk_nand_status_ready(STA_NAND_BUSY))
            {
                goto error;
            }
            if (!mtk_nand_mcu_read_data(buf + spare_per_sector * sector, read_len))    // TODO: and this 8
            {
                printk(KERN_WARNING "mtk_nand_mcu_read_data return failed first 16\n");
                res = -EIO;
                goto error;
            }
            mtk_nand_stop_read();
            sector++;
            len -= read_len;
        }
        //dump_data(&testbuf[16],16);
        //printk(KERN_ERR "\n");
    }
  error:
    NFI_CLN_REG32(NFI_CON_REG16, CON_NFI_BRD);
    return res;
}

static int mtk_nand_write_oob_raw(struct mtd_info *mtd, const uint8_t * buf, int page_addr, int len)
{
    struct nand_chip *chip = mtd->priv;
    // int i;
    u32 col_addr = 0;
    u32 sector = 0;
    // int res = 0;
    // u32 colnob=2, rawnob=devinfo.addr_cycle-2;
    // int randomread =0;
    int write_len = 0;
    int status;
    int sec_num = 1<<(chip->page_shift-host->hw->nand_sec_shift);
    int spare_per_sector = mtd->oobsize/sec_num;
	u32 sector_size = NAND_SECTOR_SIZE;
	if(devinfo.sectorsize == 1024)
		sector_size = 1024;

    if (len >  NAND_MAX_OOBSIZE || len % OOB_AVAI_PER_SECTOR || !buf)
    {
        printk(KERN_WARNING "[%s] invalid parameter, len: %d, buf: %p\n", __FUNCTION__, len, buf);
        return -EINVAL;
    }

    while (len > 0)
    {
        write_len = min(len,  spare_per_sector);
        col_addr = sector * (sector_size +  spare_per_sector) + sector_size;
        if (!mtk_nand_ready_for_write(chip, page_addr, col_addr, false, NULL))
        {
            return -EIO;
        }

        if (!mtk_nand_mcu_write_data(mtd, buf + sector * spare_per_sector, write_len))
        {
            return -EIO;
        }

        (void)mtk_nand_check_RW_count(write_len);
        NFI_CLN_REG32(NFI_CON_REG16, CON_NFI_BWR);
        (void)mtk_nand_set_command(NAND_CMD_PAGEPROG);

        while (DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY) ;

        status = chip->waitfunc(mtd, chip);
        if (status & NAND_STATUS_FAIL)
        {
            printk(KERN_INFO "status: %d\n", status);
            return -EIO;
        }

        len -= write_len;
        sector++;
    }

    return 0;
}

static int mtk_nand_write_oob_hw(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
    // u8 *buf = chip->oob_poi;
    int i, iter;

    int sec_num = 1<<(chip->page_shift-host->hw->nand_sec_shift);
    int spare_per_sector = mtd->oobsize/sec_num;

    memcpy(local_oob_buf, chip->oob_poi, mtd->oobsize);

    // copy ecc data
    for (i = 0; i < chip->ecc.layout->eccbytes; i++)
    {
        iter = (i / OOB_AVAI_PER_SECTOR) * spare_per_sector + OOB_AVAI_PER_SECTOR + i % OOB_AVAI_PER_SECTOR;
        local_oob_buf[iter] = chip->oob_poi[chip->ecc.layout->eccpos[i]];
        // chip->oob_poi[chip->ecc.layout->eccpos[i]] = local_oob_buf[iter];
    }

    // copy FDM data
    for (i = 0; i < sec_num; i++)
    {
        memcpy(&local_oob_buf[i * spare_per_sector], &chip->oob_poi[i * OOB_AVAI_PER_SECTOR], OOB_AVAI_PER_SECTOR);
    }

    return mtk_nand_write_oob_raw(mtd, local_oob_buf, page, mtd->oobsize);
}

static int mtk_nand_write_oob(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
//    int block_size = 1 << (chip->phys_erase_shift);
    int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
//	int page_per_block1 = page_per_block;
    u32 block;
    u16 page_in_block;
    u32 mapped_block;
	
	//block = page / page_per_block1;
	//mapped_block = get_mapping_block_index(block);
	page_in_block = mtk_nand_page_transform(mtd,chip,page,&block,&mapped_block);

    if (mapped_block != block)
    {
        set_bad_index_to_oob(chip->oob_poi, block);
    } else
    {
        set_bad_index_to_oob(chip->oob_poi, FAKE_INDEX);
    }

    if (mtk_nand_write_oob_hw(mtd, chip, page_in_block + mapped_block * page_per_block /* page */ ))
    {
        MSG(INIT, "write oob fail at block: 0x%x, page: 0x%x\n", mapped_block, page_in_block);
        if (update_bmt((u64)((u64)page_in_block + (u64)mapped_block * page_per_block) << chip->page_shift, UPDATE_WRITE_FAIL, NULL, chip->oob_poi))
        {
            MSG(INIT, "Update BMT success\n");
            return 0;
        } else
        {
            MSG(INIT, "Update BMT fail\n");
            return -EIO;
        }
    }

    return 0;
}

int mtk_nand_block_markbad_hw(struct mtd_info *mtd, loff_t offset)
{
    struct nand_chip *chip = mtd->priv;
    int block = (int)(offset >> chip->phys_erase_shift);
    int page = block * (1 << (chip->phys_erase_shift - chip->page_shift));
    int ret;

    u8 buf[8];
    memset(buf, 0xFF, 8);
    buf[0] = 0;

    ret = mtk_nand_write_oob_raw(mtd, buf, page, 8);
    return ret;
}

static int mtk_nand_block_markbad(struct mtd_info *mtd, loff_t offset)
{
    struct nand_chip *chip = mtd->priv;
    u32 block = (u32)(offset >> chip->phys_erase_shift);
	int page = block * (1 << (chip->phys_erase_shift - chip->page_shift));
    u32 mapped_block;
    int ret;
#if defined (CONFIG_NAND_BOOTLOADER)
    nand_get_device(chip, mtd, FL_WRITING);
#else
nand_get_device(mtd, FL_WRITING);
#endif
    //mapped_block = get_mapping_block_index(block);
	page = mtk_nand_page_transform(mtd,chip,page,&block,&mapped_block);
    ret = mtk_nand_block_markbad_hw(mtd, mapped_block << chip->phys_erase_shift);

    nand_release_device(mtd);

    return ret;
}

int mtk_nand_read_oob_hw(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
    int i;
    u8 iter = 0;

    int sec_num = 1<<(chip->page_shift-host->hw->nand_sec_shift);
    int spare_per_sector = mtd->oobsize/sec_num;
#ifdef TESTTIME
    unsigned long long time1, time2;

    time1 = sched_clock();
#endif

    if (mtk_nand_read_oob_raw(mtd, chip->oob_poi, page, mtd->oobsize))
    {
         printk(KERN_ERR "[%s]mtk_nand_read_oob_raw return failed\n", __FUNCTION__);
        return -EIO;
    }
#ifdef TESTTIME
    time2 = sched_clock() - time1;
    if (!readoobflag)
    {
        readoobflag = 1;
        printk(KERN_ERR "[%s] time is %llu", __FUNCTION__, time2);
    }
#endif
		
    // adjust to ecc physical layout to memory layout
    /*********************************************************/
    /* FDM0 | ECC0 | FDM1 | ECC1 | FDM2 | ECC2 | FDM3 | ECC3 */
    /*  8B  |  8B  |  8B  |  8B  |  8B  |  8B  |  8B  |  8B  */
    /*********************************************************/

    memcpy(local_oob_buf, chip->oob_poi, mtd->oobsize);

    // copy ecc data
    for (i = 0; i < chip->ecc.layout->eccbytes; i++)
    {
        iter = (i / (spare_per_sector - OOB_AVAI_PER_SECTOR)) *  spare_per_sector \
			   + OOB_AVAI_PER_SECTOR + i % (spare_per_sector - OOB_AVAI_PER_SECTOR);
        chip->oob_poi[chip->ecc.layout->eccpos[i]] = local_oob_buf[iter];
    }

    // copy FDM data
    for (i = 0; i < sec_num; i++)
    {
        memcpy(&chip->oob_poi[i * OOB_AVAI_PER_SECTOR], &local_oob_buf[i *  spare_per_sector], OOB_AVAI_PER_SECTOR);
    }

    return 0;
}

static int mtk_nand_read_oob(struct mtd_info *mtd, struct nand_chip *chip, int page, int sndcmd)
{
#if defined (CONFIG_NAND_BOOTLOADER)
    int block_size = 1 << (chip->phys_erase_shift);
    int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
    int page_per_block1 = page_per_block;
    int block;
    u16 page_in_block;
    int mapped_block;
	//u8* buf = (u8*)kzalloc(mtd->writesize, GFP_KERNEL);
	
//page
	page_in_block = mtk_nand_page_transform(mtd,chip,page,&block,&mapped_block);
#endif
#if defined (CONFIG_NAND_BOOTLOADER)	
/*
	if(block_size != mtd->erasesize)
	{
		page_per_block1 = page_per_block>>1;
	}
	block = page / page_per_block1;
	mapped_block = get_mapping_block_index(block);
	if(block_size != mtd->erasesize)
		//page_in_block = devinfo.feature_set.PairPage[page % page_per_block1];
		page_in_block = functArray[devinfo.feature_set.ptbl_idx](page_in_block);
	else
		page_in_block = page % page_per_block1;
*/
    mtk_nand_read_oob_hw(mtd, chip, page_in_block + mapped_block * page_per_block);
#else
	mtk_nand_read_page(mtd,chip,temp_buffer_16_align,page);
	//kfree(buf);
#endif

    return 0;                   // the return value is sndcmd
}

int mtk_nand_block_bad_hw(struct mtd_info *mtd, loff_t ofs)
{
    struct nand_chip *chip = (struct nand_chip *)mtd->priv;
    int page_addr = (int)(ofs >> chip->page_shift);
	u32 block, mapped_block;
	int ret;
    unsigned int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);

    //unsigned char oob_buf[128];
	//char* buf = (char*) kmalloc(mtd->writesize + mtd->oobsize, GFP_KERNEL);

	//page_addr = mtk_nand_page_transform(mtd, chip, page_addr, &block, &mapped_block);
	
    page_addr &= ~(page_per_block - 1);

	//ret = mtk_nand_read_page(mtd,chip,buf,(ofs >> chip->page_shift));
	memset(temp_buffer_16_align,0xFF,LPAGE);
	ret = mtk_nand_read_subpage(mtd,chip,temp_buffer_16_align,(ofs >> chip->page_shift),0, 1);
	page_addr = mtk_nand_page_transform(mtd, chip, page_addr, &block, &mapped_block);
	//ret = mtk_nand_exec_read_page(mtd, page_addr+mapped_block*page_per_block, mtd->writesize, buf, oob_buf);
    if (0 != ret)
    {
        printk(KERN_WARNING "mtk_nand_read_oob_raw return error %d\n",ret);
//		kfree(buf);
        return 1;
    }

    if (chip->oob_poi[0] != 0xff)
    {
        printk(KERN_WARNING "Bad block detected at 0x%x, oob_buf[0] is 0x%x\n", block*page_per_block, chip->oob_poi[0]);
		//kfree(buf);
        // dump_nfi();
        return 1;
    }
	//kfree(buf);
    return 0;                   // everything is OK, good block
}

static int mtk_nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{
    int chipnr = 0;

    struct nand_chip *chip = (struct nand_chip *)mtd->priv;
    int block = (int)(ofs >> chip->phys_erase_shift);
    int mapped_block;
	int page = (int)(ofs >> chip->page_shift);
	int page_in_block;
	int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);

    int ret;

    if (getchip)
    {
        chipnr = (int)(ofs >> chip->chip_shift);
#if defined (CONFIG_NAND_BOOTLOADER)
        nand_get_device(chip, mtd, FL_READING);
#else
nand_get_device(mtd, FL_READING);
#endif
        /* Select the NAND device */
        chip->select_chip(mtd, chipnr);
    }
	//page = mtk_nand_page_transform(mtd, chip, page, &block, &mapped_block);
//    mapped_block = get_mapping_block_index(block);

    ret = mtk_nand_block_bad_hw(mtd, ofs);
	//page_in_block = mtk_nand_page_transform(mtd, chip, page, &block, &mapped_block);

    if (ret)
    {
    		page_in_block = mtk_nand_page_transform(mtd, chip, page, &block, &mapped_block);
        MSG(INIT, "Unmapped bad block: 0x%x %d\n", mapped_block,ret);
        if (update_bmt((u64)((u64)page_in_block + (u64)mapped_block * page_per_block)<<chip->page_shift, UPDATE_UNMAPPED_BLOCK, NULL, NULL))
        {
            MSG(INIT, "Update BMT success\n");
            ret = 0;
        } else
        {
            MSG(INIT, "Update BMT fail\n");
            ret = 1;
        }
    }

    if (getchip)
    {
        nand_release_device(mtd);
    }

    return ret;
}
/******************************************************************************
 * mtk_nand_init_size
 *
 * DESCRIPTION:
 *   initialize the pagesize, oobsize, blocksize
 *
 * PARAMETERS:
 *   struct mtd_info *mtd, struct nand_chip *this, u8 *id_data
 *
 * RETURNS:
 *   Buswidth
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/

static int mtk_nand_init_size(struct mtd_info *mtd, struct nand_chip *this, u8 *id_data)
{
    /* Get page size */
    mtd->writesize = devinfo.pagesize ;

    /* Get oobsize */
    mtd->oobsize = devinfo.sparesize;

    /* Get blocksize. */
    mtd->erasesize = devinfo.blocksize*1024;
    /* Get buswidth information */
    if(devinfo.iowidth==16)
    {
        return NAND_BUSWIDTH_16;
    }
    else
    {
        return 0;
    }

}

/******************************************************************************
 * mtk_nand_verify_buf
 *
 * DESCRIPTION:
 *   Verify the NAND write data is correct or not !
 *
 * PARAMETERS:
 *   struct mtd_info *mtd, const uint8_t *buf, int len
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
#ifdef CONFIG_MTD_NAND_VERIFY_WRITE

char gacBuf[LPAGE + LSPARE];

static int mtk_nand_verify_buf(struct mtd_info *mtd, const uint8_t * buf, int len)
{
#if 1
    struct nand_chip *chip = (struct nand_chip *)mtd->priv;
    struct NAND_CMD *pkCMD = &g_kCMD;
    u32 u4PageSize = mtd->writesize;
    u32 *pSrc, *pDst;
    int i;

    mtk_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize, gacBuf, gacBuf + u4PageSize);

    pSrc = (u32 *) buf;
    pDst = (u32 *) gacBuf;
    len = len / sizeof(u32);
    for (i = 0; i < len; ++i)
    {
        if (*pSrc != *pDst)
        {
            MSG(VERIFY, "mtk_nand_verify_buf page fail at page %d\n", pkCMD->u4RowAddr);
            return -1;
        }
        pSrc++;
        pDst++;
    }

    pSrc = (u32 *) chip->oob_poi;
    pDst = (u32 *) (gacBuf + u4PageSize);

    if ((pSrc[0] != pDst[0]) || (pSrc[1] != pDst[1]) || (pSrc[2] != pDst[2]) || (pSrc[3] != pDst[3]) || (pSrc[4] != pDst[4]) || (pSrc[5] != pDst[5]))
        // TODO: Ask Designer Why?
        //(pSrc[6] != pDst[6]) || (pSrc[7] != pDst[7]))
    {
        MSG(VERIFY, "mtk_nand_verify_buf oob fail at page %d\n", pkCMD->u4RowAddr);
        MSG(VERIFY, "0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", pSrc[0], pSrc[1], pSrc[2], pSrc[3], pSrc[4], pSrc[5], pSrc[6], pSrc[7]);
        MSG(VERIFY, "0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", pDst[0], pDst[1], pDst[2], pDst[3], pDst[4], pDst[5], pDst[6], pDst[7]);
        return -1;
    }
    /*
       for (i = 0; i < len; ++i) {
       if (*pSrc != *pDst) {
       printk(KERN_ERR"mtk_nand_verify_buf oob fail at page %d\n", g_kCMD.u4RowAddr);
       return -1;
       }
       pSrc++;
       pDst++;
       }
     */
    //printk(KERN_INFO"mtk_nand_verify_buf OK at page %d\n", g_kCMD.u4RowAddr);

    return 0;
#else
    return 0;
#endif
}
#endif

/******************************************************************************
 * mtk_nand_init_hw
 *
 * DESCRIPTION:
 *   Initial NAND device hardware component !
 *
 * PARAMETERS:
 *   struct mtk_nand_host *host (Initial setting data)
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void mtk_nand_init_hw(struct mtk_nand_host *host)
{
    struct mtk_nand_host_hw *hw = host->hw;


    g_bInitDone = false;
    g_kCMD.u4OOBRowAddr = (u32) - 1;

    /* Set default NFI access timing control */
    DRV_WriteReg32(NFI_ACCCON_REG32, hw->nfi_access_timing);
    DRV_WriteReg16(NFI_CNFG_REG16, 0);
    DRV_WriteReg16(NFI_PAGEFMT_REG16, 4);

    /* Reset the state machine and data FIFO, because flushing FIFO */
    (void)mtk_nand_reset();

    /* Set the ECC engine */
    if (hw->nand_ecc_mode == NAND_ECC_HW)
    {
        MSG(INIT, "%s : Use HW ECC ! \n", MODULE_NAME);
        if (g_bHwEcc)
        {
            NFI_SET_REG32(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
        }
        ECC_Config(host->hw,4);
        mtk_nand_configure_fdm(8);
    }

    /* Initilize interrupt. Clear interrupt, read clear. */
    DRV_Reg16(NFI_INTR_REG16);

    /* Interrupt arise when read data or program data to/from AHB is done. */
    DRV_WriteReg16(NFI_INTR_EN_REG16, 0);

    // Enable automatic disable ECC clock when NFI is busy state
    DRV_WriteReg16(NFI_DEBUG_CON1_REG16, (NFI_BYPASS|WBUF_EN|HWDCM_SWCON_ON));

    #ifdef CONFIG_PM
    host->saved_para.suspend_flag = 0;
    #endif
    // Reset
}

//-------------------------------------------------------------------------------
static int mtk_nand_dev_ready(struct mtd_info *mtd)
{
    return !(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY);
}

/******************************************************************************
 * mtk_nand_proc_read
 *
 * DESCRIPTION:
 *   Read the proc file to get the interrupt scheme setting !
 *
 * PARAMETERS:
 *   char *page, char **start, off_t off, int count, int *eof, void *data
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
#if !defined (CONFIG_NAND_BOOTLOADER)
static int mtk_nand_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char 	*p = page;
	int 	len = 0;
    int i;
    p += sprintf(p, "ID:");
    for(i=0;i<devinfo.id_length;i++){
        p += sprintf(p, " 0x%x", devinfo.id[i]);
    }
    p += sprintf(p, "\n");
    p += sprintf(p, "total size: %dMiB; part number: %s\n", devinfo.totalsize,devinfo.devciename);
    p += sprintf(p, "Current working in %s mode\n", g_i4Interrupt ? "interrupt" : "polling");
    p += sprintf(p, "NFI_ACCON(0x%x)=0x%x\n",(NFI_BASE+0x000C),DRV_Reg32(NFI_ACCCON_REG32));
	p += sprintf(p, "NFI_NAND_TYPE_CNFG_REG32= 0x%x\n",DRV_Reg32(NFI_NAND_TYPE_CNFG_REG32));
    #if CFG_FPGA_PLATFORM
    p += sprintf(p, "[FPGA Dummy]DRV_CFG_NFIA(0x0)=0x0\n");
    p += sprintf(p, "[FPGA Dummy]DRV_CFG_NFIB(0x0)=0x0\n");
    #else
    p += sprintf(p, "DRV_CFG_NFIA(IO PAD:0x%x)=0x%x\n",(GPIO_BASE+0xC20),*((volatile u32 *)(GPIO_BASE+0xC20)));
    p += sprintf(p, "DRV_CFG_NFIB(CTRL PAD:0x%x)=0x%x\n",(GPIO_BASE+0xB50),*((volatile u32 *)(GPIO_BASE+0xB50)));
    #endif
    #if CFG_PERFLOG_DEBUG
    p += sprintf(p, "Read Page Count:%d, Read Page totalTime:%lu, Avg. RPage:%lu\r\n",
                 g_NandPerfLog.ReadPageCount,g_NandPerfLog.ReadPageTotalTime,
                 g_NandPerfLog.ReadPageCount ? (g_NandPerfLog.ReadPageTotalTime/g_NandPerfLog.ReadPageCount): 0);
                 
    p += sprintf(p, "Read subPage Count:%d, Read subPage totalTime:%lu, Avg. RPage:%lu\r\n",
                 g_NandPerfLog.ReadSubPageCount,g_NandPerfLog.ReadSubPageTotalTime,
                 g_NandPerfLog.ReadSubPageCount? (g_NandPerfLog.ReadSubPageTotalTime/g_NandPerfLog.ReadSubPageCount): 0);

    p += sprintf(p, "Read Busy Count:%d, Read Busy totalTime:%lu, Avg. R Busy:%lu\r\n",
                 g_NandPerfLog.ReadBusyCount,g_NandPerfLog.ReadBusyTotalTime,
                 g_NandPerfLog.ReadBusyCount? (g_NandPerfLog.ReadBusyTotalTime/g_NandPerfLog.ReadBusyCount): 0);
    
    p += sprintf(p, "Read DMA Count:%d, Read DMA totalTime:%lu, Avg. R DMA:%lu\r\n",
                 g_NandPerfLog.ReadDMACount,g_NandPerfLog.ReadDMATotalTime,
                 g_NandPerfLog.ReadDMACount? (g_NandPerfLog.ReadDMATotalTime/g_NandPerfLog.ReadDMACount): 0);

    p += sprintf(p, "Write Page Count:%d, Write Page totalTime:%lu, Avg. WPage:%lu\r\n",
                 g_NandPerfLog.WritePageCount,g_NandPerfLog.WritePageTotalTime,
                 g_NandPerfLog.WritePageCount? (g_NandPerfLog.WritePageTotalTime/g_NandPerfLog.WritePageCount): 0);

    p += sprintf(p, "Write Busy Count:%d, Write Busy totalTime:%lu, Avg. W Busy:%lu\r\n",
                 g_NandPerfLog.WriteBusyCount,g_NandPerfLog.WriteBusyTotalTime,
                 g_NandPerfLog.WriteBusyCount? (g_NandPerfLog.WriteBusyTotalTime/g_NandPerfLog.WriteBusyCount): 0);

    p += sprintf(p, "Write DMA Count:%d, Write DMA totalTime:%lu, Avg. W DMA:%lu\r\n",
                 g_NandPerfLog.WriteDMACount,g_NandPerfLog.WriteDMATotalTime,
                 g_NandPerfLog.WriteDMACount? (g_NandPerfLog.WriteDMATotalTime/g_NandPerfLog.WriteDMACount): 0);

    p += sprintf(p, "EraseBlock Count:%d, EraseBlock totalTime:%lu, Avg. Erase:%lu\r\n",
                 g_NandPerfLog.EraseBlockCount,g_NandPerfLog.EraseBlockTotalTime,
                 g_NandPerfLog.EraseBlockCount? (g_NandPerfLog.EraseBlockTotalTime/g_NandPerfLog.EraseBlockCount): 0);

    #endif
    *start = page + off;
    len = p - page;

    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len : count;
}
#endif
/******************************************************************************
 * mtk_nand_proc_write
 *
 * DESCRIPTION:
 *   Write the proc file to set the interrupt scheme !
 *
 * PARAMETERS:
 *   struct file* file, const char* buffer,	unsigned long count, void *data
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
#if !defined (CONFIG_NAND_BOOTLOADER)
static int mtk_nand_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    struct mtd_info *mtd = &host->mtd;
    char buf[16];
    char cmd;
    int value;
    int len = count;//, n;

    if (len >= sizeof(buf))
    {
        len = sizeof(buf) - 1;
    }

    if (copy_from_user(buf, buffer, len))
    {
        return -EFAULT;
    }

    sscanf(buf, "%c%x",&cmd, &value);

    switch(cmd)
    {
        case 'A':  // NFIA driving setting
            #if CFG_FPGA_PLATFORM
            printk(KERN_INFO "[FPGA Dummy]NFIA driving setting\n");
            #else
            if ((value >= 0x0) && (value <= 0x7)) // driving step
            {
                printk(KERN_INFO "[NAND]IO PAD driving setting value(0x%x)\n\n", value);
                *((volatile u32 *)(GPIO_BASE+0xC20)) = value; //pad 7 6 4 3 0 1 5 8 2
            }
            else
                printk(KERN_ERR "[NAND]IO PAD driving setting value(0x%x) error\n", value);
            #endif
            break;
        case 'B': // NFIB driving setting
            #if CFG_FPGA_PLATFORM
            printk(KERN_INFO "[FPGA Dummy]NFIB driving setting\n");
            #else
            if ((value >= 0x0) && (value <= 0x7)) // driving step
            {
                printk(KERN_INFO "[NAND]Ctrl PAD driving setting value(0x%x)\n\n", value);
                *((volatile u32 *)(GPIO_BASE+0xB50)) = value; //CLE CE1 CE0 RE RB
                *((volatile u32 *)(GPIO_BASE+0xC10)) = value; //ALE
                *((volatile u32 *)(GPIO_BASE+0xC00)) = value; //WE
            }
            else
                printk(KERN_ERR "[NAND]Ctrl PAD driving setting value(0x%x) error\n", value);
            #endif
            break;
        case 'D':
   #ifdef _MTK_NAND_DUMMY_DRIVER_
               printk(KERN_INFO "Enable dummy driver\n");
               dummy_driver_debug = 1;
   #endif
            break;
        case 'I':   // Interrupt control
            if ((value > 0 && !g_i4Interrupt) || (value== 0 && g_i4Interrupt))
             {
                 nand_get_device(mtd->priv, mtd, FL_READING);

                 g_i4Interrupt = value;

                 if (g_i4Interrupt)
                 {
                     DRV_Reg16(NFI_INTR_REG16);
                     enable_irq(MT_NFI_IRQ_ID);
                 } else
                     disable_irq(MT_NFI_IRQ_ID);

                 nand_release_device(mtd);
             }
            break;
         case 'P': // Reset Performance monitor counter
             #ifdef NAND_PFM
                 /* Reset values */
                 g_PFM_R = 0;
                 g_PFM_W = 0;
                 g_PFM_E = 0;
                 g_PFM_RD = 0;
                 g_PFM_WD = 0;
                 g_kCMD.pureReadOOBNum = 0;
            #endif
            break;
        case 'R': // Reset NFI performance log
             #if CFG_PERFLOG_DEBUG
               g_NandPerfLog.ReadPageCount = 0;
               g_NandPerfLog.ReadPageTotalTime = 0;
			   g_NandPerfLog.ReadBusyCount = 0;
			   g_NandPerfLog.ReadBusyTotalTime = 0;
               g_NandPerfLog.ReadDMACount = 0;
               g_NandPerfLog.ReadDMATotalTime = 0;
               g_NandPerfLog.ReadSubPageCount = 0;
               g_NandPerfLog.ReadSubPageTotalTime = 0;

               g_NandPerfLog.WritePageCount = 0;
               g_NandPerfLog.WritePageTotalTime = 0;
               g_NandPerfLog.WriteBusyCount = 0;
               g_NandPerfLog.WriteBusyTotalTime = 0;
               g_NandPerfLog.WriteDMACount = 0;
               g_NandPerfLog.WriteDMATotalTime = 0;

               g_NandPerfLog.EraseBlockCount = 0;
               g_NandPerfLog.EraseBlockTotalTime = 0;
             #endif
           break;
         case 'T':  // ACCCON Setting
         	   nand_get_device(mtd->priv,mtd, FL_READING);
             DRV_WriteReg32(NFI_ACCCON_REG32,value);
             nand_release_device(mtd);
            break;
         default:
            break;
    }

    return len;
}
#endif

/*******************************************************************************
 * GPIO(PinMux) register definition
 *******************************************************************************/
#define EFUSE_GPIO_CFG	((volatile u32 *)(0x102061c0))
#define EFUSE_GPIO_1_8_ENABLE 0x00000008
static unsigned short NFI_gpio_uffs(unsigned short x)
{
	unsigned int r = 1;

	if (!x)
		return 0;
    
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
    
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
    
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
    
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
    
	return r;
}

static void NFI_GPIO_SET_FIELD(U32 reg, U32 field, U32 val) 
{
    unsigned short tv = (unsigned short)(*(volatile uint16*)(reg)); 
    tv &= ~(field); 
    tv |= ((val) << (NFI_gpio_uffs((unsigned short)(field)) - 1));
    (*(volatile uint16*)(reg) = (uint16)(tv)); 
}
#define   EFUSE_Is_IO_33V()    (((*EFUSE_GPIO_CFG)&EFUSE_GPIO_1_8_ENABLE)?FALSE:TRUE) // 0 : 3.3v (MT8130 default), 

static void mtk_nand_gpio_init(void)
{
    u32 value;
    U16 ori_gpio, offset;
    P_U16 gpio_mode_p;
	u8 is33v;
		u32 val;

	 NFI_GPIO_SET_FIELD(0x10005cc0,   0x700, 0x2);	  //pullup with 50Kohm	 ----PAD_MSDC0_CLK for 1.8v/3.3v
	 NFI_GPIO_SET_FIELD(0x10005cd0,   0x700, 0x3);   //pulldown with 50Kohm ----PAD_MSDC0_CMD for 1.8v/3.3v
	//	NFI_GPIO_SET_FIELD(0x10005c30,	 0x70,	0x3); //pulldown with 50Kohm ----PAD_MSDC0_DAT1 for 1.8v/3.3v 

     mt_set_gpio_mode(GPIO43, 1);    //Switch NCLE to non-default mode  
     mt_set_gpio_mode(GPIO44, 1);    //Switch NCEB1 to non-default mode 
     mt_set_gpio_mode(GPIO45, 1);    //Switch NCEB0 to non-default mode 
     mt_set_gpio_mode(GPIO47, 1);    //Switch NREB to non-default mode  
     mt_set_gpio_mode(GPIO48, 1);   //Switch NRNB to non-default mode 
            
     mt_set_gpio_mode(GPIO111, 4);     //Switch NLD7 to non-default mode
     mt_set_gpio_mode(GPIO112, 4);     //Switch NLD6 to non-default mode
     mt_set_gpio_mode(GPIO113, 4);     //Switch NLD5 to non-default mode
     mt_set_gpio_mode(GPIO114, 4);     //Switch NLD4 to non-default mode
     mt_set_gpio_mode(GPIO115, 4);     //Switch NLD8 to non-default mode
     mt_set_gpio_mode(GPIO116, 4);     //Switch NALE to non-default mode
     mt_set_gpio_mode(GPIO117, 4);     //Switch NWEB to non-default mode
     mt_set_gpio_mode(GPIO118, 4);     //Switch NLD3 to non-default mode
     mt_set_gpio_mode(GPIO119, 4);     //Switch NLD2 to non-default mode
     mt_set_gpio_mode(GPIO120, 4);     //Switch NLD1 to non-default mode
     mt_set_gpio_mode(GPIO121, 4);     //Switch NLD0 to non-default mode

	

    mt_set_gpio_pull_enable(GPIO48, 1);
    mt_set_gpio_pull_select(GPIO48, 1);

	is33v = EFUSE_Is_IO_33V();
    val = is33v ? 0x0c : 0x00;

	NFI_GPIO_SET_FIELD(0x10005e20, 0xf,   0x0a); 	/* TDSEL change value to 0x0a*/
    NFI_GPIO_SET_FIELD(0x10005e20, 0x3f0, val); 	/* RDSEL change value to 0x0c*/

    NFI_GPIO_SET_FIELD(0x10005e30, 0xf,   0x0a);	/* TDSEL change value to 0x0a*/
    NFI_GPIO_SET_FIELD(0x10005e30, 0x3f0, val);	/* RDSEL change value to 0x0c*/

   	NFI_GPIO_SET_FIELD(0x10005d20, 0xf,   0x0a);	/* TDSEL change value to 0x0a*/
    NFI_GPIO_SET_FIELD(0x10005d20, 0x3f0,val);	    /* RDSEL change value to val*/  

	 if (is33v)
	 	 NFI_GPIO_SET_FIELD(0x10005eb0, 0x000f,  0x5);

	 NFI_GPIO_SET_FIELD(0x10005cc0, 0x7, 0x3);
  	 NFI_GPIO_SET_FIELD(0x10005cd0, 0x7, 0x3);
  	 NFI_GPIO_SET_FIELD(0x10005ce0, 0x7, 0x3);
 	 NFI_GPIO_SET_FIELD(0x10005f70, 0x7000, 0x3);	//set driving more than 4mA
     NFI_GPIO_SET_FIELD(0x10005f80, 0x7, 0x3);	//set driving more than 4mA
  
			
}




/******************************************************************************
 * mtk_nand_probe
 *
 * DESCRIPTION:
 *   register the nand device file operations !
 *
 * PARAMETERS:
 *   struct platform_device *pdev : device structure
 *
 * RETURNS:
 *   0 : Success
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
#define KERNEL_NAND_UNIT_TEST 0
#define NAND_READ_PERFORMANCE 0
#if KERNEL_NAND_UNIT_TEST
int mtk_nand_unit_test(struct nand_chip *nand_chip, struct mtd_info *mtd)
{
    MSG(INIT, "Begin to Kernel nand unit test ... \n");
    int err = 0;
    int patternbuff[128] = {
    0x0103D901, 0xFF1802DF, 0x01200400, 0x00000021, 0x02040122, 0x02010122, 0x03020407, 0x1A050103,
    0x00020F1B, 0x08C0C0A1, 0x01550800, 0x201B0AC1, 0x41990155, 0x64F0FFFF, 0x201B0C82, 0x4118EA61,
    0xF00107F6, 0x0301EE1B, 0x0C834118, 0xEA617001, 0x07760301, 0xEE151405, 0x00202020, 0x20202020,
    0x00202020, 0x2000302E, 0x3000FF14, 0x00FF0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x01D90301, 0xDF0218FF, 0x00042001, 0x21000000, 0x22010402, 0x22010102, 0x07040203, 0x0301051A,
    0x1B0F0200, 0xA1C0C008, 0x00085501, 0xC10A1B20, 0x55019941, 0xFFFFF064, 0x820C1B20, 0x61EA1841,
    0xF60701F0, 0x1BEE0103, 0x1841830C, 0x017061EA, 0x01037607, 0x051415EE, 0x20202000, 0x20202020,
    0x20202000, 0x2E300020, 0x14FF0030, 0x0000FF00, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
    };
    u32 j, k, p = g_block_size/g_page_size;
    printk("[P] %x\n", p);
    struct gFeatureSet *feature_set = &(devinfo.feature_set.FeatureSet);
	u32 val = 0x05, TOTAL=1000;
    for (j = 0x400; j< 0x7A0; j++)
    {
        memset(local_buffer, 0x00, 8192);
        mtk_nand_read_page(mtd, nand_chip, local_buffer, j*p);
        MSG(INIT,"[1]0x%x %x %x %x\n", *(int *)local_buffer, *((int *)local_buffer+1), *((int *)local_buffer+2), *((int *)local_buffer+3));
        mtk_nand_erase(mtd, j*p);
        memset(local_buffer, 0x00, 8192);
        if(mtk_nand_read_page(mtd, nand_chip, local_buffer, j*p))
            printk("Read page 0x%x fail!\n", j*p);
        MSG(INIT,"[2]0x%x %x %x %x\n", *(int *)local_buffer, *((int *)local_buffer+1), *((int *)local_buffer+2), *((int *)local_buffer+3));
        if (mtk_nand_block_bad(mtd, j*g_block_size, 0))
        {
            printk("Bad block at %x\n", j);
            continue;
        }
        for (k = 0; k < p; k++)
        {
        	//static int mtk_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip, const u8 * buf, int oob_required, int page, int cached, int raw)

            if(mtk_nand_write_page(mtd, nand_chip,(u8 *)patternbuff, 1, j*p+k, 0, 0))
                printk("Write page 0x%x fail!\n", j*p+k);
        #if 1
        }
        TOTAL=1000;
        do{
        for (k = 0; k < p; k++)
        {
        #endif
            memset(local_buffer, 0x00, g_page_size);
            if(mtk_nand_read_page(mtd, nand_chip, local_buffer, j*p+k))
                printk("Read page 0x%x fail!\n", j*p+k);
            MSG(INIT,"[3]0x%x %x %x %x\n", *(int *)local_buffer, *((int *)local_buffer+1), *((int *)local_buffer+2), *((int *)local_buffer+3));
            if(memcmp((u8 *)patternbuff, local_buffer, 128*4))
            {
                MSG(INIT, "[KERNEL_NAND_UNIT_TEST] compare fail!\n");
                err = -1;
                while(1);
            }else
            {
                TOTAL--;
                MSG(INIT, "[KERNEL_NAND_UNIT_TEST] compare OK!\n");
            }
        }
        }while(TOTAL);
        #if 0
		mtk_nand_SetFeature(mtd, (u16) feature_set->sfeatureCmd, \
		feature_set->Async_timing.address, (u8 *)&val,\
		sizeof(feature_set->Async_timing.feature));
		mtk_nand_GetFeature(mtd, feature_set->gfeatureCmd, \
		feature_set->Async_timing.address, (u8 *)&val,4);
        printk("[ASYNC Interface]0x%X\n", val);
    err = mtk_nand_interface_config(mtd);
		MSG(INIT, "[nand_interface_config] %d\n",err);
		#endif
    } 
    return err;
}    
#endif

#if CFG_2CS_NAND
//#define CHIP_ADDRESS (0x100000)
static int mtk_nand_cs_check(struct mtd_info *mtd, u8 *id, u16 cs)
{
    u8 ids[NAND_MAX_ID]; 
    int i = 0;
    //if(devinfo.ttarget == TTYPE_2DIE)
    //{
    //    MSG(INIT,"2 Die Flash\n");
    //    g_bTricky_CS = TRUE;
    //    return 0;
    //} 
    DRV_WriteReg16(NFI_CSEL_REG16, cs);
    mtk_nand_command_bp(mtd, NAND_CMD_READID, 0, -1);
    for(i=0;i<NAND_MAX_ID;i++)
    {
		ids[i]=mtk_nand_read_byte(mtd);
		if(ids[i] != id[i])
		{
	        MSG(INIT, "Nand cs[%d] not support(%d,%x)\n", cs, i, ids[i]);
	        DRV_WriteReg16(NFI_CSEL_REG16, NFI_DEFAULT_CS);
		    
            return 0;
		}
	}
	DRV_WriteReg16(NFI_CSEL_REG16, NFI_DEFAULT_CS);
	return 1;
}

static u32 mtk_nand_cs_on(struct nand_chip *nand_chip, u16 cs, u32 page)
{
    u32 cs_page = page / g_nanddie_pages;
    if(cs_page)
    {
        DRV_WriteReg16(NFI_CSEL_REG16, cs);
        //if(devinfo.ttarget == TTYPE_2DIE)
        //   return page;//return (page | CHIP_ADDRESS);
        return (page - g_nanddie_pages);
    }
    DRV_WriteReg16(NFI_CSEL_REG16, NFI_DEFAULT_CS);
    return page;
}

#else

#define mtk_nand_cs_check(mtd, id, cs)  (1)
#define mtk_nand_cs_on(nand_chip, cs, page)   (page)
#endif

#if defined (CONFIG_NAND_BOOTLOADER)
int board_nand_init(struct nand_chip *nand_chip)
#else
static int mtk_nand_probe(struct platform_device *pdev)
#endif
{

    struct mtk_nand_host_hw *hw;
    struct mtd_info *mtd;
#if !defined (CONFIG_NAND_BOOTLOADER)
		struct nand_chip *nand_chip;    
    struct resource *res = pdev->resource;
#endif    
    int err = 0;
    u8 id[NAND_MAX_ID];
    int i;
		u32 sector_size = NAND_SECTOR_SIZE;
#if CFG_COMBO_NAND
    int bmt_sz = 0;
#endif
#if defined (CONFIG_NAND_BOOTLOADER)
		struct mtk_nand_host* host_ptr;
		hw = (struct mtk_nand_host_hw *)&mtk_nand_hw;
#else
    hw = (struct mtk_nand_host_hw *)pdev->dev.platform_data;
    BUG_ON(!hw);

    if (pdev->num_resources != 4 || res[0].flags != IORESOURCE_MEM || res[1].flags != IORESOURCE_MEM || res[2].flags != IORESOURCE_IRQ || res[3].flags != IORESOURCE_IRQ)
    {
        MSG(INIT, "%s: invalid resource type\n", __FUNCTION__);
        return -ENODEV;
    }

    /* Request IO memory */
    if (!request_mem_region(res[0].start, res[0].end - res[0].start + 1, pdev->name))
    {
        return -EBUSY;
    }
    if (!request_mem_region(res[1].start, res[1].end - res[1].start + 1, pdev->name))
    {
        return -EBUSY;
    }
#endif
    /* Allocate memory for the device structure (and zero it) */
#if defined (CONFIG_NAND_BOOTLOADER)
    host_ptr = kzalloc(sizeof(struct mtk_nand_host), GFP_KERNEL);
    if (!host_ptr)
    {
        MSG(INIT, "mtk_nand: failed to allocate device structure.\n");
        return -ENOMEM;
    }
#else
    host = kzalloc(sizeof(struct mtk_nand_host), GFP_KERNEL);
    if (!host)
    {
        MSG(INIT, "mtk_nand: failed to allocate device structure.\n");
        return -ENOMEM;
    }
#endif
    /* Allocate memory for 16 byte aligned buffer */
    local_buffer_16_align = local_buffer;
		temp_buffer_16_align  = temp_buffer;
    //printk(KERN_INFO "Allocate 16 byte aligned buffer: %p\n", local_buffer_16_align);
#if defined (CONFIG_NAND_BOOTLOADER)
    host_ptr->hw = hw;
#else
host->hw = hw;
#endif
    PL_TIME_PROG(10);
    PL_TIME_ERASE(10);
    PL_TIME_PROG_WDT_SET(1);
    PL_TIME_ERASE_WDT_SET(1);

    /* init mtd data structure */
#if defined (CONFIG_NAND_BOOTLOADER)
		host = host_ptr;
		mtd = &host->mtd;
		nand_chip->priv = mtd; 
		mtd->priv = nand_chip;
#else    
    nand_chip = &host->nand_chip;
    nand_chip->priv = host;     /* link the private data structures */
    mtd = &host->mtd;
    mtd->priv = nand_chip;
#endif
#if !defined (CONFIG_NAND_BOOTLOADER)    
    mtd->owner = THIS_MODULE;
#endif
    mtd->name = "MTK-Nand";
		mtd->eraseregions = host->erase_region;

    hw->nand_ecc_mode = NAND_ECC_HW;

    /* Set address of NAND IO lines */
    nand_chip->IO_ADDR_R = (void __iomem *)NFI_DATAR_REG32;
    nand_chip->IO_ADDR_W = (void __iomem *)NFI_DATAW_REG32;
    nand_chip->chip_delay = 20; /* 20us command delay time */
    nand_chip->ecc.mode = hw->nand_ecc_mode;    /* enable ECC */

    nand_chip->read_byte = mtk_nand_read_byte;
    nand_chip->read_buf = mtk_nand_read_buf;
    nand_chip->write_buf = mtk_nand_write_buf;
#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
    nand_chip->verify_buf = mtk_nand_verify_buf;
#endif
    nand_chip->select_chip = mtk_nand_select_chip;
    nand_chip->dev_ready = mtk_nand_dev_ready;
    nand_chip->cmdfunc = mtk_nand_command_bp;
    nand_chip->ecc.read_page = mtk_nand_read_page_hwecc;
    nand_chip->ecc.write_page = mtk_nand_write_page_hwecc;

    nand_chip->ecc.layout = &nand_oob_64;
    nand_chip->ecc.size = hw->nand_ecc_size;    //2048
    nand_chip->ecc.bytes = hw->nand_ecc_bytes;  //32

    nand_chip->options = NAND_SKIP_BBTSCAN;

    // For BMT, we need to revise driver architecture
    nand_chip->write_page = mtk_nand_write_page;
    nand_chip->read_page = mtk_nand_read_page;
	nand_chip->read_subpage = mtk_nand_read_subpage;
    nand_chip->ecc.write_oob = mtk_nand_write_oob;
    nand_chip->ecc.read_oob = mtk_nand_read_oob;
    nand_chip->block_markbad = mtk_nand_block_markbad;   // need to add nand_get_device()/nand_release_device().
    nand_chip->erase = mtk_nand_erase;  
    nand_chip->block_bad = mtk_nand_block_bad;
    nand_chip->init_size = mtk_nand_init_size;
#if CFG_FPGA_PLATFORM
    MSG(INIT, "[FPGA Dummy]Enable NFI and NFIECC Clock\n");
#else
    MSG(INIT, "[NAND]Enable NFI and NFIECC Clock\n");
    nand_enable_clock();
#endif
#if !defined (CONFIG_NAND_BOOTLOADER)
    mtk_nand_gpio_init();
#endif
mtk_nand_init_hw(host);
	/* Select the device */
    nand_chip->select_chip(mtd, NFI_DEFAULT_CS);

    /*
     * Reset the chip, required by some chips (e.g. Micron MT29FxGxxxxx)
     * after power-up
     */
    nand_chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

    /* Send the command for reading device ID */
    nand_chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

    for(i=0;i<NAND_MAX_ID;i++){
        id[i]=nand_chip->read_byte(mtd);
		MSG(INIT, "ID[%d]:0x%x \r" , i,id[i]);
    }
	 MSG(INIT, " \n");
    manu_id = id[0];
    dev_id = id[1];

    if (!get_device_info(id,&devinfo))
    {
        MSG(INIT, "Not Support this Device! \r\n");
    }
#if CFG_2CS_NAND
    if (mtk_nand_cs_check(mtd, id, NFI_TRICKY_CS))
    {
        MSG(INIT, "Twins Nand\n");
        g_bTricky_CS = TRUE;
        g_b2Die_CS = TRUE; 
    }
#endif

    if (devinfo.pagesize == 16384)
    {
        nand_chip->ecc.layout = &nand_oob_128;
        hw->nand_ecc_size = 16384;
    } else if (devinfo.pagesize == 8192)
    {
        nand_chip->ecc.layout = &nand_oob_128;
        hw->nand_ecc_size = 8192;
    } else if (devinfo.pagesize == 4096)
    {
        nand_chip->ecc.layout = &nand_oob_128;
        hw->nand_ecc_size = 4096;
    } else if (devinfo.pagesize == 2048)
    {
        nand_chip->ecc.layout = &nand_oob_64;
        hw->nand_ecc_size = 2048;
    } else if (devinfo.pagesize == 512)
    {
        nand_chip->ecc.layout = &nand_oob_16;
        hw->nand_ecc_size = 512;
    }
	if(devinfo.sectorsize == 1024)
	{
		sector_size = 1024;
		hw->nand_sec_shift = 10;
		hw->nand_sec_size = 1024;
		NFI_CLN_REG32(NFI_PAGEFMT_REG16, PAGEFMT_SECTOR_SEL);
	}
	if(devinfo.pagesize <= 4096)
	{
	    nand_chip->ecc.layout->eccbytes = devinfo.sparesize-OOB_AVAI_PER_SECTOR*(devinfo.pagesize/sector_size);
	    hw->nand_ecc_bytes = nand_chip->ecc.layout->eccbytes;
	    // Modify to fit device character    
	    nand_chip->ecc.size = hw->nand_ecc_size;    
	    nand_chip->ecc.bytes = hw->nand_ecc_bytes;
#if defined (CONFIG_NAND_BOOTLOADER)
	    nand_chip->ecc.strength = \
			nand_chip->ecc.bytes * 8 / fls(8 * nand_chip->ecc.size);  
#endif
	}
	else
	{
		nand_chip->ecc.layout->eccbytes = 64;//devinfo.sparesize-OOB_AVAI_PER_SECTOR*(devinfo.pagesize/sector_size);
	    hw->nand_ecc_bytes = nand_chip->ecc.layout->eccbytes;
	    // Modify to fit device character    
	    nand_chip->ecc.size = hw->nand_ecc_size;    
	    nand_chip->ecc.bytes = hw->nand_ecc_bytes; 
#if defined (CONFIG_NAND_BOOTLOADER)	    
	    nand_chip->ecc.strength = \
			nand_chip->ecc.bytes * 8 / fls(8 * nand_chip->ecc.size);  
#endif	     
	}
	nand_chip->subpagesize = devinfo.sectorsize;
#if !defined (CONFIG_NAND_BOOTLOADER)
    nand_chip->subpage_size = devinfo.sectorsize;   
#endif
    for(i=0;i<nand_chip->ecc.layout->eccbytes;i++){
	nand_chip->ecc.layout->eccpos[i]=OOB_AVAI_PER_SECTOR*(devinfo.pagesize/sector_size)+i;
    }
    //MSG(INIT, "[NAND] pagesz:%d , oobsz: %d,eccbytes: %d\n",
    //   devinfo.pagesize,  sizeof(g_kCMD.au1OOB),nand_chip->ecc.layout->eccbytes);


    MSG(INIT, "Support this Device in MTK table! %x \r\n", id);
#if CFG_RANDOMIZER    
    if(devinfo.vendor != VEND_NONE)
    {
	    //mtk_nand_randomizer_config(&devinfo.feature_set.randConfig);
	    #if 0
	    if ((devinfo.feature_set.randConfig.type == RAND_TYPE_SAMSUNG) ||
	        (devinfo.feature_set.randConfig.type == RAND_TYPE_TOSHIBA))
	    {
	        MSG(INIT, "[NAND]USE Randomizer\n");
            use_randomizer = TRUE;
        }
        else
        {
            MSG(INIT, "[NAND]OFF Randomizer\n");
            use_randomizer = FALSE;
        }
        #endif // only charge for efuse bonding
	    if((*EFUSE_RANDOM_CFG)&EFUSE_RANDOM_ENABLE)
        {
            MSG(INIT, "[NAND]EFUSE RANDOM CFG is ON\n");
            use_randomizer = TRUE; 
        	pre_randomizer = TRUE;
        }
        else
        {
            MSG(INIT, "[NAND]EFUSE RANDOM CFG is OFF\n");
            use_randomizer = FALSE;
        	pre_randomizer = FALSE;
        }	
	}
#endif

	if((devinfo.feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM) || (devinfo.feature_set.FeatureSet.rtype == RTYPE_HYNIX))
	    HYNIX_RR_TABLE_READ(&devinfo);
	
    hw->nfi_bus_width = devinfo.iowidth;
#if 1
	if(devinfo.vendor == VEND_MICRON)
	{
		if(devinfo.feature_set.FeatureSet.Async_timing.feature != 0xFF)
		{
		struct gFeatureSet *feature_set = &(devinfo.feature_set.FeatureSet);
		//u32 val = 0;
		mtk_nand_SetFeature(mtd, (u16) feature_set->sfeatureCmd, \
		feature_set->Async_timing.address, (u8 *)(&feature_set->Async_timing.feature),\
		sizeof(feature_set->Async_timing.feature));
		//mtk_nand_GetFeature(mtd, feature_set->gfeatureCmd, \
		//feature_set->Async_timing.address, (u8 *)(&val),4);
        //printk("[ASYNC Interface]0x%X\n", val);
        #if CFG_2CS_NAND
        if(g_bTricky_CS)
        {
            DRV_WriteReg16(NFI_CSEL_REG16, NFI_TRICKY_CS);
            mtk_nand_SetFeature(mtd, (u16) feature_set->sfeatureCmd, \
		feature_set->Async_timing.address, (u8 *)(&feature_set->Async_timing.feature),\
		sizeof(feature_set->Async_timing.feature));
		    DRV_WriteReg16(NFI_CSEL_REG16, NFI_DEFAULT_CS);
        }
        #endif
		}
	}
#endif
	if (devinfo.feature_set.ptbl_idx == PPTBL_NOT_SUPPORT)
	{
		MLC_DEVICE = FALSE;
		MSG(INIT,"SLC NAND\n");
	}
    //MSG(INIT, "AHB Clock(0x%x) ",DRV_Reg32(PERICFG_BASE+0x5C));
	//DRV_WriteReg32(PERICFG_BASE+0x5C, 0x1);
	//MSG(INIT, "AHB Clock(0x%x)",DRV_Reg32(PERICFG_BASE+0x5C));
    DRV_WriteReg32(NFI_ACCCON_REG32, devinfo.timmingsetting);
    //MSG(INIT, "Kernel Nand Timing:0x%x!\n", DRV_Reg32(NFI_ACCCON_REG32));

    /* 16-bit bus width */
    if (hw->nfi_bus_width == 16)
    {
        MSG(INIT, "%s : Set the 16-bit I/O settings!\n", MODULE_NAME);
        nand_chip->options |= NAND_BUSWIDTH_16;
    }
#if defined (CONFIG_NAND_BOOTLOADER)
#if __INTERNAL_USE_AHB_MODE__
		g_i4Interrupt = 1;
#else
		g_i4Interrupt = 0;		
#endif		
#else 
    mt_irq_set_sens(MT_NFI_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
    mt_irq_set_polarity(MT_NFI_IRQ_ID, MT65xx_POLARITY_LOW);
    
    err = request_irq(MT_NFI_IRQ_ID, mtk_nand_irq_handler, IRQF_DISABLED, "mtk-nand", NULL);

	if (0 != err)
    {
        MSG(INIT, "%s : Request IRQ fail: err = %d\n", MODULE_NAME, err);
        goto out;
    }

    if (g_i4Interrupt)
        enable_irq(MT_NFI_IRQ_ID);
    else
        disable_irq(MT_NFI_IRQ_ID);
#endif

#if 0
    if (devinfo.advancedmode & CACHE_READ)
    {
        nand_chip->ecc.read_multi_page_cache = NULL;
        // nand_chip->ecc.read_multi_page_cache = mtk_nand_read_multi_page_cache;
        // MSG(INIT, "Device %x support cache read \r\n",id);
    } else
        nand_chip->ecc.read_multi_page_cache = NULL;
#endif
	mtd->oobsize = devinfo.sparesize;
    /* Scan to find existance of the device */
#if defined (CONFIG_NAND_BOOTLOADER)
	nand_chip->options |= mtk_nand_init_size(mtd, nand_chip, NULL);
	mtd->writesize = devinfo.pagesize;
	nand_chip->chipsize = (((uint64_t)devinfo.totalsize) <<20);
	nand_chip->page_shift = ffs(mtd->writesize) - 1;
	nand_chip->phys_erase_shift = ffs(devinfo.blocksize*1024)-1;
	nand_chip->numchips = 1;
	nand_chip->pagemask = (nand_chip->chipsize >> nand_chip->page_shift) - 1;
	mtd->size = nand_chip->chipsize;	
#else	
	if (nand_scan(mtd, hw->nfi_cs_num))
    {
        MSG(INIT, "%s : nand_scan fail.\n", MODULE_NAME);
        err = -ENXIO;
        goto out;
    }
#endif
    g_page_size = mtd->writesize;
    g_block_size = devinfo.blocksize << 10;
    PAGES_PER_BLOCK = (u32)(g_block_size / g_page_size);
    //MSG(INIT, "g_page_size(%d) g_block_size(%d)\n",g_page_size, g_block_size);
#if CFG_2CS_NAND
   g_nanddie_pages = (u32)(nand_chip->chipsize >> nand_chip->page_shift);
   //if(devinfo.ttarget == TTYPE_2DIE)
   //{
   //   g_nanddie_pages = g_nanddie_pages / 2;
   //}
   if(g_b2Die_CS)
   {
       nand_chip->chipsize <<= 1;
       MSG(INIT, "[Bean]%dMB\n", (u32)(nand_chip->chipsize/1024/1024));
   }
   //MSG(INIT, "[Bean]g_nanddie_pages %x\n", g_nanddie_pages);
   //MSG(INIT, "[Bean]nand_chip->chipsize : %dMB\n", (u32)(nand_chip->chipsize/1024/1024));
#endif
    #if CFG_COMBO_NAND
	#ifdef PART_SIZE_BMTPOOL
    if (PART_SIZE_BMTPOOL)
    {
        bmt_sz = (PART_SIZE_BMTPOOL) >> nand_chip->phys_erase_shift;
    }else
    #endif
    {
        bmt_sz = (int)(((u32)(nand_chip->chipsize >> nand_chip->phys_erase_shift))/100*6);
    }
#if defined (CONFIG_NAND_BOOTLOADER)   
    g_bmt_sz = bmt_sz;
#endif
    //if (manu_id == 0x45)
    //{
    //    bmt_sz = bmt_sz * 2;
    //}
    #endif 
		MSG(INIT, "[bayi]0x%x \n", bmt_sz);

#if !defined (CONFIG_NAND_BOOTLOADER)   
    platform_set_drvdata(pdev, host);
#endif
    if (hw->nfi_bus_width == 16)
    {
        NFI_SET_REG16(NFI_PAGEFMT_REG16, PAGEFMT_DBYTE_EN);
    }

    nand_chip->select_chip(mtd, 0);
    #if defined(MTK_COMBO_NAND_SUPPORT)
#if !defined (CONFIG_NAND_BOOTLOADER)   
    #if CFG_COMBO_NAND
    	nand_chip->chipsize -= (bmt_sz * g_block_size);
    #else
    	nand_chip->chipsize -= (PART_SIZE_BMTPOOL);
    #endif
#endif
    	//#if CFG_2CS_NAND
        //if(g_b2Die_CS)
        //{
        //    nand_chip->chipsize -= (PART_SIZE_BMTPOOL);  // if 2CS nand need cut down again
        //}
    	//#endif
    #else
#if !defined (CONFIG_NAND_BOOTLOADER)
	    nand_chip->chipsize -= (BMT_POOL_SIZE) << nand_chip->phys_erase_shift;
#endif
    #endif
    mtd->size = nand_chip->chipsize;
#if NAND_READ_PERFORMANCE
    struct timeval stimer,etimer;
    do_gettimeofday(&stimer);
    for (i = 256; i < 512; i++)
    {
        mtk_nand_read_page(mtd, nand_chip, local_buffer, i);
        MSG(INIT,"[%d]0x%x %x %x %x\n",i, *(int *)local_buffer, *((int *)local_buffer+1), *((int *)local_buffer+2), *((int *)local_buffer+3));
        MSG(INIT,"[%d]0x%x %x %x %x\n",i, *(int *)local_buffer+4, *((int *)local_buffer+5), *((int *)local_buffer+6), *((int *)local_buffer+7));
        MSG(INIT,"[%d]0x%x %x %x %x\n",i, *(int *)local_buffer+8, *((int *)local_buffer+9), *((int *)local_buffer+10), *((int *)local_buffer+11));
        MSG(INIT,"[%d]0x%x %x %x %x\n",i, *(int *)local_buffer+12, *((int *)local_buffer+13), *((int *)local_buffer+14), *((int *)local_buffer+15));
    }
    do_gettimeofday(&etimer);
    printk("[NAND Read Perf.Test] %ld MB/s\n", (g_page_size*256)/Cal_timediff(&etimer,&stimer));
#endif

	if(devinfo.vendor != VEND_NONE)
	{
		err = mtk_nand_interface_config(mtd);
		#if CFG_2CS_NAND
        if(g_bTricky_CS)
        {
            DRV_WriteReg16(NFI_CSEL_REG16, NFI_TRICKY_CS);
            err = mtk_nand_interface_config(mtd);
            DRV_WriteReg16(NFI_CSEL_REG16, NFI_DEFAULT_CS);
        }
    	#endif
		//MSG(INIT, "[nand_interface_config] %d\n",err);
		//u32 regp;
		//for (regp = 0xF0206000; regp <= 0xF020631C; regp+=4)
		//    printk("[%08X]0x%08X\n", regp, DRV_Reg32(regp));
		#if NAND_READ_PERFORMANCE
        do_gettimeofday(&stimer);
        for (i = 256; i < 512; i++)
        {
            mtk_nand_read_page(mtd, nand_chip, local_buffer, i);
            MSG(INIT,"[%d]0x%x %x %x %x\n",i, *(int *)local_buffer, *((int *)local_buffer+1), *((int *)local_buffer+2), *((int *)local_buffer+3));
            MSG(INIT,"[%d]0x%x %x %x %x\n",i, *(int *)local_buffer+4, *((int *)local_buffer+5), *((int *)local_buffer+6), *((int *)local_buffer+7));
            MSG(INIT,"[%d]0x%x %x %x %x\n",i, *(int *)local_buffer+8, *((int *)local_buffer+9), *((int *)local_buffer+10), *((int *)local_buffer+11));
            MSG(INIT,"[%d]0x%x %x %x %x\n",i, *(int *)local_buffer+12, *((int *)local_buffer+13), *((int *)local_buffer+14), *((int *)local_buffer+15));
        }
        do_gettimeofday(&etimer);
        printk("[NAND Read Perf.Test] %d MB/s\n", (g_page_size*256)/Cal_timediff(&etimer,&stimer));
        while(1);
        #endif
	}

#if !defined (CONFIG_NAND_BOOTLOADER)
    if (!g_bmt)
    {
    		#if defined(MTK_COMBO_NAND_SUPPORT)
    		#if CFG_COMBO_NAND
            if (!(g_bmt = init_bmt(nand_chip, bmt_sz)))
    		#else
    		if (!(g_bmt = init_bmt(nand_chip, ((PART_SIZE_BMTPOOL) >> nand_chip->phys_erase_shift))))
    		#endif
    		#else    		    		
        printf("BMT_POOL_SIZE=%d\n",BMT_POOL_SIZE);
        if (!(g_bmt = init_bmt(nand_chip, BMT_POOL_SIZE)))
        #endif
        {
            MSG(INIT, "Error: init bmt failed\n");
            return 0;
        }
    }

    nand_chip->chipsize -= (PMT_POOL_SIZE) << nand_chip->phys_erase_shift;
#endif    
mtd->size = nand_chip->chipsize;
#if KERNEL_NAND_UNIT_TEST
		MSG(INIT, "Do mtk_nand_unit_test!!\n");
    err = mtk_nand_unit_test(nand_chip, mtd);
    if (err == 0)
    {
        printk("Thanks to GOD, UNIT Test OK!\n");
    }
#endif
#ifdef PMT
#if !defined (CONFIG_NAND_BOOTLOADER)
    part_init_pmt(mtd, (u8 *) & g_exist_Partition[0]);
    err = mtd_device_register(mtd, g_exist_Partition, part_num);
#endif
#else
#if !defined (CONFIG_NAND_BOOTLOADER)
    err = mtd_device_register(mtd, g_pasStatic_Partition, part_num);
#endif
#endif

#ifdef _MTK_NAND_DUMMY_DRIVER_
    dummy_driver_debug = 0;
#endif

    /* Successfully!! */
    if (!err)
    {
        MSG(INIT, "[mtk_nand] probe successfully!\n");
        nand_disable_clock();
        return err;
    }

    /* Fail!! */
  out:
    MSG(INIT, "[NFI] mtk_nand_probe fail, err = %d!\n", err);
    nand_release(mtd);
#if !defined (CONFIG_NAND_BOOTLOADER)    
    platform_set_drvdata(pdev, NULL);
    kfree(host);
#endif    
    nand_disable_clock();
    return err;
}
/******************************************************************************
 * mtk_nand_suspend
 *
 * DESCRIPTION:
 *   Suspend the nand device!
 *
 * PARAMETERS:
 *   struct platform_device *pdev : device structure
 *
 * RETURNS:
 *   0 : Success
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
#if !defined (CONFIG_NAND_BOOTLOADER)
static int mtk_nand_suspend(struct platform_device *pdev, pm_message_t state)
{
      struct mtk_nand_host *host = platform_get_drvdata(pdev);
//      struct mtd_info *mtd = &host->mtd;
      // backup register
      #ifdef CONFIG_PM

      if(host->saved_para.suspend_flag==0)
      {
          nand_enable_clock();
          // Save NFI register
          host->saved_para.sNFI_CNFG_REG16 = DRV_Reg16(NFI_CNFG_REG16);
          host->saved_para.sNFI_PAGEFMT_REG16 = DRV_Reg16(NFI_PAGEFMT_REG16);
          host->saved_para.sNFI_CON_REG16 = DRV_Reg32(NFI_CON_REG16);
          host->saved_para.sNFI_ACCCON_REG32 = DRV_Reg32(NFI_ACCCON_REG32);
          host->saved_para.sNFI_INTR_EN_REG16 = DRV_Reg16(NFI_INTR_EN_REG16);
          host->saved_para.sNFI_IOCON_REG16 = DRV_Reg16(NFI_IOCON_REG16);
          host->saved_para.sNFI_CSEL_REG16 = DRV_Reg16(NFI_CSEL_REG16);
          host->saved_para.sNFI_DEBUG_CON1_REG16 = DRV_Reg16(NFI_DEBUG_CON1_REG16);

          // save ECC register
          host->saved_para.sECC_ENCCNFG_REG32 = DRV_Reg32(ECC_ENCCNFG_REG32);
//          host->saved_para.sECC_FDMADDR_REG32 = DRV_Reg32(ECC_FDMADDR_REG32);
          host->saved_para.sECC_DECCNFG_REG32 = DRV_Reg32(ECC_DECCNFG_REG32);
		  // for sync mode
		  if (g_bSyncOrToggle)
		  {
			  host->saved_para.sNFI_DLYCTRL_REG32 = DRV_Reg32(NFI_DLYCTRL_REG32);
			  host->saved_para.sPERI_NFI_MAC_CTRL = DRV_Reg32(PERI_NFI_MAC_CTRL);
			  host->saved_para.sNFI_NAND_TYPE_CNFG_REG32 = DRV_Reg32(NFI_NAND_TYPE_CNFG_REG32);
			  host->saved_para.sNFI_ACCCON1_REG32 = DRV_Reg32(NFI_ACCCON1_REG3);
		  }
		  #ifdef MTK_PMIC_MT6397
		  hwPowerDown(MT65XX_POWER_LDO_VMCH, "NFI");
		  #else
		  hwPowerDown(MT6323_POWER_LDO_VMCH, "NFI");
		  #endif
          nand_disable_clock();
          host->saved_para.suspend_flag=1;
      }
      else
      {
          MSG(POWERCTL, "[NFI] Suspend twice !\n");
      }
      #endif

      MSG(POWERCTL, "[NFI] Suspend !\n");
      return 0;
}
#endif
/******************************************************************************
 * mtk_nand_resume
 *
 * DESCRIPTION:
 *   Resume the nand device!
 *
 * PARAMETERS:
 *   struct platform_device *pdev : device structure
 *
 * RETURNS:
 *   0 : Success
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
#if !defined (CONFIG_NAND_BOOTLOADER)
static int mtk_nand_resume(struct platform_device *pdev)
{
    struct mtk_nand_host *host = platform_get_drvdata(pdev);
    //struct mtd_info *mtd = &host->mtd;  //for test
//	struct nand_chip *chip = mtd->priv;
	//struct gFeatureSet *feature_set = &(devinfo.feature_set.FeatureSet); //for test
	//int val = -1;   // for test

#ifdef CONFIG_PM

        if(host->saved_para.suspend_flag==1)
        {
            nand_enable_clock();
            // restore NFI register
			#ifdef MTK_PMIC_MT6397
			hwPowerOn(MT65XX_POWER_LDO_VMCH, VOL_3300, "NFI");
			#else
			hwPowerOn(MT6323_POWER_LDO_VMCH, VOL_3300, "NFI");
			#endif
				mtk_nand_device_reset();
            DRV_WriteReg16(NFI_CNFG_REG16 ,host->saved_para.sNFI_CNFG_REG16);
            DRV_WriteReg16(NFI_PAGEFMT_REG16 ,host->saved_para.sNFI_PAGEFMT_REG16);
            DRV_WriteReg32(NFI_CON_REG16 ,host->saved_para.sNFI_CON_REG16);
            DRV_WriteReg32(NFI_ACCCON_REG32 ,host->saved_para.sNFI_ACCCON_REG32);
            DRV_WriteReg16(NFI_IOCON_REG16 ,host->saved_para.sNFI_IOCON_REG16);
            DRV_WriteReg16(NFI_CSEL_REG16 ,host->saved_para.sNFI_CSEL_REG16);
            DRV_WriteReg16(NFI_DEBUG_CON1_REG16 ,host->saved_para.sNFI_DEBUG_CON1_REG16);

            // restore ECC register
            DRV_WriteReg32(ECC_ENCCNFG_REG32 ,host->saved_para.sECC_ENCCNFG_REG32);
//            DRV_WriteReg32(ECC_FDMADDR_REG32 ,host->saved_para.sECC_FDMADDR_REG32);
            DRV_WriteReg32(ECC_DECCNFG_REG32 ,host->saved_para.sECC_DECCNFG_REG32);

            // Reset NFI and ECC state machine
            /* Reset the state machine and data FIFO, because flushing FIFO */
            (void)mtk_nand_reset();
            // Reset ECC
            DRV_WriteReg16(ECC_DECCON_REG16, DEC_DE);
            while (!DRV_Reg16(ECC_DECIDLE_REG16));

            DRV_WriteReg16(ECC_ENCCON_REG16, ENC_DE);
            while (!DRV_Reg32(ECC_ENCIDLE_REG32));


            /* Initilize interrupt. Clear interrupt, read clear. */
            DRV_Reg16(NFI_INTR_REG16);

            DRV_WriteReg16(NFI_INTR_EN_REG16 ,host->saved_para.sNFI_INTR_EN_REG16);
			
			//mtk_nand_interface_config(&host->mtd);
			if (g_bSyncOrToggle)
			{
				NFI_CLN_REG32(NFI_DEBUG_CON1_REG16,HWDCM_SWCON_ON);
				NFI_CLN_REG32(NFI_DEBUG_CON1_REG16,NFI_BYPASS);
				NFI_CLN_REG32(ECC_BYPASS_REG32,ECC_BYPASS);
				DRV_WriteReg32(PERICFG_BASE+0x5C, 0x0);
				NFI_SET_REG32(PERI_NFI_CLK_SOURCE_SEL, NFI_PAD_1X_CLOCK);
#if !defined (CONFIG_CLKMGR_BRINGUP)				
				clkmux_sel(MT_MUX_NFI2X,g_iNFI2X_CLKSRC,"NFI");
#endif				
				DRV_WriteReg32(NFI_DLYCTRL_REG32, host->saved_para.sNFI_DLYCTRL_REG32);
				DRV_WriteReg32(PERI_NFI_MAC_CTRL, host->saved_para.sPERI_NFI_MAC_CTRL);
				while(0 == (DRV_Reg32(NFI_STA_REG32) && STA_FLASH_MACRO_IDLE));
				DRV_WriteReg16(NFI_NAND_TYPE_CNFG_REG32, host->saved_para.sNFI_NAND_TYPE_CNFG_REG32);
				DRV_WriteReg32(NFI_ACCCON1_REG3,host->saved_para.sNFI_ACCCON1_REG32);
			}
			//mtk_nand_GetFeature(mtd, feature_set->gfeatureCmd, \
			//feature_set->Interface.address, (u8 *)&val,4);
			//MSG(POWERCTL, "[NFI] Resume feature %d!\n", val);
            nand_disable_clock();
            host->saved_para.suspend_flag = 0;
        }
        else
        {
            MSG(POWERCTL, "[NFI] Resume twice !\n");
        }
#endif
    MSG(POWERCTL, "[NFI] Resume !\n");
    return 0;
}
#endif
/******************************************************************************
 * mtk_nand_remove
 *
 * DESCRIPTION:
 *   unregister the nand device file operations !
 *
 * PARAMETERS:
 *   struct platform_device *pdev : device structure
 *
 * RETURNS:
 *   0 : Success
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
#if !defined (CONFIG_NAND_BOOTLOADER)
static int mtk_nand_remove(struct platform_device *pdev)
{
    struct mtk_nand_host *host = platform_get_drvdata(pdev);
    struct mtd_info *mtd = &host->mtd;

    nand_release(mtd);

    kfree(host);

    nand_disable_clock();

    return 0;
}
#endif
/******************************************************************************
 * NAND OTP operations
 * ***************************************************************************/
#if (defined(NAND_OTP_SUPPORT) && SAMSUNG_OTP_SUPPORT)
unsigned int samsung_OTPQueryLength(unsigned int *QLength)
{
    *QLength = SAMSUNG_OTP_PAGE_NUM * g_page_size;
    return 0;
}

unsigned int samsung_OTPRead(unsigned int PageAddr, void *BufferPtr, void *SparePtr)
{
    struct mtd_info *mtd = &host->mtd;
    unsigned int rowaddr, coladdr;
    unsigned int u4Size = g_page_size;
    unsigned int timeout = 0xFFFF;
    unsigned int bRet;
    unsigned int sec_num = mtd->writesize >> host->hw->nand_sec_shift;

    if (PageAddr >= SAMSUNG_OTP_PAGE_NUM)
    {
        return OTP_ERROR_OVERSCOPE;
    }

    /* Col -> Row; LSB first */
    coladdr = 0x00000000;
    rowaddr = Samsung_OTP_Page[PageAddr];

    MSG(OTP, "[%s]:(COLADDR) [0x%08x]/(ROWADDR)[0x%08x]\n", __func__, coladdr, rowaddr);

    /* Power on NFI HW component. */
#if defined (CONFIG_NAND_BOOTLOADER)
    nand_get_device(mtd->priv, mtd, L_READING);
#else
nand_get_device(mtd, FL_READING);
#endif
    mtk_nand_reset();
    (void)mtk_nand_set_command(0x30);
    mtk_nand_reset();
    (void)mtk_nand_set_command(0x65);

    MSG(OTP, "[%s]: Start to read data from OTP area\n", __func__);

    if (!mtk_nand_reset())
    {
        bRet = OTP_ERROR_RESET;
        goto cleanup;
    }

    mtk_nand_set_mode(CNFG_OP_READ);
    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
    DRV_WriteReg32(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

    DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(BufferPtr));
    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);

    if (g_bHwEcc)
    {
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
    } else
    {
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
    }
    mtk_nand_set_autoformat(true);
    if (g_bHwEcc)
    {
        ECC_Decode_Start();
    }
    if (!mtk_nand_set_command(NAND_CMD_READ0))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mtk_nand_set_address(coladdr, rowaddr, 2, 3))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mtk_nand_set_command(NAND_CMD_READSTART))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mtk_nand_status_ready(STA_NAND_BUSY))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mtk_nand_read_page_data(mtd, BufferPtr, u4Size))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mtk_nand_status_ready(STA_NAND_BUSY))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    mtk_nand_read_fdm_data(SparePtr, sec_num);

    mtk_nand_stop_read();

    MSG(OTP, "[%s]: End to read data from OTP area\n", __func__);

    bRet = OTP_SUCCESS;

  cleanup:

    mtk_nand_reset();
    (void)mtk_nand_set_command(0xFF);
    nand_release_device(mtd);
    return bRet;
}

unsigned int samsung_OTPWrite(unsigned int PageAddr, void *BufferPtr, void *SparePtr)
{
    struct mtd_info *mtd = &host->mtd;
    unsigned int rowaddr, coladdr;
    unsigned int u4Size = g_page_size;
    unsigned int timeout = 0xFFFF;
    unsigned int bRet;
    unsigned int sec_num = mtd->writesize >> 9;

    if (PageAddr >= SAMSUNG_OTP_PAGE_NUM)
    {
        return OTP_ERROR_OVERSCOPE;
    }

    /* Col -> Row; LSB first */
    coladdr = 0x00000000;
    rowaddr = Samsung_OTP_Page[PageAddr];

    MSG(OTP, "[%s]:(COLADDR) [0x%08x]/(ROWADDR)[0x%08x]\n", __func__, coladdr, rowaddr);
#if defined (CONFIG_NAND_BOOTLOADER)
    nand_get_device(mtd->priv, mtd, FL_READING);
#else
nand_get_device(mtd, FL_READING);
#endif
    mtk_nand_reset();
    (void)mtk_nand_set_command(0x30);
    mtk_nand_reset();
    (void)mtk_nand_set_command(0x65);

    MSG(OTP, "[%s]: Start to write data to OTP area\n", __func__);

    if (!mtk_nand_reset())
    {
        bRet = OTP_ERROR_RESET;
        goto cleanup;
    }

    mtk_nand_set_mode(CNFG_OP_PRGM);

    NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_READ_EN);

    DRV_WriteReg32(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

    DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(BufferPtr));
    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);

    if (g_bHwEcc)
    {
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
    } else
    {
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
    }
    mtk_nand_set_autoformat(true);

    ECC_Encode_Start();

    if (!mtk_nand_set_command(NAND_CMD_SEQIN))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mtk_nand_set_address(coladdr, rowaddr, 2, 3))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mtk_nand_status_ready(STA_NAND_BUSY))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    mtk_nand_write_fdm_data((struct nand_chip *)mtd->priv, BufferPtr, sec_num);
    (void)mtk_nand_write_page_data(mtd, BufferPtr, u4Size);
    if (!mtk_nand_check_RW_count(u4Size))
    {
        MSG(OTP, "[%s]: Check RW count timeout !\n", __func__);
        bRet = OTP_ERROR_TIMEOUT;
        goto cleanup;
    }

    mtk_nand_stop_write();
    (void)mtk_nand_set_command(NAND_CMD_PAGEPROG);
    while (DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY) ;

    bRet = OTP_SUCCESS;

    MSG(OTP, "[%s]: End to write data to OTP area\n", __func__);

  cleanup:
    mtk_nand_reset();
    (void)mtk_nand_set_command(	NAND_CMD_RESET);
    nand_release_device(mtd);
    return bRet;
}

static int mt_otp_open(struct inode *inode, struct file *filp)
{
    MSG(OTP, "[%s]:(MAJOR)%d:(MINOR)%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
    filp->private_data = (int *)OTP_MAGIC_NUM;
    return 0;
}

static int mt_otp_release(struct inode *inode, struct file *filp)
{
    MSG(OTP, "[%s]:(MAJOR)%d:(MINOR)%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
    return 0;
}

static int mt_otp_access(unsigned int access_type, unsigned int offset, void *buff_ptr, unsigned int length, unsigned int *status)
{
    unsigned int i = 0, ret = 0;
    char *BufAddr = (char *)buff_ptr;
    unsigned int PageAddr, AccessLength = 0;
    int Status = 0;

    static char *p_D_Buff = NULL;
    char S_Buff[64];

    if (!(p_D_Buff = kmalloc(g_page_size, GFP_KERNEL)))
    {
        ret = -ENOMEM;
        *status = OTP_ERROR_NOMEM;
        goto exit;
    }

    MSG(OTP, "[%s]: %s (0x%x) length:(%d bytes) !\n", __func__, access_type ? "WRITE" : "READ", offset, length);

    while (1)
    {
        PageAddr = offset / g_page_size;
        if (FS_OTP_READ == access_type)
        {
            memset(p_D_Buff, 0xff, g_page_size);
            memset(S_Buff, 0xff, (sizeof(char) * 64));

            MSG(OTP, "[%s]: Read Access of page (%d)\n", __func__, PageAddr);

            Status = g_mtk_otp_fuc.OTPRead(PageAddr, p_D_Buff, &S_Buff);
            *status = Status;

            if (OTP_SUCCESS != Status)
            {
                MSG(OTP, "[%s]: Read status (%d)\n", __func__, Status);
                break;
            }

            AccessLength = g_page_size - (offset % g_page_size);

            if (length >= AccessLength)
            {
                memcpy(BufAddr, (p_D_Buff + (offset % g_page_size)), AccessLength);
            } else
            {
                //last time
                memcpy(BufAddr, (p_D_Buff + (offset % g_page_size)), length);
            }
        } else if (FS_OTP_WRITE == access_type)
        {
            AccessLength = g_page_size - (offset % g_page_size);
            memset(p_D_Buff, 0xff, g_page_size);
            memset(S_Buff, 0xff, (sizeof(char) * 64));

            if (length >= AccessLength)
            {
                memcpy((p_D_Buff + (offset % g_page_size)), BufAddr, AccessLength);
            } else
            {
                //last time
                memcpy((p_D_Buff + (offset % g_page_size)), BufAddr, length);
            }

            Status = g_mtk_otp_fuc.OTPWrite(PageAddr, p_D_Buff, &S_Buff);
            *status = Status;

            if (OTP_SUCCESS != Status)
            {
                MSG(OTP, "[%s]: Write status (%d)\n", __func__, Status);
                break;
            }
        } else
        {
            MSG(OTP, "[%s]: Error, not either read nor write operations !\n", __func__);
            break;
        }

        offset += AccessLength;
        BufAddr += AccessLength;
        if (length <= AccessLength)
        {
            length = 0;
            break;
        } else
        {
            length -= AccessLength;
            MSG(OTP, "[%s]: Remaining %s (%d) !\n", __func__, access_type ? "WRITE" : "READ", length);
        }
    }
  error:
    kfree(p_D_Buff);
  exit:
    return ret;
}

static long mt_otp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0, i = 0;
    static char *pbuf = NULL;

    void __user *uarg = (void __user *)arg;
    struct otp_ctl otpctl;

    /* Lock */
    spin_lock(&g_OTPLock);

    if (copy_from_user(&otpctl, uarg, sizeof(struct otp_ctl)))
    {
        ret = -EFAULT;
        goto exit;
    }

    if (false == g_bInitDone)
    {
        MSG(OTP, "ERROR: NAND Flash Not initialized !!\n");
        ret = -EFAULT;
        goto exit;
    }

    if (!(pbuf = kmalloc(sizeof(char) * otpctl.Length, GFP_KERNEL)))
    {
        ret = -ENOMEM;
        goto exit;
    }

    switch (cmd)
    {
      case OTP_GET_LENGTH:
          MSG(OTP, "OTP IOCTL: OTP_GET_LENGTH\n");
          g_mtk_otp_fuc.OTPQueryLength(&otpctl.QLength);
          otpctl.status = OTP_SUCCESS;
          MSG(OTP, "OTP IOCTL: The Length is %d\n", otpctl.QLength);
          break;
      case OTP_READ:
          MSG(OTP, "OTP IOCTL: OTP_READ Offset(0x%x), Length(0x%x) \n", otpctl.Offset, otpctl.Length);
          memset(pbuf, 0xff, sizeof(char) * otpctl.Length);

          mt_otp_access(FS_OTP_READ, otpctl.Offset, pbuf, otpctl.Length, &otpctl.status);

          if (copy_to_user(otpctl.BufferPtr, pbuf, (sizeof(char) * otpctl.Length)))
          {
              MSG(OTP, "OTP IOCTL: Copy to user buffer Error !\n");
              goto error;
          }
          break;
      case OTP_WRITE:
          MSG(OTP, "OTP IOCTL: OTP_WRITE Offset(0x%x), Length(0x%x) \n", otpctl.Offset, otpctl.Length);
          if (copy_from_user(pbuf, otpctl.BufferPtr, (sizeof(char) * otpctl.Length)))
          {
              MSG(OTP, "OTP IOCTL: Copy from user buffer Error !\n");
              goto error;
          }
          mt_otp_access(FS_OTP_WRITE, otpctl.Offset, pbuf, otpctl.Length, &otpctl.status);
          break;
      default:
          ret = -EINVAL;
    }

    ret = copy_to_user(uarg, &otpctl, sizeof(struct otp_ctl));

  error:
    kfree(pbuf);
  exit:
    spin_unlock(&g_OTPLock);
    return ret;
}

static struct file_operations nand_otp_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = mt_otp_ioctl,
    .open = mt_otp_open,
    .release = mt_otp_release,
};

static struct miscdevice nand_otp_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "otp",
    .fops = &nand_otp_fops,
};
#endif

/******************************************************************************
Device driver structure
******************************************************************************/
#if !defined (CONFIG_NAND_BOOTLOADER)
static struct platform_driver mtk_nand_driver = {
    .probe = mtk_nand_probe,
    .remove = mtk_nand_remove,
    .suspend = mtk_nand_suspend,
    .resume = mtk_nand_resume,
    .driver = {
               .name = "mtk-nand",
               .owner = THIS_MODULE,
               },
};
#endif
/******************************************************************************
 * mtk_nand_init
 *
 * DESCRIPTION:
 *   Init the device driver !
 *
 * PARAMETERS:
 *   None
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
#if !defined (CONFIG_NAND_BOOTLOADER)
static const struct file_operations mtk_nand_fops = {
    .read = mtk_nand_proc_read,
	.write = mtk_nand_proc_write,
};
static int __init mtk_nand_init(void)
#else
static int mtk_nand_init(void)
#endif
{
    struct proc_dir_entry *entry;
    g_i4Interrupt = 0;

#if defined(NAND_OTP_SUPPORT)
    int err = 0;
    MSG(OTP, "OTP: register NAND OTP device ...\n");
    err = misc_register(&nand_otp_dev);
    if (unlikely(err))
    {
        MSG(OTP, "OTP: failed to register NAND OTP device!\n");
        return err;
    }
    spin_lock_init(&g_OTPLock);
#endif

#if (defined(NAND_OTP_SUPPORT) && SAMSUNG_OTP_SUPPORT)
    g_mtk_otp_fuc.OTPQueryLength = samsung_OTPQueryLength;
    g_mtk_otp_fuc.OTPRead = samsung_OTPRead;
    g_mtk_otp_fuc.OTPWrite = samsung_OTPWrite;
#endif

#if !defined (CONFIG_NAND_BOOTLOADER)
    entry = proc_create(PROCNAME, 0664, NULL, &mtk_nand_fops);
	if(entry)
	 {
		  printk("[%s]: successfully create /driver/nand \n", __func__);
	 }else{
		  printk("[%s]: failed to create /driver/nand \n", __func__);
	 }
#endif
	#if 0//removed in kernel 3.10
	entry = create_proc_entry(PROCNAME, 0664, NULL);
    if (entry == NULL)
    {
        MSG(INIT, "MTK Nand : unable to create /proc entry\n");
        return -ENOMEM;
    }
    entry->read_proc = mtk_nand_proc_read;
    entry->write_proc = mtk_nand_proc_write;
	#endif

#if !defined (CONFIG_NAND_BOOTLOADER)
    return platform_driver_register(&mtk_nand_driver);
#endif    
}

/******************************************************************************
 * mtk_nand_exit
 *
 * DESCRIPTION:
 *   Free the device driver !
 *
 * PARAMETERS:
 *   None
 *
 * RETURNS:
 *   None
 *
 * NOTES:
 *   None
 *
 ******************************************************************************/
static void mtk_nand_exit(void)
{
    MSG(INIT, "MediaTek Nand driver exit, version %s\n", VERSION);
#if defined(NAND_OTP_SUPPORT)
    misc_deregister(&nand_otp_dev);
#endif

#ifdef SAMSUNG_OTP_SUPPORT
    g_mtk_otp_fuc.OTPQueryLength = NULL;
    g_mtk_otp_fuc.OTPRead = NULL;
    g_mtk_otp_fuc.OTPWrite = NULL;
#endif
#if !defined (CONFIG_NAND_BOOTLOADER)
    platform_driver_unregister(&mtk_nand_driver);
    remove_proc_entry(PROCNAME, NULL);
#endif    
}

#if !defined (CONFIG_NAND_BOOTLOADER)
module_init(mtk_nand_init);
module_exit(mtk_nand_exit);
MODULE_LICENSE("GPL");
#endif
