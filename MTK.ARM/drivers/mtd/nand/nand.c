/*
 * (C) Copyright 2005
 * 2N Telekomunikace, a.s. <www.2n.cz>
 * Ladislav Michl <michl@2n.cz>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <nand.h>
#include <errno.h>

#ifndef CONFIG_SYS_NAND_BASE_LIST
#define CONFIG_SYS_NAND_BASE_LIST { CONFIG_SYS_NAND_BASE }
#endif

#if defined (CONFIG_MTK_MTD_NAND)
#include <linux/mtd/partitions.h>
#include <asm/arch/nand/partition_define.h>
#include <asm/arch/nand/bmt.h>
extern struct mtd_partition g_exist_Partition[PART_MAX_COUNT];
extern int part_num;
extern u32 g_bmt_sz;
extern bmt_struct *g_bmt;
#endif /* CONFIG_MTK_MTD_NAND */ 

DECLARE_GLOBAL_DATA_PTR;

int nand_curr_device = -1;


nand_info_t nand_info[CONFIG_SYS_MAX_NAND_DEVICE];

#ifndef CONFIG_SYS_NAND_SELF_INIT
static struct nand_chip nand_chip[CONFIG_SYS_MAX_NAND_DEVICE];
static ulong base_address[CONFIG_SYS_MAX_NAND_DEVICE] = CONFIG_SYS_NAND_BASE_LIST;
#endif

static char dev_name[CONFIG_SYS_MAX_NAND_DEVICE][8];

static unsigned long total_nand_size; /* in kiB */

/* Register an initialized NAND mtd device with the U-Boot NAND command. */
int nand_register(int devnum)
{
	struct mtd_info *mtd;

	if (devnum >= CONFIG_SYS_MAX_NAND_DEVICE)
		return -EINVAL;

	mtd = &nand_info[devnum];

	sprintf(dev_name[devnum], "nand%d", devnum);
	mtd->name = dev_name[devnum];

#ifdef CONFIG_MTD_DEVICE
	/*
	 * Add MTD device so that we can reference it later
	 * via the mtdcore infrastructure (e.g. ubi).
	 */
	add_mtd_device(mtd);
#endif

	total_nand_size += mtd->size / 1024;

	if (nand_curr_device == -1)
		nand_curr_device = devnum;

	return 0;
}

#define UBOOT_NAND_UNIT_TEST	0
#if UBOOT_NAND_UNIT_TEST
#define SNAND_MAX_PAGE_SIZE	(4096)
#define _SNAND_CACHE_LINE_SIZE  (64)
static u8 *local_buffer_16_align;   // 16 byte aligned buffer, for HW issue
__attribute__((aligned(_SNAND_CACHE_LINE_SIZE))) static u8 local_buffer[SNAND_MAX_PAGE_SIZE + _SNAND_CACHE_LINE_SIZE];
static int g_page_size = 2;
static int g_block_size = 128;
int mtk_nand_unit_test_(struct nand_chip *nand_chip, struct mtd_info *mtd)
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
    printk("[P] 0x%x\n", p);
    u32 val = 0x05, TOTAL=1000;

    local_buffer_16_align = local_buffer;

    for (j = 0x400; j< 0x410; j++)
    {
        memset(local_buffer_16_align, 0x00, SNAND_MAX_PAGE_SIZE + _SNAND_CACHE_LINE_SIZE);
        nand_chip->read_page(mtd, nand_chip, local_buffer_16_align, j*p);
        MSG(INIT,"[1]0x%x %x %x %x\n", *(int *)local_buffer_16_align, *((int *)local_buffer_16_align+1), *((int *)local_buffer_16_align+2), *((int *)local_buffer_16_align+3));
        nand_chip->erase(mtd, j*p);
        memset(local_buffer_16_align, 0x00, SNAND_MAX_PAGE_SIZE + _SNAND_CACHE_LINE_SIZE);
        if(nand_chip->read_page(mtd, nand_chip, local_buffer_16_align, j*p))
            printk("Read page 0x%x fail!\n", j*p);
        MSG(INIT,"[2]0x%x %x %x %x\n", *(int *)local_buffer_16_align, *((int *)local_buffer_16_align+1), *((int *)local_buffer_16_align+2), *((int *)local_buffer_16_align+3));
        if (nand_chip->block_bad(mtd, j*g_block_size, 0))
        {
            printk("Bad block at %x\n", j);
            continue;
        }
	//printk("[%s]mtk_nand_block_bad() PASS!!!\n", __func__);
	k = 0;
        {
		if(nand_chip->write_page(mtd, nand_chip, (u8 *)patternbuff, 0, j*p+k, 0, 0))
			printk("Write page 0x%x fail!\n", j*p+k);
        }
	//printk("[%s]mtk_nand_write_page() PASS!!!\n", __func__);

	memset(local_buffer_16_align, 0x00, g_page_size);
	if(nand_chip->read_page(mtd, nand_chip, local_buffer_16_align, j*p+k))
		printk("Read page 0x%x fail!\n", j*p+k);
	MSG(INIT,"[3]0x%x %x %x %x\n", *(int *)local_buffer_16_align, *((int *)local_buffer_16_align+1), *((int *)local_buffer_16_align+2), *((int *)local_buffer_16_align+3));
	if(memcmp((u8 *)patternbuff, local_buffer_16_align, 128*4))
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
    return err;
}    
#endif	/* UBOOT_NAND_UNIT_TEST */

#ifndef CONFIG_SYS_NAND_SELF_INIT
static void nand_init_chip(int i)
{
	struct mtd_info *mtd = &nand_info[i];
	struct nand_chip *nand = &nand_chip[i];
	ulong base_addr = base_address[i];
	int maxchips = CONFIG_SYS_NAND_MAX_CHIPS;
	int err;

	if (maxchips < 1)
		maxchips = 1;

	mtd->priv = nand;
	nand->IO_ADDR_R = nand->IO_ADDR_W = (void  __iomem *)base_addr;

	if (board_nand_init(nand))
		return;
#if defined (CONFIG_MTK_MTD_NAND)
	memcpy(mtd, nand->priv, sizeof(struct mtd_info));
#endif
	if (nand_scan(mtd, maxchips))
		return;

	nand_register(i);
	
#if defined (CONFIG_MTK_MTD_NAND)	
	memcpy(nand->priv, mtd, sizeof(struct mtd_info));
	if (!(g_bmt = init_bmt(nand, g_bmt_sz)))
        {
		printf("[%s]Error: init bmt failed\n", __func__);
        }
	part_init_pmt(mtd, (u8 *) & g_exist_Partition[0]);
	add_mtd_partitions(mtd, g_exist_Partition, part_num);

#if UBOOT_NAND_UNIT_TEST
	err = mtk_nand_unit_test_(nand, mtd);
	if (err == 0)
	{
		printk("Thanks to GOD, UNIT Test OK!\n");
	}
#endif	/* UBOOT_NAND_UNIT_TEST */
#endif	
}
#endif

void nand_init(void)
{
#ifdef CONFIG_SYS_NAND_SELF_INIT
	board_nand_init();
#else
	int i;

	for (i = 0; i < CONFIG_SYS_MAX_NAND_DEVICE; i++)
		nand_init_chip(i);
#endif

	printf("%lu MiB\n", total_nand_size / 1024);

#ifdef CONFIG_SYS_NAND_SELECT_DEVICE
	/*
	 * Select the chip in the board/cpu specific driver
	 */
	board_nand_select_device(nand_info[nand_curr_device].priv, nand_curr_device);
#endif
}
