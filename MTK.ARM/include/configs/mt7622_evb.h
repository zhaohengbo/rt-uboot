/*
 * (C) Copyright 2012 Stephen Warren
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>
#include <configs/autoconf.h>

#define DEBUG

/**********************************************************************************************
 *                                      ARM Cortex A7
 **********************************************************************************************/
#if 1
/*
 *  Iverson 20140326 : 
 *      We can disable dcache for accessing non-cachable address instead of using mmu page table to provide noncachable address.
 */
/* Cache */
// Iverson debug
#define CONFIG_SYS_DCACHE_OFF
#endif

/* Machine ID */
#define CONFIG_MACH_TYPE                    7622

/**********************************************************************************************
 *                                          Memory
 **********************************************************************************************/
/* Memory layout */
/* DRAM definition */
/* 
 * Iverson 20140521 : We detect ram size automatically.
 *      CONFIG_SYS_SDRAM_SIZE define max uboot size.
 *      The max size that auto detection support is 256MB.
 */
#define CONFIG_NR_DRAM_BANKS		        1
#define CONFIG_SYS_SDRAM_BASE		        0x40000000
#if defined(ON_BOARD_1024Mb_DRAM_COMPONENT)
#define CONFIG_SYS_SDRAM_SIZE               SZ_128M
#elif defined(ON_BOARD_2048Mb_DRAM_COMPONENT) 
#define CONFIG_SYS_SDRAM_SIZE               SZ_256M
#elif defined(ON_BOARD_4096Mb_DRAM_COMPONENT)
#define CONFIG_SYS_SDRAM_SIZE               SZ_512M
#elif defined(ON_BOARD_8192Mb_DRAM_COMPONENT)
#define CONFIG_SYS_SDRAM_SIZE               SZ_1G
#else
#error "memory defined error!!!")
#endif

/* Code Layout */
//#define CONFIG_SYS_TEXT_BASE		        0x80000000
#define CONFIG_SYS_TEXT_BASE		        0x41E00000

/* Uboot definition */
#define CONFIG_SYS_UBOOT_BASE		        CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_UBOOT_MAX_SIZE           SZ_2M
#define CONFIG_SYS_INIT_SP_ADDR             (CONFIG_SYS_TEXT_BASE + \
                                                CONFIG_SYS_UBOOT_MAX_SIZE - \
                                                GENERATED_GBL_DATA_SIZE)


#if (CONFIG_SYS_SDRAM_SIZE == SZ_128M) || (CONFIG_SYS_SDRAM_SIZE == SZ_256M) || \
    (CONFIG_SYS_SDRAM_SIZE == SZ_512M) || (CONFIG_SYS_SDRAM_SIZE == SZ_1G)
#if defined(MT7622_FPGA_BOARD)
#define CONFIG_SYS_MALLOC_LEN               SZ_4M
#else
#define CONFIG_SYS_MALLOC_LEN               SZ_32M
#endif

/* RichOS memory partitions */
#define CONFIG_SYS_DECOMP_ADDR              0x40080000
#if defined(MT7622_FPGA_BOARD)
#define CONFIG_SYS_LOAD_ADDR                0x4007FF28
#else
#define CONFIG_SYS_LOAD_ADDR                0x4007FF28
#endif
#define CONFIG_SYS_IMAGE_HDR_ADDR           CONFIG_SYS_LOAD_ADDR

/* Linux DRAM definition */
#define CONFIG_LOADADDR			            CONFIG_SYS_LOAD_ADDR

/*
 * For booting Linux, the board info and command line data
 * have to be in the first 64 MB of memory, since this is
 * the maximum mapped by the Linux kernel during initialization.
 */
#define CONFIG_SYS_BOOTM_LEN	            0x4000000
#else
#error "memory partition error!!!"

#endif

/**********************************************************************************************
 *                                           Board
 **********************************************************************************************/

/* Board */
#if defined(MT7622_FPGA_BOARD)
#define CFG_FPGA_PLATFORM   1
#endif
#define CONFIG_ADD_MTK_HEADER               "y"

#define CONFIG_BOARD_LATE_INIT

/**********************************************************************************************
 *                                          Devices
 **********************************************************************************************/

/********************** Flash *************************/
#if defined(ON_BOARD_SPI_NOR_FLASH_COMPONENT)
/* SPI Flash Configuration */

/* Parallel Nor Flash */
#define CONFIG_SYS_NO_FLASH

/* SPI NOR Flash */
#define CONFIG_CMD_NOR

#define CONFIG_ENV_IS_IN_NOR

#define CONFIG_ENV_SIZE                     SZ_4K
#define CONFIG_ENV_SECT_SIZE                0x10000	/* 64KB */
#define CONFIG_ENV_OFFSET                   0xC0000

#define CONFIG_NOR_HEADER_OFFSET           0x00000
#define CONFIG_NOR_PL_OFFSET               0x00600

#define CONFIG_NOR_UBOOT_OFFSET            0x40000
#define CONFIG_NOR_LINUX_OFFSET            0x140000
#define CONFIG_NOR_ATF_OFFSET              0x1740000
#define CONFIG_NOR_CTP_OFFSET              0x140000

#define ENV_BOOT_WRITE_IMAGE \
	"boot_wr_img=image_blks " __stringify(CONFIG_ENV_SECT_SIZE) \
	";snor erase " __stringify(CONFIG_NOR_LINUX_OFFSET) " ${img_align_size}" \
	";image_blks 1" \
	";snor write ${loadaddr} " __stringify(CONFIG_NOR_LINUX_OFFSET) " ${img_align_size}\0"
#define ENV_BOOT_READ_IMAGE \
    	"boot_rd_img=snor read ${loadaddr} " __stringify(CONFIG_NOR_LINUX_OFFSET) " 2000" \
	";image_blks 1" \
	";snor read ${loadaddr} " __stringify(CONFIG_NOR_LINUX_OFFSET) " ${img_align_size}\0"
#define ENV_WRITE_UBOOT \
	"wr_uboot=mtk_image_blks " __stringify(CONFIG_ENV_SECT_SIZE) \
	";snor erase " __stringify(CONFIG_NOR_UBOOT_OFFSET) " ${img_align_size} "\
	";mtk_image_blks 1" \
	";snor write ${loadaddr} " __stringify(CONFIG_NOR_UBOOT_OFFSET) " ${img_align_size}\0"
#define ENV_WRITE_ATF \
	"wr_atf=mtk_image_blks " __stringify(CONFIG_ENV_SECT_SIZE) \
	";snor erase " __stringify(CONFIG_NOR_ATF_OFFSET) " ${img_align_size} "\
	";mtk_image_blks 1" \
	";snor write ${loadaddr} " __stringify(CONFIG_NOR_ATF_OFFSET) " ${img_align_size}\0"
#define ENV_WRITE_PRELOADER \
	"wr_pl=snor erase 0x10000 " __stringify(CONFIG_ENV_SECT_SIZE) \
	";snor write ${loadaddr} " __stringify(CONFIG_NOR_PL_OFFSET) " 0x18000\0"
#define ENV_WRITE_ROM_HEADER \
	"wr_rom_hdr=snor erase " __stringify(CONFIG_NOR_HEADER_OFFSET) " " __stringify(CONFIG_ENV_SECT_SIZE) \
	";snor write ${loadaddr} " __stringify(CONFIG_NOR_HEADER_OFFSET) " 0x600\0"
#define ENV_WRITE_CTP \
		"wr_ctp=snor erase " __stringify(CONFIG_NOR_CTP_OFFSET) " 0x500000 "\
		";snor write ${loadaddr} " __stringify(CONFIG_NOR_CTP_OFFSET) " 0x500000\0" 	
#define ENV_BOOT_READ_CTP \
		"boot_rd_ctp=snor read 0x40000000 " __stringify(CONFIG_NOR_CTP_OFFSET) " 0x500000\0"


#elif defined(ON_BOARD_NAND_FLASH_COMPONENT) || \
      defined(ON_BOARD_SPI_NAND_FLASH_COMPONENT)
#define CONFIG_SYS_NO_FLASH

/* NAND Flash Configuration */
#define CONFIG_SYS_MAX_NAND_DEVICE	        1	
#define CONFIG_SYS_NAND_BASE                NFI_BASE
#define CONFIG_SYS_MAX_FLASH_SECT			256	
#define CONFIG_SYS_MAX_FLASH_BANKS			1
#define CONFIG_CMD_NAND
//#define CONFIG_MTD_DEBUG	/* NOTE(Nelson): add debug logs */
//#define CONFIG_MTD_DEBUG_VERBOSE  7
#define CONFIG_CMD_MTDPARTS     1
#define CONFIG_MTD_PARTITIONS   1
#define CONFIG_ENV_IS_IN_NAND
#define CONFIG_MTD_NAND_VERIFY_WRITE            1

#if defined (CONFIG_MTK_MLC_NAND_SUPPORT)
#define CONFIG_ENV_SIZE                     SZ_4K
#define CONFIG_NAND_HEADER_OFFSET              0x0000000
#define CONFIG_NAND_PL_OFFSET                  0x0800000
#define CONFIG_NAND_UBOOT_OFFSET               0x1000000
#define CONFIG_ENV_OFFSET                      0x2000000
#define CONFIG_NAND_LINUX_OFFSET               0x4000000
#define CONFIG_NAND_ATF_OFFSET                 0x63000000
#define CONFIG_MAX_NAND_PAGE_SIZE               8192
#define CONFIG_MAX_NAND_BLOCK_SIZE              2097152
#define CONFIG_MAX_UBOOT_SIZE                   393216  //262144
#define CONFIG_NAND_CTP_OFFSET                 0x4000000
#else
#define CONFIG_ENV_SIZE                     SZ_4K
#define CONFIG_NAND_HEADER_OFFSET           0x00000
#define CONFIG_NAND_PL_OFFSET               0x20000
#define CONFIG_NAND_UBOOT_OFFSET            0x40000
#define CONFIG_ENV_OFFSET                   0xC0000
#define CONFIG_NAND_LINUX_OFFSET            0x140000
#define CONFIG_NAND_ATF_OFFSET              0x2140000
#define CONFIG_MAX_NAND_PAGE_SIZE               2048
#define CONFIG_MAX_NAND_BLOCK_SIZE              131072
#define CONFIG_MAX_UBOOT_SIZE                           393216  //262144
#define CONFIG_NAND_CTP_OFFSET              0x140000
#endif

#define ENV_BOOT_WRITE_IMAGE \
	    "boot_wr_img=image_blks " __stringify(CONFIG_MAX_NAND_BLOCK_SIZE) \
		";nand erase " __stringify(CONFIG_NAND_LINUX_OFFSET) "  ${img_align_size}" \
		";image_blks " __stringify(CONFIG_MAX_NAND_PAGE_SIZE) \
		";nand write ${loadaddr} " __stringify(CONFIG_NAND_LINUX_OFFSET) " ${img_align_size}\0"	
#define ENV_BOOT_READ_IMAGE \
    "boot_rd_img=nand read ${loadaddr} " __stringify(CONFIG_NAND_LINUX_OFFSET) " 2000" \
	";image_blks " __stringify(CONFIG_MAX_NAND_PAGE_SIZE) \
	";nand read ${loadaddr} " __stringify(CONFIG_NAND_LINUX_OFFSET) " ${img_align_size}\0"
#define ENV_WRITE_UBOOT \
	"wr_uboot=mtk_image_blks " __stringify(CONFIG_MAX_NAND_BLOCK_SIZE) \
	";nand erase " __stringify(CONFIG_NAND_UBOOT_OFFSET) "  ${img_align_size} "\
	";mtk_image_blks " __stringify(CONFIG_MAX_NAND_PAGE_SIZE) \
	";nand write ${loadaddr} " __stringify(CONFIG_NAND_UBOOT_OFFSET) " ${img_align_size}\0"
#define ENV_WRITE_ATF \
	"wr_atf=mtk_image_blks " __stringify(CONFIG_MAX_NAND_BLOCK_SIZE) \
	";nand erase " __stringify(CONFIG_NAND_ATF_OFFSET) "	${img_align_size} "\
	";mtk_image_blks " __stringify(CONFIG_MAX_NAND_PAGE_SIZE) \
	";nand write ${loadaddr} " __stringify(CONFIG_NAND_ATF_OFFSET) " ${img_align_size}\0"
#define ENV_WRITE_PRELOADER \
	"wr_pl=nand erase " __stringify(CONFIG_NAND_PL_OFFSET) " 20000 "\
	";nand write ${loadaddr} " __stringify(CONFIG_NAND_PL_OFFSET) " 20000\0"
#define ENV_WRITE_ROM_HEADER \
	"wr_rom_hdr=nand erase " __stringify(CONFIG_NAND_HEADER_OFFSET) " 20000 "\
	";nand write ${loadaddr} " __stringify(CONFIG_NAND_HEADER_OFFSET) " 20000\0"
#define ENV_WRITE_CTP \
	"wr_ctp=nand erase " __stringify(CONFIG_NAND_CTP_OFFSET) " 3000000 "\
	";nand write ${loadaddr} " __stringify(CONFIG_NAND_CTP_OFFSET) " 3000000\0"		
#define ENV_BOOT_READ_CTP \
	"boot_rd_ctp=nand read 0x40000000 " __stringify(CONFIG_NAND_CTP_OFFSET) " 3000000\0"

#elif defined(ON_BOARD_EMMC_COMPONENT)
/*EMMC Configuration*/
/* Parallel Nor Flash */
/* Build error elimination*/
#define CONFIG_SYS_NO_FLASH
/**/
#define CONFIG_CMD_MMC
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_ENV_SIZE                     SZ_4K
#define CONFIG_ENV_OFFSET                   0x23C00
#define CONFIG_SYS_MMC_ENV_DEV              0
#define CONFIG_SYS_MAX_FLASH_SECT           512

#define CONFIG_EMMC_UBOOT_BLOCK             0xe900
#define CONFIG_MAX_UBOOT_SIZE               0x200           // 0x200 * 512 = 256KB
#define CONFIG_EMMC_LINUX_BLOCK             0xec00
#define CONFIG_EMMC_CTP_BLOCK               0x20000  //CTP
#define CONFIG_MAX_CTP_SIZE                 0x30000  //CTP  0x30000 * 512 = 48MB
#define ENV_BOOT_WRITE_IMAGE \
    "boot_wr_img=image_blks " __stringify(CONFIG_SYS_MAX_FLASH_SECT) \
    "  ${filesize};mmc device 0;mmc write $(loadaddr) " __stringify(CONFIG_EMMC_LINUX_BLOCK) " $(img_blks)\0"
#define ENV_BOOT_READ_IMAGE \
    "boot_rd_img=mmc device 0;mmc read $(loadaddr) " __stringify(CONFIG_EMMC_LINUX_BLOCK) " 1" \
    ";image_blks " __stringify(CONFIG_SYS_MAX_FLASH_SECT) \
    ";mmc read $(loadaddr) " __stringify(CONFIG_EMMC_LINUX_BLOCK) " $(img_blks)\0"
#define ENV_WRITE_UBOOT \
    "wr_uboot=mmc device 0;mmc write $(loadaddr) " __stringify(CONFIG_EMMC_UBOOT_BLOCK) \
    " " __stringify(CONFIG_MAX_UBOOT_SIZE) "\0"
#define ENV_WRITE_ATF \
	"wr_atf=mmc device 1;mmc write $(loadaddr) " __stringify(CONFIG_EMMC_ATF_BLOCK) \
	" " __stringify(CONFIG_MAX_ATF_SIZE) "\0"
#define ENV_WRITE_PRELOADER \
	"wr_pl=mmc device 1;mmc write $(loadaddr) " __stringify(CONFIG_EMMC_PL_BLOCK) \
	" " __stringify(CONFIG_MAX_PL_SIZE) "\0"
#define ENV_WRITE_ROM_HEADER \
	"wr_rom_hdr=mmc device 1;mmc write $(loadaddr) " __stringify(CONFIG_EMMC_HEADER_BLOCK) \
	" " __stringify(CONFIG_MAX_HEADER_SIZE) "\0"
#define ENV_WRITE_CTP \
    "wr_ctp=ctp_check;if test ${ctp_result} = good; then mmc device 0;mmc write ${loadaddr} " __stringify(CONFIG_EMMC_CTP_BLOCK) \
    " " __stringify(CONFIG_MAX_CTP_SIZE) ";reset; fi\0"

#define ENV_BOOT_READ_CTP \
    "boot_rd_ctp=mmc device 0;mmc read ${loadaddr} " __stringify(CONFIG_EMMC_CTP_BLOCK) \
    " " __stringify(CONFIG_MAX_CTP_SIZE) "\0"

#elif defined(OFF_BOARD_SD_CARD_COMPONENT)
/*SD card configuration*/
/* Build error elimination*/
#define CONFIG_CMD_MMC
#define CONFIG_SYS_NO_FLASH
/**/
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_ENV_IS_IN_SD
#define CONFIG_ENV_SIZE                     SZ_128K
#define CONFIG_ENV_OFFSET                   0xD2800 
#define CONFIG_SYS_MMC_ENV_DEV              1

#define ENV_BOOT_FROM_SD  \
    "boot5=mmc init; fatload mmc 0:1 0x84000000 uimage ; bootm\0"

#define CONFIG_SYS_MAX_FLASH_SECT           512

#define CONFIG_EMMC_UBOOT_BLOCK             0xe900
#define CONFIG_MAX_UBOOT_SIZE               0x200           // 0x200 * 512 = 256KB
#define CONFIG_EMMC_LINUX_BLOCK             0xec00
#define CONFIG_EMMC_LINUX_SIZE              0xa0000         //0xa0000*512 =20M
#define CONFIG_EMMC_PL_BLOCK                0x18C00
#define CONFIG_EMMC_ATF_BLOCK               0x18E00
#define CONFIG_EMMC_HEADER_BLOCK            0x0
#define CONFIG_EMMC_CTP_BLOCK               0xec00

#define ENV_BOOT_WRITE_IMAGE \
    "boot_wr_img=image_blks " __stringify(CONFIG_SYS_MAX_FLASH_SECT) \
    "  ${filesize};mmc device 1;mmc write $(loadaddr) " __stringify(CONFIG_EMMC_LINUX_BLOCK) " $(img_blks)\0"
#define ENV_BOOT_READ_IMAGE \
    "boot_rd_img=mmc device 1;mmc read $(loadaddr) " __stringify(CONFIG_EMMC_LINUX_BLOCK) " 1" \
    ";image_blks " __stringify(CONFIG_SYS_MAX_FLASH_SECT) \
    ";mmc read $(loadaddr) " __stringify(CONFIG_EMMC_LINUX_BLOCK) " $(img_blks)\0"
#define ENV_WRITE_UBOOT \
    "wr_uboot=mmc device 1;mmc write $(loadaddr) " __stringify(CONFIG_EMMC_UBOOT_BLOCK) \
    " " __stringify(CONFIG_MAX_UBOOT_SIZE) "\0"
#define ENV_WRITE_ATF \
	"wr_atf=mmc device 1;mmc write $(loadaddr) " __stringify(CONFIG_EMMC_ATF_BLOCK) \
	" " __stringify(CONFIG_MAX_ATF_SIZE) "\0"
#define ENV_WRITE_PRELOADER \
	"wr_pl=mmc device 1;mmc write $(loadaddr) " __stringify(CONFIG_EMMC_PL_BLOCK) \
	" " __stringify(CONFIG_MAX_PL_SIZE) "\0"
#define ENV_WRITE_ROM_HEADER \
	"wr_rom_hdr=mmc device 1;mmc write $(loadaddr) " __stringify(CONFIG_EMMC_HEADER_BLOCK) \
	" " __stringify(CONFIG_MAX_HEADER_SIZE) "\0"
#define ENV_WRITE_CTP \
	"wr_ctp=mmc device 1;mmc write $(loadaddr) " __stringify(CONFIG_EMMC_CTP_BLOCK) \
	" " __stringify(CONFIG_MAX_CTP_SIZE) "\0"
#define ENV_BOOT_READ_CTP \
	"boot_rd_ctp=mmc device 1;mmc read $(loadaddr) " __stringify(CONFIG_EMMC_CTP_BLOCK) \
	" " __stringify(CONFIG_MAX_CTP_SIZE) "\0"



#else
#define CONFIG_SYS_NO_FLASH
#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_SIZE                     SZ_16K

#define ENV_BOOT_WRITE_IMAGE "boot_wr_img=none\0"
#define ENV_BOOT_READ_IMAGE "boot_rd_img=none\0"
#define ENV_WRITE_UBOOT "wr_uboot=none\0"
#define ENV_WRITE_ATF
#define ENV_WRITE_PRELOADER
#define ENV_WRITE_ROM_HEADER
#define ENV_WRITE_CTP
#define ENV_BOOT_READ_CTP

#endif

#define CONFIG_ENV_VARS_UBOOT_CONFIG

#if defined(ON_BOARD_EMMC_COMPONENT) || defined(OFF_BOARD_SD_CARD_COMPONENT)
/********************** MMC ***********************************/
#define PART_DEBUG
#define CONFIG_MMC
#define CONFIG_FS_FAT
#define CONFIG_CMD_FAT
#define CONFIG_MEDIATEK_MMC

#define CONFIG_DOS_PARTITION
#endif

/********************** Watchdog *************************/
#define CFG_HW_WATCHDOG 1
#define CONFIG_WATCHDOG_OFF


/********************** Console UART *************************/
/* Uart baudrate */
#define CONFIG_BAUDRATE                     921600
 
/* Console configuration */
#define CONFIG_SYS_CBSIZE		            1024
#define CONFIG_SYS_PBSIZE		            (CONFIG_SYS_CBSIZE +		\
					                            sizeof(CONFIG_SYS_PROMPT) + 16)
//#define CONFIG_CONSOLE_MUX
#define CONFIG_SYS_CONSOLE_IS_IN_ENV

#define ENV_DEVICE_SETTINGS \
	"stdin=serial\0" \
	"stdout=serial\0" \
	"stderr=serial\0"

/********************** Ethernet *************************/
#define CONFIG_ETHADDR                      00:0C:E7:11:22:33
#define CONFIG_IPADDR                       10.10.10.254
#define CONFIG_SERVERIP                     10.10.10.3
#define CONFIG_BOOTFILE                     "iverson_uImage"
#define CONFIG_CMD_NET
#define CONFIG_RT2880_ETH

#define RALINK_REG(x)		(*((volatile u32 *)(x)))


/**********************************************************************************************
 *                                       Boot Menu
 **********************************************************************************************/
#if 1
#if defined(MT7622_FPGA_BOARD)
#define CONFIG_BOOTDELAY                    5
#else
#define CONFIG_BOOTDELAY                    3
#endif
#define CONFIG_BOOTCOMMAND                  "No"
#define CONFIG_CMD_BOOTMENU
#define CONFIG_MENU
#define CONFIG_MENU_SHOW
#define CONFIG_MENU_ACTIVE_ENTRY            2

#define ENV_BOOT_CMD0 \
    "boot0=tftpboot; bootm\0"

#define ENV_BOOT_CMD1 \
    "boot1=tftpboot;run boot_wr_img;run boot_rd_img;bootm\0"

#define ENV_BOOT_CMD2 \
    "boot2=run boot_rd_img;bootm\0"

#define ENV_BOOT_CMD3 \
    "boot3=tftpboot $(loadaddr) u-boot-mtk.bin;run wr_uboot;reset\0"

#define ENV_BOOT_CMD4 \
    "boot4=go " __stringify(CONFIG_SYS_TEXT_BASE) "\0"

#define ENV_BOOT_CMD5 \
    "boot5=tftpboot $(loadaddr) trustzone.bin;run wr_atf;reset\0"

#define ENV_BOOT_CMD6 \
    "boot6=tftpboot $(loadaddr) preloader_fpga7622_64_ldvt.bin;run wr_pl;reset\0"

#define ENV_BOOT_CMD7 \
    "boot7=tftpboot $(loadaddr) hdr.binary;run wr_rom_hdr;reset\0"
#define ENV_BOOT_CMD8 \
    "boot8=tftpboot ${loadaddr} ctp.bin;run wr_ctp\0"

#define ENV_BOOT_CMD9 \
    "boot9=run boot_rd_ctp;boot_to_ctp\0"

#define ENV_BOOT_CMD \
    ENV_BOOT_WRITE_IMAGE \
    ENV_BOOT_READ_IMAGE \
    ENV_WRITE_UBOOT \
    ENV_WRITE_ATF \
    ENV_WRITE_PRELOADER \
    ENV_WRITE_ROM_HEADER \
    ENV_WRITE_CTP \
    ENV_BOOT_READ_CTP \
    ENV_BOOT_CMD0 \
    ENV_BOOT_CMD1 \
    ENV_BOOT_CMD2 \
    ENV_BOOT_CMD3 \
    ENV_BOOT_CMD4 \
    ENV_BOOT_CMD5 \
    ENV_BOOT_CMD6 \
    ENV_BOOT_CMD7 \
	ENV_BOOT_CMD8 \
	ENV_BOOT_CMD9 

#define ENV_BOOT_MENU \
    "bootmenu_0=1. System Load Linux to SDRAM via TFTP.=run boot0\0" \
    "bootmenu_1=2. System Load Linux Kernel then write to Flash via TFTP.=run boot1\0" \
    "bootmenu_2=3. Boot system code via Flash.=run boot2\0" \
    "bootmenu_3=4. System Load Boot Loader then write to Flash via TFTP.=run boot3\0" \
    "bootmenu_4=5. System Load Boot Loader then write to Flash via Serial.=run boot4\0" \
    "bootmenu_5=6. System Load ATF then write to Flash via TFTP.=run boot5\0" \
    "bootmenu_6=7. System Load Preloader then write to Flash via TFTP.=run boot6\0" \
    "bootmenu_7=8. System Load ROM header then write to Flash via TFTP.=run boot7\0" \
	"bootmenu_8=9. System Load CTP then write to Flash via TFTP.=run boot8\0" \
	"bootmenu_9=10. Boot to CTP.=run boot9\0" \
    "bootmenu_delay=30\0" \
    ""

#define CONFIG_EXTRA_ENV_SETTINGS \
    "fdt_high=0xffffffff\0" \
	ENV_DEVICE_SETTINGS \
	ENV_BOOT_CMD \
	ENV_BOOT_MENU
#else
#define CONFIG_EXTRA_ENV_SETTINGS \
    "fdt_high=0xffffffff\0" \
	ENV_DEVICE_SETTINGS

#endif

/**********************************************************************************************
 *                                      FDT
 **********************************************************************************************/
#define CONFIG_FIT
#define CONFIG_OF_LIBFDT
#define CONFIG_FDT_DEBUG

/**********************************************************************************************
 *                                      ATF
 **********************************************************************************************/
#define CONFIG_MTK_ATF

/**********************************************************************************************
 *                                       UBoot Command
 **********************************************************************************************/
/* Shell */
#define CONFIG_SYS_MAXARGS		            8
#define CONFIG_SYS_PROMPT		            "MT7622> "
#define CONFIG_COMMAND_HISTORY

/* Commands */
#include <config_cmd_default.h>

/* Device tree support */
#define CONFIG_OF_BOARD_SETUP
/* ATAGs support for bootm/bootz */
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG

#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_NFS

#define CONFIG_SYS_LONGHELP
#define CONFIG_CMD_PING

/**********************************************************************************************
 *                                      Compression
 **********************************************************************************************/
/* 
 * Iverson 20150510 :  
 *      Denali-2 use lk to decompress kernel.
 */
#define CONFIG_LZMA                             
#define CONFIG_MTGPIO

#endif
