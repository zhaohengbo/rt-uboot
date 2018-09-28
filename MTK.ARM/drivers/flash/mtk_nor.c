/*----------------------------------------------------------------------------*
 * No Warranty                                                                *
 * Except as may be otherwise agreed to in writing, no warranties of any      *
 * kind, whether express or implied, are given by MTK with respect to any MTK *
 * Deliverables or any use thereof, and MTK Deliverables are provided on an   *
 * "AS IS" basis.  MTK hereby expressly disclaims all such warranties,        *
 * including any implied warranties of merchantability, non-infringement and  *
 * fitness for a particular purpose and any warranties arising out of course  *
 * of performance, course of dealing or usage of trade.  Parties further      *
 * acknowledge that Company may, either presently and/or in the future,       *
 * instruct MTK to assist it in the development and the implementation, in    *
 * accordance with Company's designs, of certain softwares relating to        *
 * Company's product(s) (the "Services").  Except as may be otherwise agreed  *
 * to in writing, no warranties of any kind, whether express or implied, are  *
 * given by MTK with respect to the Services provided, and the Services are   *
 * provided on an "AS IS" basis.  Company further acknowledges that the       *
 * Services may contain errors, that testing is important and Company is      *
 * solely responsible for fully testing the Services and/or derivatives       *
 * thereof before they are used, sublicensed or distributed.  Should there be *
 * any third party action brought against MTK, arising out of or relating to  *
 * the Services, Company agree to fully indemnify and hold MTK harmless.      *
 * If the parties mutually agree to enter into or continue a business         *
 * relationship or other arrangement, the terms and conditions set forth      *
 * hereunder shall remain effective and, unless explicitly stated otherwise,  *
 * shall prevail in the event of a conflict in the terms in any agreements    *
 * entered into between the parties.                                          *
 *---------------------------------------------------------------------------*/
/******************************************************************************
* [File]			mtk_nor.c
* [Version]			v1.0
* [Revision Date]	2011-05-04
* [Author]			Shunli Wang, shunli.wang@mediatek.inc, 82896, 2012-04-27
* [Description]
*	SPI-NOR Driver Source File
* [Copyright]
*	Copyright (C) 2011 MediaTek Incorporation. All Rights Reserved.
******************************************************************************/

#define __UBOOT_NOR__ 1


#if defined(__UBOOT_NOR__)
#include <errno.h>
#include <linux/mtd/mtd.h>
#include <common.h>
#include <command.h>
#include <malloc.h>
#include "mtk_nor.h"
#else
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/math64.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mod_devicetable.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/delay.h>
#include <asm/delay.h>
#include <linux/semaphore.h>
#include <linux/dma-mapping.h>
#include <linux/completion.h>
#include <linux/time.h>
#include <linux/semaphore.h>

#include <mach/irqs.h>

#include "mtk_nor.h"

#ifdef BD_NOR_ENABLE
//#include <config/arch/chip_ver.h>
#include <linux/mtd/mtd.h>
//#include "../nand/mt85xx_part_tbl.h"
//#include "linux/mtd/mt85xx_part_oper.h"
#endif
#define  NOR_DEBUG 0
#include <asm/uaccess.h>

#include <linux/proc_fs.h>
static struct proc_dir_entry *nor_proc_dir = NULL;
static struct proc_dir_entry *nor_write_enable = NULL;
static struct proc_dir_entry *nor_protect_2nd_nor = NULL;
#endif // !defined(__UBOOT_NOR__)

int enable_protect_ro=0;
int protect_2nd_nor=0;
UINT8 u4MenuID=0;
// get ns time

#if !defined(__UBOOT_NOR__)
extern int add_mtd_device(struct mtd_info *mtd);
extern int del_mtd_device (struct mtd_info *mtd);
extern int add_mtd_partitions(struct mtd_info *master,
		       const struct mtd_partition *parts,
		       int nbparts);
extern int parse_mtd_partitions(struct mtd_info *master, const char * const *types,
				 struct mtd_partition **pparts,
				 struct mtd_part_parser_data *data);

extern void do_gettimeofday(struct timeval * tv);
/* For mtdchar.c and mtdsdm.c used 
  */
#ifndef BD_NOR_ENABLE
extern char *mtdchar_buf;
extern struct semaphore mtdchar_share;
#endif
// Aligned the buffer address for DMA used.
#define ENABLE_LOCAL_BUF 1
#define LOCAL_BUF_SIZE (SFLASH_MAX_DMA_SIZE*20)   //160k
static u8 _pu1NorDataBuf[LOCAL_BUF_SIZE]  __attribute__((aligned(0x40)));
struct mtd_partition *mtdparts;
static  UINT32  partnum = 0;
int maxSize=0;

static struct semaphore dma_complete;
/* handle interrupt waiting 
  */
static struct completion comp;


#ifdef BD_NOR_ENABLE
//#define CONFIG_MTD_MYSELF  1
#if CONFIG_MTD_MYSELF
extern struct mtd_partition _partition[64];
extern int mt85xx_fill_partition(void);
extern void mt85xx_part_tbl_init(struct mtd_info *mtd);

//UINT32 partnum=0;
#define NUM_PARTITIONS 	partnum
#else

#ifdef CONFIG_MTD_CMDLINE_PARTS
static const char *part_probes[] = { "cmdlinepart", NULL };

#else
//static const char *part_probes[] = { "RedBoot", NULL };
static struct mtd_partition _partition[] = {
	{
		.name = "Bootloader",
		.offset =	0,
		.size = 0x30000,
	},
	{
		.name = "Config",
		.offset = (0x30000),
		.size       = 0x10000,
	},
	{
		.name = "Factory",
		.offset =	(0x40000),
		.size = 0x10000,
	},
	{
		.name = "Kernel",
		.offset = (0x50000),
		.size       = 0xC00000,
	},
	{
		.name = "test",
		.offset = (0xC50000),
		.size       = 0x50000,
	},

    };

#define NUM_PARTITIONS 		ARRAY_SIZE(_partition)
#endif
#endif
#endif

#endif // !defined(__UBOOT_NOR__)

struct mt53xx_nor
{
  //struct mutex lock;
  //struct mtd_info mtd;

  /* flash information struct obejct pointer
     */
  SFLASHHW_VENDOR_T *flash_info;

  /* flash number
     * total capacity
     */
  u32 total_sz;
  u8 flash_num;
  
  /* current accessed flash index
     * current command
     */
  u8 cur_flash_index;
  u8 cur_cmd_val;

  /* isr enable or not
     * dma read enable or not
     */
  u8 host_act_attr;

  /* current sector index
     */

  u32 cur_sector_index;
  u32 cur_addr_offset;
  
};

static struct mt53xx_nor *mt53xx_nor;

static SFLASHHW_VENDOR_T _aVendorFlash[] =
{
    { 0x01, 0x02, 0x12, 0x0,  0x80000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "Spansion(S25FL004A)" },
    { 0x01, 0x02, 0x13, 0x0, 0x100000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "Spansion(S25FL08A)" },
    { 0x01, 0x02, 0x14, 0x0, 0x200000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "Spansion(S25FL016A)" },
    { 0x01, 0x02, 0x15, 0x0, 0x400000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "Spansion(S25FL032A)" },
    { 0x01, 0x02, 0x16, 0x0, 0x800000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "Spansion(S25FL064A)" },
    { 0x01, 0x20, 0x18, 0x0, 0x1000000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "Spansion(S25FL128P)" },
	{ 0x01, 0x40, 0x15, 0x0, 0x200000,	0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "Spansion(S25FL216K)" },
    { 0x01, 0x02, 0x19, 0x0, 0x1000000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "Spansion(S25FL256S)" },

    { 0xC2, 0x20, 0x13, 0x0,  0x80000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "MXIC(25L400)" },
    { 0xC2, 0x20, 0x14, 0x0, 0x100000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "MXIC(25L800)" },
    { 0xC2, 0x20, 0x15, 0x0, 0x200000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x3B, SFLASH_WRITE_PROTECTION_VAL, "MXIC(25L160)" },
    { 0xC2, 0x24, 0x15, 0x0, 0x200000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x3B, SFLASH_WRITE_PROTECTION_VAL, "MXIC(25L1635D)" },
    { 0xC2, 0x20, 0x16, 0x0, 0x400000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x3B, SFLASH_WRITE_PROTECTION_VAL, "MXIC(25L320)" },
    { 0xC2, 0x20, 0x17, 0x0, 0x800000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x3B, SFLASH_WRITE_PROTECTION_VAL, "MXIC(25L640)" },
    { 0xC2, 0x20, 0x18, 0x0, 0x1000000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0xBB, SFLASH_WRITE_PROTECTION_VAL, "MXIC(25L128)" },
    { 0xC2, 0x5E, 0x16, 0x0, 0x400000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "MXIC(25L3235D)" },
    { 0xC2, 0x20, 0x19, 0x0, 0x2000000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0xBB, SFLASH_WRITE_PROTECTION_VAL, "MXIC(25L256)" },
    { 0xC2, 0x20, 0x1a, 0x0, 0x4000000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0xBB, SFLASH_WRITE_PROTECTION_VAL, "MXIC(25L512)" },

	{ 0xC8, 0x40, 0x17, 0x0, 0x800000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x3B, SFLASH_WRITE_PROTECTION_VAL, "GD(GD25QBSIG)" },

    { 0x20, 0x20, 0x14, 0x0, 0x100000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "ST(M25P80)" },
    { 0x20, 0x20, 0x15, 0x0, 0x200000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "ST(M25P16)" },
    { 0x20, 0x20, 0x16, 0x0, 0x400000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "ST(M25P32)" },
    { 0x20, 0x20, 0x17, 0x0, 0x800000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "ST(M25P64)" },
    { 0x20, 0x20, 0x18, 0x0, 0x1000000, 0x40000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "ST(M25P128)" },
    { 0x20, 0x71, 0x16, 0x0, 0x400000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "ST(M25PX32)" },
    { 0x20, 0x71, 0x17, 0x0, 0x800000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "ST(M25PX64)" },
	{ 0x20, 0xBA, 0x17, 0x0, 0x800000,	0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "ST(N25Q064A13ESE40)" },

    { 0xEF, 0x30, 0x13, 0x0,  0x80000,   0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00,SFLASH_WRITE_PROTECTION_VAL, "WINBOND(W25X40)" },
    { 0xEF, 0x30, 0x14, 0x0, 0x100000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "WINBOND(W25X80)" },
    { 0xEF, 0x30, 0x15, 0x0, 0x200000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "WINBOND(W25X16)" },
    { 0xEF, 0x30, 0x16, 0x0, 0x400000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "WINBOND(W25X32)" },
    { 0xEF, 0x30, 0x17, 0x0, 0x800000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "WINBOND(W25X64)" },

	{ 0xEF, 0x40, 0x15, 0x0, 0x200000,	0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0xBB, SFLASH_WRITE_PROTECTION_VAL, "WINBOND(W25Q16FV)" },
	{ 0xEF, 0x40, 0x16, 0x0, 0x400000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "WINBOND(W25Q32BV)" },
    { 0xEF, 0x40, 0x17, 0x0, 0x800000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0xBB, SFLASH_WRITE_PROTECTION_VAL, "WINBOND(W25Q64FV)" },
    { 0xEF, 0x40, 0x18, 0x0, 0x1000000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00,SFLASH_WRITE_PROTECTION_VAL, "WINBOND(W25Q128BV)" },
#if 0 // sector size is not all 64KB, not support!!
    { 0x1C, 0x20, 0x15, 0x0, 0x200000,  0x1000,  60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "EON(EN25B16)" },
    { 0x1C, 0x20, 0x16, 0x0, 0x400000,  0x1000,  60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "EON(EN25B32)" },
    { 0x7F, 0x37, 0x20, 0x0, 0x200000,  0x1000,  60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "AMIC(A25L16P)"},
    { 0xBF, 0x25, 0x41, 0x0, 0x200000,  0x1000,  60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0xAD, 0xAD, 0x00, SFLASH_WRITE_PROTECTION_VAL, "SST(25VF016B)" },
    { 0x8C, 0x20, 0x15, 0x0, 0x200000,  0x1000,  60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0xAD, 0xAD, 0x00, SFLASH_WRITE_PROTECTION_VAL, "ESMT(F25L016A)" },
#endif

#if (defined(CC_MT5360B) || defined(CC_MT5387) ||defined(CC_MT5363) ||defined(ENABLE_AAIWRITE))
    { 0xBF, 0x43, 0x10, 0x0,  0x40000,  0x8000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0x52, 0x60, 0x02, 0xAF, 0x00,  SFLASH_WRITE_PROTECTION_VAL,"SST(SST25VF020)" },
    { 0xBF, 0x25, 0x8D, 0x0,  0x80000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0xAD, 0x00,  SFLASH_WRITE_PROTECTION_VAL,"SST(SST25VF040B)" },
    { 0xBF, 0x25, 0x8E, 0x0, 0x100000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0xAD, 0x00,  SFLASH_WRITE_PROTECTION_VAL,"SST(SST25VF080B)" },
    { 0xBF, 0x25, 0x41, 0x0, 0x200000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0xAD, 0x00,  SFLASH_WRITE_PROTECTION_VAL,"SST(SST25VF016B)" },
    { 0xBF, 0x25, 0x4A, 0x0, 0x400000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0xAD, 0x00,  SFLASH_WRITE_PROTECTION_VAL,"SST(SST25VF032B)" },
    { 0xBF, 0x25, 0x4B, 0x0, 0x800000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0xAD, 0x00,  SFLASH_WRITE_PROTECTION_VAL,"SST(SST25VF064C)" },
#else
    { 0xBF, 0x43, 0x10, 0x1,  0x40000,  0x8000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0x52, 0x60, 0x02, 0xAF, 0x00,  SFLASH_WRITE_PROTECTION_VAL,"SST(SST25VF020)" },
    { 0xBF, 0x25, 0x8D, 0x1,  0x80000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0xAD, 0x00,  SFLASH_WRITE_PROTECTION_VAL,"SST(SST25VF040B)" },
    { 0xBF, 0x25, 0x8E, 0x1, 0x100000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0xAD, 0x00,  SFLASH_WRITE_PROTECTION_VAL,"SST(SST25VF080B)" },
    { 0xBF, 0x25, 0x41, 0x1, 0x200000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0xAD, 0x00,  SFLASH_WRITE_PROTECTION_VAL,"SST(SST25VF016B)" },
    { 0xBF, 0x25, 0x4A, 0x1, 0x400000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0xAD, 0x00,  SFLASH_WRITE_PROTECTION_VAL,"SST(SST25VF032B)" },
    { 0xBF, 0x25, 0x4B, 0x1, 0x800000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0xAD, 0x00,  SFLASH_WRITE_PROTECTION_VAL,"SST(SST25VF064C)" },
#endif

    { 0x1F, 0x47, 0x00, 0x0, 0x400000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "ATMEL(AT25DF321)" },
    { 0x1F, 0x48, 0x00, 0x0, 0x800000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "ATMEL(AT25DF641)" },

    { 0x1C, 0x20, 0x13, 0x0,  0x80000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "EON(EN25B40)" },
    { 0x1C, 0x31, 0x14, 0x0, 0x100000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "EON(EN25F80)" },
    { 0x1C, 0x20, 0x15, 0x0, 0x200000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "EON(EN25P16)" },
    { 0x1C, 0x70, 0x15, 0x0, 0x200000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0xBB, SFLASH_WRITE_PROTECTION_VAL, "EON(EN25NEW)" },
    { 0x1C, 0x20, 0x16, 0x0, 0x400000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0xBB, SFLASH_WRITE_PROTECTION_VAL, "EON(EN25P32)" },
    { 0x1C, 0x20, 0x17, 0x0, 0x800000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0xBB, SFLASH_WRITE_PROTECTION_VAL, "EON(EN25P64)" },
    { 0x1C, 0x30, 0x17, 0x0, 0x800000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0xBB, SFLASH_WRITE_PROTECTION_VAL, "EON(EN25Q64)" },
    { 0x1C, 0x30, 0x18, 0x0, 0x1000000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "EON(EN25P128)" },

    { 0x7F, 0x37, 0x20, 0x0, 0x200000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "AMIC(A25L40P)" },
    { 0x37, 0x30, 0x13, 0x0, 0x100000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "AMIC(A25L040)" },
    { 0x37, 0x30, 0x16, 0x0, 0x400000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "AMIC(A25L032)" },

    { 0xFF, 0xFF, 0x10, 0x0,  0x10000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xC7, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "512Kb" },
//    { 0xFF, 0xFF, 0x11, 0x0,  0x20000,  0x8000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "1Mb" },
    { 0xFF, 0xFF, 0x12, 0x0,  0x40000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "2Mb" },
    { 0xFF, 0xFF, 0x13, 0x0,  0x80000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "4Mb" },
    { 0xFF, 0xFF, 0x14, 0x0, 0x100000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "8Mb" },
    { 0xFF, 0xFF, 0x15, 0x0, 0x200000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "16Mb" },
    { 0xFF, 0xFF, 0x16, 0x0, 0x400000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "32Mb" },
    { 0xFF, 0xFF, 0x17, 0x0, 0x800000,  0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "64Mb" },
    { 0xFF, 0xFF, 0x18, 0x0, 0x1000000, 0x10000, 60000, 0x06, 0x04, 0x05, 0x01, 0x03, 0x0B, 0x9F, 0xD8, 0xC7, 0x02, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "128Mb" },

    { 0x00, 0x00, 0x00, 0x0, 0x000000,  0x00000, 0x000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, SFLASH_WRITE_PROTECTION_VAL, "NULL Device" },
};

#if !defined(__UBOOT_NOR__)
static inline struct mt53xx_nor *mtd_to_mt53xx_nor(struct mtd_info *mtd)
{
  return container_of(mtd, struct mt53xx_nor, mtd);
  
}
#endif

#if 0
static void mt53xx_nor_DumpReg(void)
{
  u32 i;
  for(i = 0;i<0x100;i+=4)
  	printf( "%08x ", SFLASH_RREG32(i));

  printf( "--------\n");
  for(i=0;i<0x10;i+=4)
  	printf( "%08x ", (*(volatile u32 *)(0xF000D400+i)));

  printf( "0xF0060F00: %08x ", (*(volatile u32 *)(0xF0060F00)));
  
}
#endif


static void mt53xx_nor_PinMux(void)
{
#ifndef BD_NOR_ENABLE
  u32 val;
  
  // Set to NOR flash
  val = SFLASH_READ32(0xF000D3B0);
  val &= ~0x4;
  SFLASH_WRITE32(0xF000D3B0, val);

  val = SFLASH_READ32(0xF00299A0);
  val &= ~(0x1<<2);
  SFLASH_WRITE32(0xF00299A0, val);
#endif

}

#if !defined(__UBOOT_NOR__)
static void mt53xx_nor_ClrInt()
{
  SFLASH_WREG8(SFLASH_SF_INTRSTUS_REG, SFLASH_RREG8(SFLASH_SF_INTRSTUS_REG));
}

static irqreturn_t mt53xx_nor_HandleIsr(int irq, void *dev_id)
{
  u8 intvect;


  intvect = SFLASH_RREG8(SFLASH_SF_INTRSTUS_REG);

	  

  if(intvect & SFLASH_EN_INT)
  {
    //complete(&comp);
    up(&dma_complete);
  }
  
  SFLASH_WREG8(SFLASH_SF_INTRSTUS_REG, intvect);
  while((SFLASH_RREG8(SFLASH_SF_INTRSTUS_REG)& SFLASH_EN_INT))
  	{
  		printf( "Nor£ºinterrupt not be clear\n");
		SFLASH_WREG8(SFLASH_SF_INTRSTUS_REG, intvect);
  	}
  	
  return IRQ_HANDLED;
}

static void mt53xx_nor_WaitInt(struct mt53xx_nor *mt53xx_nor)
{
  u8 attr = mt53xx_nor->host_act_attr;

  if(attr & SFLASH_USE_ISR)
  {
    //wait_for_completion(&comp);
	down(&dma_complete);
  }
  else
  {
    while(!SFLASH_DMA_IS_COMPLETED);  
  }
}

static void mt53xx_nor_SetIsr(struct mt53xx_nor *mt53xx_nor)
{
  SFLASH_WREG32(SFLASH_SF_INTREN_REG, SFLASH_EN_INT);

  enable_irq(VECTOR_FLASH);    
 //s init_completion(&comp);

}

static u32 mt53xx_nor_RegIsr(struct mt53xx_nor *mt53xx_nor)
{
  mt53xx_nor_ClrInt();

  if (request_irq(VECTOR_FLASH, mt53xx_nor_HandleIsr, IRQF_SHARED, 
  	                                  mt53xx_nor->mtd.name, mt53xx_nor) != 0)	 
  {		  
    printf( "request serial flash IRQ fail!\n");		  
    return -1;	
  }

  disable_irq(VECTOR_FLASH);

  return 0;
}

static u32 mt53xx_nor_CheckFlashIndex(struct mt53xx_nor *mt53xx_nor)
{
  u8 index = mt53xx_nor->cur_flash_index;
  
  if((index < 0) || (index > MAX_FLASHCOUNT - 1))
  {
    printf( "invalid flash index:%d!\n", index);
    return -EINVAL;
  }

  return 0;
  
}
#endif // !defined(__UBOOT_NOR__)

static u32 mt53xx_nor_Offset2Index(struct mt53xx_nor *mt53xx_nor, u32 offset)
{
  u8 index;
  u32 chipLimit = 0, chipStart = 0, chipSectorSz;

  if(offset >= mt53xx_nor->total_sz)
  {
    printf( "invalid offset exceeds max possible address!\b");
	return -EINVAL;
  }

  for(index = 0; index < mt53xx_nor->flash_num; index++)
  {
    chipStart = chipLimit;
    chipLimit += mt53xx_nor->flash_info[index].u4ChipSize;
	chipSectorSz = mt53xx_nor->flash_info[index].u4SecSize;
	
    if(offset < chipLimit)
    {
      mt53xx_nor->cur_flash_index = index;
	  mt53xx_nor->cur_addr_offset = offset - chipStart;
	  mt53xx_nor->cur_sector_index = mt53xx_nor->cur_addr_offset/(chipSectorSz - 1); 
	  break;
    }
  }

  return 0;
  
}

static u8 mt53xx_nor_Region2Info(struct mt53xx_nor *mt53xx_nor, u32 offset, u32 len)
{
  u8 info = 0, flash_cout = mt53xx_nor->flash_num;
  u32 first_chip_sz = mt53xx_nor->flash_info[0].u4ChipSize;

  if(len == mt53xx_nor->total_sz)
  {
    info |= REGION_RANK_TWO_WHOLE_FLASH;
    return info;
  }

  if(offset == 0)
  {
    info |= REGION_RANK_FIRST_FLASH;
	if(len >= first_chip_sz)
	{
      info |= REGION_RANK_FIRST_WHOLE_FLASH;
	  if((len > first_chip_sz) && (flash_cout > 1))
	  {
        info |= REGION_RANK_SECOND_FLASH;
	  }
	}
  }
  else if(offset < first_chip_sz)
  {
    info |= REGION_RANK_FIRST_FLASH;
	if(((offset + len) == mt53xx_nor->total_sz) && (flash_cout > 1))
	{
      info |= (REGION_RANK_SECOND_FLASH | REGION_RANK_SECOND_WHOLE_FLASH);
	}
  }
  else if((offset == first_chip_sz) && (flash_cout > 1))
  {
    info |= REGION_RANK_SECOND_FLASH;
    if(len == mt53xx_nor->total_sz - first_chip_sz)
    {
      info |= REGION_RANK_SECOND_WHOLE_FLASH;
    }
  }
  else if((offset > first_chip_sz) && (flash_cout > 1))
  {
    info |= REGION_RANK_SECOND_FLASH;
  }

  return info;
  
}

static void mt53xx_nor_SetDualRead(struct mt53xx_nor *mt53xx_nor)
{
  u8 attr = mt53xx_nor->host_act_attr;
  u8 dualread = mt53xx_nor->flash_info[0].u1DualREADCmd;
  u32 u4DualReg = SFLASH_RREG32(SFLASH_DUAL_REG);
  

  /* make sure the same dual read command about two sflash
     */
  if((attr & SFLASH_USE_DUAL_READ) &&
  	 (dualread != 0x00))
  {
    SFLASH_WREG8(SFLASH_PRGDATA3_REG, dualread);
    if (dualread == 0xbb)
	    u4DualReg |= 0x3;
    else
	    u4DualReg |= 0x1;
    SFLASH_WREG8(SFLASH_DUAL_REG, u4DualReg & 0xff);
  }
  else
  {
    u4DualReg &= ~0x3;
    SFLASH_WREG8(SFLASH_DUAL_REG, u4DualReg & 0xff);
  }
}

static void mt53xx_nor_DisableDualRead(struct mt53xx_nor *mt53xx_nor)
{
  u8 attr = mt53xx_nor->host_act_attr;
  u8 dualread = mt53xx_nor->flash_info[0].u1DualREADCmd;
  u32 u4DualReg = SFLASH_RREG32(SFLASH_DUAL_REG);
  
  if((attr & SFLASH_USE_DUAL_READ) &&
  	 (dualread != 0x00))
  {
	SFLASH_WREG8(SFLASH_PRGDATA3_REG, 0x0);
	u4DualReg &= ~0x3;
	SFLASH_WREG8(SFLASH_DUAL_REG, u4DualReg & 0xff);

  }
}

static void mt53xx_nor_SendCmd(struct mt53xx_nor *mt53xx_nor)
{
  u8 index = mt53xx_nor->cur_flash_index;
  u8 cmdval = mt53xx_nor->cur_cmd_val;
  
  if((index < 0) || (index > MAX_FLASHCOUNT - 1))
  {
    printf( "invalid flash index:%d!\n", index);
    return ;
  }
  SFLASH_WREG8(SFLASH_CMD_REG, index*0x40 + cmdval);
  
}

static u32 mt53xx_nor_PollingReg(u8 u1RegOffset, u8 u8Compare)
{
  u32 u4Polling;
  u8 u1Reg;

  u4Polling = 0;
  while(1)
  {
    u1Reg = SFLASH_RREG8(u1RegOffset);
    if (0x00 == (u1Reg & u8Compare))
    {
      break;
    }
    u4Polling++;
    if(u4Polling > SFLASH_POLLINGREG_COUNT)
    {
      printf("polling reg%02X using compare val%02X timeout!\n", 
	  	                                 u1RegOffset, u8Compare);
      return -1;
    }
  }
  
  return 0;
  
}

static u32 mt53xx_nor_ExecuteCmd(struct mt53xx_nor *mt53xx_nor)
{
  u8 val = (mt53xx_nor->cur_cmd_val) & 0x1F;

  mt53xx_nor_SendCmd(mt53xx_nor);

  return mt53xx_nor_PollingReg(SFLASH_CMD_REG, val);
  
}

static void mt53xx_nor_SetCfg1(u8 sf_num, u32 first_sf_cap)
{
    u8 u1Reg;

    u1Reg = SFLASH_RREG8(SFLASH_CFG1_REG);
    u1Reg &= (~0x1C);

#if (MAX_FLASHCOUNT > 1)
    if(sf_num > 1)
    {
        switch(first_sf_cap)
        {
        case 0x200000:
            u1Reg |= 0xC;
            break;
        case 0x400000:
            u1Reg |= 0x8;
            break;
        case 0x800000:
            u1Reg |= 0x4;
            break;
		case 0x1000000:
			u1Reg |= 0x14;
            break;
        default:
            u1Reg |= 0x4;
            break;
        }
    }
#endif
	
	SFLASH_WREG8(SFLASH_CFG1_REG, u1Reg);

	return;
	
}

static u32 mt53xx_nor_GetFlashInfo(SFLASHHW_VENDOR_T *tmp_flash_info)
{
  u32 i = 0;

  while(_aVendorFlash[i].u1MenuID != 0x00)
  {
    if((_aVendorFlash[i].u1MenuID == tmp_flash_info->u1MenuID) &&
       (_aVendorFlash[i].u1DevID1 == tmp_flash_info->u1DevID1) &&
       (_aVendorFlash[i].u1DevID2 == tmp_flash_info->u1DevID2))
    {	
      memcpy((VOID*)tmp_flash_info, (VOID*)&(_aVendorFlash[i]), sizeof(SFLASHHW_VENDOR_T));
      printf( "Setup flash information successful, support list index: %d\n", i);

      return 0;
    }
	
    i++;
  }

  return -1;
  
}

static u32 mt53xx_nor_GetID(struct mt53xx_nor *mt53xx_nor)
{
  u32 u4Index, u4Ret;
  u8 cmdval;
  SFLASHHW_VENDOR_T *tmp_flash_info;

  mt53xx_nor->flash_num = 0;
  mt53xx_nor->total_sz = 0;
  
  for(u4Index = 0;u4Index < MAX_FLASHCOUNT;u4Index++)
  {
    /* we can not use this sentence firstly because of empty flash_info:
        * cmdval = mt53xx_nor->flash_info[u4Index].u1READIDCmd;
        * so it is necessary to consider read id command as constant.
        */
    cmdval = 0x9F;
    SFLASH_WREG8(SFLASH_PRGDATA5_REG, cmdval);
    SFLASH_WREG8(SFLASH_PRGDATA4_REG, 0x00);
    SFLASH_WREG8(SFLASH_PRGDATA3_REG, 0x00);
    SFLASH_WREG8(SFLASH_PRGDATA2_REG, 0x00);
    SFLASH_WREG8(SFLASH_CNT_REG, 32);

	
    /* pointer to the current flash info struct object
        * Save the current flash index
        */
    tmp_flash_info = mt53xx_nor->flash_info + u4Index;
	mt53xx_nor->cur_flash_index = u4Index;
	mt53xx_nor->cur_cmd_val = 0x04;

	if(0 != mt53xx_nor_ExecuteCmd(mt53xx_nor))
	{
      break;
	}

    tmp_flash_info->u1DevID2 = SFLASH_RREG8(SFLASH_SHREG0_REG);
    tmp_flash_info->u1DevID1 = SFLASH_RREG8(SFLASH_SHREG1_REG);	
    tmp_flash_info->u1MenuID = SFLASH_RREG8(SFLASH_SHREG2_REG);
	u4MenuID=tmp_flash_info->u1MenuID;

	mt53xx_nor->cur_cmd_val = 0x00;
    mt53xx_nor_SendCmd(mt53xx_nor);
    printf( "Flash Index: %d, MenuID: %02x, DevID1: %02x, DevID2: %02x\n", 
                                   u4Index, tmp_flash_info->u1MenuID,
                                            tmp_flash_info->u1DevID1,
                                            tmp_flash_info->u1DevID2);

    u4Ret = mt53xx_nor_GetFlashInfo(tmp_flash_info);
	if(0 == u4Ret)
	{
	    mt53xx_nor->flash_num++;
		mt53xx_nor->total_sz += tmp_flash_info->u4ChipSize;
		
#if (MAX_FLASHCOUNT > 1) 
	    if(u4Index == 0)
	    {
	        /* It is necessary when the first flash is identified sucessfully and
	              *   more than one flash is intended to be supported.
	              */
            mt53xx_nor_SetCfg1(2, tmp_flash_info->u4ChipSize);    
	    }
#endif
	}
	else
	{
      break;
	}
	
  }

  if(0 == mt53xx_nor->flash_num)
  {
    printf( "any flash is not found!\n");
	return -1;
  }

  mt53xx_nor_SetCfg1(mt53xx_nor->flash_num, mt53xx_nor->flash_info[0].u4ChipSize); 

  return 0;
  
}

static u32 mt53xx_nor_ReadStatus(struct mt53xx_nor *mt53xx_nor, u8 *status)
{
  if(status == NULL)
  {
    return -1;
  }

  mt53xx_nor->cur_cmd_val = 0x02;
  if(-1 == mt53xx_nor_ExecuteCmd(mt53xx_nor))
  {
    printf( "read status failed!\n");
    return -1;
  }
	
  *status = SFLASH_RREG8(SFLASH_RDSR_REG);

  return 0;
  
}

static u32 mt53xx_nor_WriteStatus(struct mt53xx_nor *mt53xx_nor, u8 status)
{
  SFLASH_WREG8(SFLASH_PRGDATA5_REG, status);
  SFLASH_WREG8(SFLASH_CNT_REG,8);

  mt53xx_nor->cur_cmd_val = 0x20;
  if(-1 == mt53xx_nor_ExecuteCmd(mt53xx_nor))
  {
    printf( "write status failed!\n");
    return -1;
  }
	  
  return 0;

}

static u32 mt53xx_nor_WaitBusy(struct mt53xx_nor *mt53xx_nor, u32 timeout)
{
  u32 count;
  u8 reg;

  count = 0;
  while(1)
  {
    if(mt53xx_nor_ReadStatus(mt53xx_nor, &reg) != 0)
    {
      return -1;
    }

    if(0 == (reg & 0x1))
    {
      break;
    }

    count++;
    if(count > timeout)
    {
      printf( "wait write busy timeout, failed!\n");
      return -1;
    }

    udelay(5);
  }

  return 0;
  
}

static u32 mt53xx_nor_WaitBusySleep(struct mt53xx_nor *mt53xx_nor, u32 timeout)
{
  u32 count;
  u8 reg;

  count = 0;
  while(1)
  {
    if(mt53xx_nor_ReadStatus(mt53xx_nor, &reg) != 0)
    {
      return -1;
    }

    if(0 == (reg & 0x1))
    {
      break;
    }

    count++;
    if(count > timeout)
    {
      printf( "wait write busy timeout, failed!\n");
      return -1;
    }

    //msleep(5);
     mdelay(5);
  }

  return 0;
  
}

static u32 mt53xx_nor_WriteEnable(struct mt53xx_nor *mt53xx_nor)
{
  u8 cur_flash_index, val;

  cur_flash_index = mt53xx_nor->cur_flash_index;
  val = mt53xx_nor->flash_info[cur_flash_index].u1WRENCmd;

  SFLASH_WREG8(SFLASH_PRGDATA5_REG, val);
  SFLASH_WREG8(SFLASH_CNT_REG,8);

  mt53xx_nor->cur_cmd_val = 0x04;
  if(-1 == mt53xx_nor_ExecuteCmd(mt53xx_nor))
  {
    printf( "write enable failed!\n");
    return -1;
  }
	
  return 0;

}

static u32 mt53xx_nor_WriteBufferEnable(struct mt53xx_nor *mt53xx_nor)
{
  u32 polling;
  u8 reg;
  
  SFLASH_WREG8(SFLASH_CFG2_REG, 0x0D);

  polling = 0;
  while(1)
  {
    reg = SFLASH_RREG8(SFLASH_CFG2_REG);
    if (0x01 == (reg & 0x01))
    {
      break;
    }

    polling++;
    if(polling > SFLASH_POLLINGREG_COUNT)
    {
      return -1;
    }
  }

  return 0;
}


static u32 mt53xx_nor_WriteBufferDisable(struct mt53xx_nor *mt53xx_nor)
{
  u32 polling;
  u8 reg;

  SFLASH_WREG8(SFLASH_CFG2_REG, 0xC);
  
  polling = 0;
  while(1)    // Wait for finish write buffer
  {
    reg = SFLASH_RREG8(SFLASH_CFG2_REG);
    if (0x0C == (reg & 0xF))
    {
      break;
    }

    polling++;
    if(polling > SFLASH_POLLINGREG_COUNT)
    {
      return -1;
    }
  }

  return 0;
  
}


static int mt53xx_nor_WriteProtect(struct mt53xx_nor *mt53xx_nor, u32 fgEn)
{  
  u8 val = fgEn?0x3C:0x00;
  
  if(0 != mt53xx_nor_WaitBusy(mt53xx_nor, SFLASH_WRITEBUSY_TIMEOUT))
  {
    printf( "wait write busy #1 failed in write protect process!\n");
    return -1;
  }

  if(0 != mt53xx_nor_WriteEnable(mt53xx_nor))
  {
    printf( "write enable failed in write protect process!\n");
    return -1;
  }

  if (0 != mt53xx_nor_WriteStatus(mt53xx_nor, val))
  {
    printf( "write status failed in write protect process!\n");
    return -1;
  }

  if(mt53xx_nor_WaitBusy(mt53xx_nor, SFLASH_WRITEBUSY_TIMEOUT) != 0)
  {
    printf( "wait write busy #2 failed in write protect process!\n");
    return -1;
  }

  return 0;
  
}

static u32 mt53xx_nor_WriteProtectAllChips(struct mt53xx_nor *mt53xx_nor, u32 fgEn)
{ 
  u8 index, cur_index = mt53xx_nor->cur_flash_index;

  for(index = 0; index < mt53xx_nor->flash_num; index++)
  {
    mt53xx_nor->cur_flash_index = index;
    if(0 != mt53xx_nor_WriteProtect(mt53xx_nor, fgEn))
    {
      printf( "Flash #%d: write protect disable failed!\n", index);
	  mt53xx_nor->cur_flash_index = cur_index;
	  return -1;
    }
	else
		printf("Flash #%d: write protect disable successfully!\n", index);
  }

  mt53xx_nor->cur_flash_index = cur_index;
  
  return 0;

}

static u32 mt53xx_nor_EraseChip(struct mt53xx_nor *mt53xx_nor)
{
  u8 cur_flash_index, val;
  
  cur_flash_index = mt53xx_nor->cur_flash_index;
  val = mt53xx_nor->flash_info[cur_flash_index].u1CHIPERASECmd;
  printf( "%s \n",__FUNCTION__);

  if(mt53xx_nor_WaitBusySleep(mt53xx_nor, SFLASH_WRITEBUSY_TIMEOUT) != 0)
  {
    printf( "wait write busy #1 failed in erase chip process!\n");
    return -1;
  }
	
  if(mt53xx_nor_WriteEnable(mt53xx_nor) != 0)
  {
    printf( "write enable failed in erase chip process!\n");
    return -1;
  }
	
  /* Execute sector erase command
     */
  SFLASH_WREG8(SFLASH_PRGDATA5_REG, val);
  SFLASH_WREG8(SFLASH_CNT_REG, 8);

  mt53xx_nor->cur_cmd_val = 0x04;
  if(-1 == mt53xx_nor_ExecuteCmd(mt53xx_nor))
  {
    printf( "erase chip#%d failed!\n", cur_flash_index);
    return -1;
  }

  if(mt53xx_nor_WaitBusySleep(mt53xx_nor, SFLASH_CHIPERASE_TIMEOUT) != 0)
  {
    printf( "wait write busy #2 failed in erase chip process!\n");
    return -1;
  }

  mt53xx_nor->cur_cmd_val = 0x00;
  mt53xx_nor_SendCmd(mt53xx_nor);
	
  return 0;

}

#if 0
static u32 mt53xx_nor_EraseAllChips(struct mt53xx_nor *mt53xx_nor)
{
  u8 index, cur_index = mt53xx_nor->cur_flash_index;
	printf( "%s \n",__FUNCTION__);
  for(index = 0; index < mt53xx_nor->flash_num; index++)
  {
    mt53xx_nor->cur_flash_index = index;
    if(0 != mt53xx_nor_EraseChip(mt53xx_nor))
    {
      printf( "Flash #%d: erase chip failed!\n", index);
      mt53xx_nor->cur_flash_index = cur_index;
      return -1;
    }
  }

  mt53xx_nor->cur_flash_index = cur_index;	
  
  return 0;

}
#endif

static u32 mt53xx_nor_EraseSector(struct mt53xx_nor *mt53xx_nor, u32 offset)
{
  u32 addr;
  u8 cmdval, index;

  if(0 != mt53xx_nor_WaitBusySleep(mt53xx_nor, SFLASH_WRITEBUSY_TIMEOUT))
  {
    printf( "wait write busy #1 failed in erase sector process!\n");
    return -1;
  }

  if(0 != mt53xx_nor_WriteEnable(mt53xx_nor))
  {
    printf( "write enable failed in erase sector process!\n");
    return -1;
  }

  if(0 != mt53xx_nor_Offset2Index(mt53xx_nor, offset))
  {
    printf( "offset parse failed in erase sector process!\n");
	return -1;
  }

  /* Execute sector erase command
     */
  index = mt53xx_nor->cur_flash_index;
  addr = mt53xx_nor->cur_addr_offset;
  cmdval = mt53xx_nor->flash_info[index].u1SECERASECmd;
  SFLASH_WREG8(SFLASH_PRGDATA5_REG, cmdval);
#ifdef BD_NOR_ENABLE
  if (mt53xx_nor->flash_info[index].u4ChipSize >= 0x2000000)
  {
  	SFLASH_WREG8(SFLASH_PRGDATA4_REG, HiByte(HiWord(addr))); // Write
  	SFLASH_WREG8(SFLASH_PRGDATA3_REG, LoByte(HiWord(addr))); // Write
  	SFLASH_WREG8(SFLASH_PRGDATA2_REG, HiByte(LoWord(addr))); // Write
  	SFLASH_WREG8(SFLASH_PRGDATA1_REG, LoByte(LoWord(addr))); // Write
  	SFLASH_WREG8(SFLASH_CNT_REG, 40);       // Write SF Bit Count
  }
  else
  {
  	SFLASH_WREG8(SFLASH_PRGDATA4_REG, LoByte(HiWord(addr)));
  	SFLASH_WREG8(SFLASH_PRGDATA3_REG, HiByte(LoWord(addr)));
  	SFLASH_WREG8(SFLASH_PRGDATA2_REG, LoByte(LoWord(addr)));
  	SFLASH_WREG8(SFLASH_CNT_REG, 32);
  }
#else
  SFLASH_WREG8(SFLASH_PRGDATA4_REG, LoByte(HiWord(addr)));
  SFLASH_WREG8(SFLASH_PRGDATA3_REG, HiByte(LoWord(addr)));
  SFLASH_WREG8(SFLASH_PRGDATA2_REG, LoByte(LoWord(addr)));
  SFLASH_WREG8(SFLASH_CNT_REG, 32);
#endif
  mt53xx_nor->cur_cmd_val = 0x04;
  if(0 != mt53xx_nor_ExecuteCmd(mt53xx_nor))
  {
    printf( "erase sector(offset:%08x flash#%d, sector index:%d, offset:%08x) failed!\n", 
		                                                      offset, index, 
		                                                      mt53xx_nor->cur_sector_index, 
		                                                      mt53xx_nor->cur_addr_offset);
    return -1;
  }

  if(0 != mt53xx_nor_WaitBusySleep(mt53xx_nor, SFLASH_ERASESECTOR_TIMEOUT))
  {
    printf( "wait write busy #2 failed in erase sector process!\n");
    return -1;
  }

  mt53xx_nor->cur_cmd_val = 0x00;
  mt53xx_nor_SendCmd(mt53xx_nor);
	
  return 0;

}

#if defined(__UBOOT_NOR__)
u32 mtk_nor_erase(u32 addr, u32 len)
{
  u32 first_chip_sz, total_sz, rem, ret=0;
  u8 info;
#else
static u32 mt53xx_nor_erase(struct mtd_info *mtd, struct erase_info *instr)
{
  struct mt53xx_nor *mt53xx_nor = mtd_to_mt53xx_nor(mtd);
  u32 addr, len, rem, first_chip_sz, total_sz, ret = 0;
  u8 info;
  
  addr = instr->addr;
  len = instr->len;
#endif
  first_chip_sz = mt53xx_nor->flash_info[0].u4ChipSize;
  total_sz = mt53xx_nor->total_sz;

//#if NOR_DEBUG
  printf( "%s addr=0x%x,len=0x%x\n",__FUNCTION__,addr,len);
//#endif

  //if the address of nor < 0x30000, the system will be assert
  if((addr < 0x30000)&&(enable_protect_ro==1))
  {
	 printf("Error: nor erase address at 0x%x < 0x30000",addr);
	  //while(1) {};
	 BUG_ON(1);
	 return -EINVAL;
  }

  if((addr + len) > mt53xx_nor->total_sz)
  {
  	printf( "Error: nor erase address at 0x%x ",addr + len);
  	return -EINVAL;
  }
  
  rem = len - (len & (~(mt53xx_nor->flash_info[0].u4SecSize - 1))); // Roger modify
  if(rem)
  {
  	return -EINVAL;
  }

  //mutex_lock(&mt53xx_nor->lock);

  /*
     * pimux switch firstly 
     */
  mt53xx_nor_PinMux(); 

  
  /* get rank-flash info of region indicated by addr and len
     */
  info = mt53xx_nor_Region2Info(mt53xx_nor, addr, len);

  /*
     * disable write protect of all related chips 
     */
  if(info & REGION_RANK_FIRST_FLASH)
  {
    mt53xx_nor->cur_flash_index = 0;
	if(0 != mt53xx_nor_WriteProtect(mt53xx_nor, 0))
    {
      printf( "disable write protect of flash#0 failed!\n");
	  ret = -1;
	  goto done;
    }
  }
  if(info & REGION_RANK_SECOND_FLASH)
  {
    mt53xx_nor->cur_flash_index = 1;
	if(0 != mt53xx_nor_WriteProtect(mt53xx_nor, 0))
    {
      printf( "disable write protect of flash#1 failed!\n");
	  ret = -1;
	  goto done;
    }
  }  
 
  //printf( "erase addr: %08x~%08x\n", addr, len);
  
  /* erase the first whole flash if necessary
     */
  if(info & REGION_RANK_FIRST_WHOLE_FLASH)
  {
    mt53xx_nor->cur_flash_index = 0;
    if(mt53xx_nor_EraseChip(mt53xx_nor))
    {
      printf( "erase whole flash#0 failed!\n");
	  ret = -EIO; 
	  goto done;
    }
	else
	printf( "erase whole flash#0 successful!\n");

    addr += first_chip_sz;
	len -= first_chip_sz;
  }

  /* erase involved region of the first flash if necessary
     */
  mt53xx_nor->cur_flash_index = 0;
  while((len)&&(addr < first_chip_sz))
  {
    if(mt53xx_nor_EraseSector(mt53xx_nor, addr))
    {
      printf( "erase part of flash#0 failed!\n");
      ret = -EIO; 
	  goto done;
    }
	else
		printf( "erase part flash#0 successful!\n");

    addr += mt53xx_nor->flash_info[0].u4SecSize;
    len -= mt53xx_nor->flash_info[0].u4SecSize;
  }

  /* erase the first whole flash if necessary
     */
  if(info & REGION_RANK_SECOND_WHOLE_FLASH)
  {
    mt53xx_nor->cur_flash_index = 1;
    if(mt53xx_nor_EraseChip(mt53xx_nor))
    {
      printf( "erase whole flash#1 failed!\n");
	  ret = -EIO; 
	  goto done;  
    }
	else
		printf( "erase whole flash#1 successful!\n");

    addr += first_chip_sz;
	len -= first_chip_sz;
  }

  /* erase involved region of the first flash if necessary
     */
  mt53xx_nor->cur_flash_index = 1;
  while((len)&&(addr < total_sz))
  {
    if(mt53xx_nor_EraseSector(mt53xx_nor, addr))
    {
      printf( "erase part of flash#1 failed!\n");
      ret = -EIO; 
	  goto done;
    }
	else
	printf( "erase part flash#1 successful!\n");

    addr += mt53xx_nor->flash_info[0].u4SecSize;
    len -= mt53xx_nor->flash_info[0].u4SecSize;
  }

done:
	
  //mutex_unlock(&mt53xx_nor->lock);
#if !defined(__UBOOT_NOR__)
  if(ret)
  {
    instr->state = MTD_ERASE_FAILED;
  }
  else
  {
    instr->state = MTD_ERASE_DONE;
    mtd_erase_callback(instr);//for QQ test
  }
#endif

  return ret;
}

static u32 mt53xx_nor_ReadPIO(struct mt53xx_nor *mt53xx_nor, u32 addr, u32 len, u32 *retlen, u8 *buf)
{
  u32 i = 0, ret = 0; 
	
  /* why addr3 should be set?
     * if 0xFFFFFF and auto-increment address are survived in the previous read operation,
     * SFLASH_RADR3_REG will be set as 1.
     * if that, 0x1a2800 will mean that 0x9a2800 in the current read operation
     * so, it is better to fill SFLASH_RADR3_REG by high byte in high word
     */
  SFLASH_WREG8(SFLASH_RADR3_REG, HiByte(HiWord(addr)));
  SFLASH_WREG8(SFLASH_RADR2_REG, LoByte(HiWord(addr)));
  SFLASH_WREG8(SFLASH_RADR1_REG, HiByte(LoWord(addr)));
  SFLASH_WREG8(SFLASH_RADR0_REG, LoByte(LoWord(addr))); 
	
  for (i=0; i<len; i++, (*retlen)++)
  {  
    mt53xx_nor->cur_cmd_val = 0x81;
    if(0 != mt53xx_nor_ExecuteCmd(mt53xx_nor))
    {
      printf( "read flash failed!\n");
      ret = -1;
      goto done;
    }
	
    buf[i] = SFLASH_RREG8(SFLASH_RDATA_REG); // Get data
  } 
	
  mt53xx_nor->cur_cmd_val = 0x00;
  mt53xx_nor_SendCmd(mt53xx_nor);

done:

	return ret;

}

#if 0
static void mt53xx_nor_detect_pattern(u8 *buf,u32 len)
{
	int i;
	u32 pattern_match=0;
	u32 delay_count=0;
	u32 phyaddr;

reMatch:
		for(i=len-1;i>0;i--)
			{
				if(pattern_match>4)
					break;
				
				if(buf[i]==0xAA)
					pattern_match++;
				else
					pattern_match=0;
			}
		if((pattern_match>4)&&(delay_count<4))
			{
				printf( "Nor:Match pattern %d,addr 0x%x, len %d \n",pattern_match,buf,len);
			pattern_match=0;
			delay_count++;
			if(virt_addr_valid((u32)buf ))
				{
				 phyaddr = dma_map_single(NULL, buf, len, DMA_FROM_DEVICE);	
				 dma_unmap_single(NULL, phyaddr, len, DMA_FROM_DEVICE);
				}
			
			msleep(5);
			goto reMatch;
			}
		/*
		if(delay_count>=4)
			{
				printf("print error data:addr 0x%x len %d\n",buf,len);
				for(i=0;i<len;i++)
				{
					
					printf( "%02x ",buf[i]);
				
				}
				printf("\n");
			}
			*/
		
}

static void mt53xx_nor_cal_checksum(u32 addr,u8 *buf,u32 len)
{
	u32 i,count=0;
	u8 *nor_baseAddress;
	u8 nor_data;

	nor_baseAddress=0xf8000000+addr;
	
	for(i=0;i<len;i++)
		{
			nor_data=nor_baseAddress[i];
			if(buf[i]!=nor_data)
				{			
					buf[i]=nor_data;
					count++;
				}
		}
	if(count)
		printf( "Nor: recover %d data\n",len);
}
static u32 mt53xx_nor_cmp_f8(u32 addr,u8 *buf,u32 len)
{
	u32 i,count=0;
	u8 *nor_baseAddress;
	u8 nor_data;
	nor_baseAddress=0xf8000000+addr;
	for(i=0;i<len;i++)
		{
			nor_data=nor_baseAddress[i];
			if(buf[i]!=nor_data)
				{			
					count++;
				}
		}
	if(count)
		{
		printf( "Nor: error data counts: %d \n",len);
		return 0;
		}
	else
		return 1;
}
#endif

#if !defined(__UBOOT_NOR__)
static u32 mt53xx_nor_ReadDMA(struct mt53xx_nor *mt53xx_nor, u32 addr, u32 len, u32 *retlen, u8 *buf)
{ 
  u32 phyaddr; 
  u32 ret;
  u8 intvect;
  u32 dmareg;
#ifdef BD_NOR_ENABLE
  dmareg = 0xF1014718;
#else
  dmareg = 0xF00085A0;
#endif

  phyaddr = dma_map_single(NULL, buf, len, DMA_FROM_DEVICE);	



  do {
	  ret = SFLASH_READ32(dmareg);
	  if (ret & 1)
	  {
		  printf( "Nor: Dma not finished befor new trigger\n");
		  
	  }
	} while (SFLASH_READ32(dmareg) & 0x1);

 while(! down_trylock(&dma_complete))
 	{
 		printf( "Nor: Get more UP signal from ISR \n");
 	}
  // reset DMA eginee
  SFLASH_DMA_RESET;

  // DMA source address in flash and des address in DRAM in DRAM

	SFLASH_DMA_ADDR_SETTING(addr, phyaddr, len);


  	

  // Trigger DMA eginee
  SFLASH_DMA_START;

  // wait DMA operation is completed
  mt53xx_nor_WaitInt(mt53xx_nor);
   do {
    ret = SFLASH_READ32(dmareg);
    if (ret & 1)
    {
    	printf( "Nor: Dma not finished after trigger\n");
		
    }
  } while (SFLASH_READ32(dmareg) & 0x1);
   
  dma_unmap_single(NULL, phyaddr, len, DMA_FROM_DEVICE);	

  
  // ret value add
  (*retlen) += len;

  return 0;

}
#endif // !defined(__UBOOT_NOR__)

static u32 mt53xx_nor_Read(struct mt53xx_nor *mt53xx_nor, u32 addr, u32 len, u32 *retlen, u8 *buf)
{
  u32 ret = 0, dram_addr_off = 0, len_align = 0, i;
  u8 info, attr;


  attr = mt53xx_nor->host_act_attr;

  mt53xx_nor_PinMux();

  mt53xx_nor_SetDualRead(mt53xx_nor);
	
  /* get rank-flash info of region indicated by addr and len
     */
  info = mt53xx_nor_Region2Info(mt53xx_nor, addr, len);
	
  /* Wait till previous write/erase is done. 
     */
  if(info & REGION_RANK_FIRST_FLASH)
  {
    mt53xx_nor->cur_flash_index = 0;
    if(0 != mt53xx_nor_WaitBusy(mt53xx_nor, SFLASH_WRITEBUSY_TIMEOUT))
    {
      printf( "Read Wait Busy of flash#0 failed!\n");
      ret = -1;
      goto done;
    }
  }
  if(info & REGION_RANK_SECOND_FLASH)
  {
    mt53xx_nor->cur_flash_index = 1;
    if(0 != mt53xx_nor_WaitBusy(mt53xx_nor, SFLASH_WRITEBUSY_TIMEOUT))
    {
      printf( " Read Wait Busy  of flash#1 failed!\n");
      ret = -1;
      goto done;
    }
  }  
	
  /* read operation is going on
     */
  mt53xx_nor->cur_flash_index = 0;
	
  /* Disable pretch write buffer  
     */
  if(0 != mt53xx_nor_WriteBufferDisable(mt53xx_nor))
  {
    printf( "disable write buffer in read process failed!\n");
    ret = -1;
    goto done;    
  }

#if !defined(__UBOOT_NOR__)
  /* For only pio mode use, we don't care whether dma addr and len is 16byte align
     */
  if(attr & SFLASH_USE_DMA)
  {	  
	  if(virt_addr_valid((u32)buf ))  //buf address can be use for dma
	  {
		  dram_addr_off = (u32)buf - (((u32)buf)&(~0x0F));
		  
		  if(dram_addr_off)
		  {
			  //printf(".......entry mt53xx_nor_Read rdram_addr_off is nor zero!\n");
			  len_align = 0x10 - dram_addr_off;
			  
			  if(len < 0x10 + len_align)
			  {
				  len_align = len;
			  }
			  	
			  if(0 != mt53xx_nor_ReadPIO(mt53xx_nor, addr, len_align, retlen, buf))
			  {
				  printf( "read in pio mode #1 failed!\n");
				  ret = -1;
				  goto done;  
			  } 
		  
			  addr += len_align;
			  len  -= len_align;
			  buf  += len_align;
		  } 	  
	  
			
		  if(len >= 0x10)
		  {
			  len_align = (len&(~0xF));  
			  
			  if(0 != mt53xx_nor_ReadDMA(mt53xx_nor, addr, len_align, retlen, buf))
			  {
				  printf( "read in DMA mode failed!\n");
				  ret = -1;
				  goto done;  
			  }
			  
			  addr += len_align;
			  len  -= len_align;
			  buf  += len_align;
	  
		  
		  }
	  
	  }
	  else		  //buf address can't be use for dma, we must use _pu1NorDataBuf for dma read
	  {  
		  if(len >= 0x10)
		  {
			  len_align = (len&(~0xF));   //len_align is 16 aligns
			  
			  //printf("..............entry mt53xx_nor_Read len =0x%x len_align =0x%x buf =0x%x !\n",len, len_align, buf);
		  
			  len  -= len_align;
			  //DMA read max size is 8k
			  while(len_align > SFLASH_MAX_DMA_SIZE)	  
			  {
				  if(0 != mt53xx_nor_ReadDMA(mt53xx_nor, addr, SFLASH_MAX_DMA_SIZE, retlen, _pu1NorDataBuf))
				  {
					printf( "read in DMA mode failed!\n");
					ret = -1;
					goto done;	
				  }
				  memcpy(buf, _pu1NorDataBuf, SFLASH_MAX_DMA_SIZE);
				  addr += SFLASH_MAX_DMA_SIZE;
				  buf += SFLASH_MAX_DMA_SIZE;
				  len_align -= SFLASH_MAX_DMA_SIZE;
			  }
			  if(len_align > 0)   // 16 < len_align < SFLASH_MAX_DMA_SIZE
			  {
				  //printf( "len_align = 0x%x !\n",len_align);
				  if(0 != mt53xx_nor_ReadDMA(mt53xx_nor, addr, len_align, retlen, _pu1NorDataBuf))
				  {
					  printf( "read in DMA mode failed!\n");
					  ret = -1;
					  goto done;  
				  }
				  memcpy(buf, _pu1NorDataBuf, len_align);
				  addr += len_align;
				  buf += len_align;
			  }
		  }
	  }

  }
#endif // !defined(__UBOOT_NOR__)
 
  if(len > 0)
  {
    len_align = len;
	 
    if(0 != mt53xx_nor_ReadPIO(mt53xx_nor, addr, len_align, retlen, buf))
    {
      printf( "read in pio mode #2 failed!\n");
      ret = -1;
      goto done;	  
    }   
	  
    addr += len_align;
    len  -= len_align;
    buf += len_align;
  } 

  mt53xx_nor_DisableDualRead(mt53xx_nor);
done:

  return ret;

}

/*[20130314]Copy nor flash data to buffer for checking data write error or memory overwrite.
It affects the nor flash performance so remove it in release version.*/
/*
typedef struct __NorReadRecord
{
    u32 nor_magic;
    u32 nor_from;
    u32 nor_len;
    u32 nor_retlen;
    u32 nor_f00085a0;
    u32 nor_f00085a4;
    u8 *nor_buf;
    u32 nor_magic2;
    u8  nor_data[1024*8];
} NorReadRecord_T;

#define NOR_READ_ARRAY_SIZE         32
static NorReadRecord_T arNorReadArray[NOR_READ_ARRAY_SIZE] __attribute__ ((aligned (16)));
static u32 nor_index = 0;
static u32 nor_init = 0;
*/
#ifndef BD_NOR_ENABLE
#define MULTI_READ_DMA
#endif
#ifdef MULTI_READ_DMA
static u8 _pu1NorTemp1Buf[SFLASH_MAX_DMA_SIZE+64]  __attribute__((aligned(0x20)));
//static u8 _pu1NorTemp2Buf[SFLASH_MAX_DMA_SIZE+64]  __attribute__((aligned(0x20)));
#endif
#if defined(__UBOOT_NOR__)
u32 mtk_nor_read(u32 from, u32 len, size_t *retlen, u_char *buf)
{
#else
static u32 mt53xx_nor_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
  struct mt53xx_nor *mt53xx_nor = mtd_to_mt53xx_nor(mtd);
#endif
  u32 ret = 0, i;
   u32 phyaddr; 
   u8 *readBuf;
   u32 readLen;
   //struct timespec tv,tv1;
   //u32 readtime = 0;
   #ifdef MULTI_READ_DMA
   u32 multiReadEnable=0;
   u32 multiReadCounts=1;
   if(len<SFLASH_MAX_DMA_SIZE )
   		multiReadEnable=1;
   #endif

  if (!len)
  {
    return 0;
  }

  //if (from + len > mt53xx_nor->mtd.size)
  if (from + len > mt53xx_nor->total_sz)
  {
    return -EINVAL;
  }

  /* Byte count starts at zero. */
  *retlen = 0;


  //mutex_lock(&mt53xx_nor->lock);
  
 //make sure the 0xFF had been write to dram
MultiRead:
 /*  memset(buf,0xAA,len);
 if(virt_addr_valid((u32)buf ))
 	{
	   phyaddr = dma_map_single(NULL, buf, len, DMA_TO_DEVICE);
	   dma_unmap_single(NULL, phyaddr, len, DMA_TO_DEVICE);
 	}

*/

#if ENABLE_LOCAL_BUF
	#ifdef MULTI_READ_DMA
		if(multiReadEnable&&(multiReadCounts>1))
			{
				if(multiReadCounts==2)
					readBuf=_pu1NorTemp1Buf;           //2nd dma read
				else
					printf("Nor: error read index\n");		
			}
		else
			readBuf=_pu1NorDataBuf;            //1st dma read
	#else
	readBuf=_pu1NorDataBuf;
	#endif

	if(len<(LOCAL_BUF_SIZE-64))
		{
	    	readLen =((len/0x10)+4)*0x10;   // fill in 48bytes+align bytes <=64
			if((readLen+from)>mt53xx_nor->total_sz)  //out of total size
			{
				readLen=len;
			}
		}
	else
		{
			readLen =len;  
			readBuf=buf;
		}
	/*
	memset(_pu1NorDataBuf,0xAA,readLen);
	if(virt_addr_valid((u32)_pu1NorDataBuf ))
 	{
	   phyaddr = dma_map_single(NULL, _pu1NorDataBuf, readLen, DMA_TO_DEVICE);
	   dma_unmap_single(NULL, phyaddr, readLen, DMA_TO_DEVICE);
 	}
*/
#else
	readBuf=buf;
    readLen=len;
#endif



  //if(len>maxSize)     //collect the maxSize
  //		 maxSize=len;
//  getnstimeofday(&tv);

  if(0 != mt53xx_nor_Read(mt53xx_nor, (u32)from, (u32)readLen, (u32 *)retlen, (u8 *)readBuf))
  {
    printf( "read failed(addr = %08x, len = %08x, retlen = %08x)\n", (u32)from,
		                                                                     (u32)readLen,
		                                                                     (u32)(*retlen)); 
	ret = -1;
  }
  else
  {
  #if ENABLE_LOCAL_BUF
  	memcpy(buf, readBuf, len);
   *retlen=len;
  #endif

#if 0
 //   mt53xx_nor_detect_pattern(buf,(u32)(*retlen));
//	mt53xx_nor_cal_checksum(from,buf,(u32)(*retlen));
	  /*check for jffs2 error*/
  if(from >= 0x970000)
  {
    if (!nor_init)
    {
        for (nor_index = 0; nor_index < NOR_READ_ARRAY_SIZE; nor_index++)
        {
            arNorReadArray[nor_index].nor_magic = 0;
        }
        nor_index = 0;
        nor_init = 1;
    }
    arNorReadArray[nor_index].nor_f00085a0 = SFLASH_READ32(0xF00085a0);
    arNorReadArray[nor_index].nor_f00085a4 = SFLASH_READ32(0xF00085a4);
    arNorReadArray[nor_index].nor_magic = 0xf4eef4ee;
    arNorReadArray[nor_index].nor_from = from;
    arNorReadArray[nor_index].nor_len = len;
    arNorReadArray[nor_index].nor_retlen = (u32)(*retlen);
    arNorReadArray[nor_index].nor_buf = (u32)buf;
    arNorReadArray[nor_index].nor_magic2 = 0xf4eef4ee;
    if ((u32)(*retlen) < (8*1024))
    {
        memcpy(arNorReadArray[nor_index].nor_data, buf, (u32)(*retlen));
    }
    nor_index = (nor_index < (NOR_READ_ARRAY_SIZE-1)) ? (nor_index+1) : 0;
  }
#endif
	#ifdef MULTI_READ_DMA
	if(multiReadEnable)
		{
	    if(multiReadCounts==2)
			if(memcmp(_pu1NorDataBuf,_pu1NorTemp1Buf,(u32)(*retlen))!=0)
				{
					printf("Nor:Dma 1st != 2nd\n");
					multiReadCounts++;
					mt53xx_nor_cal_checksum(from,buf,(u32)(*retlen));        //recover data again
					goto done;
				}
			else
				goto done;
		multiReadCounts++;
		if(multiReadCounts<=2)
			goto MultiRead;
  }
#endif
  }

done:
//	getnstimeofday(&tv1);
//	readtime = (1000000000 * tv1.tv_sec + tv1.tv_nsec) - (1000000000 * tv.tv_sec + tv.tv_nsec);

  //mutex_unlock(&mt53xx_nor->lock);
//  printf( "%s (addr = %08x, len = %08x, retlen = %08x buf = %08x  readtime=%08x speed=%08x %08x)\n", __FUNCTION__,(u32)from,(u32)len,(u32)(*retlen),buf,readtime, tv1.tv_nsec, tv.tv_nsec);

  return ret;

}
#ifndef BD_NOR_ENABLE
static u32 mt53xx_nor_WaitHostIdle(struct mt53xx_nor *mt53xx_nor)
{
  u32 polling;
  u8 reg;
	
  polling = 0;
  while(1)
  {
    reg = SFLASH_RREG8(SFLASH_SAMPLE_EDGE_REG);
    if (0x00 != (reg & (0x01<<6)))
    {
      break;
    }
	
    polling++;
    if(polling > SFLASH_POLLINGREG_COUNT)
    {
      printf( "timeout for wait host idle!\n");
      return -1;
    }

	udelay(5);
  }
	
  return 0;

}
#else
//-----------------------------------------------------------------------------
/*
*Enter 4 bytes address mode
*return 0 success, return others fial
*/
#if 0
static s32 _SetFlashEnter4Byte(struct mt53xx_nor *mt53xx_nor)
{
	u32 u4Index, u4DualReg, u4Polling;
	u4Index = 0;
	if(mt53xx_nor_WriteEnable(mt53xx_nor) != 0)
    {
        return 1;
    }	
	
    if( (mt53xx_nor->flash_info[0].u1MenuID == 0x01) &&(mt53xx_nor->flash_info[0].u1DevID1 == 0x02) &&(mt53xx_nor->flash_info[0].u1DevID2 == 0x19) )	//for spansion S25FL256s flash
    {
        SFLASH_WREG8(SFLASH_PRGDATA5_REG, 0x17);	//Enter EN4B cmd
        SFLASH_WREG8(SFLASH_PRGDATA4_REG, 0x80);	
        SFLASH_WREG8(SFLASH_CNT_REG,16); 			// Write SF Bit Count
    }
    else
    {
        SFLASH_WREG8(SFLASH_PRGDATA5_REG, 0xb7);	//Enter EN4B cmd
        SFLASH_WREG8(SFLASH_CNT_REG,8); 			// Write SF Bit Count
    }
   
    mt53xx_nor->cur_cmd_val = 0x04;
    if(-1 == mt53xx_nor_ExecuteCmd(mt53xx_nor))
    {
        printf( "4byte addr enable failed!\n");
        return -1;
    }
	
	u4DualReg = SFLASH_RREG32(SFLASH_DUAL_REG); 
	u4DualReg |= 0x10;
	SFLASH_WREG32(SFLASH_DUAL_REG, u4DualReg);

    u4Polling = 0;
    while(1)
    {
        u4DualReg = SFLASH_RREG32(SFLASH_DUAL_REG); 
        if (0x10 == (u4DualReg & 0x10))
        {
            break;
        }
        u4Polling++;
        if(u4Polling > SFLASH_POLLINGREG_COUNT)
        {
            return 1;
        }
    }	
	//LOG(0, "...cdd Enter 4 bytes address!\n");
	return 0;
}
#endif // #if 0
#endif

#if 0
static u32 mt53xx_nor_ReadSingleByte(struct mt53xx_nor *mt53xx_nor, u32 addr, u8 *buf)
{
  u32 ret = 0, retlen = 0;
	
  if(addr >= mt53xx_nor->total_sz)
  {
    printf( "invalid write address exceeds MAX possible address!\n");
	ret = -1;
	goto done;
  }
	
  if(0 != mt53xx_nor_Read(mt53xx_nor, addr, 1, &retlen, buf))
  {
    printf( "read single byte failed!\n");
	ret = -1;
	goto done; 
  }

done:

  return ret;

}
#endif

static u32 mt53xx_nor_WriteSingleByte(struct mt53xx_nor *mt53xx_nor, u32 u4addr, u8 u1data)
{
  u32 addr;
  u8 index;
#if 0
  u8 data;
#endif

  if(u4addr >= mt53xx_nor->total_sz)
  {
    printf( "invalid write address exceeds MAX possible address!\n");
	return -1;
  }

#if 0
  if(0 != mt53xx_nor_ReadSingleByte(mt53xx_nor, u4addr, &data))
  {
    printf( "read byte before write failed!\n");
  }
  if(data != 0xFF)
  {
    printf( "read byte before write value: %02x(addr%08x, target:%02x)\n", data, u4addr, u1data);
	while(1);
  }
#endif

  if(0 != mt53xx_nor_Offset2Index(mt53xx_nor, u4addr))
  {
    printf( "offset parse failed in write byte process!\n");
	return -1;
  }

  index = mt53xx_nor->cur_flash_index;
  addr = mt53xx_nor->cur_addr_offset;

  SFLASH_WREG8(SFLASH_WDATA_REG, u1data);
  SFLASH_WREG8(SFLASH_RADR3_REG, HiByte(HiWord(addr)));
  SFLASH_WREG8(SFLASH_RADR2_REG, LoByte(HiWord(addr)));
  SFLASH_WREG8(SFLASH_RADR1_REG, HiByte(LoWord(addr)));
  SFLASH_WREG8(SFLASH_RADR0_REG, LoByte(LoWord(addr)));

  mt53xx_nor->cur_cmd_val = 0x10;
  if(0 != mt53xx_nor_ExecuteCmd(mt53xx_nor))
  {
    printf( "write byte(offset:%08x flash#%d, sector index:%d, offset:%08x) failed!\n", 
		                                                      u4addr, index, 
		                                                      mt53xx_nor->cur_sector_index, 
		                                                      mt53xx_nor->cur_addr_offset);
    return -1;
  }

  mt53xx_nor->cur_cmd_val = 0x00;
  mt53xx_nor_SendCmd(mt53xx_nor);

  if(0 != mt53xx_nor_WaitBusy(mt53xx_nor, SFLASH_WRITEBUSY_TIMEOUT))
  {
    printf( "wait write busy failed in write byte process!\n");
    return -1;
  }

#if 0
	if(0 != mt53xx_nor_ReadSingleByte(mt53xx_nor, u4addr, &data))
	{
	  printf( "read byte after write failed!\n");
	}
	if(data != u1data)
	{
	  printf( "read byte after write value: %02x(addr%08x, target:%02x)\n", data, u4addr, u1data);
	  while(1);
	}
#endif


  return 0;
  
}

static u32 mt53xx_nor_WriteBuffer(struct mt53xx_nor *mt53xx_nor, 
	                                      u32 u4addr, u32 u4len, const u8 *buf)
{
  u32 i, j, bufidx, data, addr;
  u8 index;

  if(buf == NULL)
  {
    return -1;
  }

  if(0 != mt53xx_nor_Offset2Index(mt53xx_nor, u4addr))
  {
    printf( "offset parse failed in write buffer process!\n");
	return -1;
  }

  index = mt53xx_nor->cur_flash_index;
  addr = mt53xx_nor->cur_addr_offset;
  SFLASH_WREG8(SFLASH_RADR3_REG, HiByte(HiWord(u4addr))); // Write
  SFLASH_WREG8(SFLASH_RADR2_REG, LoByte(HiWord(u4addr))); // Write
  SFLASH_WREG8(SFLASH_RADR1_REG, HiByte(LoWord(u4addr))); // Write
  SFLASH_WREG8(SFLASH_RADR0_REG, LoByte(LoWord(u4addr))); // Write

  bufidx = 0;
  for(i=0; i<u4len; i+=4)
  {
    for(j=0; j<4; j++)
    {
      (*((u8 *)&data + j)) = buf[bufidx];
      bufidx++;
    }
    SFLASH_WREG32(SFLASH_PP_DATA_REG, data);
  }

  mt53xx_nor->cur_cmd_val = 0x10;
  if(0 != mt53xx_nor_ExecuteCmd(mt53xx_nor))
  {
    printf( "write buffer(offset:%08x flash#%d, sector index:%d, offset:%08x) failed!\n", 
		                                                      u4addr, index, 
		                                                      mt53xx_nor->cur_sector_index, 
		                                                      mt53xx_nor->cur_addr_offset);
    return -1;
  }

  mt53xx_nor->cur_cmd_val = 0x00;
  mt53xx_nor_SendCmd(mt53xx_nor);

  if(0 != mt53xx_nor_WaitBusy(mt53xx_nor, SFLASH_WRITEBUSY_TIMEOUT))
  {
    printf( "wait write buffer failed in write buffer process!\n");
    return -1;
  }

  return 0;
  
}

static u32 mt53xx_nor_Write(struct mt53xx_nor *mt53xx_nor, u32 addr, u32 len, u32 *retlen, const u8 *buf)
{
  u32 i, ret = 0;
  u8 info;
  
#ifdef BD_NOR_ENABLE
  u32 count, pgalign = 0;
#endif

  mt53xx_nor_PinMux();
	  
  /* get rank-flash info of region indicated by addr and len
     */

  info = mt53xx_nor_Region2Info(mt53xx_nor, addr, len);
	
  /*
     * disable write protect of all related chips 
     * make sure all of related flash status is idle
     * make sure flash can be write
     */
  if(info & REGION_RANK_FIRST_FLASH)
  {
    mt53xx_nor->cur_flash_index = 0;
	
    if(0 != mt53xx_nor_WriteProtect(mt53xx_nor, 0))
    {
      printf( "disable write protect of flash#0 failed!\n");
      ret = -1;
      goto done;
    }

   
    if(0 != mt53xx_nor_WaitBusy(mt53xx_nor, SFLASH_WRITEBUSY_TIMEOUT))
    {
      printf( "wait write busy of flash#0 failed!\n");
      ret = -1;
      goto done;
    }

    if(0 != mt53xx_nor_WriteEnable(mt53xx_nor))
    {
      printf( "write enable of flash#0 failed!\n");
      ret = -1;
      goto done;
    }


  }
	  
  if(info & REGION_RANK_SECOND_FLASH)
  {
    mt53xx_nor->cur_flash_index = 1;

    if(protect_2nd_nor)
    	{
    		printf( "Cancel the protect at 2nd nor before write data\n");
    		//printf( "[%s(%d)]%s (addr = %08x, len = %08x, retlen = %08x)\n", current->comm, current->pid, __FUNCTION__, 
    		//         (u32)addr,(u32)len,(u32)(*retlen));
		    if(0 != mt53xx_nor_WriteProtect(mt53xx_nor, 0))
		    {
		      printf( "disable write protect of flash#1 failed!\n");
		      ret = -1;
		      goto done;
		    }
    	}



    if(0 != mt53xx_nor_WaitBusy(mt53xx_nor, SFLASH_WRITEBUSY_TIMEOUT))
    {
      printf( "wait write busy of flash#1 failed!\n");
      ret = -1;
      goto done;
    }


    if(0 != mt53xx_nor_WriteEnable(mt53xx_nor))
    {
      printf( "write enable of flash#1 failed!\n");
      ret = -1;
      goto done;
    }	


  } 
#ifdef BD_NOR_ENABLE
  /* handle no-buffer-align case */ 
  pgalign = addr % SFLASH_WRBUF_SIZE;
  if(pgalign != 0)
  {
    for(i=0; i<(SFLASH_WRBUF_SIZE - pgalign); i++)
    {
      if(mt53xx_nor_WriteSingleByte(mt53xx_nor, addr, *buf) != 0)
      {
        ret = -1;
        goto done;
      }
      addr++;
      buf++;
      len--;
      (*retlen)++;
	  
      if(len == 0)
      {
        ret = 0;
        len = (u32)len;
        goto done;		
      }
    }
  }
	
  /* handle buffer case */
  if(mt53xx_nor_WriteBufferEnable(mt53xx_nor) != 0)
  {
    return -1;
  }
  while((u32)len > 0)
  {
    if(len >= SFLASH_WRBUF_SIZE)
    {
      count = SFLASH_WRBUF_SIZE;
    }
    else
    {
      // Not write-buffer alignment
      break;
    }
	
    if(mt53xx_nor_WriteBuffer(mt53xx_nor, addr, count, buf) != 0)
    {
      if(mt53xx_nor_WriteBufferDisable(mt53xx_nor) != 0)
      {
        ret = -1;
      }
      ret = -1;
      goto done;
    }
		
    len -= count;
    (*retlen) += count;
    addr += count;
    buf += count;
  }
  if(mt53xx_nor_WriteBufferDisable(mt53xx_nor) != 0)
  {
    ret = -1;
    goto done;
  }
	
  /* handle remain case */
  if((INT32)len > 0)
  {
    for(i=0; i<len; i++,  (*retlen)++)
    {
      if(mt53xx_nor_WriteSingleByte(mt53xx_nor, addr, *buf) != 0)
      {
        if(mt53xx_nor_WriteBufferDisable(mt53xx_nor) != 0)
        {
          ret =  -1;
        }
        ret =  -1;
        goto done;
      }
		  
      addr++;
      buf++;
    }
  }
  
#else


  for(i=0; i<((u32)len); i++, (*retlen)++)
  {
    if(mt53xx_nor_WriteSingleByte(mt53xx_nor, addr, *buf) != 0)
    {
      ret = -1;
      goto done;
    }
    addr++;
    buf++;
  }
  if(!enable_protect_ro)
  	 printf("%s (%d)\n",__FUNCTION__,mt53xx_nor->cur_flash_index,mt53xx_nor->cur_addr_offset,mt53xx_nor->cur_sector_index);
  if(info & REGION_RANK_SECOND_FLASH)
   {

  		if(protect_2nd_nor)
    	{
    		printf( "  Protect 2nd nor after  write data\n");
    		printf( "[%s(%d)]%s (addr = %08x, len = %08x, retlen = %08x)\n", current->comm, current->pid, __FUNCTION__, 
    		         (u32)addr,(u32)len,(u32)(*retlen));
		    if(0 != mt53xx_nor_WriteProtect(mt53xx_nor, 1))
		    {
		      printf( "disable write protect of flash#1 failed!\n");
		      ret = -1;
		      goto done;
		    }
    	}
  	}

#endif

done:

	return ret;

}
#if NOR_DEBUG
static u32 Get_Kernel_rootfs_checksum()
{
	u32 i,j,count=0;
	u8 *nor_baseAddress;
	u32 ichecksum;

	nor_baseAddress=0xf8000000+0x30000;
	for(i=0;i<0x93;i++)
		{
		nor_baseAddress +=0x10000;
		ichecksum=0;
		for(j=0;j<0x10000;j++)
			{
				ichecksum +=nor_baseAddress[j];

			}
		
		printf( "0X%08x checksum:0x%08x\n",nor_baseAddress,ichecksum);
		
		}

}
#endif

#if defined(__UBOOT_NOR__)
u32 mtk_nor_write(u32 to, u32 len, size_t *retlen, const u_char *buf)
{
#else
static u32 mt53xx_nor_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf)
{
  struct mt53xx_nor *mt53xx_nor = mtd_to_mt53xx_nor(mtd);
#endif
  u32 addr, u4len, i, count, pgalign, ret = 0, j;
  u8 *u1buf, info;
#if NOR_DEBUG
  struct timeval t0;
#endif
  if (retlen)
    *retlen = 0;

  //if the address of nor < 0x30000, the system will be assert
 if((to < 0x30000)&&(enable_protect_ro==1))
  {
	 printf("Error: nor write address at 0x%x < 0x30000\n",to);
	 //while(1) {};
	 BUG_ON(1);
	 return -EINVAL;
  }
  if (!len)
  {
    return 0;
  }
	
  //if (to + len > mt53xx_nor->mtd.size)
  if (to + len > mt53xx_nor->total_sz)
  {
  	printf( "Error: nor write address to +len at 0x%x >0xc00000\n",to+len);
	//while(1) {};
    return -EINVAL;
  }

  /* Byte count starts at zero. */
  *retlen = 0;

  //mutex_lock(&mt53xx_nor->lock);
#if NOR_DEBUG
    printf( "%s (addr = %08x, len = %08x, retlen = %08x)\n",__FUNCTION__, (u32)to,(u32)len,(u32)(*retlen)); 
#endif	

  if(0 != mt53xx_nor_Write(mt53xx_nor, (u32)to, (u32)len, (u32 *)retlen, (u8 *)buf))
  {
    printf( "write failed(addr = %08x, len = %08x, retlen = %08x)\n", (u32)to,
																		      (u32)len,
																			  (u32)(*retlen)); 
    ret = -1;
  }
 #if NOR_DEBUG
  do_gettimeofday(&t0);
  printf( "[Time trace:%s(%d) time = %ld\n", __FUNCTION__,__LINE__,t0.tv_sec*1000+t0.tv_usec); 
#endif	
done:
	
  //mutex_unlock(&mt53xx_nor->lock);

  return ret;
  
}

//static int mt53xx_nor_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
#if !defined(__UBOOT_NOR__)
int NORPART_Read(unsigned long long u8Offset, unsigned int u4MemPtr, unsigned int u4MemLen)
{
  int part;
  loff_t offset;
  size_t len, retlen, size;
  static const char *part_parsers[] = {"cmdlinepart", NULL};
  //struct mtd_partition *parts;
  
  // Find part offset and size.
  part = u8Offset >> 32;
  offset = u8Offset & 0xffffffff;
  if (part >= partnum)
  
      return -EINVAL;

  if (offset >= mtdparts[part].size)
      return -EINVAL;

  len = mtdparts[part].size - offset;
  offset += mtdparts[part].offset;
  len = (u4MemLen > len) ? len : u4MemLen;

  // readit
  return mt53xx_nor_read(&mt53xx_nor->mtd, offset, len, &retlen, (void*)u4MemPtr);
}
EXPORT_SYMBOL(NORPART_Read);

static int  mt53xx_nor_RequestIrq(struct mt53xx_nor *mt53xx_nor)
{
  u8 attr = mt53xx_nor->host_act_attr;
  
  if(attr & SFLASH_USE_ISR)
  {
    if(0 != mt53xx_nor_RegIsr(mt53xx_nor))
    {
      printf( "request flash isr failed!\n");
	  return -1;
    }
	
	mt53xx_nor_SetIsr(mt53xx_nor);
  }

  return 0;
}

//-----------------------------------------------------------------------------
/** add_dynamic_parts()
 */
//-----------------------------------------------------------------------------

static inline int add_dynamic_parts(struct mtd_info *mtd)
{
#if 1
    static const char *part_parsers[] = {"cmdlinepart", NULL};
    
    INT32 err;

    printf( "add_dynamic_parts\n");

    partnum = parse_mtd_partitions(mtd, part_parsers, &mtdparts, 0);
    if (partnum <= 0)
    {
        return -EIO;
    }

    err = add_mtd_partitions(mtd, mtdparts, partnum);
    if (err != 0)
    {
        return -EIO;
    }
#endif
    return add_mtd_device(&mt53xx_nor->mtd);

	
}

static int proc_read_protect_disable(char *page, char **start,
                             off_t off, int count,
                             int *eof, void *data)
{
    int  len;
	printf( "enable_protect_ro=%d\n",enable_protect_ro);			
	*eof = 1;
    return len;
}
static int proc_write_protect_disable(struct file *file,
                             const char *buffer,
                             unsigned long count,
                             void *data)
{
	
	enable_protect_ro=0;
	printf( "Disable protect the READ_ONLY partition :enable_protect_ro=%d\n",enable_protect_ro);
	
    return count;
}
static int proc_read_protect_2nd_nor(char *page, char **start,
                             off_t off, int count,
                             int *eof, void *data)
{
    int  len;
	printf( "MAXSIZE=%d\n",maxSize);
	if(protect_2nd_nor)
         printf( "2nd Nor had been protect at write process\n");
	else
		 printf( "2nd Nor not be protect at write process\n");
	*eof = 1;
    return len;
}
static int proc_write_protect_2nd_nor(struct file *file,
                             const char *buffer,
                             unsigned long count,
                             void *data)
{
	
	protect_2nd_nor=1;
	printf( "Setting protect 2nd nor :protect_2nd_nor=%d\n",protect_2nd_nor);
    return count;
}


static int __init nor_init_procfs(void)
{
#if 0
	printf( "%s entered\n", __FUNCTION__);
	 nor_proc_dir = proc_mkdir("driver/nor", NULL);
    if(nor_proc_dir == NULL) {
		 printf( "nor_proc_dir create failed\n" );
        goto proc_init_fail;
    }
    nor_write_enable = create_proc_entry("protect_disable", 0755, nor_proc_dir);
    if(nor_write_enable == NULL) {
		printf( "nor_write_enable create failed\n" );
        goto proc_init_fail;
    }
    nor_write_enable->read_proc = proc_read_protect_disable;
    nor_write_enable->write_proc = proc_write_protect_disable;
	//protect the 2nd nor flash 
	 nor_protect_2nd_nor = create_proc_entry("protect_2nd_nor", 0755, nor_proc_dir);
    if(nor_protect_2nd_nor == NULL) {
		printf( "nor_write_enable create failed\n" );
        goto proc_init_fail;
    }
    nor_protect_2nd_nor->read_proc = proc_read_protect_2nd_nor;
    nor_protect_2nd_nor->write_proc = proc_write_protect_2nd_nor;
	

    return 0;
 proc_init_fail:
     return -1;
#else
    return 0;
#endif
}

/**
 * nand_block_isbad - [MTD Interface] Check if block at offset is bad
 * @mtd:	MTD device structure
 * @offs:	offset relative to mtd start
 */
 #ifdef BD_NOR_ENABLE
static int nor_block_isbad(struct mtd_info *mtd, loff_t offs)
{	
    return 0;
}

/**
 * nand_block_markbad - [MTD Interface] Mark block at the given offset as bad
 * @mtd:	MTD device structure
 * @ofs:	offset relative to mtd start
 */
static int nor_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
    return 0;
}
#endif

#endif // !defined(__UBOOT_NOR__ )

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

static void NFI_GPIO_SET_FIELD(unsigned int  reg, unsigned int  field, unsigned int  val)
{
	unsigned short tv = (unsigned short)(*(volatile unsigned short *)(reg));
	tv &= ~(field);
	tv |= ((val) << (NFI_gpio_uffs((unsigned short)(field)) - 1));
	(*(volatile unsigned short *)(reg) = (unsigned short)(tv));
}

static void NFI_set_gpio_mode(int gpio, int value)
{
	unsigned short ori_gpio, offset, rev_gpio;
	unsigned short* gpio_mode_p;

	gpio_mode_p = (unsigned short *)( 0x10005760+ (gpio/5) * 0x10); // Use APMCU COMPILE

	ori_gpio = *gpio_mode_p;
	offset = gpio % 5;
	rev_gpio = ori_gpio & (0xffff ^ (0x7<<(offset*3)));
	rev_gpio = rev_gpio |(value << (offset*3));
	*gpio_mode_p = rev_gpio;
}
#define             EFUSE_Is_IO_33V()               (((*((volatile unsigned int *)(0x102061c0)))&0x8)?0:1) // 0 : 3.3v (MT8130 default),


#if !defined(__UBOOT_NOR__)
static int __init mt53xx_nor_init(void)
{
  int ret = -ENODEV;
  int result;
  u8 status;
  
	{
		unsigned char is33v;
		unsigned int  val;

		NFI_set_gpio_mode(236, 1);    //Switch EXT_SDIO3 to non-default mode
		NFI_set_gpio_mode(237, 1);    //Switch EXT_SDIO2 to non-default mode
		NFI_set_gpio_mode(238, 1);    //Switch EXT_SDIO1 to non-default mode
		NFI_set_gpio_mode(239, 1);    //Switch EXT_SDIO0 to non-default mode
		NFI_set_gpio_mode(240, 1);    //Switch EXT_XCS to non-default mode
		NFI_set_gpio_mode(241, 1);    //Switch EXT_SCK to non-default mode

		NFI_GPIO_SET_FIELD(0xf0005360, 0xF000, 0xF);    //set NOR IO pull up
		NFI_GPIO_SET_FIELD(0xf0005370, 0x3, 0x3);   //set NOR IO pull up


		is33v = EFUSE_Is_IO_33V();
		val = is33v ? 0x0c : 0x00;


		NFI_GPIO_SET_FIELD(0xf0005c00, 0xf,   0x0a);    /* TDSEL change value to 0x0a*/
		NFI_GPIO_SET_FIELD(0xf0005c00, 0x3f0, val);     /* RDSEL change value to val*/
	}


  /*
     * malloc the necessary memory space for mt53xx_nor
     */
  //mt53xx_nor = kzalloc(sizeof(mt53xx_nor), GFP_KERNEL);	//error
  mt53xx_nor = kzalloc(sizeof (*mt53xx_nor), GFP_KERNEL);
  if(!mt53xx_nor)
  	return -ENOMEM;

  mt53xx_nor->flash_info = kzalloc(MAX_FLASHCOUNT*sizeof( SFLASHHW_VENDOR_T), GFP_KERNEL);
  //mt53xx_nor->flash_info = kzalloc(MAX_FLASHCOUNT*sizeof(struct SFLASHHW_VENDOR_T), GFP_KERNEL);
  if(!mt53xx_nor->flash_info)
  {
    kfree(mt53xx_nor);
	return -ENOMEM;
  }

  printf( "<mt53xx_nor_init>hank:V0.6.9m(protect 0 flash)...\n");

  /*
     * lock initialization
     */
  mutex_init(&mt53xx_nor->lock);

  mt53xx_nor->cur_flash_index = 0;
  mt53xx_nor->cur_cmd_val = 0;

  /*
     * switch pinmux for nor flash
     */
  mt53xx_nor_PinMux();

  /*
     * allow flash controller to send write/write status
     * 
     */
  SFLASH_WREG32(SFLASH_CFG1_REG, 0x20);

  /*
     * wait nor controller idle before identify the emmc
     * 
     */
#ifndef BD_NOR_ENABLE
  mt53xx_nor_WaitHostIdle(mt53xx_nor);
#else
  SFLASH_WREG32(SFLASH_WRPROT_REG, 0x30);
  //set clock
  //SFLASH_WRITE32(0xFD000070, SFLASH_READ32(0xFD000070) | 0x3000);//54mhz
  SFLASH_WRITE32(0xF00000B0, SFLASH_READ32(0xF00000B0) & ~0x3);
#endif
  /*
     * identify the all nor flash and get necessary information 
     */
  mt53xx_nor_GetID(mt53xx_nor);

  /*
     * disable write protect at first 
     */
  mt53xx_nor_WriteProtectAllChips(mt53xx_nor, 0);

  // protect the first Nor flash( partiton: "loader","kernel","root"
   mt53xx_nor->cur_flash_index = 0;
    if(0 != mt53xx_nor_WriteProtect(mt53xx_nor, 1))
    {
        printf( "Flash #%d: write protect disable failed!\n", mt53xx_nor->cur_flash_index);
	 
    }
	else
	{
		printf("Flash #%d: write protect enable successfully\n", mt53xx_nor->cur_flash_index);
	}


  /* host action attribute
     */
  mt53xx_nor->host_act_attr = SFLASH_USE_DMA | SFLASH_USE_ISR | SFLASH_USE_DUAL_READ;
  /*
     * initialization the mtd-related variant
     */
  mt53xx_nor->mtd.type = MTD_NORFLASH;
  mt53xx_nor->mtd.flags = MTD_CAP_NORFLASH;
  mt53xx_nor->mtd.size = mt53xx_nor->total_sz;
  mt53xx_nor->mtd._erase = mt53xx_nor_erase;
  mt53xx_nor->mtd._read = mt53xx_nor_read;
  mt53xx_nor->mtd._write = mt53xx_nor_write;
  mt53xx_nor->mtd.owner = THIS_MODULE;
  mt53xx_nor->mtd.priv = NULL;
  mt53xx_nor->mtd.writebufsize = 0x1000;
#ifdef BD_NOR_ENABLE
  mt53xx_nor->mtd.writesize = 0x100;//if change to 1 , ubifs vid header error
  mt53xx_nor->mtd.erasesize = 0x10000;//only support 64K sector size
  mt53xx_nor->mtd.name = "All";
  mt53xx_nor->mtd._block_isbad   = nor_block_isbad;
  mt53xx_nor->mtd._block_markbad = nor_block_markbad;
#else
  mt53xx_nor->mtd.writesize = 0x1;
  mt53xx_nor->mtd.erasesize = 0x10000;
  mt53xx_nor->mtd.name = "mt53xx-nor";
#endif

  
  printf("mt53xx_nor->total_sz =0x%x !\n",mt53xx_nor->total_sz);

#ifndef BD_NOR_ENABLE
  mtdchar_buf = kmalloc(0x200, GFP_KERNEL);
  //init_MUTEX(&mtdchar_share);
  sema_init(&mtdchar_share, 1);

#else
  mt53xx_nor->host_act_attr = SFLASH_USE_DMA | SFLASH_USE_DUAL_READ;
 // _SetFlashEnter4Byte(mt53xx_nor);
#endif
  //init_MUTEX_LOCKED(&dma_complete);
  sema_init(&dma_complete, 0);

  /* flash isr request
     */
  mt53xx_nor_RequestIrq(mt53xx_nor);
#ifdef BD_NOR_ENABLE // BD test code flow 
  //test code
  #if 0
  int i= 0;
  size_t retlen;
  //struct erase_info instr;
  //instr.addr = 0x1000000;
  //instr.len  = 0x100000;

  //u_char * w_buf = kmalloc(0x100000, GFP_KERNEL);
  u_char * r_buf = kmalloc(0x200000, GFP_KERNEL);
  //printf("nor buf w_buf=0x%08x , r_buf=0x%08x!\n",w_buf,r_buf);
 
  //mt53xx_nor_erase(&(mt53xx_nor->mtd), &instr);
  
  //for(i=0; i<0x10000; i++)
  {
  //  w_buf[i] = (UINT8)(i&0xFF);
  }
  
  //mt53xx_nor_write(&(mt53xx_nor->mtd), 0x1000000, 0x10000, &retlen, w_buf);
  
  printf("read test strat total_sz =0x%x !\n",0x200000);
  mt53xx_nor_read(&(mt53xx_nor->mtd), 0x500000, 0x200000, &retlen, r_buf);
  printf("read test end total_sz =0x%x !\n",0x200000);
  //if(memcmp((void*)w_buf, (void*)r_buf, 0x10000) != 0)
  {
   // printf("nor RW test fail!!! buf w_buf=0x%08x , r_buf=0x%08x!\n",w_buf,r_buf);
  }
  
  //test code end
  #endif
#endif
  /* flash host dual read setting
     */
  //mt53xx_nor_SetDualRead(mt53xx_nor);

  /* register mtd partition layout
     */  
#ifdef BD_NOR_ENABLE

//#if PARTITION_TABLE_INIT
#if CONFIG_MTD_MYSELF
	  mt85xx_part_tbl_init(&mt53xx_nor->mtd);
#endif
//#endif
	  /* We register the whole device first, separate from the partitions */
	  add_mtd_device(&mt53xx_nor->mtd);
#if CONFIG_MTD_MYSELF
	  partnum = mt85xx_fill_partition();
	  add_mtd_partitions(&mt53xx_nor->mtd, _partition, NUM_PARTITIONS);
#else
#ifdef CONFIG_MTD_CMDLINE_PARTS
	  mtd->name = "mt85xx_nand";
	  nr_parts = parse_mtd_partitions(&mt53xx_nor->mtd, part_probes, &parts, 0);
	  if (nr_parts > 0) {
		  mt85xx->parts = parts;
		  //dev_info(&cafe->pdev->dev, "%d RedBoot partitions found\n", nr_parts);
		  add_mtd_partitions(&mt53xx_nor->mtd, parts, nr_parts);
	  }
#else
	  add_mtd_partitions(&mt53xx_nor->mtd, _partition, NUM_PARTITIONS);
#endif
#endif
#endif
  ret = add_dynamic_parts(&mt53xx_nor->mtd);


  nor_init_procfs();

  return ret;
}

static void __exit mt53xx_nor_exit(void)
{
#ifndef BD_NOR_ENABLE
  kfree(mtdchar_buf);
  mtdchar_buf = NULL;
#endif

}

module_init(mt53xx_nor_init);
module_exit(mt53xx_nor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qingqing Li <qingqing.li@mediatek.com>");
MODULE_DESCRIPTION("MTD NOR driver for mediatek BD boards");
#endif // !defined(__UBOOT_NOR__)

static s32 _SetFlashExit4Byte(struct mt53xx_nor *mt53xx_nor)
{
	u32 u4Index, u4DualReg, u4Polling;
	u4Index = 0;

	u4DualReg = SFLASH_RREG32(SFLASH_DUAL_REG); 
	u4DualReg &= ~0x10;
	SFLASH_WREG32(SFLASH_DUAL_REG, u4DualReg);

	u4Polling = 0;
	while(1)
	{
		u4DualReg = SFLASH_RREG32(SFLASH_DUAL_REG); 
		if (!(u4DualReg & 0x10))
		{
			break;
        	}
		u4Polling++;
		if(u4Polling > SFLASH_POLLINGREG_COUNT)
		{
			printk("exit 4 byte mode polling fail %x\n", u4DualReg);
			return 1;
		}
	}

	if(mt53xx_nor_WriteEnable(mt53xx_nor) != 0)
	{
		printk("enter 4 byte mode write enable fail\n");
		return 1;
	}

	if( (mt53xx_nor->flash_info[0].u1MenuID == 0x01) &&(mt53xx_nor->flash_info[0].u1DevID1 == 0x02) &&(mt53xx_nor->flash_info[0].u1DevID2 == 0x19) )	//for spansion S25FL256s flash
	{
		SFLASH_WREG8(SFLASH_PRGDATA5_REG, 0x17);	// Write Bank register
		SFLASH_WREG8(SFLASH_PRGDATA4_REG, 0x0);		// Clear 4B mode
		SFLASH_WREG8(SFLASH_CNT_REG,16); 		// Write SF Bit Count
	}
		else
	{
		SFLASH_WREG8(SFLASH_PRGDATA5_REG, 0xE9);	//Exit 4B cmd
		SFLASH_WREG8(SFLASH_CNT_REG,8); 		// Write SF Bit Count
	}
   
	mt53xx_nor->cur_cmd_val = 0x04;
	if(-1 == mt53xx_nor_ExecuteCmd(mt53xx_nor))
	{
		printk(KERN_ERR "4byte addr enable failed!\n");
		return -1;
	}
	return 0;
}

static s32 _SetFlashEnter4Byte(struct mt53xx_nor *mt53xx_nor)
{
	u32 u4Index, u4DualReg, u4Polling;
	u4Index = 0;

    // dont enter 4 byte mode if size < 32MB
    if (mt53xx_nor->flash_info[0].u4ChipSize < 0x2000000)
    {
	    printk("size = %x, exit 4 byte mode\n", mt53xx_nor->flash_info[0].u4ChipSize);
	    return _SetFlashExit4Byte(mt53xx_nor);
    }
    else
	    printk("size = %x, enter 4 byte mode\n", mt53xx_nor->flash_info[0].u4ChipSize);

    if(mt53xx_nor_WriteEnable(mt53xx_nor) != 0)
    {
	printk("enter 4 byte mode write enable fail\n");
        return 1;
    }	

	
    if( (mt53xx_nor->flash_info[0].u1MenuID == 0x01) &&(mt53xx_nor->flash_info[0].u1DevID1 == 0x02) &&(mt53xx_nor->flash_info[0].u1DevID2 == 0x19) )	//for spansion S25FL256s flash
    {
        SFLASH_WREG8(SFLASH_PRGDATA5_REG, 0x17);	//Enter EN4B cmd
        SFLASH_WREG8(SFLASH_PRGDATA4_REG, 0x80);	
        SFLASH_WREG8(SFLASH_CNT_REG,16); 			// Write SF Bit Count
    }
    else
    {
        SFLASH_WREG8(SFLASH_PRGDATA5_REG, 0xb7);	//Enter EN4B cmd
        SFLASH_WREG8(SFLASH_CNT_REG,8); 			// Write SF Bit Count
    }
   
    mt53xx_nor->cur_cmd_val = 0x04;
    if(-1 == mt53xx_nor_ExecuteCmd(mt53xx_nor))
    {
        printk(KERN_ERR "4byte addr enable failed!\n");
        return -1;
    }
	
	u4DualReg = SFLASH_RREG32(SFLASH_DUAL_REG); 
	u4DualReg |= 0x10;
	SFLASH_WREG32(SFLASH_DUAL_REG, u4DualReg);

    u4Polling = 0;
    while(1)
    {
        u4DualReg = SFLASH_RREG32(SFLASH_DUAL_REG); 
        if (0x10 == (u4DualReg & 0x10))
        {
            break;
        }
        u4Polling++;
        if(u4Polling > SFLASH_POLLINGREG_COUNT)
        {
	    printk("enter 4 byte mode polling fail\n");
            return 1;
        }
    }	
	//LOG(0, "...cdd Enter 4 bytes address!\n");
        printk(KERN_ERR "4byte addr enable done!\n");
	return 0;
}

#if defined(__UBOOT_NOR__)
int mtk_nor_init(void)
{
	int reg;

	// to do: need to remove later
	{
		unsigned char is33v;
		unsigned int  val;

		NFI_set_gpio_mode(236, 1);    //Switch EXT_SDIO3 to non-default mode
		NFI_set_gpio_mode(237, 1);    //Switch EXT_SDIO2 to non-default mode
		NFI_set_gpio_mode(238, 1);    //Switch EXT_SDIO1 to non-default mode
		NFI_set_gpio_mode(239, 1);    //Switch EXT_SDIO0 to non-default mode
		NFI_set_gpio_mode(240, 1);    //Switch EXT_XCS to non-default mode
		NFI_set_gpio_mode(241, 1);    //Switch EXT_SCK to non-default mode

		NFI_GPIO_SET_FIELD(0x10005360, 0xF000, 0xF);    //set NOR IO pull up
		NFI_GPIO_SET_FIELD(0x10005370, 0x3, 0x3);   //set NOR IO pull up


		is33v = EFUSE_Is_IO_33V();
		val = is33v ? 0x0c : 0x00;


		NFI_GPIO_SET_FIELD(0x10005c00, 0xf,   0x0a);    /* TDSEL change value to 0x0a*/
		NFI_GPIO_SET_FIELD(0x10005c00, 0x3f0, val);     /* RDSEL change value to val*/
	}
	
	if (!mt53xx_nor)
	{
 		mt53xx_nor = malloc(sizeof (*mt53xx_nor));
 		if(!mt53xx_nor)
 			return -ENOMEM;
	}

	if (!mt53xx_nor->flash_info)
	{
		mt53xx_nor->flash_info = malloc(MAX_FLASHCOUNT*sizeof( SFLASHHW_VENDOR_T));
  		if(!mt53xx_nor->flash_info)
  		{
  			kfree(mt53xx_nor);
  			return -ENOMEM;
		}
  	}
                    
 	mt53xx_nor->cur_flash_index = 0;
 	mt53xx_nor->cur_cmd_val = 0;
 
  	/*
     	 * switch pinmux for nor flash
     	 */
 	mt53xx_nor_PinMux();

 	SFLASH_WREG32(SFLASH_CFG1_REG, 0x20);

 	SFLASH_WREG32(SFLASH_WRPROT_REG, 0x30);
 	//set clock
 	SFLASH_WRITE32(0x100000B0, SFLASH_READ32(0x100000B0) & ~0x3); // to do: remove Hard code
	
	mt53xx_nor_GetID(mt53xx_nor);
	printf("Roger debug: size=%x, flash_num=%d\n", mt53xx_nor->total_sz, mt53xx_nor->flash_num); // Roger debug

 	mt53xx_nor_WriteProtectAllChips(mt53xx_nor, 0);

	mt53xx_nor->cur_flash_index = 0;
 	
	if(0 != mt53xx_nor_WriteProtect(mt53xx_nor, 1))
 	{
        	printf( "Flash #%d: write protect disable failed!\n", mt53xx_nor->cur_flash_index);
	 
 	}
	else
	{
		printf("Flash #%d: write protect enable successfully\n", mt53xx_nor->cur_flash_index);
	}

 	mt53xx_nor->host_act_attr = SFLASH_USE_DUAL_READ; // no DMA/ISR in Uboot
 	_SetFlashEnter4Byte(mt53xx_nor);

	return 0;
}

#define MTK_NOR_DBG_CMD

static int mtk_nor_command(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned int addr;
	int len, i, retlen = 0;
	u8 *p = NULL;

	if (!strncmp(argv[1], "id", 3)) {
		SFLASHHW_VENDOR_T tmp_flash_info;
		u8 cmdval = 0x9F;
		SFLASH_WREG8(SFLASH_PRGDATA5_REG, cmdval);
		SFLASH_WREG8(SFLASH_PRGDATA4_REG, 0x00);
		SFLASH_WREG8(SFLASH_PRGDATA3_REG, 0x00);
		SFLASH_WREG8(SFLASH_PRGDATA2_REG, 0x00);
		SFLASH_WREG8(SFLASH_CNT_REG, 32);

		mt53xx_nor->cur_flash_index = 0;
		mt53xx_nor->cur_cmd_val = 0x04;

		if(0 != mt53xx_nor_ExecuteCmd(mt53xx_nor))
		{
			return -EIO;
		}

		tmp_flash_info.u1DevID2 = SFLASH_RREG8(SFLASH_SHREG0_REG);
		tmp_flash_info.u1DevID1 = SFLASH_RREG8(SFLASH_SHREG1_REG);	
		tmp_flash_info.u1MenuID = SFLASH_RREG8(SFLASH_SHREG2_REG);
		u4MenuID=tmp_flash_info.u1MenuID;

		mt53xx_nor->cur_cmd_val = 0x00;
		mt53xx_nor_SendCmd(mt53xx_nor);
		printf( "Flash MenuID: %02x, DevID1: %02x, DevID2: %02x\n", 
                                            tmp_flash_info.u1MenuID,
                                            tmp_flash_info.u1DevID1,
                                            tmp_flash_info.u1DevID2);

	}
	else if (!strncmp(argv[1], "read", 5)) {
		addr = (unsigned int)simple_strtoul(argv[2], NULL, 16);
		len = (int)simple_strtoul(argv[3], NULL, 16);
		p = (u8 *)malloc(len);
		if (!p) {
			printf("malloc failed\n");
			return 0;
		}
		if (mtk_nor_read(addr, len, &retlen, p))
		{
			free(p);
			printf("read fail\n");
			return 0;
		}
		printf("read len: %d\n", retlen);
		for (i = 0; i < retlen; i++)
		{
			printf("%02x ", p[i]);
			if (!((i+1) & 0x1f))
				printf("\n");
		}
		printf("\n");
		free(p);

	}
	else if (!strncmp(argv[1], "erase", 6)) {
		addr = (unsigned int)simple_strtoul(argv[2], NULL, 16);
		len = (int)simple_strtoul(argv[3], NULL, 16);
		if (mtk_nor_erase(addr, len))
			printf("Erase Fail\n");
	}
	else if (!strncmp(argv[1], "write", 6)) {
		u8 *porig;
		u8 t[3] = {0};
		addr = (unsigned int)simple_strtoul(argv[2], NULL, 16);
		len = strlen(argv[3]) / 2;

		porig = malloc(len+32);
		if (!porig)
			printf("malloc fail\n");
		p = (((u32)porig+31)/32)*32;
		for (i = 0; i < len; i++) {
			t[0] = argv[3][2*i];
			t[1] = argv[3][2*i+1];
			*(p + i) = simple_strtoul(t, NULL, 16);
		}
		mtk_nor_write(addr, len, &retlen, p);

		free(porig);
	}
	else if (!strncmp(argv[1], "init", 5)) {
		mtk_nor_init();
	}
	else
		printf("Usage:\n%s\n use \"help nor\" for detail!\n", cmdtp->usage);
					     
	return 0;
}


#ifdef MTK_NOR_DBG_CMD

static int mtk_nor_command(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

U_BOOT_CMD(
		nor,   4,      1,      mtk_nor_command,
		"nor   - nor flash command\n",
		"nor usage:\n"
		"  nor id\n"
		"  nor read <addr> <len>\n"
		"  nor write <addr> <data...>\n"
		"  nor erase <addr> <len>\n"
		"  nor init\n"
	  );

#endif // MTK_NOR_DBG_CMD

static int mtk_uboot_nor_command(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned int addr;
	int len, i, retlen = 0;
	u8 *p = NULL;

	if (!strncmp(argv[1], "read", 5)) {
		p = (u8 *)simple_strtoul(argv[2], NULL, 16);
		addr = (unsigned int)simple_strtoul(argv[3], NULL, 16);
		len = (int)simple_strtoul(argv[4], NULL, 16);
		if (!p) {
			printf("malloc failed\n");
			return 0;
		}
		if (mtk_nor_read(addr, len, &retlen, p))
		{
			printf("read fail\n");
			return 0;
		}
		printf("read len: %d\n", retlen);
#if 0
		for (i = 0; i < retlen; i++)
		{
			printf("%02x ", p[i]);
			if (!((i+1) & 0x1f))
				printf("\n");
		}
		printf("\n");
#endif
	}
	else if (!strncmp(argv[1], "erase", 6)) {
		addr = (unsigned int)simple_strtoul(argv[2], NULL, 16);
		len = (int)simple_strtoul(argv[3], NULL, 16);
		if (mtk_nor_erase(addr, len))
			printf("Erase Fail\n");
	}
	else if (!strncmp(argv[1], "write", 6)) {
		p = (u8 *)simple_strtoul(argv[2], NULL, 16);
		addr = (unsigned int)simple_strtoul(argv[3], NULL, 16);
		len = (int)simple_strtoul(argv[4], NULL, 16);
		mtk_nor_write(addr, len, &retlen, p);
	}
	else
		printf("Usage:\n%s\n use \"help nor\" for detail!\n", cmdtp->usage);
					     
	return 0;
}

U_BOOT_CMD(
		snor,   5,      1,      mtk_uboot_nor_command,
		"snor   - spi-nor flash command\n",
		"snor usage:\n"
		"  snor read <load_addr> <addr> <len>\n"
		"  snor write <load_addr> <addr> <len>\n"
		"  snor erase <addr> <len>\n"
	  );

#endif // defined(__UBOOT_NOR__)


