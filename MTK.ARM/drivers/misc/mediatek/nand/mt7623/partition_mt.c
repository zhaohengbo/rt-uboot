/******************************************************************************
* partition_mt.c - MT6516 NAND partition managment Driver
 *
* Copyright 2009-2010 MediaTek Co.,Ltd.
 *
* DESCRIPTION:
* 	This file provid the other drivers partition relative functions
 *
* modification history
* ----------------------------------------
* v1.0, 28 Feb 2011, mtk80134 written
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
#include <linux/mtd/nand_ecc.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/miscdevice.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>
#include <asm/cacheflush.h>
#include <asm/uaccess.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_clkmgr.h>
#include <mach/mtk_nand.h>
#include "board-custom.h"
#include "pmt.h"
//#include "partition.h"
#include <mach/board.h>
#else
#include <common.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <asm/io.h>
//#include <asm/cacheflush.h>
//#include <asm/uaccess.h>

#include <asm/arch/mt_typedefs.h>
#include <asm/arch/mt_clkmgr.h>
#include <asm/arch/nand/mtk_nand.h>
#include <asm/arch/nand/partition_define.h>
#include <asm/arch/nand/pmt.h>
#include <asm/arch/nand/partition_define_private.h>

#include "cust_part.h"


#endif
#define PMT 1
#ifdef PMT

extern bool MLC_DEVICE;// to build pass xiaolei

unsigned long long  partition_type_array[PART_MAX_COUNT];
#define REGION_LOW_PAGE 0x004C4F57
#define REGION_FULL_PAGE 0x46554C4C

pt_resident new_part[PART_MAX_COUNT];
pt_resident lastest_part[PART_MAX_COUNT];
unsigned char part_name[PART_MAX_COUNT][MAX_PARTITION_NAME_LEN];
#if defined (CONFIG_NAND_BOOTLOADER)
struct mtd_partition g_pasStatic_Partition[PART_MAX_COUNT];
int part_num=0;
struct excel_info *PartInfo;
#endif

#define MTD_SECFG_STR "seccnfg"
#define MTD_BOOTIMG_STR "boot"
#define MTD_ANDROID_STR "system"
#define MTD_SECRO_STR "secstatic"
#define MTD_USRDATA_STR "userdata"

int block_size;
int page_size;
pt_info pi;
u8 sig_buf[PT_SIG_SIZE];
//not support add new partition automatically.

//extern struct mtd_partition g_pasStatic_Partition[];
#if !defined (CONFIG_NAND_BOOTLOADER)
extern int part_num;
#endif

DM_PARTITION_INFO_PACKET pmtctl;
struct mtd_partition g_exist_Partition[PART_MAX_COUNT];

//#define LPAGE 2048
//char page_buf[LPAGE+64];
//char page_readbuf[LPAGE];
char *page_buf;
char *page_readbuf;

#define  PMT_MAGIC	 'p'
#define PMT_READ		_IOW(PMT_MAGIC, 1, int)
#define PMT_WRITE 		_IOW(PMT_MAGIC, 2, int)
#define PMT_VERSION 	_IOW(PMT_MAGIC, 3, int)


extern bool g_bInitDone;
extern struct mtk_nand_host *host;

#if defined(MTK_SPI_NAND_SUPPORT)
extern snand_flashdev_info devinfo;
#else
extern flashdev_info devinfo;
#endif
typedef u32 (*GetLowPageNumber)(u32 pageNo);
extern GetLowPageNumber functArray[];
#if 0
struct pmt_config
{
    int (*read_pmt) (char *buf);
    int (*write_pmt) (char *buf);
}
static struct pmt_config g_partition_fuc;
#endif
static bool init_pmt_done = FALSE;
int new_part_tab(u8 * buf, struct mtd_info *mtd);
int update_part_tab(struct mtd_info *mtd);
static int read_pmt(void __user *arg);
void get_part_tab_from_complier(void)
{
    int index=0;
#if defined (CONFIG_NAND_BOOTLOADER)
 		part_t *part = cust_part_tbl();
#endif
    printk(KERN_INFO "get_pt_from_complier \n");
#if defined (CONFIG_NAND_BOOTLOADER)
    while(part->flags!= PART_FLAG_END)
    {
				memset(&lastest_part[index], 0, sizeof(pt_resident));
        memcpy(lastest_part[index].name, part->name, MAX_PARTITION_NAME_LEN);
        
        lastest_part[index].size = part->size;
        //lastest_part[index].offset = part->startblk * PAGE_SIZE;
        //if (index > 0)
        //	lastest_part[index].offset = lastest_part[index-1].offset + lastest_part[index-1].size;
        
        if (!strcmp(PART_PRELOADER, lastest_part[index].name))
        	lastest_part[index].offset = PART_OFFSET_PRELOADER;
        if (!strcmp(PART_UBOOT, lastest_part[index].name))
        	lastest_part[index].offset = PART_OFFSET_UBOOT;
        if (!strcmp(PART_CONFIG, lastest_part[index].name))
        	lastest_part[index].offset = PART_OFFSET_CONFIG;
        if (!strcmp(PART_FACTORY, lastest_part[index].name))
        	lastest_part[index].offset = PART_OFFSET_FACTORY;
        if (!strcmp(PART_BOOTIMG, lastest_part[index].name))
        	lastest_part[index].offset = PART_OFFSET_BOOTIMG;
        if (!strcmp(PART_RECOVERY, lastest_part[index].name))
        	lastest_part[index].offset = PART_OFFSET_RECOVERY;
        if (!strcmp(PART_ROOTFS, lastest_part[index].name))
        	lastest_part[index].offset = PART_OFFSET_ROOTFS;
        if (!strcmp(PART_USER, lastest_part[index].name))
        	lastest_part[index].offset = PART_OFFSET_USRDATA;
        if (!strcmp(PART_BMTPOOL, lastest_part[index].name))
        	lastest_part[index].offset = PART_OFFSET_BMTPOOL;
		if (MLC_DEVICE==TRUE)						
	        lastest_part[index].part_id = ((part->flags==TYPE_LOW) ? REGION_LOW_PAGE : REGION_FULL_PAGE);
		else
			lastest_part[index].part_id = 0;
		lastest_part[index].mask_flags = PART_FLAG_NONE;  //this flag in kernel should be fufilled even though in flash is 0.
        printk(KERN_INFO "get_ptr  %s offset=%llx size=%llx partsize=%x\n",lastest_part[index].name,lastest_part[index].offset, lastest_part[index].size,part->size);
        index++; part++;
    }
#endif
	
#if 0
    while (g_pasStatic_Partition[index].size != MTDPART_SIZ_FULL)   //the last partition sizefull ==0
    {

        memcpy(lastest_part[index].name, g_pasStatic_Partition[index].name, MAX_PARTITION_NAME_LEN);
        lastest_part[index].size = g_pasStatic_Partition[index].size;
        	lastest_part[index].offset = g_pasStatic_Partition[index].offset;
        lastest_part[index].mask_flags = g_pasStatic_Partition[index].mask_flags;   //this flag in kernel should be fufilled even though in flash is 0.
        printk(KERN_INFO "get_ptr  %s %llx \n", lastest_part[index].name, lastest_part[index].offset);
        index++;
    }
    //get last partition info
    memcpy(lastest_part[index].name, g_pasStatic_Partition[index].name, MAX_PARTITION_NAME_LEN);
    lastest_part[index].size = g_pasStatic_Partition[index].size;
    lastest_part[index].offset = g_pasStatic_Partition[index].offset;
    lastest_part[index].mask_flags = g_pasStatic_Partition[index].mask_flags;   //this flag in kernel should be fufilled even though in flash is 0.
    printk(KERN_INFO "get_ptr  %s %llx \n", lastest_part[index].name, lastest_part[index].offset);
#endif
}

u64 part_get_startaddress(u64 byte_address, u32 *idx)
{
    int index = 0;
	if(TRUE == init_pmt_done)
	{
    while (index < part_num) {
		//MSG(INIT, "g_exist_Partition[%d].offset %x\n",index, g_exist_Partition[index].offset);
		if(g_exist_Partition[index].offset > byte_address)
		{
			//MSG(INIT, "find g_exist_Partition[%d].offset %x\n",index-1, g_exist_Partition[index-1].offset);
			*idx = index-1;
			return g_exist_Partition[index-1].offset;
		}
        index++;
    }
	}
#if defined (CONFIG_NAND_BOOTLOADER)
	else
	{
		*idx = PART_MAX_COUNT -1;
		return byte_address;
	}
#endif
	*idx = part_num-1;
    return byte_address;
}

bool raw_partition(u32 index)
{
	if(partition_type_array[index] == REGION_LOW_PAGE)
		return TRUE;
	
	return FALSE;
}

int find_empty_page_from_top(u64 start_addr, struct mtd_info *mtd)
{
    int page_offset;//,i;
    u64 current_add;
//#if defined(MTK_MLC_NAND_SUPPORT)
    int i;
//#endif
    struct mtd_oob_ops ops_pt;
    struct erase_info ei;

    ei.mtd = mtd;
    ei.len = mtd->erasesize;
    ei.time = 1000;
    ei.retries = 2;
    ei.callback = NULL;

    ops_pt.datbuf = (uint8_t *) page_buf;
    ops_pt.mode = MTD_OPS_AUTO_OOB;
    ops_pt.len = mtd->writesize;
    ops_pt.retlen = 0;
    ops_pt.ooblen = 16;
    ops_pt.oobretlen = 0;
    ops_pt.oobbuf = page_buf + page_size;
    ops_pt.ooboffs = 0;
    memset(page_buf, 0xFF, page_size + mtd->oobsize);
    memset(page_readbuf, 0xFF, page_size);
    //mt6577_nand_erase(start_addr); //for test
//#if defined(MTK_MLC_NAND_SUPPORT)
//    for (page_offset = 0,i=0; page_offset < (block_size / page_size); page_offset = functArray[devinfo.feature_set.ptbl_idx](i++))
//#else
//    for (page_offset = 0; page_offset < (block_size / page_size); page_offset++)
//#endif
	for (page_offset = 0; page_offset < (block_size / page_size); )
    {
        current_add = start_addr + (page_offset * page_size);
        if (mtd->_read_oob(mtd, (loff_t) current_add, &ops_pt) != 0)
        {
            printk(KERN_INFO "find_emp read failed %llx \n", current_add);
            //continue;
			goto NEXT;
        } else
        {
            if (memcmp(page_readbuf, page_buf, page_size) || memcmp(page_buf + page_size, page_readbuf, 32))
            {
                //continue;
				goto NEXT;
            } else
            {
                printk(KERN_INFO "find_emp  at %x \n", page_offset);
                break;
            }

        }
NEXT:
		if (MLC_DEVICE==TRUE)
			page_offset = functArray[devinfo.feature_set.ptbl_idx](i++);
		else
			page_offset++;	
    }
    printk(KERN_INFO "find_emp find empty at %x \n", page_offset);

    //i=(0x40);
    //printk (KERN_INFO "test code %x \n",i);
    //page_offset = 0x40;
    if (page_offset != 0x40)
    {
        printk(KERN_INFO "find_emp at  %x\n", page_offset);
        return page_offset;
    } else
    {
        printk(KERN_INFO "find_emp no empty \n");
	ei.addr =  start_addr;
        if (mtd->_erase(mtd, &ei) != 0)
        {                       //no good block for used in replace pool
            printk(KERN_INFO "find_emp erase mirror failed %llx\n", start_addr);
            pi.mirror_pt_has_space = 0;
            return 0xFFFF;
        } else
        {
            return 0;           //the first page is empty
        }

    }
}

bool find_mirror_pt_from_bottom(u64 *start_addr, struct mtd_info * mtd)
{
    int mpt_locate,i;
    u64 mpt_start_addr;
    u64 current_start_addr = 0;
    u8 pmt_spare[4];
    struct mtd_oob_ops ops_pt;
#if defined (CONFIG_NAND_BOOTLOADER)
    mpt_start_addr = (mtd->size)-(u64)(((PMT_POOL_SIZE)+(g_bmt_sz)) * block_size) + block_size;//((mtd->size) + block_size);
#else
mpt_start_addr = ((mtd->size) + block_size);
#endif
   //mpt_start_addr=MPT_LOCATION*block_size-page_size;
    memset(page_buf, 0xFF, page_size + mtd->oobsize);

    ops_pt.datbuf = (uint8_t *) page_buf;
    ops_pt.mode = MTD_OPS_AUTO_OOB;
    ops_pt.len = mtd->writesize;
    ops_pt.retlen = 0;
    ops_pt.ooblen = 16;
    ops_pt.oobretlen = 0;
    ops_pt.oobbuf = page_buf + page_size;
    ops_pt.ooboffs = 0;
    printk(KERN_INFO "find_mirror find begain at %llx \n", mpt_start_addr);
	
    for (mpt_locate = ((block_size / page_size) - 1),i = ((block_size / page_size) - 1); mpt_locate >= 0; mpt_locate--)//mpt_locate--)
    {
        memset(pmt_spare, 0xFF, PT_SIG_SIZE);

        current_start_addr = mpt_start_addr + mpt_locate * page_size;
        if (mtd->_read_oob(mtd, (loff_t) current_start_addr, &ops_pt) != 0)
        {
            printk(KERN_INFO "find_mirror read  failed %llx %x \n", current_start_addr, mpt_locate);
        }
        memcpy(pmt_spare, &page_buf[page_size], PT_SIG_SIZE);   //auto do need skip bad block
        //need enhance must be the larget sequnce number
#if 0
        {
            int i;
            for (i = 0; i < 8; i++)
            {
                printk(KERN_INFO "%x %x \n", page_buf[i], page_buf[2048 + i]);
            }

        }
#endif
        if (is_valid_mpt(page_buf) && is_valid_mpt(pmt_spare))
        {
            //if no pt, pt.has space is 0;
            pi.sequencenumber = page_buf[PT_SIG_SIZE + page_size];
            printk(KERN_INFO "find_mirror find valid pt at %llx sq %x \n", current_start_addr, pi.sequencenumber);
            break;
        } else
        {
            continue;
        }
    }
    if (mpt_locate == -1)
    {
        printk(KERN_INFO "no valid mirror page\n");
        pi.sequencenumber = 0;
        return FALSE;
    } else
    {
        *start_addr = current_start_addr;
        return TRUE;
    }
}

//int load_exist_part_tab(u8 *buf,struct mtd_info *mtd)
int load_exist_part_tab(u8 * buf)
{
    u64 pt_start_addr;
    u64 pt_cur_addr;
    int pt_locate,i;
    int reval = DM_ERR_OK;
    u64 mirror_address;

    //u8 pmt_spare[PT_SIG_SIZE];
    struct mtd_oob_ops ops_pt;
    struct mtd_info *mtd;
    mtd = &host->mtd;

	
		block_size = mtd->erasesize;    // devinfo.blocksize*1024;
    page_size = mtd->writesize; // devinfo.pagesize;
    //if(host->hw->nand_sec_shift == 10) //MLC
    //	block_size = block_size >> 1;
#if defined (CONFIG_NAND_BOOTLOADER)
    pt_start_addr = (mtd->size)-(u64)(((PMT_POOL_SIZE)+(g_bmt_sz)) * block_size) ;
#else
pt_start_addr = (mtd->size);
#endif
    //pt_start_addr=PT_LOCATION*block_size;
    printk(KERN_INFO "load_exist_part_tab %llx mtd->size=%llx block_size=%x\n", pt_start_addr,mtd->size , block_size);
    ops_pt.datbuf = (uint8_t *) page_buf;
    ops_pt.mode = MTD_OPS_PLACE_OOB;//MTD_OPS_AUTO_OOB;
    ops_pt.len = mtd->writesize;
    ops_pt.retlen = 0;
    ops_pt.ooblen = mtd->oobsize;//16;
    ops_pt.oobretlen = 0;
    ops_pt.oobbuf = (page_buf + page_size);
    ops_pt.ooboffs = 0;

    printk(KERN_INFO "ops_pt.len %x \n", ops_pt.len);
    if (mtd->_read_oob == NULL)
    {
        printk(KERN_INFO "shoud not happpen mtd=%08X host=%08X\n",mtd,host);
    }
	
    for (pt_locate = 0,i = 0; pt_locate < (block_size / page_size); pt_locate++)
    {
        pt_cur_addr = pt_start_addr + pt_locate * page_size;
        //memset(pmt_spare,0xFF,PT_SIG_SIZE);

        printk (KERN_INFO "load_pt read pt %llx writesize=%d oobsize=%d\n",pt_cur_addr,mtd->writesize,mtd->oobsize);

        if (mtd->_read_oob(mtd, (loff_t) pt_cur_addr, &ops_pt) != 0)
        {
            printk(KERN_INFO "load_pt read pt failded: %llx\n", (u64) pt_cur_addr);
        }
#if 0
        {
            int i;
            for (i = 0; i < 8; i++)
            {
                printk(KERN_INFO "%x %x \n", *(page_buf + i), *(page_buf + mtd->writesize + i));
            }

        }
#endif
        //memcpy(pmt_spare,&page_buf[LPAGE] ,PT_SIG_SIZE); //do not need skip bad block flag
        if (is_valid_pt(page_buf) && is_valid_pt(page_buf + mtd->writesize+1))
        {
            pi.sequencenumber = page_buf[PT_SIG_SIZE + page_size];
            printk(KERN_INFO "load_pt find valid pt at %llx sq %x \n", pt_start_addr, pi.sequencenumber);
            break;
        } else
        {
        		//printk("buf[%02x %02x %02x %02x]\n",page_buf[0],page_buf[1],page_buf[2],page_buf[3]);
            continue;
        }
    }
    //for test
    //pt_locate=(block_size/page_size);
    if (pt_locate == (block_size / page_size))
    {
        //first download or download is not compelte after erase or can not download last time
        printk(KERN_INFO "load_pt find pt failed \n");
        pi.pt_has_space = 0;    //or before download pt power lost

        if (!find_mirror_pt_from_bottom(&mirror_address, mtd))
        {
            printk(KERN_INFO "First time download \n");
            reval = ERR_NO_EXIST;
            return reval;
        } else
        {
            //used the last valid mirror pt, at lease one is valid.
            mtd->_read_oob(mtd, (loff_t) mirror_address, &ops_pt);
        }
    }
    memcpy(&lastest_part, &page_buf[PT_SIG_SIZE], sizeof(lastest_part));

    return reval;
}
#if !defined (CONFIG_NAND_BOOTLOADER)
static int pmt_open(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "[%s]:(MAJOR)%d:(MINOR)%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
    //filp->private_data = (int*);
    return 0;
}

static int pmt_release(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "[%s]:(MAJOR)%d:(MINOR)%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
    return 0;
}

static long pmt_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret = 0;               // , i=0;
    ulong version = PT_SIG;

    void __user *uarg = (void __user *)arg;
    printk(KERN_INFO "PMT IOCTL: Enter\n");


    if (false == g_bInitDone)
    {
        printk(KERN_INFO "ERROR: NAND Flash Not initialized !!\n");
        ret = -EFAULT;
        goto exit;
    }

    switch (cmd)
    {
      case PMT_READ:
          printk(KERN_INFO "PMT IOCTL: PMT_READ\n");
 	  ret = read_pmt(uarg);
          break;
      case PMT_WRITE:
          printk(KERN_INFO "PMT IOCTL: PMT_WRITE\n");
	    if (copy_from_user(&pmtctl, uarg, sizeof(DM_PARTITION_INFO_PACKET)))
	    {
		ret = -EFAULT;
		goto exit;
	    }
          new_part_tab((u8 *) & pmtctl, (struct mtd_info *)&host->mtd);
          update_part_tab((struct mtd_info *)&host->mtd);

          break;
	  case PMT_VERSION:
	  		if(copy_to_user((void __user *)arg,&version,PT_SIG_SIZE))
				ret= -EFAULT;
			else
				ret=0;
	  	break;
      default:
          ret = -EINVAL;
    }
  exit:
    return ret;
}
#endif
static int read_pmt(void __user *arg)
{
	printk(KERN_ERR "read_pmt\n");

	if(copy_to_user(arg,&lastest_part,sizeof(pt_resident)*PART_MAX_COUNT))
		return -EFAULT;
	return 0;
}
#if !defined (CONFIG_NAND_BOOTLOADER)
static struct file_operations pmt_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = pmt_ioctl,
    .open = pmt_open,
    .release = pmt_release,
};

static struct miscdevice pmt_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "pmt",
    .fops = &pmt_fops,
};
#endif
static int lowercase(int c)
{
	if((c >= 'A') && (c <= 'Z'))
		c += 'a' - 'A';
	return c;
}

void construct_mtd_partition(struct mtd_info *mtd)
{
	int i,j;
	
	 if (PartInfo==NULL)
    {
        PartInfo = kzalloc(PART_NUM * sizeof(struct excel_info), GFP_KERNEL);
        if (!PartInfo) {
            printk("%s: malloc PartInfo fail\n",__FILE__);
            return;
        }
        memcpy(PartInfo, PartInfo_Private, PART_NUM * sizeof(struct excel_info));
    }
	for(i = 0; i < PART_MAX_COUNT; i++)
	{
		if(lastest_part[i].size == 0)
			break;
		for(j=0; j < MAX_PARTITION_NAME_LEN; j++)
		{
			if(lastest_part[i].name[j] == 0)
				break;
			if (j>0)
			part_name[i][j] = lowercase(lastest_part[i].name[j]);
			else
				part_name[i][j] = lastest_part[i].name[j];
		}
		PartInfo[i].name = part_name[i];
		g_exist_Partition[i].name = part_name[i];
		if(!strcmp(lastest_part[i].name,"SECCFG"))
			g_exist_Partition[i].name = MTD_SECFG_STR;

		if(!strcmp(lastest_part[i].name,"BOOTIMG"))
			g_exist_Partition[i].name = MTD_BOOTIMG_STR;
		
		if(!strcmp(lastest_part[i].name,"SEC_RO"))
			g_exist_Partition[i].name = MTD_SECRO_STR;
		
		if(!strcmp(lastest_part[i].name,"ANDROID"))
			g_exist_Partition[i].name = MTD_ANDROID_STR;
		
		if(!strcmp(lastest_part[i].name,"USRDATA"))
			g_exist_Partition[i].name = MTD_USRDATA_STR;
		
		g_exist_Partition[i].size = (u_int32_t) lastest_part[i].size;//mtd partition
        g_exist_Partition[i].offset = (u_int32_t) lastest_part[i].offset;
		
		g_exist_Partition[i].mask_flags = lastest_part[i].mask_flags; 

		//PartInfo[i].name = part_name[i];  //dumchar
		PartInfo[i].type = NAND;
		PartInfo[i].start_address = lastest_part[i].offset;
		PartInfo[i].size = lastest_part[i].size;
		partition_type_array[i]= lastest_part[i].part_id;
		printk("partition %s %s offset %llx size=%llx ID=%08X\n", lastest_part[i].name,PartInfo[i].name, g_exist_Partition[i].offset,PartInfo[i].size,partition_type_array[i]);

		#if 1
		if(MLC_DEVICE == TRUE)
		{
			mtd->eraseregions[i].offset = lastest_part[i].offset;
			mtd->eraseregions[i].erasesize = mtd->erasesize;
			
			//QWERT Force UBOOT R/W only LOWPAGE :
			partition_type_array[i] = REGION_LOW_PAGE;
			if(partition_type_array[i] == REGION_LOW_PAGE)
			{
				mtd->eraseregions[i].erasesize = mtd->erasesize/2;
			}
			
			mtd->numeraseregions++;
		}
		#endif
	}
	part_num = i;
	g_exist_Partition[i-1].size = MTDPART_SIZ_FULL;
}

void part_init_pmt(struct mtd_info *mtd, u8 * buf)
{
    struct mtd_partition *part;
    u64 lastblk;
    int retval = 0;
    int i = 0;
    int err = 0;
    printk(KERN_INFO "part_init_pmt  %s\n", __TIME__);
    page_buf = kzalloc(mtd->writesize + mtd->oobsize+64, GFP_KERNEL);
    page_readbuf = kzalloc(mtd->writesize+64, GFP_KERNEL);
	#if 0
    part = &g_pasStatic_Partition[0];
    lastblk = part->offset + part->size;
    printk(KERN_INFO "offset  %llx part->size %llx %s\n", part->offset, part->size, part->name);

    while (1)
    {
        part++;
		if(part->offset == MTDPART_OFS_APPEND)
        part->offset = lastblk;
        lastblk = part->offset + part->size;
        printk(KERN_INFO "mt_part_init_pmt %llx\n", part->offset);
        if (part->size == 0)    ////the last partition sizefull ==0
        {
            break;
        }
    }
	#endif

    memset(&pi, 0xFF, sizeof(pi));
    memset(&lastest_part, 0, PART_MAX_COUNT * sizeof(pt_resident));
    retval = load_exist_part_tab(buf);

    if (retval == ERR_NO_EXIST) //first run preloader before dowload
    {
        //and valid mirror last download or first download 
        printk(KERN_INFO "%s no pt \n", __func__);
        get_part_tab_from_complier();   //get from complier
#if defined (CONFIG_NAND_BOOTLOADER)
        construct_mtd_partition(mtd);
        new_part_tab(NULL, mtd);
        update_part_tab(mtd);
#endif
        if(MLC_DEVICE == TRUE)
			mtd->numeraseregions = 0;
		for (i = 0; i < part_num; i++)
		{
			#if 1
			if(MLC_DEVICE == TRUE)
			{
				mtd->eraseregions[i].offset = lastest_part[i].offset;
				mtd->eraseregions[i].erasesize = mtd->erasesize;
				if(partition_type_array[i] == REGION_LOW_PAGE)
				{
					mtd->eraseregions[i].erasesize = mtd->erasesize/2;
				}
				
				mtd->numeraseregions++;
			}
			#endif
        }
    } else
    {
        printk(KERN_INFO "Find pt or mpt \n");
		if(MLC_DEVICE == TRUE)
			mtd->numeraseregions = 0;
		#if 0
        memcpy(&g_exist_Partition, &g_pasStatic_Partition, sizeof(struct mtd_partition) * part_num);
        for (i = 0; i < part_num; i++)
        {
            printk(KERN_INFO "partition %s size %llx %llx \n", lastest_part[i].name, lastest_part[i].offset, lastest_part[i].size);
            //still use the name in g_pasSatic_partition
            g_exist_Partition[i].size =  lastest_part[i].size;
            g_exist_Partition[i].offset =  lastest_part[i].offset;
			#if 1
			if(MLC_DEVICE == TRUE)
			{
				mtd->eraseregions[i].offset = lastest_part[i].offset;
				mtd->eraseregions[i].erasesize = mtd->erasesize;
				#if 0//to build pass xiaolei
				if(partition_type_array[i] == TYPE_RAW)
				{
					mtd->eraseregions[i].erasesize = mtd->erasesize/2;
				}
				#endif
				mtd->numeraseregions++;
			}
			#endif
            //still use the mask flag in g_pasSatic_partition
            if (i == (part_num - 1))
            {
                g_exist_Partition[i].size = MTDPART_SIZ_FULL;
            }
            printk(KERN_INFO "partition %s size %llx\n", lastest_part[i].name, g_exist_Partition[i].offset);
        }
		#endif
		construct_mtd_partition(mtd);
    }
	init_pmt_done = TRUE;
#if !defined (CONFIG_NAND_BOOTLOADER)
    printk(KERN_INFO ": register NAND PMT device ...\n");
#ifndef MTK_EMMC_SUPPORT

    err = misc_register(&pmt_dev);
    if (unlikely(err))
    {
        printk(KERN_INFO "PMT failed to register device!\n");
        //return err;
    }
#endif
#endif
}

int new_part_tab(u8 * buf, struct mtd_info *mtd)
{
    DM_PARTITION_INFO_PACKET *dm_part = (DM_PARTITION_INFO_PACKET *) buf;
    int part_num, change_index, i = 0;
    int retval;
    int pageoffset;
#if defined (CONFIG_NAND_BOOTLOADER)
    u64 start_addr = (mtd->size)-(u64)(((PMT_POOL_SIZE)+(g_bmt_sz)) * block_size) + block_size;// (u64)((mtd->size) + block_size);
#else
u64 start_addr = (u64)((mtd->size) + block_size);
#endif
    u64 current_addr = 0;
    struct mtd_oob_ops ops_pt;

    pi.pt_changed = 0;
    pi.tool_or_sd_update = 2;   //tool download is 1.

    ops_pt.mode = MTD_OPS_AUTO_OOB;
    ops_pt.len = mtd->writesize;
    ops_pt.retlen = 0;
    ops_pt.ooblen = 16;
    ops_pt.oobretlen = 0;
    ops_pt.oobbuf = page_buf + page_size;
    ops_pt.ooboffs = 0;
    //the first image is ?
#if defined (CONFIG_NAND_BOOTLOADER)
		if (dm_part)
		{	
#endif
	    for (part_num = 0; part_num < PART_MAX_COUNT; part_num++)
	    {
	        memcpy(new_part[part_num].name, dm_part->part_info[part_num].part_name, MAX_PARTITION_NAME_LEN);
	        new_part[part_num].offset = dm_part->part_info[part_num].start_addr;
	        new_part[part_num].size = dm_part->part_info[part_num].part_len;
	        new_part[part_num].mask_flags = 0;
	        //MSG (INIT, "DM_PARTITION_INFO_PACKET %s size %x %x \n",dm_part->part_info[part_num].part_name,dm_part->part_info[part_num].part_len,part_num);
	        printk(KERN_INFO "new_pt %s size %llx \n", new_part[part_num].name, new_part[part_num].size);
	        if (dm_part->part_info[part_num].part_len == 0)
	        {
	            printk(KERN_INFO "new_pt last %x \n", part_num);
	            break;
	        }
	    }
#if defined (CONFIG_NAND_BOOTLOADER)
		}
		else
		{	
			start_addr-=block_size;
			memcpy(new_part, lastest_part, sizeof(new_part));
			pi.pt_changed = 1;
		}
#endif
    //++++++++++for test
#if 0
    part_num = 13;
    memcpy(&new_part[0], &lastest_part[0], sizeof(new_part));
    MSG(INIT, "new_part  %x size  \n", sizeof(new_part));
    for (i = 0; i < part_num; i++)
    {
        MSG(INIT, "npt partition %s size  \n", new_part[i].name);
        //MSG (INIT, "npt %x size  \n",new_part[i].offset);
        //MSG (INIT, "npt %x size  \n",lastest_part[i].offset);
        //MSG (INIT, "npt %x size  \n",new_part[i].size);
        dm_part->part_info[5].part_visibility = 1;
        dm_part->part_info[5].dl_selected = 1;
        new_part[5].size = lastest_part[5].size + 0x100000;
    }
#endif
    //------------for test
    //Find the first changed partition, whether is visible
    for (change_index = 0; change_index <= part_num; change_index++)
    {
        if ((new_part[change_index].size != lastest_part[change_index].size) || (new_part[change_index].offset != lastest_part[change_index].offset))
        {
            printk(KERN_INFO "new_pt %x size changed from %llx to %llx\n", change_index, lastest_part[change_index].size, new_part[change_index].size);
            pi.pt_changed = 1;
            break;
        }
    }

    if (pi.pt_changed == 1)
    {
        //Is valid image update
#if defined (CONFIG_NAND_BOOTLOADER)
        if (dm_part)
        {	
#endif
	        for (i = change_index; i <= part_num; i++)
	        {
	
	            if (dm_part->part_info[i].dl_selected == 0 && dm_part->part_info[i].part_visibility == 1)
	            {
	                printk(KERN_INFO "Full download is need %x \n", i);
	                retval = DM_ERR_NO_VALID_TABLE;
	                return retval;
	            }
	        }
#if defined (CONFIG_NAND_BOOTLOADER)
				}
#endif
        pageoffset = find_empty_page_from_top(start_addr, mtd);
        //download partition used the new partition
        //write mirror at the same 2 page
        memset(page_buf, 0xFF, page_size + 64);
        *(int *)sig_buf = MPT_SIG;
        memcpy(page_buf, &sig_buf, PT_SIG_SIZE);
        memcpy(&page_buf[PT_SIG_SIZE], &new_part[0], sizeof(new_part));
        memcpy(&page_buf[page_size], &sig_buf, PT_SIG_SIZE);
        pi.sequencenumber += 1;
        memcpy(&page_buf[page_size + PT_SIG_SIZE], &pi, PT_SIG_SIZE);

        if (pageoffset != 0xFFFF)
        {
            if ((pageoffset % 2) != 0)
            {
                printk(KERN_INFO "new_pt mirror block may destroy last time%x\n", pageoffset);
                pageoffset += 1;
            }
            for (i = 0; i < 2; i++)
            {
                current_addr = start_addr + (pageoffset + i) * page_size;
                ops_pt.datbuf = (uint8_t *) page_buf;
                if (mtd->_write_oob(mtd, (loff_t) current_addr, &ops_pt) != 0)
                {
                    printk(KERN_INFO "new_pt write m first page failed %llx\n", current_addr);
                } else
                {
                    printk(KERN_INFO "new_pt write mirror at %llx\n", current_addr);
                    ops_pt.datbuf = (uint8_t *) page_readbuf;
                    //read back verify
                    if ((mtd->_read_oob(mtd, (loff_t) current_addr, &ops_pt) != 0) || memcmp(page_buf, page_readbuf, page_size))
                    {
                        printk(KERN_INFO "new_pt read or verify first mirror page failed %llx \n", current_addr);
                        ops_pt.datbuf = (uint8_t *) page_buf;
                        memset(page_buf, 0, PT_SIG_SIZE);
                        if (mtd->_read_oob(mtd, (loff_t) current_addr, &ops_pt) != 0)
                        {
                            printk(KERN_INFO "new_pt mark failed %llx\n", current_addr);
                        }
                    } else
                    {
                        printk(KERN_INFO "new_pt write mirror ok %x\n", i);
                        //any one success set this flag?
                        pi.mirror_pt_dl = 1;
                    }
                }
            }
        }
    } else
    {
        printk(KERN_INFO "new_part_tab no pt change %x\n", i);
    }

    retval = DM_ERR_OK;
    return retval;
}

int update_part_tab(struct mtd_info *mtd)
{
    int retval = 0;
    int retry_w;
    int retry_r;
#if defined (CONFIG_NAND_BOOTLOADER)
    u64 start_addr = (mtd->size)-(u64)(((PMT_POOL_SIZE)+(g_bmt_sz)) * block_size);//(u64)(mtd->size);  //PT_LOCATION*block_size;
#else
u64 start_addr = (u64)(mtd->size);  //PT_LOCATION*block_size;
#endif
    u64 current_addr = 0;
    struct erase_info ei;
    struct mtd_oob_ops ops_pt;

    memset(page_buf, 0xFF, page_size + 64);

    ei.mtd = mtd;
    ei.len =  mtd->erasesize;
    ei.time = 1000;
    ei.retries = 2;
    ei.callback = NULL;

    ops_pt.mode = MTD_OPS_AUTO_OOB;
    ops_pt.len = mtd->writesize;
    ops_pt.retlen = 0;
    ops_pt.ooblen = 16;
    ops_pt.oobretlen = 0;
    ops_pt.oobbuf = page_buf + page_size;
    ops_pt.ooboffs = 0;

    if ((pi.pt_changed == 1 || pi.pt_has_space == 0) && pi.tool_or_sd_update == 2)
    {
        printk(KERN_INFO "update_pt pt changes\n");

        ei.addr = start_addr;
        if (mtd->_erase(mtd, &ei) != 0)
        {                       //no good block for used in replace pool
            printk(KERN_INFO "update_pt erase failed %llx\n", start_addr);
            if (pi.mirror_pt_dl == 0)
                retval = DM_ERR_NO_SPACE_FOUND;
            return retval;
        }

        for (retry_r = 0; retry_r < RETRY_TIMES; retry_r++)
        {
            for (retry_w = 0; retry_w < RETRY_TIMES; retry_w++)
            {
                current_addr = start_addr + (retry_w + retry_r * RETRY_TIMES) * page_size;
                *(int *)sig_buf = PT_SIG;
                memcpy(page_buf, &sig_buf, PT_SIG_SIZE);
                memcpy(&page_buf[PT_SIG_SIZE], &new_part[0], sizeof(new_part));
                memcpy(&page_buf[page_size], &sig_buf, PT_SIG_SIZE);
                memcpy(&page_buf[page_size + PT_SIG_SIZE], &pi, PT_SIG_SIZE);

                ops_pt.datbuf = (uint8_t *) page_buf;
                if (mtd->_write_oob(mtd, (loff_t) current_addr, &ops_pt) != 0)
                {               //no good block for used in replace pool . still used the original ones
                    printk(KERN_INFO "update_pt write failed %x\n", retry_w);
                    memset(page_buf, 0, PT_SIG_SIZE);
                    if (mtd->_write_oob(mtd, (loff_t) current_addr, &ops_pt) != 0)
                    {
                        printk(KERN_INFO "write error mark failed\n");
                        //continue retry
                        continue;
                    }
                } else
                {
                    printk(KERN_INFO "write pt success %llx %x \n", current_addr, retry_w);
                    break;      // retry_w should not count.
                }
            }
            if (retry_w == RETRY_TIMES)
            {
                printk(KERN_INFO "update_pt retry w failed\n");
                if (pi.mirror_pt_dl == 0)   //mirror also can not write down
                {
                    retval = DM_ERR_NO_SPACE_FOUND;
                    return retval;
                } else
                {
                    return DM_ERR_OK;
                }
            }
            current_addr = (start_addr + (((retry_w) + retry_r * RETRY_TIMES) * page_size));
            ops_pt.datbuf = (uint8_t *) page_readbuf;
            if ((mtd->_read_oob(mtd, (loff_t) current_addr, &ops_pt) != 0) || memcmp(page_buf, page_readbuf, page_size))
            {

                printk(KERN_INFO "v or r failed %x\n", retry_r);
                memset(page_buf, 0, PT_SIG_SIZE);
                ops_pt.datbuf = (uint8_t *) page_buf;
                if (mtd->_write_oob(mtd, (loff_t) current_addr, &ops_pt) != 0)
                {
                    printk(KERN_INFO "read error mark failed\n");
                    //continue retryp
                    continue;
                }

            } else
            {
                printk(KERN_INFO "update_pt r&v ok%llx\n", current_addr);
                break;
            }
        }
    } else
    {
        printk(KERN_INFO "update_pt no change \n");
    }
    return DM_ERR_OK;
}

int get_part_num_nand()
{
	return part_num;
}

EXPORT_SYMBOL(get_part_num_nand);

#endif /* #ifdef PMT */
