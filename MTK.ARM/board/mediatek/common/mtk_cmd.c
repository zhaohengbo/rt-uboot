#include <common.h>
#include <config.h>
#include <command.h>
#include <image.h>

#if defined(ON_BOARD_NAND_FLASH_COMPONENT)
#include <asm/arch/nand/partition_define.h>
#endif

#define PART_MAGIC          0x58881688
#define PART_FDT_MAGIC      0xD00DFEED

ulong ctp_load_addr = 0x40004400;


typedef union
{
    struct
    {
        unsigned int magic;     /* partition magic */
        unsigned int dsize;     /* partition data size */
        char name[32];          /* partition name */
        unsigned int maddr;     /* partition memory address */
        unsigned int mode;      /* memory addressing mode */
    } info;
    unsigned char data[512];
} part_hdr_t;

int do_read_image_blks (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    unsigned long blk_size;
    unsigned long img_blks;
	char *endp;
    image_header_t *img_hdr = (image_header_t *)load_addr;
    unsigned long img_size = 0;
    char s[32];
#if defined(CONFIG_FIT)
    unsigned long img_addr = (unsigned long)load_addr;
#endif

	if ((argc < 2) || (argc > 3))
		goto usage;

    blk_size = simple_strtoul(argv[1], &endp, 10);
    if (blk_size < 1)
    {
        printf("Block Size should be positive numble.\n");
        return 0;
    }
    
    if (argc == 3)
    {
        img_size = simple_strtoul(argv[2], &endp, 16);

        if (img_size < 1)
        {
            printf("Image Size should be positive numble.\n");
            return 0;
        }
    }

#if defined(CONFIG_FIT)
	if (PART_FDT_MAGIC != (unsigned long) fdt_magic(img_addr))
	{
		printf("[%s]Bad Magic Number = 0x%x\n", 
			__func__, (unsigned long) fdt_magic(img_addr));
		
		return 0;
	}

    /* 
     * If img_size is large than 0, it specifies that user assigns a image size.
     * Or we read image size from image header.
     */
    if (img_size == 0)
    {
	    img_size = (unsigned long) fdt_totalsize(img_addr);
    }
	printf("[%s]img_size = 0x%x\n", __func__, img_size);
#else
    if ( uimage_to_cpu(img_hdr->ih_magic) != IH_MAGIC )
    {
#if defined (CONFIG_MTK_MTD_NAND)		
	 	if (*((unsigned long*)load_addr) != PART_MAGIC)
#endif
		{	
			printf("Bad Magic Number.\n");
			sprintf((char*) s,"%lX", (unsigned long)0);
			setenv("img_blks", (const char *)s);
#if defined (CONFIG_MTK_MTD_NAND)
			sprintf((char*) s,"%lX", (unsigned long)0);
			setenv("img_align_size", (const char *)s);
#endif

			return 0;
		}
#if defined (CONFIG_MTK_MTD_NAND)		
		else
        {      
			img_size = getenv_ulong("filesize", 16, 0);
        }
#endif
    }
	else
    {   
        /* 
         * If img_size is large than 0, it specifies that user assigns a image size.
         * Or we read image size from image header.
         */
        if (img_size == 0)
        {
            img_size = uimage_to_cpu(img_hdr->ih_size)+sizeof(image_header_t);
        }

    }
#endif	/* CONFIG_FIT */

	img_blks = (img_size + (blk_size - 1))/blk_size;

    sprintf((char*) s,"%lX", img_blks);
    setenv("img_blks", (const char *)s);
#if defined (CONFIG_MTK_MTD_NAND) || defined(ON_BOARD_SPI_NOR_FLASH_COMPONENT)
	sprintf((char*) s,"%lX", img_blks*blk_size);
	setenv("img_align_size", (const char *)s);
	printf("[%s]img_blks = 0x%x\n", __func__, img_blks);
	printf("[%s]img_align_size = 0x%x\n", __func__, img_blks*blk_size);
#endif

    return 0;

usage:
	return CMD_RET_USAGE;    
}

int do_read_mtk_image_blks (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    unsigned long blk_size;
    unsigned long img_blks;
    char *endp;
    image_header_t *img_hdr = (image_header_t *)load_addr;
    unsigned long img_size;
    char s[32];
    part_hdr_t* img_addr = (part_hdr_t*)load_addr;

    int i = 0;

    if (argc != 2)
    	goto usage;
    
    blk_size = simple_strtoul(argv[1], &endp, 10);
    if (*argv[1] == 0 || *endp != 0)
    	goto usage;

    if(blk_size < 1)
    {
        printf("Block Size should be positive numble.\n");
        return 0;
    }

    if (PART_MAGIC != (unsigned long) img_addr->info.magic)
    {
    	printf("[%s]Bad Magic Number = 0x%x\n", 
    		__func__, (unsigned long) img_addr->info.magic);
    	
    	return 0;
    }
    printf("[%s]Good data[] = ", __func__);
    for(i=0; i<64; i++)
    	printf("0x%x ", (unsigned char) img_addr->data[i]);
    printf("\n");
    
    printf("[%s]Good Magic Number = 0x%x\n", 
    		__func__, (unsigned long) img_addr->info.magic);
    printf("[%s]Good Data size = 0x%x\n", 
    		__func__, (unsigned long) img_addr->info.dsize);
    printf("[%s]Good name = %s\n", 
    		__func__, (unsigned long) img_addr->info.name);
    img_size = (unsigned long) img_addr->info.dsize;
    img_size += 0x200;	/* NOTE(Nelson): Add FIT header size */
    printf("[%s]img_size = 0x%x\n", __func__, img_size);
    
	img_blks = (img_size + (blk_size - 1))/blk_size;

    sprintf((char*) s,"%lX", img_blks);
    setenv("img_blks", (const char *)s);
#if defined (CONFIG_MTK_MTD_NAND) || defined(ON_BOARD_SPI_NOR_FLASH_COMPONENT)
	sprintf((char*) s,"%lX", img_blks*blk_size);
	setenv("img_align_size", (const char *)s);
    	printf("[%s]img_align_size = %1X\n", __func__, img_blks*blk_size);
    img_size = (unsigned long) img_addr->info.dsize;
#endif

    return 0;

usage:
	return CMD_RET_USAGE;    
}

int do_image_check (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    image_header_t *img_hdr = (image_header_t *)load_addr;
    char s[32];
    
    if ( uimage_to_cpu(img_hdr->ih_magic) != IH_MAGIC )
    {
        printf("Image_Check Bad Magic Number.\n");        
        sprintf((char*) s, "%s", "bad");        
        setenv("img_result", (const char *)s);

        return 0;
    }

    sprintf((char*) s, "%s", "good");        
    setenv("img_result", (const char *)s);    

    return 0;
}

int do_serious_image_check (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	image_header_t *img_hdr = (image_header_t *)load_addr;

	image_header_t header;
	char s[32];
	__u32 chksum = 0, chksum2 = 0, len = 0; 

	printf("serious check\n"); 
	if ( uimage_to_cpu(img_hdr->ih_magic) != IH_MAGIC )
	{
		printf("Bad Magic Number.\n");        
		goto error; 

	} 
	/////////////////////////
	if (!image_check_hcrc(img_hdr)) {
		printf("Bad Header Checksum\n");
		goto error; 
	}
	//////////////////
	if (!image_check_dcrc(img_hdr)) {
		printf("Bad Data CRC\n");
		goto error; 
	}	


	sprintf((char*) s, "%s", "good");        
	setenv("img_result", (const char *)s);    

	return 0;

error:
	
	sprintf((char*) s, "%s", "bad");       
	setenv("img_result", (const char *)s);
	return 1;
}

int do_backup_message (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{

	printf("Recovery partition is empty. Copy kernel image to it!!!\n"); 
}

int do_reco_message (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{

	printf("Main kernel image is damaged. Recover kernel image!!!\n"); 
}

int do_uboot_check (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    unsigned long magic_no;    
    char s[32];
    
    magic_no = *(unsigned long*)load_addr;
    if ( magic_no != PART_MAGIC )
    {
        printf("Bad Magic Number.\n");        
        sprintf((char*) s, "%s", "bad");        
        setenv("uboot_result", (const char *)s);

        return 0;
    }

    sprintf((char*) s, "%s", "good");        
    setenv("uboot_result", (const char *)s);

    return 0;
}

#if defined (MT7622_FPGA_BOARD) || defined (MT7622_ASIC_BOARD)
int do_ctp_check (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{   
    char *s;
	int *t;
    int i = 0;
	s = (char*)ctp_load_addr;
	for(i=0; i<=31; i++)
    {
    	printf("%2x\t",s[i]);
		if((i+1)%8 == 0)
		{
			printf("\n");
		}
    }
    setenv("ctp_result", "good");

    return 0;
}

int do_boot_to_ctp (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{   
    void (*func)(void);
	int i;
	char *t; 
	t = (char*)ctp_load_addr;
    for(i=0; i<=31; i++)
    {
    	printf("%2x\t",t[i]);
		if((i+1)%8 == 0)
		{
			printf("\n");
		}
    }
	func = (void*)(ctp_load_addr);
	func();
    return 0;
}
#endif

U_BOOT_CMD(
	image_blks, 3,	1, do_read_image_blks,
	"read image size from img_size or image header if no specifying img_size, and divided by blk_size and save image blocks in image_blks variable.",
	"[blk_size]\n"
	"   - specify block size in decimal byte.\n"
	"[img_size]\n"
	"   - specify image size in hex byte."	
);

U_BOOT_CMD(
	mtk_image_blks, 2, 1, do_read_mtk_image_blks,
	"read image size from image header (MTK format) located at load_addr, divided by blk_size and save image blocks in image_blks variable.",
	"[blk_size]\n"
	"   - specify block size in byte."
);

U_BOOT_CMD(
	image_check, 2,	1, do_image_check,
	"check if image in load_addr is normal.",
	""
);
U_BOOT_CMD(
	serious_image_check, 2,	1, do_serious_image_check,
	"seriously check if image in load_addr is normal.",
	""
);

U_BOOT_CMD(
	backup_message, 2,	1, do_backup_message,
	"print backup message.",
	""
);

U_BOOT_CMD(
	reco_message, 2,	1, do_reco_message,
	"print recovery message.",
	""
);

U_BOOT_CMD(
	uboot_check, 2,	1, do_uboot_check,
	"check if uboot in load_addr is normal.",
	""
);

#if defined (MT7622_FPGA_BOARD) || defined (MT7622_ASIC_BOARD)
U_BOOT_CMD(
	ctp_check, 2,	1, do_ctp_check,
	"check if ctp in load_addr is normal.",
	""
);

U_BOOT_CMD(
	boot_to_ctp,	CONFIG_SYS_MAXARGS,	1,	do_boot_to_ctp,
	"boot to ctp", 
	""
);
#endif

