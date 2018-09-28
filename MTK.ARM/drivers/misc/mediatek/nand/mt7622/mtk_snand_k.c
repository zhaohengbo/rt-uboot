/******************************************************************************
* mtk_snand.c - MTK NAND Flash Device Driver
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

//#if !defined (ON_BOARD_NAND_FLASH_COMPONENT)
#if 0
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
#include <mach/mtk_snand_k.h>
#include <mach/dma.h>
#include <mach/devs.h>
//#include <mach/mt_reg_base.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_clkmgr.h>
#include <mach/bmt.h>
#include <mach/mt_irq.h>
#include "partition.h"
//#include <asm/system.h>
#include <mach/partition_define.h>
#include <mach/mt_boot.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

//#include "../../../../../../source/kernel/drivers/aee/ipanic/ipanic.h"
#include <linux/rtc.h>
#else
#include <common.h>
#include <linux/string.h>
#include <config.h>
#include <malloc.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand_ecc.h>
#include <asm/arch-mt7622/mt_typedefs.h>
#include <asm/arch-mt7622/nand/mtk_nand.h>
#include <asm/arch-mt7622/nand/mtk_snand_k.h>
#include <asm/arch-mt7622/nand/bmt.h>
#include <asm/arch-mt7622/nand/snand_device_list.h>
#include <asm/arch-mt7622/nand/partition_define.h>
#define printk	printf

/* NAND driver */
struct mtk_nand_host_hw {
    unsigned int nfi_bus_width;            /* NFI_BUS_WIDTH */ 
    unsigned int nfi_access_timing;        /* NFI_ACCESS_TIMING */  
    unsigned int nfi_cs_num;               /* NFI_CS_NUM */
    unsigned int nand_sec_size;            /* NAND_SECTOR_SIZE */
    unsigned int nand_sec_shift;           /* NAND_SECTOR_SHIFT */
    unsigned int nand_ecc_size;
    unsigned int nand_ecc_bytes;
    unsigned int nand_ecc_mode;
};

#define NFI_DEFAULT_ACCESS_TIMING        0x30c77fff//0x44333
#define NFI_CS_NUM                  	 (2)
struct mtk_nand_host_hw mtk_nand_hw = {
	.nfi_bus_width          	= 8,
	.nfi_access_timing		= NFI_DEFAULT_ACCESS_TIMING,
	.nfi_cs_num			= NFI_CS_NUM,
	.nand_sec_size			= 512,
	.nand_sec_shift			= 9,
	.nand_ecc_size			= 2048,
	.nand_ecc_bytes			= 32,
	.nand_ecc_mode			= NAND_ECC_HW,
};

#define xlog_printk	fprintf
#define ANDROID_LOG_WARN	stderr
#define ANDROID_LOG_INFO	stderr
#define ANDROID_LOG_ERROR	stderr
#define MTD_MAX_OOBFREE_ENTRIES      16	
#define NFI_DEFAULT_CS                          (0)
#define mb() __asm__ __volatile__("": : :"memory")
#define ASSERT	assert
#endif

#define VERSION  	"v1.0"
#define MODULE_NAME	"# MTK SNAND #"
#define PROCNAME    "driver/nand"
#define PMT 							1
#define _MTK_NAND_DUMMY_DRIVER_
#define __INTERNAL_USE_AHB_MODE__ 	(1)

void show_stack(struct task_struct *tsk, unsigned long *sp);
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq,unsigned int polarity);

static void mtk_snand_reset_dev(void);

int mtk_nand_unit_test(struct nand_chip *nand_chip, struct mtd_info *mtd);

void __iomem *mtk_nfi_base;
void __iomem *mtk_nfiecc_base;
void __iomem *mtk_io_base;

struct device_node *mtk_nfi_node = NULL;
struct device_node *mtk_nfiecc_node = NULL;
struct device_node *mtk_io_node = NULL;

unsigned int nfi_irq = 0;
#define MT_NFI_IRQ_ID nfi_irq

#if defined(MTK_MLC_NAND_SUPPORT)
bool MLC_DEVICE = TRUE;// to build pass xiaolei
#else
bool MLC_DEVICE = FALSE;
#endif

typedef enum
{
     SNAND_RB_DEFAULT    = 0
    ,SNAND_RB_READ_ID    = 1
    ,SNAND_RB_CMD_STATUS = 2
    ,SNAND_RB_PIO        = 3
} SNAND_Read_Byte_Mode;

typedef enum
{
     SNAND_IDLE                 = 0
    ,SNAND_DEV_ERASE_DONE
    ,SNAND_DEV_PROG_DONE
    ,SNAND_BUSY
    ,SNAND_NFI_CUST_READING
    ,SNAND_NFI_AUTO_ERASING
    ,SNAND_DEV_PROG_EXECUTING
} SNAND_Status;

SNAND_Read_Byte_Mode    g_snand_read_byte_mode 	= SNAND_RB_DEFAULT;
SNAND_Status            g_snand_dev_status     	= SNAND_IDLE;    // used for mtk_snand_dev_ready() and interrupt waiting service
int						g_snand_cmd_status		= NAND_STATUS_READY;

u8 g_snand_id_data[SNAND_MAX_ID + 1];
u8 g_snand_id_data_idx = 0;

// Read Split related definitions and variables
#define SNAND_RS_BOUNDARY_BLOCK                     (2)
#define SNAND_RS_BOUNDARY_KB                        (1024)
#define SNAND_RS_SPARE_PER_SECTOR_FIRST_PART_VAL    (16)                                        // MT6572 shoud fix this as 16
#define SNAND_RS_SPARE_PER_SECTOR_FIRST_PART_NFI    (PAGEFMT_SPARE_16 << PAGEFMT_SPARE_SHIFT)   // MT6572 shoud fix this as 16
#define SNAND_RS_ECC_BIT_FIRST_PART                 (4)                                         // MT6572 shoud fix this as 4

u32 g_snand_rs_ecc_bit_second_part;
u32 g_snand_rs_spare_per_sector_second_part_nfi;
u32 g_snand_rs_num_page = 0;
u32 g_snand_rs_cur_part = 0xFFFFFFFF;

u32 g_snand_spare_per_sector = 0;   // because Read Split feature will change spare_per_sector in run-time, thus use a variable to indicate current spare_per_sector

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

#define NAND_SECTOR_SIZE    (512)
#define OOB_PER_SECTOR      (28)
#define OOB_AVAI_PER_SECTOR (8)

__attribute__((aligned(16))) unsigned char g_snand_spare[128];
__attribute__((aligned(16))) unsigned char g_snand_temp[4096 + 128];


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
u32 g_bmt_sz = BMT_POOL_SIZE;
#endif

#define PMT_POOL_SIZE	(2)
/* NOTE(Bayi) */
#define GPIO_BASE    0x10211000
#define GPIO_MODE(x) (GPIO_BASE + 0x300 + 0x10*x)
u32 SNFI_GPIO_BACKUP[3];
// delay latency
#define SPI_NAND_RESET_LATENCY_US               (3 * 10)    // 0.3 us * 100
#define SPI_NAND_MAX_RDY_AFTER_RST_LATENCY_US   (10000)     // 10 ms
#define SPI_NAND_MAX_READ_BUSY_US               (150 * 40)  // period * 40, safe value (plus ECC engine processing time)
#define NFI_MAX_RESET_US                        (10)         // [By Designer Curtis Tsai] 50T @ 26Mhz (1923 ns) is enough

/*******************************************************************************
 * Gloable Varible Definition
 *******************************************************************************/
struct nand_perf_log
{
    unsigned int ReadPageCount;
    suseconds_t  ReadPageTotalTime;
    unsigned int ReadBusyCount;
    suseconds_t  ReadBusyTotalTime;
    unsigned int ReadDMACount;
    suseconds_t  ReadDMATotalTime;

    unsigned int WritePageCount;
    suseconds_t  WritePageTotalTime;
    unsigned int WriteBusyCount;
    suseconds_t  WriteBusyTotalTime;
    unsigned int WriteDMACount;
    suseconds_t  WriteDMATotalTime;

    unsigned int EraseBlockCount;
    suseconds_t  EraseBlockTotalTime;

    unsigned int ReadSectorCount;
    suseconds_t  ReadSectorTotalTime;
};

static struct nand_perf_log g_NandPerfLog={0};
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
static u32 g_value = 0;
static int g_page_size;
static int g_block_size;

#if __INTERNAL_USE_AHB_MODE__
BOOL g_bHwEcc = true;
#else
BOOL g_bHwEcc = false;
#endif

#define SEQ_printf(m, x...) \
do { \
   if (m) \
       seq_printf(m, x); \
   else \
       printk(x); \
} while (0)

#define SNAND_MAX_PAGE_SIZE	(4096)
#define _SNAND_CACHE_LINE_SIZE  (128)

static u8 *local_buffer_16_align;   // 16 byte aligned buffer, for HW issue
__attribute__((aligned(_SNAND_CACHE_LINE_SIZE))) static u8 local_buffer[SNAND_MAX_PAGE_SIZE*5 + _SNAND_CACHE_LINE_SIZE];

extern void nand_release_device(struct mtd_info *mtd);
extern int nand_get_device(struct nand_chip *chip, struct mtd_info *mtd, int new_state);

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
extern struct mtd_partition g_pasStatic_Partition[];
//int part_num = NUM_PARTITIONS;	/* TODO(Nelson): check it!!! */
extern int part_num;
#ifdef PMT
extern void part_init_pmt(struct mtd_info *mtd, u8 * buf);
extern struct mtd_partition g_exist_Partition[];
#endif
int manu_id;
int dev_id;

static u8 local_oob_buf[NAND_MAX_OOBSIZE];

#ifdef _MTK_NAND_DUMMY_DRIVER_
int dummy_driver_debug;
#endif

#if CFG_FPGA_PLATFORM
void nand_enable_clock(void)
{
}

void nand_disable_clock(void)
{
}
#else
void nand_enable_clock(void)
{
    enable_clock(MT_CG_NFI_SW_CG, "NAND");
    enable_clock(MT_CG_NFIECC_SW_CG, "NAND");
    enable_clock(MT_CG_SPINFI_SW_CG, "NAND");
}

void nand_disable_clock(void)
{
    disable_clock(MT_CG_NFI_SW_CG, "NAND");
    disable_clock(MT_CG_NFIECC_SW_CG, "NAND");
    disable_clock(MT_CG_SPINFI_SW_CG, "NAND");
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

#if defined (CONFIG_NAND_BOOTLOADER)
snand_flashdev_info devinfo;
#else
static snand_flashdev_info devinfo;
#endif

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

//#define RW_GPIO_MODE6_SNAND         ((P_U32)(GPIO_BASE+0x0360)) // SFCK
//#define RW_GPIO_MODE7_SNAND         ((P_U32)(GPIO_BASE+0x0370)) // SFWP, SFOUT, SFHOLD, SFIN, SFCS1
//#define RW_GPIO_DRV0                ((P_U32)(0xF020B060))
#endif

void dump_nfi(void)
{
#if __DEBUG_NAND
    printk("~~~~Dump NFI/SNF/GPIO Register in Kernel~~~~\n");
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
    printk("NFI_LOCKSTA_REG16: 0x%x\n", DRV_Reg16(NFI_LOCKSTA_REG16));
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
    printk("NFI_SPIADDRCNTR_REG32: 0x%x\n", DRV_Reg32(NFI_SPIADDRCNTR_REG32));
    printk("NFI_SPIBYTELEN_REG32: 0x%x\n", DRV_Reg32(NFI_SPIBYTELEN_REG32));

    //printk("NFI clock : %s\n", (DRV_Reg32((volatile u32 *)(PERICFG_BASE+0x18)) & (0x1)) ? "Clock Disabled" : "Clock Enabled");
    //printk("NFI clock SEL (MT6572):0x%x: %s\n",(PERICFG_BASE+0x5C), (DRV_Reg32((volatile u32 *)(PERICFG_BASE+0x5C)) & (0x1)) ? "Half clock" : "Quarter clock");

	printk("RW_SNAND_MAC_CTL: 0x%x\n", DRV_Reg32(RW_SNAND_MAC_CTL));
	printk("RW_SNAND_MAC_OUTL: 0x%x\n", DRV_Reg32(RW_SNAND_MAC_OUTL));
	printk("RW_SNAND_MAC_INL: 0x%x\n", DRV_Reg32(RW_SNAND_MAC_INL));

	printk("RW_SNAND_RD_CTL1: 0x%x\n", DRV_Reg32(RW_SNAND_RD_CTL1));
	printk("RW_SNAND_RD_CTL2: 0x%x\n", DRV_Reg32(RW_SNAND_RD_CTL2));
	printk("RW_SNAND_RD_CTL3: 0x%x\n", DRV_Reg32(RW_SNAND_RD_CTL3));

	printk("RW_SNAND_GF_CTL1: 0x%x\n", DRV_Reg32(RW_SNAND_GF_CTL1));
	printk("RW_SNAND_GF_CTL3: 0x%x\n", DRV_Reg32(RW_SNAND_GF_CTL3));

	printk("RW_SNAND_PG_CTL1: 0x%x\n", DRV_Reg32(RW_SNAND_PG_CTL1));
	printk("RW_SNAND_PG_CTL2: 0x%x\n", DRV_Reg32(RW_SNAND_PG_CTL2));
	printk("RW_SNAND_PG_CTL3: 0x%x\n", DRV_Reg32(RW_SNAND_PG_CTL3));

	printk("RW_SNAND_ER_CTL: 0x%x\n", DRV_Reg32(RW_SNAND_ER_CTL));
	printk("RW_SNAND_ER_CTL2: 0x%x\n", DRV_Reg32(RW_SNAND_ER_CTL2));

	printk("RW_SNAND_MISC_CTL: 0x%x\n", DRV_Reg32(RW_SNAND_MISC_CTL));
	printk("RW_SNAND_MISC_CTL2: 0x%x\n", DRV_Reg32(RW_SNAND_MISC_CTL2));

	printk("RW_SNAND_DLY_CTL1: 0x%x\n", DRV_Reg32(RW_SNAND_DLY_CTL1));
	printk("RW_SNAND_DLY_CTL2: 0x%x\n", DRV_Reg32(RW_SNAND_DLY_CTL2));
	printk("RW_SNAND_DLY_CTL3: 0x%x\n", DRV_Reg32(RW_SNAND_DLY_CTL3));
	printk("RW_SNAND_DLY_CTL4: 0x%x\n", DRV_Reg32(RW_SNAND_DLY_CTL4));

	printk("RW_SNAND_STA_CTL1: 0x%x\n", DRV_Reg32(RW_SNAND_STA_CTL1));
	printk("RW_SNAND_STA_CTL2: 0x%x\n", DRV_Reg32(RW_SNAND_STA_CTL2));
	printk("RW_SNAND_STA_CTL3: 0x%x\n", DRV_Reg32(RW_SNAND_STA_CTL3));

	printk("RW_SNAND_CNFG: 0x%x\n", DRV_Reg32(RW_SNAND_CNFG));

//	printk("RW_GPIO_MODE6_SNAND: 0x%x\n", DRV_Reg32(RW_GPIO_MODE6_SNAND));
//	printk("RW_GPIO_MODE7_SNAND: 0x%x\n", DRV_Reg32(RW_GPIO_MODE7_SNAND));

//  	printk("Driving: 0x%x\n", (DRV_Reg32(RW_GPIO_DRV0) & 0x0007));

//	printk("Clock State - MT_CG_NFI_SW_CG (1=Enable): %d\n", clock_is_on(MT_CG_NFI_SW_CG));
//	printk("Clock State - MT_CG_NFIECC_SW_CG (1=Enable): %d\n", clock_is_on(MT_CG_NFIECC_SW_CG));
//	printk("Clock State - MT_CG_SPINFI_SW_CG (1=Enable): %d\n", clock_is_on(MT_CG_SPINFI_SW_CG));

#if 0	/* Nelson remove */	
	dump_clk_info_by_id(MT_CG_NFI_SW_CG);
	dump_clk_info_by_id(MT_CG_NFIECC_SW_CG);
	dump_clk_info_by_id(MT_CG_SPINFI_SW_CG);
#endif
#endif
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

#if !defined (CONFIG_NAND_BOOTLOADER)
unsigned long nand_virt_to_phys_add(unsigned long va)
{
    unsigned long pageOffset = (va & (PAGE_SIZE - 1));
    pgd_t *pgd;
    pmd_t *pmd;
    pte_t *pte;
    unsigned long pa;

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

bool mtk_snand_get_device_info(u8*id, snand_flashdev_info *devinfo)
{
    u32 i,m,n,mismatch;
    int target=-1;
    u8 target_id_len=0;

    for (i = 0; i < SNAND_CHIP_CNT; i++)
    {
		mismatch=0;

		for(m=0;m<gen_snand_FlashTable[i].id_length;m++)
		{
			if(id[m]!=gen_snand_FlashTable[i].id[m])
			{
				mismatch=1;
				break;
			}
		}

		if(mismatch == 0 && gen_snand_FlashTable[i].id_length > target_id_len)
		{
				target=i;
				target_id_len=gen_snand_FlashTable[i].id_length;
		}
    }

    if(target != -1)
    {
		MSG(INIT, "Recognize NAND: ID [");

		for(n=0;n<gen_snand_FlashTable[target].id_length;n++)
		{
			devinfo->id[n] = gen_snand_FlashTable[target].id[n];
			MSG(INIT, "%x ",devinfo->id[n]);
		}

		MSG(INIT, "], Device Name [%s], Page Size [%d]B Spare Size [%d]B Total Size [%d]MB\n",gen_snand_FlashTable[target].devicename,gen_snand_FlashTable[target].pagesize,gen_snand_FlashTable[target].sparesize,gen_snand_FlashTable[target].totalsize);
		devinfo->id_length=gen_snand_FlashTable[target].id_length;
		devinfo->blocksize = gen_snand_FlashTable[target].blocksize;    // KB
		devinfo->advancedmode = gen_snand_FlashTable[target].advancedmode;

		// SW workaround for SNAND_ADV_READ_SPLIT
		// Bayi add this workaround way for all spi nand 
        if (0xC8 == devinfo->id[0] && 0xF4 == devinfo->id[1])
        {
            devinfo->advancedmode |= (SNAND_ADV_READ_SPLIT | SNAND_ADV_VENDOR_RESERVED_BLOCKS);
        }

		devinfo->pagesize = gen_snand_FlashTable[target].pagesize;
		devinfo->sparesize = gen_snand_FlashTable[target].sparesize;
		devinfo->totalsize = gen_snand_FlashTable[target].totalsize;

		memcpy(devinfo->devicename, gen_snand_FlashTable[target].devicename, sizeof(devinfo->devicename));

		devinfo->SNF_DLY_CTL1 = gen_snand_FlashTable[target].SNF_DLY_CTL1;
		devinfo->SNF_DLY_CTL2 = gen_snand_FlashTable[target].SNF_DLY_CTL2;
		devinfo->SNF_DLY_CTL3 = gen_snand_FlashTable[target].SNF_DLY_CTL3;
		devinfo->SNF_DLY_CTL4 = gen_snand_FlashTable[target].SNF_DLY_CTL4;
		devinfo->SNF_MISC_CTL = gen_snand_FlashTable[target].SNF_MISC_CTL;
		devinfo->SNF_DRIVING = gen_snand_FlashTable[target].SNF_DRIVING;

        // init read split boundary
        //g_snand_rs_num_page = SNAND_RS_BOUNDARY_BLOCK * ((devinfo->blocksize << 10) / devinfo->pagesize);
        g_snand_rs_num_page = SNAND_RS_BOUNDARY_KB * 1024 / devinfo->pagesize;
        g_snand_spare_per_sector = devinfo->sparesize / (devinfo->pagesize / NAND_SECTOR_SIZE);

    	return true;
	}
	else
	{
	    MSG(INIT, "Not Found NAND: ID [");
		for(n=0;n<SNAND_MAX_ID;n++){
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

bool mtk_snand_is_vendor_reserved_blocks(u32 row_addr)
{
    u32 page_per_block = (devinfo.blocksize << 10) / devinfo.pagesize;
    u32 target_block = row_addr / page_per_block;

    if (devinfo.advancedmode & SNAND_ADV_VENDOR_RESERVED_BLOCKS)
    {
        if (target_block >= 2045 && target_block <= 2048)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static void mtk_snand_wait_us(u32 us)
{
    udelay(us);
}

static void mtk_snand_dev_mac_enable(SNAND_Mode mode)
{
    u32 mac;

    mac = DRV_Reg32(RW_SNAND_MAC_CTL);

    // SPI
    if (mode == SPI)
    {
        mac &= ~SNAND_MAC_SIO_SEL;   // Clear SIO_SEL to send command in SPI style
        mac |= SNAND_MAC_EN;         // Enable Macro Mode
    }
    // QPI
    else
    {
        /*
         * SFI V2: QPI_EN only effects direct read mode, and it is moved into DIRECT_CTL in V2
         *         There's no need to clear the bit again.
         */
        mac |= (SNAND_MAC_SIO_SEL | SNAND_MAC_EN);  // Set SIO_SEL to send command in QPI style, and enable Macro Mode
    }

    DRV_WriteReg32(RW_SNAND_MAC_CTL, mac);
}

/**
 * @brief This funciton triggers SFI to issue command to serial flash, wait SFI until ready.
 *
 * @remarks: !NOTE! This function must be used with mtk_snand_dev_mac_enable in pair!
 */
static void mtk_snand_dev_mac_trigger(void)
{
    u32 mac;

    mac = DRV_Reg32(RW_SNAND_MAC_CTL);

    // Trigger SFI: Set TRIG and enable Macro mode
    mac |= (SNAND_TRIG | SNAND_MAC_EN);
    DRV_WriteReg32(RW_SNAND_MAC_CTL, mac);

    /*
     * Wait for SFI ready
     * Step 1. Wait for WIP_READY becoming 1 (WIP register is ready)
     */
    while (!(DRV_Reg32(RW_SNAND_MAC_CTL) & SNAND_WIP_READY));

    /*
     * Step 2. Wait for WIP becoming 0 (Controller finishes command write process)
     */
    while ((DRV_Reg32(RW_SNAND_MAC_CTL) & SNAND_WIP));


}

/**
 * @brief This funciton leaves Macro mode and enters Direct Read mode
 *
 * @remarks: !NOTE! This function must be used after mtk_snand_dev_mac_trigger
 */
static void mtk_snand_dev_mac_leave(void)
{
    u32 mac;

    // clear SF_TRIG and leave mac mode
    mac = DRV_Reg32(RW_SNAND_MAC_CTL);

    /*
     * Clear following bits
     * SF_TRIG: Confirm the macro command sequence is completed
     * SNAND_MAC_EN: Leaves macro mode, and enters direct read mode
     * SNAND_MAC_SIO_SEL: Always reset quad macro control bit at the end
     */
    mac &= ~(SNAND_TRIG | SNAND_MAC_EN | SNAND_MAC_SIO_SEL);
    DRV_WriteReg32(RW_SNAND_MAC_CTL, mac);
}

static void mtk_snand_dev_mac_op(SNAND_Mode mode)
{
    mtk_snand_dev_mac_enable(mode);
    mtk_snand_dev_mac_trigger();
    mtk_snand_dev_mac_leave();
}

static void mtk_snand_dev_command_ext(SNAND_Mode mode, const U8 cmd[], U8 data[], const u32 outl, const u32 inl)
{
    u32   tmp;
    u32   i, j;
    P_U8  p_data, p_tmp;

    p_tmp = (P_U8)(&tmp);

    // Moving commands into SFI GPRAM
    for (i = 0, p_data = ((P_U8)RW_SNAND_GPRAM_DATA); i < outl; p_data += 4)
    {
        // Using 4 bytes aligned copy, by moving the data into the temp buffer and then write to GPRAM
        for (j = 0, tmp = 0; i < outl && j < 4; i++, j++)
        {
            p_tmp[j] = cmd[i];
        }

        DRV_WriteReg32(p_data, tmp);
    }

    DRV_WriteReg32(RW_SNAND_MAC_OUTL, outl);
    DRV_WriteReg32(RW_SNAND_MAC_INL, inl);
    mtk_snand_dev_mac_op(mode);

    // for NULL data, this loop will be skipped
    for (i = 0, p_data = ((P_U8)RW_SNAND_GPRAM_DATA + outl); i < inl; ++i, ++data, ++p_data)
    {
        *data = DRV_Reg8(p_data);
    }

    return;
}

static void mtk_snand_dev_command(const u32 cmd, u8 outlen)
{
    DRV_WriteReg32(RW_SNAND_GPRAM_DATA, cmd);
    DRV_WriteReg32(RW_SNAND_MAC_OUTL, outlen);
    DRV_WriteReg32(RW_SNAND_MAC_INL , 0);
    mtk_snand_dev_mac_op(SPI);
	mtk_snand_dev_mac_trigger();
    mtk_snand_dev_mac_leave();

    return;
}

static void mtk_snand_reset_dev()
{
    u8 cmd = SNAND_CMD_SW_RESET;

    // issue SW RESET command to device
    mtk_snand_dev_command_ext(SPI, &cmd, NULL, 1, 0);

    // wait for awhile, then polling status register (required by spec)
    mtk_snand_wait_us(SNAND_DEV_RESET_LATENCY_US);

    *RW_SNAND_GPRAM_DATA = (SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_STATUS << 8));
    *RW_SNAND_MAC_OUTL = 2;
    *RW_SNAND_MAC_INL = 1;

    // polling status register

    for (;;)
    {
        mtk_snand_dev_mac_op(SPI);

        cmd = DRV_Reg8(((P_U8)RW_SNAND_GPRAM_DATA + 2));

        if (0 == (cmd & SNAND_STATUS_OIP))
        {
            break;
        }
    }
}

static u32 mtk_snand_reverse_byte_order(u32 num)
{
   u32 ret = 0;

   ret |= ((num >> 24) & 0x000000FF);
   ret |= ((num >> 8)  & 0x0000FF00);
   ret |= ((num << 8)  & 0x00FF0000);
   ret |= ((num << 24) & 0xFF000000);

   return ret;
}

static u32 mtk_snand_gen_c1a3(const u32 cmd, const u32 address)
{
    return ((mtk_snand_reverse_byte_order(address) & 0xFFFFFF00) | (cmd & 0xFF));
}

static void mtk_snand_dev_enable_spiq(bool enable)
{
    u8   regval;
    u32  cmd;

    // read QE in status register
    cmd = SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_OTP << 8);
    DRV_WriteReg32(RW_SNAND_GPRAM_DATA, cmd);
    DRV_WriteReg32(RW_SNAND_MAC_OUTL, 2);
    DRV_WriteReg32(RW_SNAND_MAC_INL , 1);

    mtk_snand_dev_mac_op(SPI);

    regval = DRV_Reg8(((volatile u8 *)RW_SNAND_GPRAM_DATA + 2));
	//printf("[SNF] [Cheng] read status register[configuration register]:0x%x \n\r", regval);

    if (FALSE == enable)    // disable x4
    {
        if ((regval & SNAND_OTP_QE) == 0)
        {
            return;
        }
        else
        {
            regval = regval & ~SNAND_OTP_QE;
        }
    }
    else    // enable x4
    {
        if ((regval & SNAND_OTP_QE) == 1)
        {
            return;
        }
        else
        {
            regval = regval | SNAND_OTP_QE;
        }
    }

    // if goes here, it means QE needs to be set as new different value

    // write status register
    cmd = SNAND_CMD_SET_FEATURES | (SNAND_CMD_FEATURES_OTP << 8) | (regval << 16);
    DRV_WriteReg32(RW_SNAND_GPRAM_DATA, cmd);
    DRV_WriteReg32(RW_SNAND_MAC_OUTL, 3);
    DRV_WriteReg32(RW_SNAND_MAC_INL , 0);

    mtk_snand_dev_mac_op(SPI);
}


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

    if (SNAND_NFI_CUST_READING == g_snand_dev_status)
    {
        if (u16IntStatus & (u16) INTR_AHB_DONE_EN)
        {
            complete(&g_comp_AHB_Done);
        }
    }
    else if (SNAND_NFI_AUTO_ERASING == g_snand_dev_status)
    {
        if (u16IntStatus & (u16) INTR_AUTO_BLKER_INTR_EN)
        {
            complete(&g_comp_AHB_Done);
        }
    }
    else if (SNAND_DEV_PROG_EXECUTING == g_snand_dev_status)
    {
        if (u16IntStatus & (u16) INTR_AHB_DONE_EN)
        {
            complete(&g_comp_AHB_Done);
        }
    }
    else
    {
        if (u16IntStatus)
        {
            complete(&g_comp_AHB_Done);
        }
    }

    return IRQ_HANDLED;
}
#endif

// Read Split related APIs
static bool mtk_snand_rs_if_require_split() // must be executed after snand_rs_reconfig_nfiecc()
{
    if (devinfo.advancedmode & SNAND_ADV_READ_SPLIT)
    {
        if (g_snand_rs_cur_part != 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}
/* Bayi: temp solution for add mtd to param */
static void mtk_snand_rs_reconfig_nfiecc(struct mtd_info *mtd, u32 row_addr)
{
    // 1. only decode part should be re-configured
    // 2. only re-configure essential part (fixed register will not be re-configured)

    u16 reg16;
    u32 ecc_bit;
    u32 ecc_bit_cfg;
    u32 u4DECODESize;
    ///struct mtd_info * mtd = &host->mtd;

    if (0 == (devinfo.advancedmode & SNAND_ADV_READ_SPLIT))
    {
        return;
    }

    if (row_addr < g_snand_rs_num_page)
    {
        if (g_snand_rs_cur_part != 0)
        {
            ecc_bit = SNAND_RS_ECC_BIT_FIRST_PART;

            reg16 = DRV_Reg(NFI_PAGEFMT_REG16);
            reg16 &= ~PAGEFMT_SPARE_MASK;
            reg16 |= SNAND_RS_SPARE_PER_SECTOR_FIRST_PART_NFI;
            DRV_WriteReg16(NFI_PAGEFMT_REG16, reg16);

            g_snand_spare_per_sector = SNAND_RS_SPARE_PER_SECTOR_FIRST_PART_VAL;

            g_snand_rs_cur_part = 0;
        }
        else
        {
            return;
        }
    }
    else
    {
        if (g_snand_rs_cur_part != 1)
        {
            ecc_bit = g_snand_rs_ecc_bit_second_part;

            reg16 = DRV_Reg(NFI_PAGEFMT_REG16);
            reg16 &= ~PAGEFMT_SPARE_MASK;
            reg16 |= g_snand_rs_spare_per_sector_second_part_nfi;
            DRV_WriteReg16(NFI_PAGEFMT_REG16, reg16);

            g_snand_spare_per_sector = mtd->oobsize / (mtd->writesize / NAND_SECTOR_SIZE);

            g_snand_rs_cur_part = 1;
        }
        else
        {
            return;
        }
    }

    switch(ecc_bit)
    {
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
    	default:
    		break;
    }
    DRV_WriteReg16(ECC_DECCON_REG16, DEC_DE);
    do
    {;
    }
    while (!DRV_Reg16(ECC_DECIDLE_REG16));

    u4DECODESize = ((NAND_SECTOR_SIZE + 1/*OOB_AVAI_PER_SECTOR*/) << 3) + ecc_bit * 13;

    /* configure ECC decoder && encoder */
    DRV_WriteReg32(ECC_DECCNFG_REG32, DEC_CNFG_CORRECT | ecc_bit_cfg | DEC_CNFG_NFI | DEC_CNFG_EMPTY_EN | (u4DECODESize << DEC_CNFG_CODE_SHIFT));
}


/******************************************************************************
 * mtk_snand_ecc_config
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
static void mtk_snand_ecc_config(struct mtk_nand_host_hw *hw,u32 ecc_bit)
{
    u32 u4ENCODESize;
    u32 u4DECODESize;
    u32 ecc_bit_cfg = ECC_CNFG_ECC4;

    switch(ecc_bit){
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
    DRV_WriteReg32(ECC_FDMADDR_REG32, NFI_FDM0L_REG32);

    /* Sector + FDM */
    u4ENCODESize = (hw->nand_sec_size + 1/* [bayi note]: use 1 for upstream format. 8*/) << 3;
    /* Sector + FDM + YAFFS2 meta data bits */
    u4DECODESize = ((hw->nand_sec_size + 1) << 3) + ecc_bit * 13;

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
 * mtk_snand_ecc_decode_start
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
static void mtk_snand_ecc_decode_start(void)
{
    /* wait for device returning idle */
    while (!(DRV_Reg16(ECC_DECIDLE_REG16) & DEC_IDLE)) ;
    DRV_WriteReg16(ECC_DECCON_REG16, DEC_EN);
}

/******************************************************************************
 * mtk_snand_ecc_decode_end
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
static void mtk_snand_ecc_decode_end(void)
{
    u32 timeout = 0xFFFF;

    /* wait for device returning idle */
    while (!(DRV_Reg16(ECC_DECIDLE_REG16) & DEC_IDLE))
    {
        timeout--;

        if (timeout == 0)   // Stanley Chu (need check timeout value again)
        {
            break;
        }
    }

    DRV_WriteReg16(ECC_DECCON_REG16, DEC_DE);
}

/******************************************************************************
 * mtk_snand_ecc_encode_start
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
static void mtk_snand_ecc_encode_start(void)
{
    /* wait for device returning idle */
    while (!(DRV_Reg32(ECC_ENCIDLE_REG32) & ENC_IDLE)) ;
    mb();
    DRV_WriteReg16(ECC_ENCCON_REG16, ENC_EN);
}

/******************************************************************************
 * mtk_snand_ecc_encode_end
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
static void mtk_snand_ecc_encode_end(void)
{
    /* wait for device returning idle */
    while (!(DRV_Reg32(ECC_ENCIDLE_REG32) & ENC_IDLE)) ;
    mb();
    DRV_WriteReg16(ECC_ENCCON_REG16, ENC_DE);
}
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
		printk("NFI", "flag byte: %x ",spare_buf[OOB_INDEX_OFFSET+i] );
		if(spare_buf[OOB_INDEX_OFFSET+i] !=0xFF){
			is_empty=false;
			break;
		}
	}
#endif
	//xlog_printk(ANDROID_LOG_INFO,"NFI", "This page is %s!\n",is_empty?"empty":"occupied");
	printk("NFI", "This page is %s!\n",is_empty?"empty":"occupied");
	return is_empty;
}
static bool return_fake_buf(u8 * data_buf, u32 page_size, u32 sec_num,u32 u4PageAddr){
	u32 i=0,j=0;
	u32 sec_zero_count=0;
	u8 t=0;
	u8 *p=data_buf;
	bool ret=true;
	for(j=0;j<sec_num;j++){
		p=data_buf+j*512;
		sec_zero_count=0;
		for(i=0;i<512;i++){
			t=p[i];
			t=~t;
			t=((t&0xaa)>>1) + (t&0x55);
			t=((t&0xcc)>>2)+(t&0x33);
			t=((t&0xf0f0)>>4)+(t&0x0f0f);
			sec_zero_count+=t;
			if(t>0){
				//xlog_printk(ANDROID_LOG_INFO,"NFI", "there is %d bit filp at sector(%d): %d in empty page \n ",t,j,i);
				printk("NFI", "there is %d bit filp at sector(%d): %d in empty page \n ",t,j,i);
			}
		}
		if(sec_zero_count > 2){
			//xlog_printk(ANDROID_LOG_ERROR,"NFI","too many bit filp=%d @ page addr=0x%x, we can not return fake buf\n",sec_zero_count,u4PageAddr);
			printk("NFI","too many bit filp=%d @ page addr=0x%x, we can not return fake buf\n",sec_zero_count,u4PageAddr);
			ret=false;
		}
	}
	return ret;
}

/******************************************************************************
 * mtk_snand_check_bch_error
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
static bool mtk_snand_check_bch_error(struct mtd_info *mtd, u8 * pDataBuf,u8 * spareBuf,u32 u4SecIndex, u32 u4PageAddr)
{
    bool ret = true;
    u16 u2SectorDoneMask = 1 << u4SecIndex;
    u32 u4ErrorNumDebug0,u4ErrorNumDebug1, i, u4ErrNum;
    u32 timeout = 0xFFFF;
    u32 correct_count = 0;
    u32 page_size=(u4SecIndex+1)*512;
    u32 sec_num=u4SecIndex+1;
	u16 failed_sec=0;

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
    u4ErrorNumDebug0 = DRV_Reg32(ECC_DECENUM0_REG32);
    u4ErrorNumDebug1 = DRV_Reg32(ECC_DECENUM1_REG32);
    if (0 != (u4ErrorNumDebug0 & 0xFFFFF) || 0 != (u4ErrorNumDebug1 & 0xFFFFF))
    {
        for (i = 0; i <= u4SecIndex; ++i)
        {
            if (i < 4)
            {
                u4ErrNum = DRV_Reg32(ECC_DECENUM0_REG32) >> (i * 5);
            } 
	    else
            {
                u4ErrNum = DRV_Reg32(ECC_DECENUM1_REG32) >> ((i - 4) * 5);
            }
            u4ErrNum &= 0x1F;
	    failed_sec =0;

            if (0x1F == u4ErrNum)
            {
		failed_sec++;
                ret = false;
                //xlog_printk(ANDROID_LOG_WARN,"NFI", "ECC-U, PA=%d, S=%d\n", u4PageAddr, i);
		printk("[%s] ECC-U, PA=%d, S=%d\n", __func__, u4PageAddr, i);
            } 
	    else
	    {
		if (u4ErrNum)
                {
		    correct_count += u4ErrNum;
                    //xlog_printk(ANDROID_LOG_INFO,"NFI"," ECC-C, #err:%d, PA=%d, S=%d\n", u4ErrNum, u4PageAddr, i);
		    printk("[%s] ECC-C, #err:%d, PA=%d, S=%d\n", __func__, u4ErrNum, u4PageAddr, i);
		}
            }
        }
	if(ret == false){
		if(is_empty_page(spareBuf,sec_num) && return_fake_buf(pDataBuf,page_size,sec_num,u4PageAddr)){
			ret=true;
			//xlog_printk(ANDROID_LOG_INFO,"NFI", "empty page have few filped bit(s) , fake buffer returned\n");
			printk("[%s] empty page have few filped bit(s) , __func__, fake buffer returned\n");
			memset(pDataBuf,0xff,page_size);
			memset(spareBuf,0xff,sec_num*8);
			failed_sec=0;
		}
	}
	mtd->ecc_stats.failed+=failed_sec;
        if (correct_count > 2 && ret)
        {
            mtd->ecc_stats.corrected++;
        } else
        {
            //xlog_printk(ANDROID_LOG_INFO,"NFI",  "Less than 2 bit error, ignore\n");
            printk("NFI",  "Less than 2 bit error, ignore\n");
        }
    }
#else
    /* We will manually correct the error bits in the last sector, not all the sectors of the page! */
    memset(au4ErrBitLoc, 0x0, sizeof(au4ErrBitLoc));
    u4ErrorNumDebug = DRV_Reg32(ECC_DECENUM_REG32);
    u4ErrNum = DRV_Reg32(ECC_DECENUM_REG32) >> (u4SecIndex << 2);
    u4ErrNum &= 0xF;

    if (u4ErrNum)
    {
        if (0xF == u4ErrNum)
        {
            mtd->ecc_stats.failed++;
            ret = false;
            //printk(KERN_ERR"UnCorrectable at PageAddr=%d\n", u4PageAddr);
        } else
        {
            for (i = 0; i < ((u4ErrNum + 1) >> 1); ++i)
            {
                au4ErrBitLoc[i] = DRV_Reg32(ECC_DECEL0_REG32 + i);
                u4ErrBitLoc1th = au4ErrBitLoc[i] & 0x1FFF;

                if (u4ErrBitLoc1th < 0x1000)
                {
                    u4ErrByteLoc = u4ErrBitLoc1th / 8;
                    u4BitOffset = u4ErrBitLoc1th % 8;
                    pDataBuf[u4ErrByteLoc] = pDataBuf[u4ErrByteLoc] ^ (1 << u4BitOffset);
                    mtd->ecc_stats.corrected++;
                } else
                {
                    mtd->ecc_stats.failed++;
                    //printk(KERN_ERR"UnCorrectable ErrLoc=%d\n", au4ErrBitLoc[i]);
                }
                u4ErrBitLoc2nd = (au4ErrBitLoc[i] >> 16) & 0x1FFF;
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
                        //printk(KERN_ERR"UnCorrectable High ErrLoc=%d\n", au4ErrBitLoc[i]);
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
 * mtk_snand_RFIFOValidSize
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
static bool mtk_snand_RFIFOValidSize(u16 u2Size)
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
 * mtk_snand_WFIFOValidSize
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
static bool mtk_snand_WFIFOValidSize(u16 u2Size)
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
 * mtk_snand_status_ready
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
static bool mtk_snand_status_ready(u32 u4Status)
{
    u32 timeout = 0xFFFF;

	u4Status &= ~STA_NAND_BUSY;

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
 * mtk_snand_reset_con
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
static bool mtk_snand_reset_con(void)
{
    u32 reg32;

    // HW recommended reset flow
    int timeout = 0xFFFF;

    // part 1. SNF

    reg32 = *RW_SNAND_MISC_CTL;
    reg32 |= SNAND_SW_RST;
    DRV_WriteReg32(RW_SNAND_MISC_CTL, reg32);
    reg32 &= ~SNAND_SW_RST;
    DRV_WriteReg32(RW_SNAND_MISC_CTL, reg32);

    // part 2. NFI

    for (;;)
    {
        if (0 == DRV_Reg16(NFI_MASTERSTA_REG16))
        {
            break;
        }
    }

    mb();

    DRV_WriteReg16(NFI_CON_REG16, CON_FIFO_FLUSH | CON_NFI_RST);

    for (;;)
    {
        if (0 == (DRV_Reg16(NFI_STA_REG32) & (STA_NFI_FSM_MASK | STA_NAND_FSM_MASK)))
        {
            break;
        }

        timeout--;

        if (!timeout)
        {
            printk("[%s] NFI_MASTERSTA_REG16 timeout!!\n", __FUNCTION__);
        }
    }

    /* issue reset operation */

    mb();

    return mtk_snand_status_ready(STA_NFI_FSM_MASK | STA_NAND_BUSY) && mtk_snand_RFIFOValidSize(0) && mtk_snand_WFIFOValidSize(0);
}

/******************************************************************************
 * mtk_snand_set_mode
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
static void mtk_snand_set_mode(u16 u2OpMode)
{
    u16 u2Mode = DRV_Reg16(NFI_CNFG_REG16);
    u2Mode &= ~CNFG_OP_MODE_MASK;
    u2Mode |= u2OpMode;
    DRV_WriteReg16(NFI_CNFG_REG16, u2Mode);
}

/******************************************************************************
 * mtk_snand_set_autoformat
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
static void mtk_snand_set_autoformat(bool bEnable)
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
 * mtk_snand_configure_fdm
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
static void mtk_snand_configure_fdm(u16 u2FDMSize, u16 u2FDMEccSize)
{
    NFI_CLN_REG16(NFI_PAGEFMT_REG16, PAGEFMT_FDM_MASK | PAGEFMT_FDM_ECC_MASK);
    NFI_SET_REG16(NFI_PAGEFMT_REG16, u2FDMSize << PAGEFMT_FDM_SHIFT);
    NFI_SET_REG16(NFI_PAGEFMT_REG16, u2FDMEccSize << PAGEFMT_FDM_ECC_SHIFT);
}


static bool mtk_snand_pio_ready(void)
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
 * mtk_snand_check_RW_count
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
static bool mtk_snand_check_RW_count(u16 u2WriteSize)
{
    u32 timeout = 0xFFFF;
    u16 u2SecNum = u2WriteSize >> 9;

    while (ADDRCNTR_CNTR(DRV_Reg16(NFI_ADDRCNTR_REG16)) < u2SecNum)
    {
        timeout--;
        if (0 == timeout)
        {
            printk(KERN_INFO "[%s] timeout\n", __FUNCTION__);
			printk(KERN_INFO "sectnum:0x%x, addrcnt:0x%x\n", u2SecNum, ADDRCNTR_CNTR(DRV_Reg16(NFI_ADDRCNTR_REG16)));
            return false;
        }
    }
    return true;
}

/******************************************************************************
 * mtk_snand_ready_for_read
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
static bool mtk_snand_ready_for_read(struct nand_chip *nand, u32 u4RowAddr, u32 u4ColAddr, u32 u4SecNum, bool full, u8 * buf)
{
    /* Reset NFI HW internal state machine and flush NFI in/out FIFO */
    bool bRet = false;
    u32 cmd, reg;
    u32 col_addr = u4ColAddr;
    SNAND_Mode mode = SPIQ;
#if __INTERNAL_USE_AHB_MODE__
    unsigned long phys = 0;
#endif
#if CFG_PERFLOG_DEBUG
    struct timeval stimer,etimer;
    do_gettimeofday(&stimer);
#endif

    if (!mtk_snand_reset_con())
    {
        goto cleanup;
    }

    // 1. Read page to cache

    cmd = mtk_snand_gen_c1a3(SNAND_CMD_PAGE_READ, u4RowAddr); // PAGE_READ command + 3-byte address

    DRV_WriteReg32(RW_SNAND_GPRAM_DATA, cmd);
    DRV_WriteReg32(RW_SNAND_MAC_OUTL, 1 + 3);
    DRV_WriteReg32(RW_SNAND_MAC_INL , 0);

    mtk_snand_dev_mac_op(SPI);

    // 2. Get features (status polling)

    cmd = SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_STATUS << 8);

    DRV_WriteReg32(RW_SNAND_GPRAM_DATA, cmd);
    DRV_WriteReg32(RW_SNAND_MAC_OUTL, 2);
    DRV_WriteReg32(RW_SNAND_MAC_INL , 1);

    for (;;)
    {
        mtk_snand_dev_mac_op(SPI);

        cmd = DRV_Reg8(((P_U8)RW_SNAND_GPRAM_DATA + 2));

        if ((cmd & SNAND_STATUS_OIP) == 0)
        {
            // use MTK ECC, not device ECC
            //if (SNAND_STATUS_TOO_MANY_ERROR_BITS == (cmd & SNAND_STATUS_ECC_STATUS_MASK) )
            //{
            //   bRet = FALSE;
            //}

            break;
        }
    }

    //------ SNF Part ------

    // set PAGE READ command & address
    reg = (SNAND_CMD_PAGE_READ << SNAND_PAGE_READ_CMD_OFFSET) | (u4RowAddr & SNAND_PAGE_READ_ADDRESS_MASK);
    DRV_WriteReg32(RW_SNAND_RD_CTL1, reg);

    // set DATA READ dummy cycle and command (use default value, ignored)
    if (mode == SPI)
    {
        reg = DRV_Reg32(RW_SNAND_RD_CTL2);
        reg &= ~SNAND_DATA_READ_CMD_MASK;
        reg |= SNAND_CMD_RANDOM_READ & SNAND_DATA_READ_CMD_MASK;
        DRV_WriteReg32(RW_SNAND_RD_CTL2, reg);

    }
    else if (mode == SPIQ)
    {
        mtk_snand_dev_enable_spiq(TRUE);

        reg = DRV_Reg32(RW_SNAND_RD_CTL2);
        reg &= ~SNAND_DATA_READ_CMD_MASK;
        reg |= SNAND_CMD_RANDOM_READ_SPIQ & SNAND_DATA_READ_CMD_MASK;
        DRV_WriteReg32(RW_SNAND_RD_CTL2, reg);
    }

    // set DATA READ address
    DRV_WriteReg32(RW_SNAND_RD_CTL3, (col_addr & SNAND_DATA_READ_ADDRESS_MASK));

    // set SNF timing
    reg = DRV_Reg32(RW_SNAND_MISC_CTL);

    reg |= SNAND_DATARD_CUSTOM_EN;

    if (mode == SPI)
    {
        reg &= ~SNAND_DATA_READ_MODE_MASK;
    }
    else if (mode == SPIQ)
    {
        reg &= ~SNAND_DATA_READ_MODE_MASK;
        reg |= ((SNAND_DATA_READ_MODE_X4 << SNAND_DATA_READ_MODE_OFFSET) & SNAND_DATA_READ_MODE_MASK);
    }

    DRV_WriteReg32(RW_SNAND_MISC_CTL, reg);

    // set SNF data length

    reg = u4SecNum * (NAND_SECTOR_SIZE + g_snand_spare_per_sector);

    DRV_WriteReg32(RW_SNAND_MISC_CTL2, (reg | (reg << SNAND_PROGRAM_LOAD_BYTE_LEN_OFFSET)));

    //------ NFI Part ------

    mtk_snand_set_mode(CNFG_OP_CUST);
    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
    DRV_WriteReg16(NFI_CON_REG16, u4SecNum << CON_NFI_SEC_SHIFT);

    if (g_bHwEcc)
    {
        /* Enable HW ECC */
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
    } else
    {
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
    }

    if (full)
    {
#if __INTERNAL_USE_AHB_MODE__
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);

#if defined (CONFIG_NAND_BOOTLOADER)
	phys = buf;
#else
        phys = nand_virt_to_phys_add((unsigned long) buf);
#endif

        if (!phys)
        {
            printk(KERN_ERR "[mtk_snand_ready_for_read]convert virt addr (%x) to phys add (%x)fail!!!", (u32) buf, phys);
            return false;
        }
        else
        {
            DRV_WriteReg32(NFI_STRADDR_REG32, phys);
        }
#else   // use MCU
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
#endif

        if (g_bHwEcc)
        {
            NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
        } else
        {
            NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
        }

    }
    else    // raw data
    {
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
    }

    mtk_snand_set_autoformat(full);

    if (full)
    {
        if (g_bHwEcc)
        {
            mtk_snand_ecc_decode_start();
        }
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
 * mtk_snand_ready_for_write
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
static bool mtk_snand_ready_for_write(struct nand_chip *nand, u32 u4RowAddr, u32 col_addr, bool full, u8 * buf)
{
    bool bRet = false;
    u32 sec_num = 1 << (nand->page_shift - 9);
    u32 reg;
    SNAND_Mode mode = SPIQ;
#if __INTERNAL_USE_AHB_MODE__
    unsigned long phys = 0;
    //u32 T_phys=0;
#endif

    /* Reset NFI HW internal state machine and flush NFI in/out FIFO */
    if (!mtk_snand_reset_con())
    {
        return false;
    }

    // 1. Write Enable
    mtk_snand_dev_command(SNAND_CMD_WRITE_ENABLE, 1);

    //------ SNF Part ------

    // set SPI-NAND command
    if (SPI == mode)
    {
        reg = SNAND_CMD_WRITE_ENABLE | (SNAND_CMD_PROGRAM_LOAD << SNAND_PG_LOAD_CMD_OFFSET) | (SNAND_CMD_PROGRAM_EXECUTE << SNAND_PG_EXE_CMD_OFFSET);
        DRV_WriteReg32(RW_SNAND_PG_CTL1, reg);
    }
    else if (SPIQ == mode)
    {
        reg = SNAND_CMD_WRITE_ENABLE | (SNAND_CMD_PROGRAM_LOAD_X4<< SNAND_PG_LOAD_CMD_OFFSET) | (SNAND_CMD_PROGRAM_EXECUTE << SNAND_PG_EXE_CMD_OFFSET);
        DRV_WriteReg32(RW_SNAND_PG_CTL1, reg);
        mtk_snand_dev_enable_spiq(TRUE);
    }

    // set program load address
    DRV_WriteReg32(RW_SNAND_PG_CTL2, col_addr & SNAND_PG_LOAD_ADDR_MASK);

    // set program execution address
    DRV_WriteReg32(RW_SNAND_PG_CTL3, u4RowAddr);

    // set SNF data length  (set in snand_xxx_write_data)

    // set SNF timing
    reg = DRV_Reg32(RW_SNAND_MISC_CTL);

    reg |= SNAND_PG_LOAD_CUSTOM_EN;    // use custom program

    if (SPI == mode)
    {
        reg &= ~SNAND_DATA_READ_MODE_MASK;
        reg |= ((SNAND_DATA_READ_MODE_X1 << SNAND_DATA_READ_MODE_OFFSET) & SNAND_DATA_READ_MODE_MASK);
        reg &=~ SNAND_PG_LOAD_X4_EN;
    }
    else if (SPIQ == mode)
    {
        reg &= ~SNAND_DATA_READ_MODE_MASK;
        reg |= ((SNAND_DATA_READ_MODE_X4 << SNAND_DATA_READ_MODE_OFFSET) & SNAND_DATA_READ_MODE_MASK);
        reg |= SNAND_PG_LOAD_X4_EN;
    }

    DRV_WriteReg32(RW_SNAND_MISC_CTL, reg);

    // set SNF data length

    reg = sec_num * (NAND_SECTOR_SIZE + g_snand_spare_per_sector);
	//printk(KERN_INFO "BAYI-TEST 5, SNF data length:0x%x, g_bHwEcc:0x%x \n", reg, g_bHwEcc);
    DRV_WriteReg32(RW_SNAND_MISC_CTL2, reg | (reg << SNAND_PROGRAM_LOAD_BYTE_LEN_OFFSET));

    //------ NFI Part ------

    mtk_snand_set_mode(CNFG_OP_PRGM);

    NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_READ_EN);

    DRV_WriteReg16(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

    if (full)
    {
#if __INTERNAL_USE_AHB_MODE__
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);

#if defined (CONFIG_NAND_BOOTLOADER)
	phys = buf;
#else
        phys = nand_virt_to_phys_add((unsigned long) buf);
#endif

        if (!phys)
        {
            printk(KERN_ERR "[mt6575_nand_ready_for_write]convert virt addr (%x) to phys add fail!!!", (u32) buf);
            return false;
        }
        else
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

    mtk_snand_set_autoformat(full);

    if (full)
    {
        if (g_bHwEcc)
        {
            mtk_snand_ecc_encode_start();
        }
    }

    bRet = true;

    return bRet;
}

static bool mtk_snand_check_dececc_done(u32 u4SecNum)
{
    u32 timeout, dec_mask;
    timeout = 0xffff;
    dec_mask = (1 << u4SecNum) - 1;
    while ((dec_mask != DRV_Reg(ECC_DECDONE_REG16)) && timeout > 0)
        timeout--;
    if (timeout == 0)
    {
        MSG(VERIFY, "ECC_DECDONE: timeout\n");
        return false;
    }
    return true;
}

/******************************************************************************
 * mtk_snand_read_page_data
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
static bool mtk_snand_dma_read_data(struct mtd_info *mtd, u8 * buf, u32 num_sec, u8 full)
{
    int interrupt_en = g_i4Interrupt;
    int timeout = 0xffff;
#if !defined (CONFIG_NAND_BOOTLOADER)    
    struct scatterlist sg;
    enum dma_data_direction dir = DMA_FROM_DEVICE;
#endif

#if CFG_PERFLOG_DEBUG
    struct timeval stimer,etimer;
    do_gettimeofday(&stimer);
#endif

#if !defined (CONFIG_NAND_BOOTLOADER)
    sg_init_one(&sg, buf, num_sec * (NAND_SECTOR_SIZE + g_snand_spare_per_sector));
    dma_map_sg(mtd->dev.parent, &sg, 1, dir);
#endif

    NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);

    if ((unsigned int)buf % 16) // TODO: can not use AHB mode here
    {
        printk(KERN_INFO "Un-16-aligned address\n");
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
    } else
    {
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
    }

    // set dummy command to trigger NFI enter custom mode
    DRV_WriteReg16(NFI_CMD_REG16, NAND_CMD_DUMMYREAD);

    mb();

    DRV_Reg16(NFI_INTR_REG16);  // read clear

    DRV_WriteReg16(NFI_INTR_EN_REG16, INTR_AHB_DONE_EN);

    if (interrupt_en)
    {
        init_completion(&g_comp_AHB_Done);
    }

    mb();

    NFI_SET_REG16(NFI_CON_REG16, CON_NFI_BRD);

    mb();

    g_snand_dev_status = SNAND_NFI_CUST_READING;
    g_snand_cmd_status = 0; // busy
    g_running_dma = 1;

    if (interrupt_en)
    {
        // Wait 10ms for AHB done
        if (!wait_for_completion_timeout(&g_comp_AHB_Done, 20))
        {
            printk(KERN_ERR "wait for completion timeout happened @ [%s]: %d\n", __FUNCTION__, __LINE__);
            dump_nfi();
            g_snand_dev_status = SNAND_IDLE;
            g_running_dma = 0;

            return false;
        }

        g_snand_dev_status = SNAND_IDLE;
        g_running_dma = 0;

        while (num_sec > ((DRV_Reg16(NFI_BYTELEN_REG16) & 0xf000) >> 12))
        {
            timeout--;

            if (0 == timeout)
            {
                printk(KERN_ERR "[%s] poll BYTELEN error\n", __FUNCTION__);
                g_snand_dev_status = SNAND_IDLE;
                g_running_dma = 0;

                return false;   //4  // AHB Mode Time Out!
            }
        }
    }
    else
    {
        while (!DRV_Reg16(NFI_INTR_REG16))
        {
            timeout--;

            if (0 == timeout)   // time out
            {
                printk(KERN_ERR "[%s] poll nfi_intr error\n", __FUNCTION__);
                dump_nfi();
                g_snand_dev_status = SNAND_IDLE;
                g_running_dma = 0;
                return false;   //4  // AHB Mode Time Out!
            }
        }

        while (num_sec > ((DRV_Reg16(NFI_BYTELEN_REG16) & 0xf000) >> 12))
        {
            timeout--;

            if (0 == timeout)
            {
                printk(KERN_ERR "[%s] poll BYTELEN error(num_sec=%d)\n", __FUNCTION__, num_sec);
                dump_nfi();
                g_snand_dev_status = SNAND_IDLE;
                g_running_dma = 0;
                return false;   //4  // AHB Mode Time Out!
            }
        }

        g_snand_dev_status = SNAND_IDLE;
        g_running_dma = 0;
    }

#if !defined (CONFIG_NAND_BOOTLOADER)
    dma_unmap_sg(mtd->dev.parent, &sg, 1, dir);
#endif

#if CFG_PERFLOG_DEBUG
    do_gettimeofday(&etimer);
    g_NandPerfLog.ReadDMATotalTime+= Cal_timediff(&etimer,&stimer);
    g_NandPerfLog.ReadDMACount++;
#endif

    return true;
}

static bool mtk_snand_mcu_read_data(u8 * buf, u32 u4SecNum, u8 full)
{
    int timeout = 0xffff;
    u32 i;
    u32 *buf32 = (u32 *) buf;
    u32 snf_len;
    u32 length;
#ifdef TESTTIME
    unsigned long long time1, time2;
    time1 = sched_clock();
#endif

    length = u4SecNum * NAND_SECTOR_SIZE;

    // set SNF data length
    if (full)
	{
		snf_len = length + devinfo.sparesize;
	}
	else
	{
		snf_len = length;
	}

    DRV_WriteReg32(RW_SNAND_MISC_CTL2, (snf_len | (snf_len << SNAND_PROGRAM_LOAD_BYTE_LEN_OFFSET)));

    // set dummy command to trigger NFI enter custom mode
    DRV_WriteReg16(NFI_CMD_REG16, NAND_CMD_DUMMYREAD);

    if ((u32) buf % 4 || length % 4)
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
    else
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);

    mb();

    NFI_SET_REG16(NFI_CON_REG16, CON_NFI_BRD);

    if ((u32) buf % 4 || length % 4)
    {
        for (i = 0; (i < (length)) && (timeout > 0);)
        {
            if (DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1)
            {
                *buf++ = (u8) DRV_Reg32(NFI_DATAR_REG32);
                i++;
            }
            else
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
    else
    {
        for (i = 0; (i < (length >> 2)) && (timeout > 0);)
        {
            if (DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1)
            {
                *buf32++ = DRV_Reg32(NFI_DATAR_REG32);
                i++;
            }
            else
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

static bool mtk_snand_read_page_data(struct mtd_info *mtd, u8 * pDataBuf, u32 u4SecNum, u8 full)
{
#if (__INTERNAL_USE_AHB_MODE__)
    return mtk_snand_dma_read_data(mtd, pDataBuf, u4SecNum, full);
#else
    return mtk_snand_mcu_read_data(mtd, pDataBuf, u4SecNum, full);
#endif
}

/******************************************************************************
 * mtk_snand_write_page_data
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
static bool mtk_snand_dma_write_data(struct mtd_info *mtd, u8 * pDataBuf, u32 u4Size, u8 full)
{
    int i4Interrupt = 0;        //g_i4Interrupt;
    u32 snf_len;
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
    dma_map_sg(mtd->dev.parent, &sg, 1, dir);
#endif

    // set SNF data length
    if (full)
	{
		snf_len = u4Size + mtd->oobsize;//64;//devinfo.sparesize;
	}
	else
	{
		snf_len = u4Size;
	}

    DRV_WriteReg32(RW_SNAND_MISC_CTL2, ((snf_len) | (snf_len << SNAND_PROGRAM_LOAD_BYTE_LEN_OFFSET)));

    NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);

    // set dummy command to trigger NFI enter custom mode
    DRV_WriteReg16(NFI_CMD_REG16, NAND_CMD_DUMMYPROG);

    DRV_Reg16(NFI_INTR_REG16);
    DRV_WriteReg16(NFI_INTR_EN_REG16, INTR_CUSTOM_PROG_DONE_INTR_EN);

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
        init_completion(&g_comp_AHB_Done);
        DRV_Reg16(NFI_INTR_REG16);  // read clear
        DRV_WriteReg16(NFI_INTR_EN_REG16, INTR_CUSTOM_PROG_DONE_INTR_EN);
    }

    mb();

    NFI_SET_REG16(NFI_CON_REG16, CON_NFI_BWR);

    g_running_dma = 3;

    if (i4Interrupt)
    {
        // Wait 10ms for AHB done
        if (!wait_for_completion_timeout(&g_comp_AHB_Done, 20))
        {
            MSG(READ, "wait for completion timeout happened @ [%s]: %d\n", __FUNCTION__, __LINE__);
            dump_nfi();
            g_running_dma = 0;
            return false;
        }

        g_running_dma = 0;
    }
    else
    {
        while (!(DRV_Reg32(RW_SNAND_STA_CTL1) & SNAND_CUSTOM_PROGRAM))
        {
            #if 0   // FIXME: Stanley Chu, use a suitable timeout value
            timeout--;

            if (0 == timeout)
            {
                printk(KERN_ERR "[%s] poll BYTELEN error\n", __FUNCTION__);
                g_running_dma = 0;
                return false;   //4  // AHB Mode Time Out!
            }
            #endif
        }

        g_running_dma = 0;
    }

#if !defined (CONFIG_NAND_BOOTLOADER)
    dma_unmap_sg(mtd->dev.parent, &sg, 1, dir);
#endif

#if CFG_PERFLOG_DEBUG
    do_gettimeofday(&etimer);
    g_NandPerfLog.WriteDMATotalTime+= Cal_timediff(&etimer,&stimer);
    g_NandPerfLog.WriteDMACount++;
#endif

    return true;
}

static bool mtk_snand_mcu_write_data(struct mtd_info *mtd, const u8 * buf, u32 length, u8 full)
{
    u32 timeout = 0xFFFF;
    u32 i;
    u32 *pBuf32;
    u32 snf_len;

	// set SNF data length
    if (full)
	{
		snf_len += length + devinfo.sparesize;
	}
	else
	{
		snf_len = length;
	}

    DRV_WriteReg32(RW_SNAND_MISC_CTL2, ((snf_len) | (snf_len << SNAND_PROGRAM_LOAD_BYTE_LEN_OFFSET)));

    // set dummy command to trigger NFI enter custom mode
    DRV_WriteReg16(NFI_CMD_REG16, NAND_CMD_DUMMYPROG);

    NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);

    mb();

    NFI_SET_REG16(NFI_CON_REG16, CON_NFI_BWR);

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

static bool mtk_snand_write_page_data(struct mtd_info *mtd, u8 * buf, u32 size, u8 full)
{
#if (__INTERNAL_USE_AHB_MODE__)
    return mtk_snand_dma_write_data(mtd, buf, size, full);
#else
    return mtk_snand_mcu_write_data(mtd, buf, size, full);
#endif
}

/******************************************************************************
 * mtk_snand_read_fdm_data
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
static void mtk_snand_read_fdm_data(u8 * pDataBuf, u32 u4SecNum)
{
    u32 i;
    u32 *pBuf32 = (u32 *) pDataBuf;

    if (pBuf32)
    {
        for (i = 0; i < u4SecNum; ++i)
        {
            *pBuf32++ = DRV_Reg32(NFI_FDM0L_REG32 + (i << 1));
            *pBuf32++ = DRV_Reg32(NFI_FDM0M_REG32 + (i << 1));
        }
    }
}

/******************************************************************************
 * mtk_snand_write_fdm_data
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
__attribute__((aligned(16))) static u8 fdm_buf[64];
static void mtk_snand_write_fdm_data(struct nand_chip *chip, u8 * pDataBuf, u32 u4SecNum)
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
            {
                empty = false;
            }

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
    }
}

/******************************************************************************
 * mtk_snand_stop_read
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
static void mtk_snand_stop_read()
{
    //------ NFI Part
    NFI_CLN_REG16(NFI_CON_REG16, CON_NFI_BRD);

    //------ SNF Part

    // set 1 then set 0 to clear done flag
    DRV_WriteReg32(RW_SNAND_STA_CTL1, SNAND_CUSTOM_PROGRAM);
    DRV_WriteReg32(RW_SNAND_STA_CTL1, 0);

    // clear essential SNF setting
    NFI_CLN_REG32(RW_SNAND_MISC_CTL, SNAND_PG_LOAD_CUSTOM_EN);

    if (g_bHwEcc)
    {
        mtk_snand_ecc_decode_end();
    }

    DRV_WriteReg16(NFI_INTR_EN_REG16, 0);

    mtk_snand_reset_con();
}

/******************************************************************************
 * mtk_snand_stop_write
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
static void mtk_snand_stop_write(void)
{
    //------ NFI Part

    NFI_CLN_REG16(NFI_CON_REG16, CON_NFI_BWR);

    //------ SNF Part

    // set 1 then set 0 to clear done flag
    DRV_WriteReg32(RW_SNAND_STA_CTL1, SNAND_CUSTOM_PROGRAM);
    DRV_WriteReg32(RW_SNAND_STA_CTL1, 0);

    // clear essential SNF setting
    NFI_CLN_REG32(RW_SNAND_MISC_CTL, SNAND_PG_LOAD_CUSTOM_EN);

    mtk_snand_dev_enable_spiq(FALSE);

    if (g_bHwEcc)
    {
        mtk_snand_ecc_encode_end();
    }

    DRV_WriteReg16(NFI_INTR_EN_REG16, 0);
}

static bool mtk_snand_read_page_part2(struct mtd_info *mtd, u32 row_addr, u32 num_sec, u8 * buf)
{
    bool    bRet = true;
    u32     reg;
    SNAND_Mode mode = SPIQ;
    u32     col_part2, i, len;
    u32     spare_per_sector;
    P_U8    buf_part2;
    u32     timeout = 0xFFFF;
    u32     old_dec_mode = 0;
    unsigned long     buf_phy;
#if !defined (CONFIG_NAND_BOOTLOADER)
    struct scatterlist sg;
    enum dma_data_direction dir = DMA_FROM_DEVICE;

    sg_init_one(&sg, buf, NAND_SECTOR_SIZE + OOB_PER_SECTOR);
    dma_map_sg(mtd->dev.parent, &sg, 1, dir);
#endif

    spare_per_sector = mtd->oobsize / (mtd->writesize / NAND_SECTOR_SIZE);

    for (i = 0; i < 2 ; i++)
    {
        mtk_snand_reset_con();

        if (0 == i)
        {
            col_part2 = (NAND_SECTOR_SIZE + spare_per_sector) * (num_sec - 1);

            buf_part2 = buf;

            len = 2112 - col_part2;
        }
        else
        {
            col_part2 = 2112;

            buf_part2 += len;   // append to first round

            len = (num_sec * (NAND_SECTOR_SIZE + spare_per_sector)) - 2112;
        }

        //------ SNF Part ------

        // set DATA READ address
        DRV_WriteReg32(RW_SNAND_RD_CTL3, (col_part2 & SNAND_DATA_READ_ADDRESS_MASK));

        if (0 == i)
        {
            // set RW_SNAND_MISC_CTL
            reg = DRV_Reg32(RW_SNAND_MISC_CTL);

            reg |= SNAND_DATARD_CUSTOM_EN;

            reg &= ~SNAND_DATA_READ_MODE_MASK;
            reg |= ((SNAND_DATA_READ_MODE_X4 << SNAND_DATA_READ_MODE_OFFSET) & SNAND_DATA_READ_MODE_MASK);

            DRV_WriteReg32(RW_SNAND_MISC_CTL, reg);
        }

        // set SNF data length
        reg = len | (len << SNAND_PROGRAM_LOAD_BYTE_LEN_OFFSET);

        DRV_WriteReg32(RW_SNAND_MISC_CTL2, reg);

        //------ NFI Part ------

        if (0 == i)
        {
            mtk_snand_set_mode(CNFG_OP_CUST);
            NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
            NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
            NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
            mtk_snand_set_autoformat(FALSE);
        }

        //DRV_WriteReg16(NFI_CON_REG16, 1 << CON_NFI_SEC_SHIFT);

#if defined (CONFIG_NAND_BOOTLOADER)
	buf_phy = buf_part2;
#else        
        buf_phy = nand_virt_to_phys_add((unsigned long)buf_part2);
#endif

        DRV_WriteReg32(NFI_STRADDR_REG32, buf_phy);

        DRV_WriteReg32(NFI_SPIDMA_REG32, SPIDMA_SEC_EN | (len & SPIDMA_SEC_SIZE_MASK));


        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);

        // set dummy command to trigger NFI enter custom mode
        DRV_WriteReg16(NFI_CMD_REG16, NAND_CMD_DUMMYREAD);

        DRV_Reg16(NFI_INTR_REG16);  // read clear
        DRV_WriteReg16(NFI_INTR_EN_REG16, INTR_AHB_DONE_EN);

        NFI_SET_REG16(NFI_CON_REG16, (1 << CON_NFI_SEC_SHIFT) | CON_NFI_BRD);     // fixed to sector number 1

        timeout = 0xFFFF;

        while (!(DRV_Reg16(NFI_INTR_REG16) & INTR_AHB_DONE))    // for custom read, wait NFI's INTR_AHB_DONE done to ensure all data are transferred to buffer
        {
            timeout--;

            if (0 == timeout)
            {
                bRet = FALSE;
                goto unmap_and_cleanup;
            }
        }

        timeout = 0xFFFF;

        while (((DRV_Reg16(NFI_BYTELEN_REG16) & 0xf000) >> 12) != 1)
        {
            timeout--;

            if (0 == timeout)
            {
                bRet = FALSE;
                goto unmap_and_cleanup;
            }
        }

        //------ NFI Part

        NFI_CLN_REG16(NFI_CON_REG16, CON_NFI_BRD);

        //------ SNF Part

        // set 1 then set 0 to clear done flag
        DRV_WriteReg32(RW_SNAND_STA_CTL1, SNAND_CUSTOM_READ);
        DRV_WriteReg32(RW_SNAND_STA_CTL1, 0);

        // clear essential SNF setting
        if (1 == i)
        {
            NFI_CLN_REG32(RW_SNAND_MISC_CTL, SNAND_DATARD_CUSTOM_EN);
        }
    }

#if !defined (CONFIG_NAND_BOOTLOADER)
    dma_unmap_sg(mtd->dev.parent, &sg, 1, dir);
#endif

    if (g_bHwEcc)
    {
#if !defined (CONFIG_NAND_BOOTLOADER)
        dir = DMA_BIDIRECTIONAL;
        sg_init_one(&sg, buf, NAND_SECTOR_SIZE + OOB_PER_SECTOR);
        dma_map_sg(mtd->dev.parent, &sg, 1, dir);
#endif

        /* configure ECC decoder && encoder */
        reg = DRV_Reg32(ECC_DECCNFG_REG32);
        old_dec_mode = reg & DEC_CNFG_DEC_MODE_MASK;
        reg &= ~DEC_CNFG_DEC_MODE_MASK;
        reg |= DEC_CNFG_AHB;
        reg |= DEC_CNFG_DEC_BURST_EN;
        DRV_WriteReg32(ECC_DECCNFG_REG32, reg);

#if defined (CONFIG_NAND_BOOTLOADER)
	buf_phy = buf;
#else
        buf_phy = nand_virt_to_phys_add((unsigned long)buf);
#endif

        DRV_WriteReg32(ECC_DECDIADDR_REG32, (u32)buf_phy);

        DRV_WriteReg16(ECC_DECCON_REG16, DEC_DE);
        DRV_WriteReg16(ECC_DECCON_REG16, DEC_EN);

        while(!((DRV_Reg32(ECC_DECDONE_REG16)) & (1 << 0)));

#if !defined (CONFIG_NAND_BOOTLOADER)
        dma_unmap_sg(mtd->dev.parent, &sg, 1, dir);
#endif

        reg = DRV_Reg32(ECC_DECENUM0_REG32);

        if (0 != reg)
        {
            reg &= 0x1F;

            if (0x1F == reg)
            {
                bRet = false;
                //xlog_printk(ANDROID_LOG_WARN,"NFI", "ECC-U(2), PA=%d, S=%d\n", row_addr, num_sec - 1);
                printk("NFI", "ECC-U(2), PA=%d, S=%d\n", row_addr, num_sec - 1);
            }
            else
            {
				if (reg)
                {
                    //xlog_printk(ANDROID_LOG_INFO,"NFI"," ECC-C(2), #err:%d, PA=%d, S=%d\n", reg, row_addr, num_sec - 1);
		    printk("NFI"," ECC-C(2), #err:%d, PA=%d, S=%d\n", reg, row_addr, num_sec - 1);
				}
            }
        }

        // restore essential NFI and ECC registers
        DRV_WriteReg32(NFI_SPIDMA_REG32, 0);
        reg = DRV_Reg32(ECC_DECCNFG_REG32);
        reg &= ~DEC_CNFG_DEC_MODE_MASK;
        reg |= old_dec_mode;
        reg &= ~DEC_CNFG_DEC_BURST_EN;
        DRV_WriteReg32(ECC_DECCNFG_REG32, reg);
        DRV_WriteReg16(ECC_DECCON_REG16, DEC_DE);
        DRV_WriteReg32(ECC_DECDIADDR_REG32, 0);
    }

cleanup:

    return bRet;

unmap_and_cleanup:

#if !defined (CONFIG_NAND_BOOTLOADER)
    dma_unmap_sg(mtd->dev.parent, &sg, 1, dir);
#endif
    return bRet;
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
    u8 * buf;
    u8 * p_buf_local;
    bool bRet = true;
    u8 retry;
    struct nand_chip *nand = mtd->priv;
    u32 u4SecNum = u4PageSize >> 9;
    u32 i;
#ifdef NAND_PFM
    struct timeval pfm_time_read;
#endif
#if 0
    unsigned short PageFmt_Reg = 0;
    unsigned int NAND_ECC_Enc_Reg = 0;
    unsigned int NAND_ECC_Dec_Reg = 0;
#endif
    PFM_BEGIN(pfm_time_read);
memset(local_buffer_16_align, 0x5a, u4PageSize);
    if (((unsigned long) pPageBuf % 16) && local_buffer_16_align)
    {
    	//printk(KERN_INFO "<READ> Data buffer not 16 bytes aligned %d:\n", u4PageSize);
        buf = local_buffer_16_align;
    }
    else
    {
        buf = pPageBuf;
    }

    mtk_snand_rs_reconfig_nfiecc(mtd, u4RowAddr);

    if (mtk_snand_rs_if_require_split())
    {
        u4SecNum--;
    }

    retry = 0;

mtk_nand_exec_read_page_retry:

    bRet = true;

    if (mtk_snand_ready_for_read(nand, u4RowAddr, 0, u4SecNum, true, buf))
    {
        if (!mtk_snand_read_page_data(mtd, buf, u4SecNum, true))
        {
            printk(KERN_ERR "[%s]mtk_snand_read_page_data() FAIL!!!\n", __func__);
            bRet = false;
        }

        if (!mtk_snand_status_ready(STA_NAND_BUSY))
        {
            bRet = false;
        }

        if (g_bHwEcc)
        {
            if (!mtk_snand_check_dececc_done(u4SecNum))
            {
                printk(KERN_ERR "[%s]mtk_snand_check_dececc_done() FAIL!!!\n", __func__);
                bRet = false;
            }
        }

        mtk_snand_read_fdm_data(pFDMBuf, u4SecNum);

        if (g_bHwEcc)
        {
            if (!mtk_snand_check_bch_error(mtd, buf, pFDMBuf,u4SecNum - 1, u4RowAddr))
            {
                //MSG(READ, "[%s]mtk_snand_check_bch_error() FAIL!!!\n", __func__);
                printk(KERN_ERR "[%s]mtk_snand_check_bch_error() FAIL!!!\n", __func__);
                bRet = false;
            }
        }
        mtk_snand_stop_read();
    }

    if (false == bRet)
    {
        if (retry <= 3)
        {
            retry++;
            printk(KERN_ERR "[%s] read retrying ... (the %d time)\n", __FUNCTION__, retry);
            mtk_snand_reset_dev();

            goto mtk_nand_exec_read_page_retry;
        }
    }

    if (mtk_snand_rs_if_require_split())
    {
        // read part II

        //u4SecNum++;
        u4SecNum = u4PageSize >> 9;

        // note: use local temp buffer to read part 2
        if (!mtk_snand_read_page_part2(mtd, u4RowAddr, u4SecNum, g_snand_temp))
        {
            bRet = false;
        }

        mb();

        // copy data

        p_buf_local = buf + NAND_SECTOR_SIZE * (u4SecNum - 1);

        memcpy(p_buf_local, g_snand_temp, NAND_SECTOR_SIZE);

        mb();

        // copy FDM data

        p_buf_local = pFDMBuf + OOB_AVAI_PER_SECTOR * (u4SecNum - 1);

        memcpy(p_buf_local, (g_snand_temp + NAND_SECTOR_SIZE), OOB_AVAI_PER_SECTOR);

    }

    mtk_snand_dev_enable_spiq(FALSE);

    if (buf == local_buffer_16_align)
    {
        memcpy(pPageBuf, buf, u4PageSize);
    }

    PFM_END_R(pfm_time_read, u4PageSize + 32);
	printk(KERN_ERR "BAYI-TEST, pPageBuf:0x%x, 0x%x, 0x%x, 0x%x, 0x%x u4RowAddr:0x%x\n", pPageBuf[0],pPageBuf[1], pPageBuf[2], pPageBuf[3], pPageBuf[4], u4RowAddr);

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

static bool mtk_snand_dev_program_execute(u32 page)
{
    u32 cmd;
    bool bRet = TRUE;

    // 3. Program Execute

	#if 1 /* added by Bayi for write success */
    g_snand_dev_status = SNAND_DEV_PROG_EXECUTING;
    g_snand_cmd_status = 0; // busy
	#else
	g_snand_dev_status = SNAND_IDLE;//SNAND_NFI_AUTO_ERASING;
    g_snand_cmd_status = NAND_STATUS_READY;//0; // busy
	#endif
    cmd = mtk_snand_gen_c1a3(SNAND_CMD_PROGRAM_EXECUTE, page);

    mtk_snand_dev_command(cmd, 4);

    return bRet;
}

int mtk_nand_exec_write_page(struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize, u8 * pPageBuf, u8 * pFDMBuf)
{
    struct nand_chip *chip = mtd->priv;
    u32 u4SecNum = u4PageSize >> 9;
    u8 *buf;
    u8 status;

    MSG(INIT, "by_write, page: 0x%x, buf: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, pagesize:0x%x\n", u4RowAddr, pPageBuf[0], pPageBuf[1], pPageBuf[2], pPageBuf[3], pPageBuf[4], u4PageSize);

#ifdef _MTK_NAND_DUMMY_DRIVER_
    if (dummy_driver_debug)
    {
#ifdef TESTTIME
        unsigned long long time = sched_clock();
        if (!((time * 123 + 59) % 32768))
        {
            printk(KERN_INFO "[NAND_DUMMY_DRIVER] Simulate write error at page: 0x%x\n", u4RowAddr);
            return -EIO;
        }
#endif
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

    mtk_snand_rs_reconfig_nfiecc(mtd, u4RowAddr);

    if (mtk_snand_ready_for_write(chip, u4RowAddr, 0, true, buf))
    {
        mtk_snand_write_fdm_data(chip, pFDMBuf, u4SecNum);

        (void)mtk_snand_write_page_data(mtd, buf, u4PageSize, true);

        (void)mtk_snand_check_RW_count(u4PageSize);

        mtk_snand_stop_write();

        {
#if CFG_PERFLOG_DEBUG
            struct timeval stimer,etimer;
            do_gettimeofday(&stimer);
#endif

            mtk_snand_dev_program_execute(u4RowAddr);

//            while (DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY) ;

#if CFG_PERFLOG_DEBUG
            do_gettimeofday(&etimer);
            g_NandPerfLog.WriteBusyTotalTime+= Cal_timediff(&etimer,&stimer);
            g_NandPerfLog.WriteBusyCount++;
#endif
        }
    }

    PFM_END_W(pfm_time_write, u4PageSize + 32);

    status = chip->waitfunc(mtd, chip); // return NAND_STATUS_READY or return NAND_STATUS_FAIL

    if (status & NAND_STATUS_FAIL)
        return -EIO;
    else
        return 0;
}

/******************************************************************************
 *
 * Write a page to a logical address
 *
 * Return values
 *  0: No error
 *  1: Error
 *
 *****************************************************************************/
#if defined (CONFIG_NAND_BOOTLOADER)
static int mtk_snand_write_page(struct mtd_info *mtd, struct nand_chip *chip, const u8 * buf, int oob_required, int page, int cached, int raw)
#else
static int mtk_snand_write_page(struct mtd_info *mtd, struct nand_chip *chip, const u8 * buf, int page, int cached, int raw)
#endif
{
    int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
    int block = page / page_per_block;
    u16 page_in_block = page % page_per_block;
    int mapped_block = get_mapping_block_index(block);
#if CFG_PERFLOG_DEBUG
    struct timeval stimer,etimer;
    do_gettimeofday(&stimer);
#endif

    if (TRUE == mtk_snand_is_vendor_reserved_blocks(page_in_block + mapped_block * page_per_block))  // return write error for reserved blocks
    {
        return 1;
    }

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
        if (update_bmt((page_in_block + mapped_block * page_per_block) << chip->page_shift, UPDATE_WRITE_FAIL, (u8 *) buf, chip->oob_poi))
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

static void mtk_snand_dev_read_id(u8 id[])
{
    u8 cmd = SNAND_CMD_READ_ID;

    mtk_snand_dev_command_ext(SPI, &cmd, id, 1, SNAND_MAX_ID + 1);
}

/******************************************************************************
 * mtk_snand_command_bp
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
static void mtk_snand_command_bp(struct mtd_info *mtd, unsigned int command, int column, int page_addr)
{
    struct nand_chip *nand = mtd->priv;
#ifdef NAND_PFM
    struct timeval pfm_time_erase;
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

      	  #if 0
          PFM_BEGIN(pfm_time_erase);
          (void)mtk_snand_reset_con();

          mtk_snand_set_mode(CNFG_OP_ERASE);
          (void)mtk_nand_set_command(NAND_CMD_ERASE1);
          (void)mtk_nand_set_address(0, page_addr, 0, devinfo.addr_cycle - 2);
          #else
          printk(KERN_ERR "[SNAND-ERROR] Not allowed NAND_CMD_ERASE1!\n");
          #endif

          break;

      case NAND_CMD_ERASE2:

      	  #if 0
          (void)mtk_nand_set_command(NAND_CMD_ERASE2);
          while (DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY) ;
          PFM_END_E(pfm_time_erase);
          #else
          printk(KERN_ERR "[SNAND-ERROR] Not allowed NAND_CMD_ERASE2!\n"	);
          #endif

          break;

      case NAND_CMD_STATUS:

			if ((SNAND_DEV_ERASE_DONE == g_snand_dev_status) ||
				(SNAND_DEV_PROG_DONE == g_snand_dev_status))
			{
				g_snand_dev_status = SNAND_IDLE;	// reset command status
			}

            g_snand_read_byte_mode = SNAND_RB_CMD_STATUS;

          break;

      case NAND_CMD_RESET:

            (void)mtk_snand_reset_con();

            g_snand_read_byte_mode = SNAND_RB_DEFAULT;

            break;

      case NAND_CMD_READID:

            mtk_snand_reset_dev();
            mtk_snand_reset_con();

            mb();

            mtk_snand_dev_read_id(g_snand_id_data);

            g_snand_id_data_idx = 1;

            g_snand_read_byte_mode = SNAND_RB_READ_ID;

            break;

      default:
          BUG();
          break;
    }
}

/******************************************************************************
 * mtk_snand_select_chip
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
static void mtk_snand_select_chip(struct mtd_info *mtd, int chip)
{
    if (chip == -1 && false == g_bInitDone)
    {
        struct nand_chip *nand = mtd->priv;

#if defined (CONFIG_NAND_BOOTLOADER)
	struct mtk_nand_host *host_ptr = host;
#else
    	struct mtk_nand_host *host_ptr = nand->priv;
#endif
    	struct mtk_nand_host_hw *hw = host_ptr->hw;
    	u32 spare_per_sector = mtd->oobsize/( mtd->writesize/512);
    	u32 ecc_bit = 4;
    	u32 spare_bit = PAGEFMT_SPARE_16;

    	if(spare_per_sector>=28){
    		spare_bit = PAGEFMT_SPARE_28;
    		ecc_bit = 12;
    		spare_per_sector = 28;
      	}else if(spare_per_sector>=27){
      		spare_bit = PAGEFMT_SPARE_27;
        		ecc_bit = 8;
     		spare_per_sector = 27;
      	}else if(spare_per_sector>=26){
      		spare_bit = PAGEFMT_SPARE_26;
        		ecc_bit = 8;
    		spare_per_sector = 26;
      	}else if(spare_per_sector>=16){
      		spare_bit = PAGEFMT_SPARE_16;
        		ecc_bit = 4;
    		spare_per_sector = 16;
      	}else{
      		MSG(INIT, "[NAND]: NFI not support oobsize: %x\n", spare_per_sector);
        		ASSERT(0);
      	}
      	 mtd->oobsize = spare_per_sector*(mtd->writesize/512);
      	 printk("[NAND]select ecc bit:%d, sparesize :%d\n",ecc_bit,mtd->oobsize);

        /* Setup PageFormat */
        if (4096 == mtd->writesize)
        {
            NFI_SET_REG16(NFI_PAGEFMT_REG16, (spare_bit << PAGEFMT_SPARE_SHIFT) | PAGEFMT_4K);
            nand->cmdfunc = mtk_snand_command_bp;
        } else if (2048 == mtd->writesize)
        {
            NFI_SET_REG16(NFI_PAGEFMT_REG16, (spare_bit << PAGEFMT_SPARE_SHIFT) | PAGEFMT_2K);
            nand->cmdfunc = mtk_snand_command_bp;
        }                       /* else if (512 == mtd->writesize) {
                                   NFI_SET_REG16(NFI_PAGEFMT_REG16, (PAGEFMT_SPARE_16 << PAGEFMT_SPARE_SHIFT) | PAGEFMT_512);
                                   nand->cmdfunc = mtk_nand_command_sp;
                                   } */

    	mtk_snand_ecc_config(hw,ecc_bit);

        g_snand_rs_ecc_bit_second_part = ecc_bit;
        g_snand_rs_spare_per_sector_second_part_nfi = (spare_bit << PAGEFMT_SPARE_SHIFT);
        g_snand_spare_per_sector = spare_per_sector;

        g_bInitDone = true;
    }
    switch (chip)
    {
      case -1:
          break;
      case 0:
#if defined(CONFIG_EARLY_LINUX_PORTING)		// FPGA NAND is placed at CS1 not CS0
			//DRV_WriteReg16(NFI_CSEL_REG16, 1);
			break;
#endif
      case 1:
          //DRV_WriteReg16(NFI_CSEL_REG16, chip);
          break;
    }
}

/******************************************************************************
 * mtk_snand_read_byte
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
static uint8_t mtk_snand_read_byte(struct mtd_info *mtd)
{
    u8      reg8;
    u32     reg32;

    if (SNAND_RB_READ_ID == g_snand_read_byte_mode)
    {
        if (g_snand_id_data_idx > SNAND_MAX_ID)
        {
            return 0;
        }
        else
        {
            return g_snand_id_data[g_snand_id_data_idx++];
        }
    }
    else if (SNAND_RB_CMD_STATUS == g_snand_read_byte_mode) // get feature to see the status of program and erase (e.g., nand_wait)
    {
		if ((SNAND_DEV_ERASE_DONE == g_snand_dev_status) ||
			(SNAND_DEV_PROG_DONE == g_snand_dev_status))
		{
			return g_snand_cmd_status;	// report the latest device operation status (program OK, erase OK, program failed, erase failed ...etc)
		}
		else if (SNAND_DEV_PROG_EXECUTING == g_snand_dev_status)    // check ready again
        {
            DRV_WriteReg32(RW_SNAND_GPRAM_DATA, (SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_STATUS << 8)));
            DRV_WriteReg32(RW_SNAND_MAC_OUTL, 2);
            DRV_WriteReg32(RW_SNAND_MAC_INL , 1);

            mtk_snand_dev_mac_op(SPI);

            reg8 = DRV_Reg8(((P_U8)RW_SNAND_GPRAM_DATA + 2));

            if (0 == (reg8 & SNAND_STATUS_OIP)) // ready
            {
            	g_snand_dev_status = SNAND_DEV_PROG_DONE;

                if (0 != (reg8 & SNAND_STATUS_PROGRAM_FAIL)) // ready but having fail report from device
                {
                	MSG(INIT, "[snand] Prog failed!\n");	// Stanley Chu (add more infomation like page address)

    				g_snand_cmd_status = NAND_STATUS_READY | NAND_STATUS_FAIL;
                }
                else
                {
    				g_snand_cmd_status = NAND_STATUS_READY;
                }

                return g_snand_cmd_status;
            }
            else
            {
                return 0;	// busy
            }
        }
        else if (SNAND_NFI_AUTO_ERASING == g_snand_dev_status)  // check ready again
        {
            // wait for auto erase finish

            reg32 = DRV_Reg32(RW_SNAND_STA_CTL1);

            if ((reg32 & SNAND_AUTO_BLKER) == 0)
            {
                return 0;	// busy
            }
            else
            {
            	g_snand_dev_status = SNAND_DEV_ERASE_DONE;

                reg8 = (u8)(DRV_Reg32(RW_SNAND_GF_CTL1) & SNAND_GF_STATUS_MASK);

                if (0 != (reg8 & SNAND_STATUS_ERASE_FAIL)) // ready but having fail report from device
                {
                	MSG(INIT, "[snand] Erase failed!\n");  // Stanley Chu (add more infomation like page address)

    				g_snand_cmd_status = NAND_STATUS_READY | NAND_STATUS_FAIL;
                }
                else
                {
    				g_snand_cmd_status = NAND_STATUS_READY;
                }

                return g_snand_cmd_status;    // return non-0
            }
        }
        else if ((SNAND_NFI_CUST_READING == g_snand_dev_status))
		{
		    g_snand_cmd_status = 0; // busy

			return g_snand_cmd_status;
		}
        else    // idle
        {
            g_snand_dev_status = SNAND_IDLE;

            g_snand_cmd_status = (NAND_STATUS_READY | NAND_STATUS_WP);

            return g_snand_cmd_status;    // return NAND_STATUS_WP to indicate SPI-NAND does NOT WP
        }
    }
    else
    {
        return 0;
    }
}

/******************************************************************************
 * mtk_snand_read_buf
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
static void mtk_snand_read_buf(struct mtd_info *mtd, uint8_t * buf, int len)
{
    struct nand_chip *nand = (struct nand_chip *)mtd->priv;
    struct NAND_CMD *pkCMD = &g_kCMD;
    u32 u4ColAddr = pkCMD->u4ColAddr;
    u32 u4PageSize = mtd->writesize;

    if (u4ColAddr < u4PageSize)
    {
        if ((u4ColAddr == 0) && (len >= u4PageSize))
        {
            mtk_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize, buf, pkCMD->au1OOB);
            if (len > u4PageSize)
            {
                u32 u4Size = min(len - u4PageSize, sizeof(pkCMD->au1OOB));
                memcpy(buf + u4PageSize, pkCMD->au1OOB, u4Size);
            }
        } else
        {
            mtk_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize, nand->buffers->databuf, pkCMD->au1OOB);
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
 * mtk_snand_write_buf
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
static void mtk_snand_write_buf(struct mtd_info *mtd, const uint8_t * buf, int len)
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
 * mtk_snand_write_page_hwecc
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
static void mtk_snand_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t * buf)
{
    mtk_snand_write_buf(mtd, buf, mtd->writesize);
    mtk_snand_write_buf(mtd, chip->oob_poi, mtd->oobsize);
}

/******************************************************************************
 * mtk_snand_read_page_hwecc
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
static int mtk_snand_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, uint8_t * buf, int page)
{
#if 0
    mtk_snand_read_buf(mtd, buf, mtd->writesize);
    mtk_snand_read_buf(mtd, chip->oob_poi, mtd->oobsize);
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
static int mtk_snand_read_page(struct mtd_info *mtd, struct nand_chip *chip, u8 * buf, int page)
{
    int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
    int block = page / page_per_block;
    u16 page_in_block = page % page_per_block;
    int mapped_block = get_mapping_block_index(block);
#if CFG_PERFLOG_DEBUG
    struct timeval stimer,etimer;
    do_gettimeofday(&stimer);
#endif

    if (mtk_nand_exec_read_page(mtd, page_in_block + mapped_block * page_per_block, mtd->writesize, buf, chip->oob_poi))
    {
#if CFG_PERFLOG_DEBUG
        do_gettimeofday(&etimer);
        g_NandPerfLog.ReadPageTotalTime+= Cal_timediff(&etimer,&stimer);
        g_NandPerfLog.ReadPageCount++;
#endif
        return 0;
    }
    /* else
       return -EIO; */
    return 0;
}

/******************************************************************************
 *
 * Erase a block at a logical address
 *
 *****************************************************************************/

static void mtk_snand_dev_stop_erase(void)
{
    u32 reg;

    // set 1 then set 0 to clear done flag
    reg = DRV_Reg32(RW_SNAND_STA_CTL1);

    DRV_WriteReg32(RW_SNAND_STA_CTL1, reg);
    reg = reg & ~SNAND_AUTO_BLKER;
    DRV_WriteReg32(RW_SNAND_STA_CTL1, reg);

    // clear trigger bit
    reg = DRV_Reg32(RW_SNAND_ER_CTL);
    reg &= ~SNAND_AUTO_ERASE_TRIGGER;
    DRV_WriteReg32(RW_SNAND_ER_CTL, reg);

    g_snand_dev_status = SNAND_IDLE;
}

/* NOTE(Bayi): added for erase ok */
void mtk_snand_dev_unlock_all_blocks()
{
    U32 cmd;
    U8  lock;
    U8  lock_new;

    printf("[SNF] Unlock all blocks ...\n\r");

    // read original block lock settings
    cmd = SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_BLOCK_LOCK << 8);
    DRV_WriteReg32(RW_SNAND_GPRAM_DATA, cmd);
    DRV_WriteReg32(RW_SNAND_MAC_OUTL, 2);
    DRV_WriteReg32(RW_SNAND_MAC_INL , 1);

	mtk_snand_dev_mac_enable(SPI);
	mtk_snand_dev_mac_trigger();
	mtk_snand_dev_mac_leave();

    lock = DRV_Reg8(((P_U8)RW_SNAND_GPRAM_DATA + 2));
	printf("[SNF] Lock register(before). lock:0x%x \n\r", lock);

    lock_new = lock & ~SNAND_BLOCK_LOCK_BITS;

    if (lock != lock_new)
    {
        // write enable
        mtk_snand_dev_command(SNAND_CMD_WRITE_ENABLE, 1);

        // set features
        cmd = SNAND_CMD_SET_FEATURES | (SNAND_CMD_FEATURES_BLOCK_LOCK << 8) | (lock_new << 16);
        mtk_snand_dev_command(cmd, 3);
    }

    // cosnfrm if unlock is successfull
    cmd = SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_BLOCK_LOCK << 8);
    DRV_WriteReg32(RW_SNAND_GPRAM_DATA, cmd);
    DRV_WriteReg32(RW_SNAND_MAC_OUTL, 2);
    DRV_WriteReg32(RW_SNAND_MAC_INL , 1);

    mtk_snand_dev_mac_enable(SPI);
	mtk_snand_dev_mac_trigger();
	mtk_snand_dev_mac_leave();

    lock = DRV_Reg8(((P_U8)RW_SNAND_GPRAM_DATA + 2));

    if (lock & SNAND_BLOCK_LOCK_BITS)
    {
        printf("[SNF] Unlock all blocks failed!\n\r");
        while(1);
    }
    else
    {
        printf("[SNF] Lock register (after) new lock: %x\n\r", lock);
    }
}

/**********************************************************
Description : SPI_NAND_Reset_Device
Input       : None
Output      : None
***********************************************************/

void mtk_snand_ecc_control(u8 enable)
{
    u32 cmd;
    u8  otp;
    u8  otp_new;

    printf("[SNF] [Cheng] ECC Control (1: Enable, 0: Disable) : %d\n\r", enable);

    // read original otp settings

    cmd = SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_OTP << 8);
    DRV_WriteReg32(RW_SNAND_GPRAM_DATA, cmd);
    DRV_WriteReg32(RW_SNAND_MAC_OUTL, 2);
    DRV_WriteReg32(RW_SNAND_MAC_INL , 1);
	mtk_snand_dev_mac_op(SPI);
	mtk_snand_dev_mac_trigger();
    mtk_snand_dev_mac_leave();

    otp = DRV_Reg8(((P_U8)RW_SNAND_GPRAM_DATA + 2));

    if (enable == TRUE)
    {
        otp_new = otp | SNAND_OTP_ECC_ENABLE;
    }
    else
    {
        otp_new = otp & ~SNAND_OTP_ECC_ENABLE;
    }

    if (otp != otp_new)
    {
        // write enable

        mtk_snand_dev_command(SNAND_CMD_WRITE_ENABLE, 1);


        // set features
        cmd = SNAND_CMD_SET_FEATURES | (SNAND_CMD_FEATURES_OTP << 8) | (otp_new << 16);

        mtk_snand_dev_command(cmd, 3);


        // cosnfrm

        // read original otp settings

        cmd = SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_OTP << 8);
        DRV_WriteReg32(RW_SNAND_GPRAM_DATA, cmd);
        DRV_WriteReg32(RW_SNAND_MAC_OUTL, 2);
        DRV_WriteReg32(RW_SNAND_MAC_INL , 1);
		mtk_snand_dev_mac_op(SPI);
		mtk_snand_dev_mac_trigger();
		mtk_snand_dev_mac_leave();
		

        otp = DRV_Reg8(((P_U8)RW_SNAND_GPRAM_DATA + 2));

        if (otp != otp_new)
        {
            while(1);
        }

    }
}

int mtk_snand_reset_device(void)
{
    u8      cmd = SNAND_CMD_SW_RESET, reg8;
    u32     cmd32 = SNAND_CMD_SW_RESET;
    bool    bTimeout = FALSE;
    u32     timeout_tick;
    u32     start_tick, reg32;
	
    mtk_snand_reset();

    // issue SW RESET command to device
    mtk_snand_dev_command_ext(SPI, &cmd, NULL, 1, 0);

    // wait for awhile, then polling status register (required by spec)
    gpt_busy_wait_us(SPI_NAND_RESET_LATENCY_US);


	// 2. Get features (status polling)

    cmd32 = SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_STATUS << 8);

    DRV_WriteReg32(RW_SNAND_GPRAM_DATA, cmd32);
    DRV_WriteReg32(RW_SNAND_MAC_OUTL, 2);
    DRV_WriteReg32(RW_SNAND_MAC_INL , 1);

    mtk_snand_dev_mac_op(SPI);

    // polling status register
    timeout_tick = gpt4_time2tick_us(SPI_NAND_MAX_RDY_AFTER_RST_LATENCY_US);
    start_tick = gpt4_get_current_tick();

    for (bTimeout = FALSE; bTimeout != TRUE;)
    {
        mtk_snand_dev_mac_enable(SPI);
        mtk_snand_dev_mac_trigger();
        mtk_snand_dev_mac_leave();

        cmd = DRV_Reg8(((P_U8)RW_SNAND_GPRAM_DATA + 2));
		if (0 == (cmd & SNAND_STATUS_OIP))
		{
            break;
        }

        bTimeout = gpt4_timeout_tick(start_tick, timeout_tick);
    }

    if(!bTimeout)
        return 0;
    else
        return -1;

}

void mtk_snand_dev_erase(u32 row_addr)   // auto erase
{
    u32  reg, polling_times;

    //mtk_snand_reset_con();

	/* Reset NFI HW internal state machine and flush NFI in/out FIFO */
	if (!mtk_snand_reset_con())
	{
		return false;
	}
	
	 // 1. Write Enable
	mtk_snand_dev_command(SNAND_CMD_WRITE_ENABLE, 1);

	mtk_snand_reset_device();
	
	// erase address
    DRV_WriteReg32(RW_SNAND_ER_CTL2, row_addr);

    // set loop limit and polling cycles
    reg = (SNAND_LOOP_LIMIT_NO_LIMIT << SNAND_LOOP_LIMIT_OFFSET) | 0x20;
    DRV_WriteReg32(RW_SNAND_GF_CTL3, reg);

    // set latch latency & CS de-select latency (ignored)

    // set erase command
    reg = SNAND_CMD_BLOCK_ERASE << SNAND_ER_CMD_OFFSET;
    DRV_WriteReg32(RW_SNAND_ER_CTL, reg);

    // trigger interrupt waiting
    reg = DRV_Reg16(NFI_INTR_EN_REG16);
    reg = INTR_AUTO_BLKER_INTR_EN;
    DRV_WriteReg16(NFI_INTR_EN_REG16, reg);

    // trigger auto erase
    reg = DRV_Reg32(RW_SNAND_ER_CTL);
    reg |= SNAND_AUTO_ERASE_TRIGGER;
    DRV_WriteReg32(RW_SNAND_ER_CTL, reg);

	//printk("BAYI TEST 3', erase over polling_times:0x%x\n", polling_times);
#if 1
    g_snand_dev_status = SNAND_NFI_AUTO_ERASING;
    g_snand_cmd_status = 0; // busy
#else
	g_snand_dev_status = SNAND_IDLE;//SNAND_NFI_AUTO_ERASING;
    g_snand_cmd_status = NAND_STATUS_READY;//0; // busy

#endif
}

#if 1

int mtk_nand_erase_hw(struct mtd_info *mtd, int page)
{
    struct nand_chip *chip = (struct nand_chip *)mtd->priv;
    int ret =0;

#ifdef _MTK_NAND_DUMMY_DRIVER_
    if (dummy_driver_debug)
    {
#ifdef TESTTIME
        unsigned long long time = sched_clock();
        if (!((time * 123 + 59) % 1024))
        {
            printk(KERN_INFO "[NAND_DUMMY_DRIVER] Simulate erase error at page: 0x%x\n", page);
            return NAND_STATUS_FAIL;
        }
#endif
    }
#endif

    if (TRUE == mtk_snand_is_vendor_reserved_blocks(page))  // return erase failed for reserved blocks
    {
        return NAND_STATUS_FAIL;
    }

    if (SNAND_BUSY < g_snand_dev_status)
    {
        printk("[%s] device is not IDLE!!\n", __FUNCTION__);
    }

    mtk_snand_dev_erase(page);

    ret = chip->waitfunc(mtd, chip);
	printk("[%s] mtk_nand_erase_hw @4249, ret:0x%x. page:0x%x \n", __FUNCTION__, ret, page);

    mtk_snand_dev_stop_erase();

    return ret;
}

#else

/**********************************************************
Description : mtk_nand_erase_hw
Remark      : Must be page alignment.
***********************************************************/
int mtk_nand_erase_hw(struct mtd_info *mtd, int page)
{
	int				status	  = 0;
	U32 			row_addr	= 0;
	U32 			cmd;

	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	row_addr = page;
	
	// 1. Write Enable
	mtk_snand_dev_command(SNAND_CMD_WRITE_ENABLE, 1);
	
	// 2. Block Erase
	cmd = mtk_snand_gen_c1a3(SNAND_CMD_BLOCK_ERASE, row_addr); // BLOCK_ERASE command + 3-byte address

	mtk_snand_dev_command(cmd, 4);

	udelay(1);	// note. spec does not require delay here!
	
	// 3. Status Polling
	cmd = SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_STATUS << 8);

	DRV_WriteReg32(RW_SNAND_GPRAM_DATA, cmd);
	DRV_WriteReg32(RW_SNAND_MAC_OUTL, 2);
	DRV_WriteReg32(RW_SNAND_MAC_INL , 1);

	while (1)
	{
		mtk_snand_dev_mac_enable(SPI);
        mtk_snand_dev_mac_trigger();
        mtk_snand_dev_mac_leave();

		cmd = DRV_Reg8(((P_U8)RW_SNAND_GPRAM_DATA + 2));

		if ((cmd & SNAND_STATUS_ERASE_FAIL) != 0)
		{
			status = 1;
		}

		if ((cmd & SNAND_STATUS_OIP) == 0)
		{
			break;
		}
	}

	g_snand_dev_status = SNAND_NFI_AUTO_ERASING;
	g_snand_cmd_status = 0; // busy
	udelay(1);
	status = chip->waitfunc(mtd, chip);
	//printk("[%s] mtk_nand_erase_hw @4066, ret:0x%x \n", __FUNCTION__, ret);
	
	mtk_snand_dev_stop_erase();

	return status;
}


#endif
static int mtk_nand_erase(struct mtd_info *mtd, int page)
{
    // get mapping
    struct nand_chip *chip = mtd->priv;
    int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
    int page_in_block = page % page_per_block;
    int block = page / page_per_block;
#if CFG_PERFLOG_DEBUG
	struct timeval stimer,etimer;
#endif
    int mapped_block;
    int status;

    mapped_block = get_mapping_block_index(block);

#if CFG_PERFLOG_DEBUG
    do_gettimeofday(&stimer);
#endif

    status = mtk_nand_erase_hw(mtd, page_in_block + page_per_block * mapped_block);

    if (status & NAND_STATUS_FAIL)
    {
        if (update_bmt((page_in_block + mapped_block * page_per_block) << chip->page_shift, UPDATE_ERASE_FAIL, NULL, NULL))
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

    if (!mtk_snand_ready_for_read(chip, page, 0, true, buf))
        return -EIO;

    while (len > 0)
    {
        mtk_snand_set_mode(CNFG_OP_CUST);
        DRV_WriteReg16(NFI_CON_REG16, 8 << CON_NFI_SEC_SHIFT);

        if (len > mtd->writesize)   // remained more than one page
        {
            if (!mtk_nand_set_command(0x31)) // todo: add cache read command
                goto ret;
        } else
        {
            if (!mtk_nand_set_command(0x3f)) // last page remained
                goto ret;
        }

        mtk_snand_status_ready(STA_NAND_BUSY);

#ifdef __INTERNAL_USE_AHB_MODE__
        //if (!mtk_snand_dma_read_data(buf, mtd->writesize))
        if (!mtk_snand_read_page_data(mtd, buf, mtd->writesize))
            goto ret;
#else
        if (!mtk_snand_mcu_read_data(buf, mtd->writesize))
            goto ret;
#endif

        // get ecc error info
        mtk_snand_check_bch_error(mtd, buf, 3, page);
        mtk_snand_ecc_decode_end();

        page++;
        len -= mtd->writesize;
        buf += mtd->writesize;
        ops->retlen += mtd->writesize;

        if (len > 0)
        {
            mtk_snand_ecc_decode_start();
            mtk_snand_reset_con();
        }

    }

    res = 0;

  ret:
    mtk_snand_stop_read();

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
 * mtk_snand_read_oob_raw
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
static int mtk_snand_read_oob_raw(struct mtd_info *mtd, uint8_t * buf, int page_addr, int len)
{
    struct nand_chip *chip = (struct nand_chip *)mtd->priv;
    int bRet = true;
    int num_sec, num_sec_original;
    u8 * p_buf_local;
    u32 i;

    if (len > NAND_MAX_OOBSIZE || len % OOB_AVAI_PER_SECTOR || !buf)
    {
        printk(KERN_WARNING "[%s] invalid parameter, len: %d, buf: %p\n", __FUNCTION__, len, buf);
        return -EINVAL;
    }

    num_sec_original = num_sec = len / OOB_AVAI_PER_SECTOR;

    mtk_snand_rs_reconfig_nfiecc(mtd, page_addr);

    if (((num_sec_original * NAND_SECTOR_SIZE) == mtd->writesize) && mtk_snand_rs_if_require_split())
    {
        num_sec--;
    }

    // read the 1st sector (including its spare area) with MTK ECC enabled
    if (mtk_snand_ready_for_read(chip, page_addr, 0, num_sec, true, g_snand_temp))
    {
        if (!mtk_snand_read_page_data(mtd, g_snand_temp, num_sec, true))  // only read 1 sector
        {
            bRet = false;
        }
        if (!mtk_snand_status_ready(STA_NAND_BUSY))
        {
            bRet = false;
        }

        if (!mtk_snand_check_dececc_done(num_sec))
        {
            bRet = false;
        }

        mtk_snand_read_fdm_data(g_snand_spare, num_sec);

        mtk_snand_stop_read();
    }

    if (((num_sec_original * NAND_SECTOR_SIZE) == mtd->writesize) && mtk_snand_rs_if_require_split())
    {
        // read part II

        num_sec++;

        // note: use local temp buffer to read part 2
        mtk_snand_read_page_part2(mtd, page_addr, num_sec, g_snand_temp + ((num_sec - 1) * NAND_SECTOR_SIZE));

        // copy spare data
        for (i = 0; i < OOB_AVAI_PER_SECTOR; i++)
        {
            g_snand_spare[(num_sec - 1) * OOB_AVAI_PER_SECTOR + i] = g_snand_temp[((num_sec - 1) * NAND_SECTOR_SIZE) + NAND_SECTOR_SIZE + i];
        }
    }

    mtk_snand_dev_enable_spiq(FALSE);

    num_sec = num_sec * OOB_AVAI_PER_SECTOR;

    for (i = 0; i < num_sec; i++)
    {
        buf[i] = g_snand_spare[i];
    }

    #if 0   // old code. it use MCU read with ECC disabled! This method has potential data loss issues.
    {
        while (len > 0)
        {
            read_len = min(len, spare_per_sector);
            col_addr = NAND_SECTOR_SIZE + sector * (NAND_SECTOR_SIZE + spare_per_sector); // TODO: Fix this hard-code 16

            if (!mtk_snand_ready_for_read(chip, page_addr, col_addr, 1, false, NULL))
            {
                printk(KERN_WARNING "mtk_snand_ready_for_read return failed\n");
                res = -EIO;
                goto error;
            }

            if (!mtk_snand_mcu_read_data(buf + spare_per_sector * sector, read_len, false))    // TODO: and this 8
            {
                printk(KERN_WARNING "mtk_snand_mcu_read_data return failed\n");
                res = -EIO;
                goto error;
            }

            SNAND_DBG_OFF();

            mtk_snand_stop_read();

            sector++;
            len -= read_len;
        }
    }
    #endif
    #if 0
    else                      //should be 64
    {
        col_addr = NAND_SECTOR_SIZE;

        if (!mtk_snand_reset_con())
        {
            goto error;
        }

        mtk_snand_set_mode(0x6000);
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
        DRV_WriteReg16(NFI_CON_REG16, 4 << CON_NFI_SEC_SHIFT);

        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);

        mtk_snand_set_autoformat(false);

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
        if (!mtk_snand_status_ready(STA_NAND_BUSY))
        {
            goto error;
        }

        read_len = min(len, spare_per_sector);
        if (!mtk_snand_mcu_read_data(buf + spare_per_sector * sector, read_len))    // TODO: and this 8
        {
            printk(KERN_WARNING "mtk_snand_mcu_read_data return failed first 16\n");
            res = -EIO;
            goto error;
        }
        sector++;
        len -= read_len;
        mtk_snand_stop_read();
        while (len > 0)
        {
            read_len = min(len,  spare_per_sector);
            if (!mtk_nand_set_command(0x05))
            {
                goto error;
            }

            col_addr = NAND_SECTOR_SIZE + sector * (NAND_SECTOR_SIZE + 16);
            if (chip->options & NAND_BUSWIDTH_16)
            {
                col_addr /= 2;
            }
            DRV_WriteReg32(NFI_COLADDR_REG32, col_addr);
            DRV_WriteReg16(NFI_ADDRNOB_REG16, 2);
            DRV_WriteReg16(NFI_CON_REG16, 4 << CON_NFI_SEC_SHIFT);

            if (!mtk_snand_status_ready(STA_ADDR_STATE))
            {
                goto error;
            }

            if (!mtk_nand_set_command(0xE0))
            {
                goto error;
            }
            if (!mtk_snand_status_ready(STA_NAND_BUSY))
            {
                goto error;
            }
            if (!mtk_snand_mcu_read_data(buf + spare_per_sector * sector, read_len))    // TODO: and this 8
            {
                printk(KERN_WARNING "mtk_snand_mcu_read_data return failed first 16\n");
                res = -EIO;
                goto error;
            }
            mtk_snand_stop_read();
            sector++;
            len -= read_len;
        }
        //dump_data(&testbuf[16],16);
        //printk(KERN_ERR "\n");
    }

  error:
    NFI_CLN_REG16(NFI_CON_REG16, CON_NFI_BRD);

    #endif

    if (true == bRet) return 0;
    else return 1;
}

static int mtk_snand_write_oob_raw(struct mtd_info *mtd, const uint8_t * buf, int page_addr, int len)
{
    struct nand_chip *chip = mtd->priv;

    u32 col_addr = 0;
    u32 sector = 0;
    int write_len = 0;
    int status;
    int sec_num = 1<<(chip->page_shift-9);
    int spare_per_sector = mtd->oobsize/sec_num;

    if (len >  NAND_MAX_OOBSIZE || len % OOB_AVAI_PER_SECTOR || !buf)
    {
        printk(KERN_WARNING "[%s] invalid parameter, len: %d, buf: %p\n", __FUNCTION__, len, buf);
        return -EINVAL;
    }

    mtk_snand_rs_reconfig_nfiecc(mtd, page_addr);

    while (len > 0)
    {
        write_len = min(len,  spare_per_sector);
        col_addr = sector * (NAND_SECTOR_SIZE +  spare_per_sector) + NAND_SECTOR_SIZE;
        if (!mtk_snand_ready_for_write(chip, page_addr, col_addr, false, NULL))
        {
            return -EIO;
        }

        if (!mtk_snand_mcu_write_data(mtd, buf + sector * spare_per_sector, write_len, false))
        {
            return -EIO;
        }

        (void)mtk_snand_check_RW_count(write_len);

        mtk_snand_stop_write();

        mtk_snand_dev_program_execute(page_addr);

        status = chip->waitfunc(mtd, chip);

        if (status & NAND_STATUS_FAIL)
        {
            printk(KERN_INFO "<by>status: %d\n", status);
            return -EIO;
        }

        len -= write_len;
        sector++;
    }

    return 0;
}

static int mtk_snand_write_oob_hw(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
    // u8 *buf = chip->oob_poi;
    int i, iter;

    int sec_num = 1<<(chip->page_shift-9);
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

    return mtk_snand_write_oob_raw(mtd, local_oob_buf, page, mtd->oobsize);
}

static int mtk_snand_write_oob(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
    int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
    int block = page / page_per_block;
    u16 page_in_block = page % page_per_block;
    int mapped_block = get_mapping_block_index(block);

    // write bad index into oob
    if (mapped_block != block)
    {
        set_bad_index_to_oob(chip->oob_poi, block);
    } else
    {
        set_bad_index_to_oob(chip->oob_poi, FAKE_INDEX);
    }

    if (mtk_snand_write_oob_hw(mtd, chip, page_in_block + mapped_block * page_per_block /* page */ ))
    {
        MSG(INIT, "write oob fail at block: 0x%x, page: 0x%x\n", mapped_block, page_in_block);
        if (update_bmt((page_in_block + mapped_block * page_per_block) << chip->page_shift, UPDATE_WRITE_FAIL, NULL, chip->oob_poi))
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
    int block = (int)offset >> chip->phys_erase_shift;
    int page = block * (1 << (chip->phys_erase_shift - chip->page_shift));
    int ret;

    u8 buf[8];
    memset(buf, 0xFF, 8);
    buf[0] = 0;

    ret = mtk_snand_write_oob_raw(mtd, buf, page, 8);
    return ret;
}

static int mtk_snand_block_markbad(struct mtd_info *mtd, loff_t offset)
{
    struct nand_chip *chip = mtd->priv;
    int block = (int)offset >> chip->phys_erase_shift;
    int mapped_block;
    int ret;

    nand_get_device(chip, mtd, FL_WRITING);

    mapped_block = get_mapping_block_index(block);
    ret = mtk_nand_block_markbad_hw(mtd, mapped_block << chip->phys_erase_shift);

    nand_release_device(mtd);

    return ret;
}

int mtk_snand_read_oob_hw(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
    int i;
    u8 iter = 0;

    int sec_num = 1<<(chip->page_shift-9);
    int spare_per_sector = mtd->oobsize/sec_num;
#ifdef TESTTIME
    unsigned long long time1, time2;

    time1 = sched_clock();
#endif

    /* NOTE(Nelson):
        * mtd->oobsize = devinfo.sparesize; (64)
        * but total available OOB size is 32
        * So modify to (sec_num * OOB_AVAI_PER_SECTOR) (32)
        */
    //if (mtk_snand_read_oob_raw(mtd, chip->oob_poi, page, mtd->oobsize))    
    if (mtk_snand_read_oob_raw(mtd, chip->oob_poi, page, (sec_num * OOB_AVAI_PER_SECTOR)))
    {
        // printk(KERN_ERR "[%s]mtk_snand_read_oob_raw return failed\n", __FUNCTION__);
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
        iter = (i / OOB_AVAI_PER_SECTOR) *  spare_per_sector + OOB_AVAI_PER_SECTOR + i % OOB_AVAI_PER_SECTOR;
        chip->oob_poi[chip->ecc.layout->eccpos[i]] = local_oob_buf[iter];
    }

    // copy FDM data
    for (i = 0; i < sec_num; i++)
    {
        memcpy(&chip->oob_poi[i * OOB_AVAI_PER_SECTOR], &local_oob_buf[i *  spare_per_sector], OOB_AVAI_PER_SECTOR);
    }

    return 0;
}

static int mtk_snand_read_oob(struct mtd_info *mtd, struct nand_chip *chip, int page, int sndcmd)
{
    int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
    int block = page / page_per_block;
    u16 page_in_block = page % page_per_block;
    int mapped_block = get_mapping_block_index(block);

    mtk_snand_read_oob_hw(mtd, chip, page_in_block + mapped_block * page_per_block);

    return 0;                   // the return value is sndcmd
}

int mtk_nand_block_bad_hw(struct mtd_info *mtd, loff_t ofs)
{
    struct nand_chip *chip = (struct nand_chip *)mtd->priv;
    int page_addr = (int)(ofs >> chip->page_shift);
    unsigned int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
    unsigned char oob_buf[8];
    page_addr &= ~(page_per_block - 1);

    if (TRUE == mtk_snand_is_vendor_reserved_blocks(page_addr)) // return bad block if it is reserved block
    {
        return 1;
    }

    if (mtk_snand_read_oob_raw(mtd, oob_buf, page_addr, sizeof(oob_buf)))   // only read 8 bytes
    {
        printk(KERN_WARNING "mtk_snand_read_oob_raw return error\n");
        return 1;
    }

    if (oob_buf[0] != 0xff)
    {
        printk(KERN_WARNING "Bad block detected at 0x%x, oob_buf[0] is 0x%x\n", page_addr, oob_buf[0]);
       // return 1;/* NOTE(Bayi, force erase) */
    }

    return 0;                   // everything is OK, good block
}

static int mtk_snand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{
    int chipnr = 0;

    struct nand_chip *chip = (struct nand_chip *)mtd->priv;
    int block = (int)ofs >> chip->phys_erase_shift;
    int mapped_block;
    int ret;

    if (getchip)
    {
        chipnr = (int)(ofs >> chip->chip_shift);
        nand_get_device(chip, mtd, FL_READING);
        /* Select the NAND device */
        chip->select_chip(mtd, chipnr);
    }

    mapped_block = get_mapping_block_index(block);

    ret = mtk_nand_block_bad_hw(mtd, mapped_block << chip->phys_erase_shift);

    if (ret)
    {
        MSG(INIT, "Unmapped bad block: 0x%x\n", mapped_block);
        if (update_bmt(mapped_block << chip->phys_erase_shift, UPDATE_UNMAPPED_BLOCK, NULL, NULL))
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

    return 0;
}

/******************************************************************************
 * mtk_snand_verify_buf
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

char gacBuf[4096 + 128];

static int mtk_snand_verify_buf(struct mtd_info *mtd, const uint8_t * buf, int len)
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
            MSG(VERIFY, "mtk_snand_verify_buf page fail at page %d\n", pkCMD->u4RowAddr);
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
        MSG(VERIFY, "mtk_snand_verify_buf oob fail at page %d\n", pkCMD->u4RowAddr);
        MSG(VERIFY, "0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", pSrc[0], pSrc[1], pSrc[2], pSrc[3], pSrc[4], pSrc[5], pSrc[6], pSrc[7]);
        MSG(VERIFY, "0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", pDst[0], pDst[1], pDst[2], pDst[3], pDst[4], pDst[5], pDst[6], pDst[7]);
        return -1;
    }
    /*
       for (i = 0; i < len; ++i) {
       if (*pSrc != *pDst) {
       printk(KERN_ERR"mtk_snand_verify_buf oob fail at page %d\n", g_kCMD.u4RowAddr);
       return -1;
       }
       pSrc++;
       pDst++;
       }
     */
    //printk(KERN_INFO"mtk_snand_verify_buf OK at page %d\n", g_kCMD.u4RowAddr);

    return 0;
#else
    return 0;
#endif
}
#endif

/**********************************************************
Description : SNAND_GPIO_config, added by bayi
***********************************************************/
void mtk_snand_gpio_config(int state)
{
	u32 reg;
    if(state)
    {
		SNFI_GPIO_BACKUP[0] = DRV_Reg32(GPIO_MODE(5));
		SNFI_GPIO_BACKUP[1] = DRV_Reg32(GPIO_MODE(0));
		reg = SNFI_GPIO_BACKUP[0];
		reg &= ~(0x0F<<20);
		reg &= ~(0x0F<<24);
		reg |= ((0x02<<20) | (0x02<<24));
			DRV_WriteReg32(GPIO_MODE(5), reg);
			reg = SNFI_GPIO_BACKUP[1];
			reg &= ~(0x0F<<8);
			reg |= (0x02<<8);
			DRV_WriteReg32(GPIO_MODE(0), reg);
    }
    else
    {	
		DRV_WriteReg32(GPIO_MODE(5), SNFI_GPIO_BACKUP[0]);
		DRV_WriteReg32(GPIO_MODE(0), SNFI_GPIO_BACKUP[1]);	
    }
}

void mtk_snand_clk_setting(void)
{
	// MT6572 NFI default clock is on
	DRV_WriteReg32(0x1000205C, 1);
	// Default is NFI mode., switch to SNAND mode    
    DRV_WriteReg32(RW_SNAND_CNFG,1);
}


/**********************************************************
Description : SNAND_Reset
Remark      :
***********************************************************/
void mtk_snand_reset(void)
{
    bool    bTimeout;
    U32     reg;
    U32     timeout_tick;
    U32     start_tick;

    //------ Part 1. NFI

    // 1-1. wait for NFI Master Status being cleared

    timeout_tick = gpt4_time2tick_us(NFI_MAX_RESET_US);
    start_tick = gpt4_get_current_tick();

    for (bTimeout = FALSE; bTimeout != TRUE;)
    {
        reg = DRV_Reg16(NFI_STA_REG32);

        if (0 == (reg & (STA_NFI_FSM_MASK|STA_NAND_FSM_MASK)))
        {
            break;
        }

        bTimeout = gpt4_timeout_tick(start_tick, timeout_tick);
    }

    // 1-2. reset NFI State Machine and Clear FIFO Counter

    reg = CON_FIFO_FLUSH|CON_NFI_RST;
    DRV_WriteReg16(NFI_CON_REG16, reg);

    // 1-3. wait for NAND Interface & NFI Internal FSM being reset

    timeout_tick = gpt4_time2tick_us(NFI_MAX_RESET_US);
    start_tick = gpt4_get_current_tick();

    while (1)
    {
        reg = DRV_Reg16(NFI_STA_REG32);

        if (0 == (reg & (STA_NFI_FSM_MASK|STA_NAND_FSM_MASK)))
        {
            break;
        }

        bTimeout = gpt4_timeout_tick(start_tick, timeout_tick);
    }

    //------ Part  2. SNF: trigger SW_RST bit to reset state machine

    reg = DRV_Reg32(RW_SNAND_MISC_CTL);
    reg |= SNAND_SW_RST;
    DRV_WriteReg32(RW_SNAND_MISC_CTL, reg);
    reg &= ~SNAND_SW_RST;
    DRV_WriteReg32(RW_SNAND_MISC_CTL, reg);

}

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
    DRV_WriteReg16(NFI_PAGEFMT_REG16, 0);

    /* Reset the state machine and data FIFO, because flushing FIFO */
    (void)mtk_snand_reset_con();

    /* Set the ECC engine */
    if (hw->nand_ecc_mode == NAND_ECC_HW)
    {
        MSG(INIT, "%s : Use HW ECC\n", MODULE_NAME);
        if (g_bHwEcc)
        {
            NFI_SET_REG32(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
        }

        mtk_snand_ecc_config(host->hw,4);
        mtk_snand_configure_fdm(8, 1);
    }

    /* Initilize interrupt. Clear interrupt, read clear. */
    DRV_Reg16(NFI_INTR_REG16);

    /* Interrupt arise when read data or program data to/from AHB is done. */
    DRV_WriteReg16(NFI_INTR_EN_REG16, 0);

    // Enable automatic disable ECC clock when NFI is busy state

    // REMARKS! HWDCM_SWCON_ON should not be enabled for SPI-NAND! Otherwise NFI will behave abnormally!
    DRV_WriteReg16(NFI_DEBUG_CON1_REG16, (WBUF_EN));

	/* NOTE(Bayi): add gpio init and clk select */
	mtk_snand_gpio_config(TRUE);
	mtk_snand_clk_setting();
    /* NOTE(Nelson): Switch mode to Serial NAND */
    DRV_WriteReg32(RW_SNAND_CNFG, 1);

	/* NOTE(Bayi): reset device and set ecc control */
	gpt_busy_wait_us(SPI_NAND_MAX_RDY_AFTER_RST_LATENCY_US);
	mtk_snand_reset_device();
	/* NOTE(Bayi): disable internal ecc, and use mtk ecc */
	mtk_snand_ecc_control(FALSE);
	mtk_snand_dev_unlock_all_blocks();


    #ifdef CONFIG_PM
    host->saved_para.suspend_flag = 0;
    #endif
    // Reset
}

//-------------------------------------------------------------------------------
static int mtk_snand_dev_ready(struct mtd_info *mtd)
{
    u8  reg8;
    u32 reg32;

    if (SNAND_DEV_PROG_EXECUTING == g_snand_dev_status)
    {
        DRV_WriteReg32(RW_SNAND_GPRAM_DATA, (SNAND_CMD_GET_FEATURES | (SNAND_CMD_FEATURES_STATUS << 8)));
        DRV_WriteReg32(RW_SNAND_MAC_OUTL, 2);
        DRV_WriteReg32(RW_SNAND_MAC_INL , 1);

        mtk_snand_dev_mac_op(SPI);

        reg8 = DRV_Reg8(((P_U8)RW_SNAND_GPRAM_DATA + 2));

        if (0 == (reg8 & SNAND_STATUS_OIP)) // ready
        {
        	g_snand_dev_status = SNAND_DEV_PROG_DONE;

            if (0 != (reg8 & SNAND_STATUS_PROGRAM_FAIL)) // ready but having fail report from device
            {
				g_snand_cmd_status = NAND_STATUS_READY | NAND_STATUS_FAIL;
            }
            else
            {
				g_snand_cmd_status = NAND_STATUS_READY;
            }
        }
        else
        {
            g_snand_cmd_status = 0;
        }
    }
    else if (SNAND_NFI_AUTO_ERASING == g_snand_dev_status)
    {
        // wait for auto erase finish

        reg32 = DRV_Reg32(RW_SNAND_STA_CTL1);

        if ((reg32 & SNAND_AUTO_BLKER) == 0)
        {
            g_snand_cmd_status = 0; // busy
        }
        else
        {
        	g_snand_dev_status = SNAND_DEV_ERASE_DONE;

            reg8 = (u8)(DRV_Reg32(RW_SNAND_GF_CTL1) & SNAND_GF_STATUS_MASK);
			//printk("Erase status: RW_SNAND_GF_CTL1: 0x%x\n", reg8);
            if (0 != (reg8 & SNAND_STATUS_ERASE_FAIL)) // ready but having fail report from device
            {
				g_snand_cmd_status = NAND_STATUS_READY | NAND_STATUS_FAIL;
            }
            else
            {
				g_snand_cmd_status = NAND_STATUS_READY;
            }
        }
    }
    else if (SNAND_NFI_CUST_READING == g_snand_dev_status)
    {
        g_snand_cmd_status = 0; // busy
    }
    else
    {
		g_snand_cmd_status = NAND_STATUS_READY; // idle
    }

    return g_snand_cmd_status;
}

int mtk_snand_proc_show(struct seq_file *m, void *v)
{
    int i;
    SEQ_printf(m, "ID:");
    for(i=0;i<devinfo.id_length;i++){
        SEQ_printf(m, " 0x%x", devinfo.id[i]);
    }
    SEQ_printf(m, "\n");
    SEQ_printf(m, "total size: %dMiB; part number: %s\n", devinfo.totalsize,devinfo.devicename);
    SEQ_printf(m, "Current working in %s mode\n", g_i4Interrupt ? "interrupt" : "polling");
    SEQ_printf(m, "SNF_DLY_CTL1: 0x%x\n", (*RW_SNAND_DLY_CTL1));
    SEQ_printf(m, "SNF_DLY_CTL2: 0x%x\n", (*RW_SNAND_DLY_CTL2));
    SEQ_printf(m, "SNF_DLY_CTL3: 0x%x\n", (*RW_SNAND_DLY_CTL3));
    SEQ_printf(m, "SNF_DLY_CTL4: 0x%x\n", (*RW_SNAND_DLY_CTL4));
    SEQ_printf(m, "SNF_MISC_CTL: 0x%x\n", (*RW_SNAND_MISC_CTL));
    //SEQ_printf(m, "Driving: 0x%x\n", (*RW_GPIO_DRV0 & (0x0007)));
    if(g_NandPerfLog.ReadPageCount!=0)
        SEQ_printf(m, "Read Page Count:%d, Read Page totalTime:%lu, Avg. RPage:%lu\r\n",
                     g_NandPerfLog.ReadPageCount,g_NandPerfLog.ReadPageTotalTime,
                     g_NandPerfLog.ReadPageTotalTime/g_NandPerfLog.ReadPageCount);

    if(g_NandPerfLog.ReadSectorCount!=0)
        SEQ_printf(m, "Read Sub-Page Count:%d, Read Sub-Page totalTime:%lu, Avg. RPage:%lu\r\n",
                     g_NandPerfLog.ReadSectorCount,g_NandPerfLog.ReadSectorTotalTime,
                     g_NandPerfLog.ReadSectorTotalTime/g_NandPerfLog.ReadSectorCount);

    if(g_NandPerfLog.ReadBusyCount!=0)
        SEQ_printf(m, "Read Busy Count:%d, Read Busy totalTime:%lu, Avg. R Busy:%lu\r\n",
                     g_NandPerfLog.ReadBusyCount,g_NandPerfLog.ReadBusyTotalTime,
                     g_NandPerfLog.ReadBusyTotalTime/g_NandPerfLog.ReadBusyCount);
    if(g_NandPerfLog.ReadDMACount!=0)
        SEQ_printf(m, "Read DMA Count:%d, Read DMA totalTime:%lu, Avg. R DMA:%lu\r\n",
                     g_NandPerfLog.ReadDMACount,g_NandPerfLog.ReadDMATotalTime,
                     g_NandPerfLog.ReadDMATotalTime/g_NandPerfLog.ReadDMACount);

    if(g_NandPerfLog.WritePageCount!=0)
        SEQ_printf(m, "Write Page Count:%d, Write Page totalTime:%lu, Avg. WPage:%lu\r\n",
                     g_NandPerfLog.WritePageCount,g_NandPerfLog.WritePageTotalTime,
                     g_NandPerfLog.WritePageTotalTime/g_NandPerfLog.WritePageCount);
    if(g_NandPerfLog.WriteBusyCount!=0)
        SEQ_printf(m, "Write Busy Count:%d, Write Busy totalTime:%lu, Avg. W Busy:%lu\r\n",
                     g_NandPerfLog.WriteBusyCount,g_NandPerfLog.WriteBusyTotalTime,
                     g_NandPerfLog.WriteBusyTotalTime/g_NandPerfLog.WriteBusyCount);
    if(g_NandPerfLog.WriteDMACount!=0)
        SEQ_printf(m, "Write DMA Count:%d, Write DMA totalTime:%lu, Avg. W DMA:%lu\r\n",
                     g_NandPerfLog.WriteDMACount,g_NandPerfLog.WriteDMATotalTime,
                     g_NandPerfLog.WriteDMATotalTime/g_NandPerfLog.WriteDMACount);
    if(g_NandPerfLog.EraseBlockCount!=0)
        SEQ_printf(m, "EraseBlock Count:%d, EraseBlock totalTime:%lu, Avg. Erase:%lu\r\n",
                     g_NandPerfLog.EraseBlockCount,g_NandPerfLog.EraseBlockTotalTime,
                     g_NandPerfLog.EraseBlockTotalTime/g_NandPerfLog.EraseBlockCount);
    return 0;
}

#if !defined (CONFIG_NAND_BOOTLOADER)
static int mt_snand_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, mtk_snand_proc_show, inode->i_private);
}

/******************************************************************************
 * mtk_snand_proc_write
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
static int mtk_snand_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    struct mtd_info *mtd = &host->mtd;
    struct nand_chip *nand_chip = &host->nand_chip;
    char buf[16];
    char cmd;
    int value;
    int len = count;

    if (len >= sizeof(buf))
    {
        len = sizeof(buf) - 1;
    }

    if (copy_from_user(buf, buffer, len))
    {
        return -EFAULT;
    }

    sscanf(buf, "%c %x",&cmd, &value);

    switch(cmd)
    {
        case 'V':  // NFIA driving setting
            //*RW_GPIO_DRV0 = *RW_GPIO_DRV0 & ~(0x0007);          // Clear Driving
            //*RW_GPIO_DRV0 = *RW_GPIO_DRV0 | value;
            break;
        case 'B': // NFIB driving setting
            //*((volatile u32 *)(IO_CFG_BOTTOM_BASE+0x0060)) = value;
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
                 nand_get_device((struct nand_chip *)mtd->priv, mtd, FL_READING);

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
               g_NandPerfLog.ReadPageCount = 0;
               g_NandPerfLog.ReadPageTotalTime = 0;
			   g_NandPerfLog.ReadBusyCount = 0;
			   g_NandPerfLog.ReadBusyTotalTime = 0;
               g_NandPerfLog.ReadDMACount = 0;
               g_NandPerfLog.ReadDMATotalTime = 0;

               g_NandPerfLog.WritePageCount = 0;
               g_NandPerfLog.WritePageTotalTime = 0;
               g_NandPerfLog.WriteBusyCount = 0;
               g_NandPerfLog.WriteBusyTotalTime = 0;
               g_NandPerfLog.WriteDMACount = 0;
               g_NandPerfLog.WriteDMATotalTime = 0;

               g_NandPerfLog.EraseBlockCount = 0;
               g_NandPerfLog.EraseBlockTotalTime = 0;

           break;
         case 'T':  // ACCCON Setting
             DRV_WriteReg32(NFI_ACCCON_REG32,value);
            break;
	case 'U':  //Unit test
		mtk_nand_unit_test(nand_chip, mtd);
		break;
         default:
            break;
    }

    return len;
}
#endif

#define KERNEL_NAND_UNIT_TEST	1
#define KERNEL_NAND_FPGA_TEST	1
#define NAND_READ_PERFORMANCE	0
#if KERNEL_NAND_UNIT_TEST
int mtk_nand_unit_test(struct nand_chip *nand_chip, struct mtd_info *mtd)
{
    MSG(INIT, "Begin to Uboot nand unit test ... \n");
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
    printk("[P] 0x%x\n", p);
    u32 val = 0x05, TOTAL=1000;

    for (j = 0x200; j< 0x210; j++)
    {
        memset(local_buffer_16_align, 0x00, SNAND_MAX_PAGE_SIZE + _SNAND_CACHE_LINE_SIZE);
        mtk_snand_read_page(mtd, nand_chip, local_buffer_16_align, j*p);
        MSG(INIT,"[1]0x%x %x %x %x\n", *(int *)local_buffer_16_align, *((int *)local_buffer_16_align+1), *((int *)local_buffer_16_align+2), *((int *)local_buffer_16_align+3));
        //mtk_nand_erase(mtd, j*p);
        //mtk_nand_erase_hw(mtd, j*p);
        memset(local_buffer_16_align, 0x00, SNAND_MAX_PAGE_SIZE + _SNAND_CACHE_LINE_SIZE);
        if(mtk_snand_read_page(mtd, nand_chip, local_buffer_16_align, j*p))
            printk("Read page 0x%x fail!\n", j*p);
        MSG(INIT,"[2]0x%x %x %x %x\n", *(int *)local_buffer_16_align, *((int *)local_buffer_16_align+1), *((int *)local_buffer_16_align+2), *((int *)local_buffer_16_align+3));
        if (mtk_snand_block_bad(mtd, j*g_block_size, 0))
        {
            printk("Bad block at %x\n", j);
            continue;
        }
		//printk("[%s]mtk_nand_block_bad() PASS!!!\n", __func__);
		k = 0;
        {
		if(mtk_snand_write_page(mtd, nand_chip, (u8 *)patternbuff, 0, j*p+k, 0, 0))
			printk("Write page 0x%x fail!\n", j*p+k);
        }
	//printk("[%s]mtk_nand_write_page() PASS!!!\n", __func__);

#if KERNEL_NAND_FPGA_TEST
	memset(local_buffer_16_align, 0x00, g_page_size);
	if(mtk_snand_read_page(mtd, nand_chip, local_buffer_16_align, j*p+k))
		printk("Read page 0x%x fail!\n", j*p+k);
	MSG(INIT,"[3]0x%x %x %x %x\n", *(int *)local_buffer_16_align, *((int *)local_buffer_16_align+1), *((int *)local_buffer_16_align+2), *((int *)local_buffer_16_align+3));
#else
        TOTAL=1000;
        do{
	        for (k = 0; k < p; k++)
	        {
	            memset(local_buffer, 0x00, g_page_size);
	            if(mtk_snand_read_page(mtd, nand_chip, local_buffer, j*p+k))
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
#endif
    } 
    return err;
}    
#endif

/******************************************************************************
 * mtk_snand_probe
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

#if defined (CONFIG_NAND_BOOTLOADER)
int board_nand_init(struct nand_chip *nand_chip)
#else
static int mtk_snand_probe(struct platform_device *pdev)
#endif
{
    struct mtk_nand_host_hw *hw;
    struct mtd_info *mtd;
#if !defined (CONFIG_NAND_BOOTLOADER)
    struct nand_chip *nand_chip;
    struct resource *res = pdev->resource;
#endif
    int err = 0;
    u8 id[SNAND_MAX_ID];
    int i;
#if defined (CONFIG_NAND_BOOTLOADER)
    struct mtk_nand_host* host_ptr;
#endif

#if !defined (CONFIG_NAND_BOOTLOADER)
    mtk_nfi_node = of_find_compatible_node(NULL, NULL, "mediatek,NFI");
    mtk_nfi_base = of_iomap(mtk_nfi_node, 0);
    
    MSG(INIT, "of_iomap for nfi base @ 0x%p\n", mtk_nfi_base);
    
    if (mtk_nfiecc_node == NULL) {
    	mtk_nfiecc_node = of_find_compatible_node(NULL, NULL, "mediatek,NFIECC");
    	mtk_nfiecc_base = of_iomap(mtk_nfiecc_node, 0);
    	MSG(INIT, "of_iomap for nfiecc base @ 0x%p\n", mtk_nfiecc_base);
    }
    if (mtk_io_node == NULL) {
    	mtk_io_node = of_find_compatible_node(NULL, NULL, "mediatek,IOCFG_B");
    	mtk_io_base = of_iomap(mtk_io_node, 0);
    	MSG(INIT, "of_iomap for io base @ 0x%p\n", mtk_io_base);
    }
    nfi_irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
#endif

    //hw = (struct mtk_nand_host_hw *)pdev->dev.platform_data;	/* TODO(Nelson): need check!!! */
    hw = &mtk_nand_hw;
    BUG_ON(!hw);

#if 0
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
    printk(KERN_INFO "Allocate 16 byte aligned buffer: %p\n", local_buffer_16_align);

#if defined (CONFIG_NAND_BOOTLOADER)
    host_ptr->hw = hw;
#else
    host->hw = hw;
#endif

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
    mtd->dev.parent = &pdev->dev;
#endif
    mtd->name = "MTK-SNand";

    hw->nand_ecc_mode = NAND_ECC_HW;

    /* Set address of NAND IO lines */
    //nand_chip->IO_ADDR_R = (void __iomem *)NFI_DATAR_REG32;
    //nand_chip->IO_ADDR_W = (void __iomem *)NFI_DATAW_REG32;
    nand_chip->IO_ADDR_R = NULL;
    nand_chip->IO_ADDR_W = NULL;
    nand_chip->chip_delay = 20; /* 20us command delay time */
    nand_chip->ecc.mode = hw->nand_ecc_mode;    /* enable ECC */

    nand_chip->read_byte = mtk_snand_read_byte;
    nand_chip->read_buf = mtk_snand_read_buf;
    nand_chip->write_buf = mtk_snand_write_buf;
#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
    nand_chip->verify_buf = mtk_snand_verify_buf;
#endif
    nand_chip->select_chip = mtk_snand_select_chip;
    nand_chip->dev_ready = mtk_snand_dev_ready;
    nand_chip->cmdfunc = mtk_snand_command_bp;
    nand_chip->ecc.read_page = mtk_snand_read_page_hwecc;
    nand_chip->ecc.write_page = mtk_snand_write_page_hwecc;

    nand_chip->ecc.layout = &nand_oob_64;
    nand_chip->ecc.size = hw->nand_ecc_size;    //2048
    nand_chip->ecc.bytes = hw->nand_ecc_bytes;  //32

    nand_chip->options = NAND_SKIP_BBTSCAN;

    // For BMT, we need to revise driver architecture
    nand_chip->write_page = mtk_snand_write_page;
    nand_chip->read_page = mtk_snand_read_page;
    nand_chip->ecc.write_oob = mtk_snand_write_oob;
    nand_chip->ecc.read_oob = mtk_snand_read_oob;
    nand_chip->block_markbad = mtk_snand_block_markbad;   // need to add nand_get_device()/nand_release_device().
    nand_chip->erase = mtk_nand_erase;
    nand_chip->block_bad = mtk_snand_block_bad;
    nand_chip->init_size = mtk_nand_init_size;

    MSG(INIT, "Enable NFI, NFIECC and SPINFI Clock\n");

#if 0	/* TOSO(Nelson): check it! */
    if (clock_is_on(MT_CG_NFI_SW_CG) == PWR_DOWN)
    {
        enable_clock(MT_CG_NFI_SW_CG, "NAND");
    }
    if (clock_is_on(MT_CG_NFIECC_SW_CG) == PWR_DOWN)
    {
        enable_clock(MT_CG_NFIECC_SW_CG, "NAND");
    }
    if (clock_is_on(MT_CG_SPINFI_SW_CG) == PWR_DOWN)
    {
        enable_clock(MT_CG_SPINFI_SW_CG, "NAND");
    }

    //clkmux_sel(MT_CLKMUX_SPINFI_MUX_SEL, MT_CG_UPLL_D12, "NAND"); // SNF is raised to 104 MHz in preloader

    dump_clk_info_by_id(MT_CG_SPINFI_SW_CG);
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

    for(i=0;i<SNAND_MAX_ID;i++)
    {
        id[i]=nand_chip->read_byte(mtd);
    }

    manu_id = id[0];
    dev_id = id[1];

    if (!mtk_snand_get_device_info(id, &devinfo))
    {
        MSG(INIT, "Not Support this Device! \r\n");
    }

    if (devinfo.pagesize == 4096)
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

    nand_chip->ecc.layout->eccbytes = devinfo.sparesize-OOB_AVAI_PER_SECTOR*(devinfo.pagesize/NAND_SECTOR_SIZE);
    hw->nand_ecc_bytes = nand_chip->ecc.layout->eccbytes;

    // Modify to fit device character
    nand_chip->ecc.size = hw->nand_ecc_size;
    nand_chip->ecc.bytes = hw->nand_ecc_bytes;
	MSG(INIT, "BAYI-TEST 6, nand_chip->ecc.size:%d , nand_chip->ecc.bytes: %d, devinfo.sparesize:0x%x\n",nand_chip->ecc.size, nand_chip->ecc.bytes, devinfo.sparesize);

    for(i=0;i<nand_chip->ecc.layout->eccbytes;i++)
    {
		nand_chip->ecc.layout->eccpos[i]=OOB_AVAI_PER_SECTOR*(devinfo.pagesize/NAND_SECTOR_SIZE)+i;
    }

    MSG(INIT, "[NAND] pagesz:%d , oobsz: %d,eccbytes: %d\n",
       devinfo.pagesize,  sizeof(g_kCMD.au1OOB),nand_chip->ecc.layout->eccbytes);

    //MSG(INIT, "Support this Device in MTK table! %x \r\n", id);
    hw->nfi_bus_width = 4;
    //DRV_WriteReg32(NFI_ACCCON_REG32, NFI_DEFAULT_ACCESS_TIMING);

#if defined (CONFIG_NAND_BOOTLOADER)
#else
    //mt_irq_set_sens(MT_NFI_IRQ_ID, MT65xx_LEVEL_SENSITIVE);	/* NOTE(Nelson) need to check it!!! */
    mt_irq_set_sens(MT_NFI_IRQ_ID, 1);
    //mt_irq_set_polarity(MT_NFI_IRQ_ID, MT65xx_POLARITY_LOW);	/* NOTE(Nelson) need to check it!!! */
    mt_irq_set_polarity(MT_NFI_IRQ_ID, 0);
    
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
	mtd->oobsize = devinfo.sparesize;

#if defined (CONFIG_NAND_BOOTLOADER)

	nand_chip->options |= mtk_nand_init_size(mtd, nand_chip, NULL);
	mtd->writesize = devinfo.pagesize;
	nand_chip->chipsize = (((uint64_t)devinfo.totalsize) <<20);
	nand_chip->page_shift = ffs(mtd->writesize) - 1;
	nand_chip->phys_erase_shift = ffs(devinfo.blocksize*1024)-1;
	nand_chip->numchips = 1;
	nand_chip->pagemask = (nand_chip->chipsize >> nand_chip->page_shift) - 1;
#else
    /* Scan to find existance of the device */

    if (nand_scan(mtd, hw->nfi_cs_num))
    {
        MSG(INIT, "%s : nand_scan fail.\n", MODULE_NAME);
        err = -ENXIO;
        goto out;
    }
#endif

    g_page_size = mtd->writesize;
    g_block_size = devinfo.blocksize << 10;

#if !defined (CONFIG_NAND_BOOTLOADER)   
    platform_set_drvdata(pdev, host);
#endif

    nand_chip->select_chip(mtd, 0);
	MSG(INIT, "by-1:nand_chip->chipsize:0x%llx, devinfo.blocksize:0x%x\n", nand_chip->chipsize, devinfo.blocksize);
    ///#if defined(MTK_COMBO_NAND_SUPPORT)
    ///	nand_chip->chipsize -= (PART_SIZE_BMTPOOL);
    ///#else
	///    nand_chip->chipsize -= (BMT_POOL_SIZE) << nand_chip->phys_erase_shift;
    ///#endif
    mtd->size = nand_chip->chipsize;
	//MSG(INIT, "by-2:page size: 0x%x, block size:0x%x, chip size 0x%llx\n", g_page_size, g_block_size, nand_chip->chipsize);
	//MSG(INIT, "nand_chip->phys_erase_shift:0x%x, g_bmt:0x%x\n", nand_chip->phys_erase_shift, g_bmt);

#if !defined (CONFIG_NAND_BOOTLOADER)
    if (!g_bmt)
    {
	#if defined(MTK_COMBO_NAND_SUPPORT)
	if (!(g_bmt = init_bmt(nand_chip, ((PART_SIZE_BMTPOOL) >> nand_chip->phys_erase_shift))))
	#else
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

#if 0
//#if KERNEL_NAND_UNIT_TEST
	MSG(INIT, "BAYI TEST'': g_bInitDone:0x%x \n", g_bInitDone);
	nand_chip->select_chip(mtd, -1);
	MSG(INIT, "BAYI TEST''': g_bInitDone:0x%x \n", g_bInitDone);
    err = mtk_nand_unit_test(nand_chip, mtd);
    if (err == 0)
    {
        printk("Thanks to GOD, UNIT Test OK!\n");
    }
#endif


/* TODO(Nelson): mark PMT temporarily caused by Kernel panic:
 * mtd_device_parse_register() -> add_mtd_device() -> device_register() -> 
 * device_add() -> get_device_parent() NG!
 */
#if !defined (CONFIG_NAND_BOOTLOADER)
#ifdef PMT
    part_init_pmt(mtd, (u8 *) & g_exist_Partition[0]);
    err = mtd_device_register(mtd, g_exist_Partition, part_num);
#else
    err = mtd_device_register(mtd, g_pasStatic_Partition, part_num);
#endif
#endif


#ifdef _MTK_NAND_DUMMY_DRIVER_
    dummy_driver_debug = 0;
#endif

    /* Successfully!! */
    if (!err)
    {
        MSG(INIT, "[mtk_snand] probe successfully!\n");
        nand_disable_clock();
        return err;
    }

    /* Fail!! */
  out:
    MSG(INIT, "[mtk_snand] mtk_snand_probe fail, err = %d!\n", err);
    nand_release(mtd);
#if !defined (CONFIG_NAND_BOOTLOADER)
    platform_set_drvdata(pdev, NULL);
#endif
    kfree(host);
    nand_disable_clock();
    return err;
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

/******************************************************************************
 * mtk_snand_suspend
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
static int mtk_snand_suspend(struct platform_device *pdev, pm_message_t state)
{
      struct mtk_nand_host *host = platform_get_drvdata(pdev);

      // backup register
      #ifdef CONFIG_PM

      if(host->saved_para.suspend_flag==0)
      {
          nand_enable_clock();

          // Save NFI register
          host->saved_para.sNFI_CNFG_REG16 = DRV_Reg16(NFI_CNFG_REG16);
          host->saved_para.sNFI_PAGEFMT_REG16 = DRV_Reg16(NFI_PAGEFMT_REG16);
          host->saved_para.sNFI_CON_REG16 = DRV_Reg16(NFI_CON_REG16);
          host->saved_para.sNFI_ACCCON_REG32 = DRV_Reg32(NFI_ACCCON_REG32);
          host->saved_para.sNFI_INTR_EN_REG16 = DRV_Reg16(NFI_INTR_EN_REG16);
          host->saved_para.sNFI_IOCON_REG16 = DRV_Reg16(NFI_IOCON_REG16);
          host->saved_para.sNFI_CSEL_REG16 = DRV_Reg16(NFI_CSEL_REG16);
          host->saved_para.sNFI_DEBUG_CON1_REG16 = DRV_Reg16(NFI_DEBUG_CON1_REG16);

          // save ECC register
          host->saved_para.sECC_ENCCNFG_REG32 = DRV_Reg32(ECC_ENCCNFG_REG32);
          host->saved_para.sECC_FDMADDR_REG32 = DRV_Reg32(ECC_FDMADDR_REG32);
          host->saved_para.sECC_DECCNFG_REG32 = DRV_Reg32(ECC_DECCNFG_REG32);

          // save SNF
          host->saved_para.sSNAND_MISC_CTL = DRV_Reg32(RW_SNAND_MISC_CTL);
          host->saved_para.sSNAND_MISC_CTL2 = DRV_Reg32(RW_SNAND_MISC_CTL2);
          host->saved_para.sSNAND_DLY_CTL1 = DRV_Reg32(RW_SNAND_DLY_CTL1);
          host->saved_para.sSNAND_DLY_CTL2 = DRV_Reg32(RW_SNAND_DLY_CTL2);
          host->saved_para.sSNAND_DLY_CTL3 = DRV_Reg32(RW_SNAND_DLY_CTL3);
          host->saved_para.sSNAND_DLY_CTL4 = DRV_Reg32(RW_SNAND_DLY_CTL4);
          host->saved_para.sSNAND_CNFG = DRV_Reg32(RW_SNAND_CNFG);

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
 * mtk_snand_resume
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
static int mtk_snand_resume(struct platform_device *pdev)
{
    struct mtk_nand_host *host = platform_get_drvdata(pdev);

#ifdef CONFIG_PM

        if(host->saved_para.suspend_flag==1)
        {
            nand_enable_clock();
            // restore NFI register
            DRV_WriteReg16(NFI_CNFG_REG16 ,host->saved_para.sNFI_CNFG_REG16);
            DRV_WriteReg16(NFI_PAGEFMT_REG16 ,host->saved_para.sNFI_PAGEFMT_REG16);
            DRV_WriteReg16(NFI_CON_REG16 ,host->saved_para.sNFI_CON_REG16);
            DRV_WriteReg32(NFI_ACCCON_REG32 ,host->saved_para.sNFI_ACCCON_REG32);
            DRV_WriteReg16(NFI_IOCON_REG16 ,host->saved_para.sNFI_IOCON_REG16);
            DRV_WriteReg16(NFI_CSEL_REG16 ,host->saved_para.sNFI_CSEL_REG16);
            DRV_WriteReg16(NFI_DEBUG_CON1_REG16 ,host->saved_para.sNFI_DEBUG_CON1_REG16);

            // restore ECC register
            DRV_WriteReg32(ECC_ENCCNFG_REG32 ,host->saved_para.sECC_ENCCNFG_REG32);
            DRV_WriteReg32(ECC_FDMADDR_REG32 ,host->saved_para.sECC_FDMADDR_REG32);
            DRV_WriteReg32(ECC_DECCNFG_REG32 ,host->saved_para.sECC_DECCNFG_REG32);

            // restore SNAND register
            DRV_WriteReg32(RW_SNAND_MISC_CTL ,host->saved_para.sSNAND_MISC_CTL);
            DRV_WriteReg32(RW_SNAND_MISC_CTL2 ,host->saved_para.sSNAND_MISC_CTL2);
            DRV_WriteReg32(RW_SNAND_DLY_CTL1 ,host->saved_para.sSNAND_DLY_CTL1);
            DRV_WriteReg32(RW_SNAND_DLY_CTL2 ,host->saved_para.sSNAND_DLY_CTL2);
            DRV_WriteReg32(RW_SNAND_DLY_CTL3 ,host->saved_para.sSNAND_DLY_CTL3);
            DRV_WriteReg32(RW_SNAND_DLY_CTL4 ,host->saved_para.sSNAND_DLY_CTL4);
            DRV_WriteReg32(RW_SNAND_CNFG ,host->saved_para.sSNAND_CNFG);

            // Reset NFI and ECC state machine
            /* Reset the state machine and data FIFO, because flushing FIFO */
            (void)mtk_snand_reset_con();
            // Reset ECC
            DRV_WriteReg16(ECC_DECCON_REG16, DEC_DE);
            while (!DRV_Reg16(ECC_DECIDLE_REG16));

            DRV_WriteReg16(ECC_ENCCON_REG16, ENC_DE);
            while (!DRV_Reg32(ECC_ENCIDLE_REG32));


            /* Initilize interrupt. Clear interrupt, read clear. */
            DRV_Reg16(NFI_INTR_REG16);

            DRV_WriteReg16(NFI_INTR_EN_REG16 ,host->saved_para.sNFI_INTR_EN_REG16);

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

/******************************************************************************
 * mtk_snand_remove
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

static int mtk_snand_remove(struct platform_device *pdev)
{
    struct mtk_nand_host *host = platform_get_drvdata(pdev);
    struct mtd_info *mtd = &host->mtd;

    nand_release(mtd);

    kfree(host);

    nand_disable_clock();

    return 0;
}

/******************************************************************************
Device driver structure
******************************************************************************/
#if !defined (CONFIG_NAND_BOOTLOADER)
static const struct of_device_id mtk_nand_of_ids[] = {
	{ .compatible = "mediatek,NFI",},
	{}
};

static struct platform_driver mtk_nand_driver = {
    .probe = mtk_snand_probe,
    .remove = mtk_snand_remove,
    .suspend = mtk_snand_suspend,
    .resume = mtk_snand_resume,
    .driver = {
               .name = "mtk-nand",
               .owner = THIS_MODULE,
               .of_match_table = mtk_nand_of_ids,
               },
};

static const struct file_operations mtk_snand_fops = {
	//.open = mtk_snand_proc_read,
	.open = mt_snand_proc_open,
	.write = mtk_snand_proc_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
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
    static int __init mtk_nand_init(void)
#else
    static int mtk_nand_init(void)
#endif
{
    struct proc_dir_entry *entry;
    g_i4Interrupt = 0;

#if !defined (CONFIG_NAND_BOOTLOADER)
    entry = proc_create(PROCNAME, 0664, NULL, &mtk_snand_fops);

    if (entry == NULL)
    {
        MSG(INIT, "MTK SNand : unable to create /proc entry\n");
        return -ENOMEM;
    }
#endif

    printk("MediaTek SNand driver init, version %s\n", VERSION);

#if !defined (CONFIG_NAND_BOOTLOADER)
    return platform_driver_register(&mtk_nand_driver);
#else
    return 0;
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
#if !defined (CONFIG_NAND_BOOTLOADER)
static void __exit mtk_nand_exit(void)
#else
static void mtk_nand_exit(void)
#endif
{
    MSG(INIT, "MediaTek SNand driver exit, version %s\n", VERSION);

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

