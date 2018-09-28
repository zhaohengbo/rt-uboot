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
#include <asm/arch/mt_reg_base.h>

//#define DEBUG

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
#define CONFIG_MACH_TYPE                    7623

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
#define CONFIG_SYS_SDRAM_BASE		        0x80000000

/* Code Layout */
//#define CONFIG_SYS_TEXT_BASE		        0x80000000
#define CONFIG_SYS_TEXT_BASE		        0x81E00000

/* Uboot definition */
#define CONFIG_SYS_UBOOT_BASE		        CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_UBOOT_MAX_SIZE           SZ_2M
#define CONFIG_SYS_INIT_SP_ADDR             (CONFIG_SYS_TEXT_BASE + \
                                                CONFIG_SYS_UBOOT_MAX_SIZE - \
                                                GENERATED_GBL_DATA_SIZE)

#define CONFIG_SYS_MALLOC_LEN               SZ_32M

/* RichOS memory partitions */
#define CONFIG_SYS_DECOMP_ADDR              0x80008000
#define CONFIG_SYS_LOAD_ADDR                0x84000000
#define CONFIG_SYS_IMAGE_HDR_ADDR           CONFIG_SYS_LOAD_ADDR

/* Linux DRAM definition */
#define CONFIG_LOADADDR			            CONFIG_SYS_LOAD_ADDR

/*
 * For booting Linux, the board info and command line data
 * have to be in the first 64 MB of memory, since this is
 * the maximum mapped by the Linux kernel during initialization.
 */
#define CONFIG_SYS_BOOTM_LEN	            0x4000000

/**********************************************************************************************
 *                                           Board
 **********************************************************************************************/

/* Board */
#if defined(MT7623_FPGA_BOARD)
#define MACH_FPGA   1
#endif
#define CONFIG_ADD_MTK_HEADER               "y"

#define CONFIG_BOARD_LATE_INIT

/**********************************************************************************************
 *                                          Devices
 **********************************************************************************************/

/********************** Flash *************************/
#if defined(ON_BOARD_SPI_FLASH_COMPONENT)
/* SPI Flash Configuration */

/* Parallel Nor Flash */
#define CONFIG_SYS_NO_FLASH

/* SPI Flash */
#define CONFIG_SPI_FLASH

#define CONFIG_SF_DEFAULT_BUS               0
#define CONFIG_SF_DEFAULT_CS                0
#define CONFIG_SF_DEFAULT_SPEED             1000000
#define CONFIG_SF_DEFAULT_MODE              SPI_MODE_3

#define CONFIG_CMD_SF

#define CONFIG_ENV_IS_IN_SPI_FLASH

#define CONFIG_ENV_SIZE                     SZ_16K
#define CONFIG_ENV_SECT_SIZE                0x40000
#define CONFIG_ENV_OFFSET                   0x0000

#define CONFIG_SF_LINUX_BLOCK               3

#define ENV_BOOT_WRITE_IMAGE \
    "boot_wr_img=image_blks 1" \
    ";sf update ${loadaddr} " __stringify(CONFIG_SF_LINUX_BLOCK) " ${img_blks}\0"
#define ENV_BOOT_READ_IMAGE \
    "boot_rd_img=sf read ${loadaddr} " __stringify(CONFIG_SF_LINUX_BLOCK) " 64" \
    ";image_blks 1;sf read ${loadaddr} " __stringify(CONFIG_SF_LINUX_BLOCK) " ${img_blks}\0"
#elif defined(ON_BOARD_NAND_FLASH_COMPONENT)
#define CONFIG_SYS_NO_FLASH

/* NAND Flash Configuration */
#define CONFIG_SYS_MAX_NAND_DEVICE	        1	
#define CONFIG_SYS_NAND_BASE                NFI_BASE
#define CONFIG_SYS_MAX_FLASH_SECT			256	
#define CONFIG_SYS_MAX_FLASH_BANKS			1
#define CONFIG_CMD_NAND
//#define CONFIG_MTD_DEBUG
//#define CONFIG_MTD_DEBUG_VERBOSE  7
#define CONFIG_CMD_MTDPARTS	1
#define CONFIG_MTD_PARTITIONS	1
#define CONFIG_ENV_IS_IN_NAND
#define CONFIG_MTD_NAND_VERIFY_WRITE		1

#if defined (CONFIG_MTK_MLC_NAND_SUPPORT)
#define CONFIG_ENV_SIZE                     SZ_4K
#define CONFIG_ENV_OFFSET                   0x2000000
#define CONFIG_NAND_UBOOT_OFFSET               0x1000000
#define CONFIG_NAND_LINUX_OFFSET               0x4000000
#define CONFIG_MAX_NAND_PAGE_SIZE		8192
#define CONFIG_MAX_NAND_BLOCK_SIZE		2097152
#define CONFIG_MAX_UBOOT_SIZE			393216	//262144
#else
#define CONFIG_ENV_SIZE                     SZ_4K
#define CONFIG_ENV_OFFSET                   0xC0000
#define CONFIG_NAND_UBOOT_OFFSET            0x40000
#define CONFIG_NAND_LINUX_OFFSET            0x140000
#define CONFIG_NAND_RECOVERY_OFFSET         0x2140000
#define CONFIG_MAX_NAND_PAGE_SIZE       	2048
#define CONFIG_MAX_NAND_BLOCK_SIZE      	131072
#define CONFIG_MAX_UBOOT_SIZE   			393216	//262144
#endif

#define ENV_BOOT_WRITE_IMAGE \
    "boot_wr_img=image_check; if test ${img_result} = good; then image_blks " __stringify(CONFIG_MAX_NAND_PAGE_SIZE) \
    " ${filesize};nand erase " __stringify(CONFIG_NAND_LINUX_OFFSET) " ${img_align_size}" \
    ";nand write ${loadaddr} " __stringify(CONFIG_NAND_LINUX_OFFSET) " ${img_align_size}; fi\0" 

#define ENV_BOOT_READ_IMAGE \
    "boot_rd_img=nand read ${loadaddr} " __stringify(CONFIG_NAND_LINUX_OFFSET) " 2000" \
	";image_blks " __stringify(CONFIG_MAX_NAND_PAGE_SIZE) \
	";nand read ${loadaddr} " __stringify(CONFIG_NAND_LINUX_OFFSET) " ${img_align_size}\0"

#define ENV_WRITE_UBOOT \
    "wr_uboot=uboot_check;if test ${uboot_result} = good; then image_blks " __stringify(CONFIG_MAX_NAND_PAGE_SIZE) \
    " ${filesize};nand erase " __stringify(CONFIG_NAND_UBOOT_OFFSET) " ${img_align_size}" \
    ";nand write ${loadaddr} " __stringify(CONFIG_NAND_UBOOT_OFFSET) " ${img_align_size};reset; fi\0" 

#define ENV_BOOT_WRITE_SEC_IMAGE \
    "boot_wr_sec_img=image_check; if test ${img_result} = good; then image_blks " __stringify(CONFIG_MAX_NAND_PAGE_SIZE) \
    " ${filesize};nand erase " __stringify(CONFIG_NAND_RECOVERY_OFFSET) " ${img_align_size}" \
    ";nand write ${loadaddr} " __stringify(CONFIG_NAND_RECOVERY_OFFSET) " ${img_align_size}; fi\0" 

#define ENV_BOOT_READ_SEC_IMAGE \
    "boot_rd_sec_img=nand read ${loadaddr} " __stringify(CONFIG_NAND_RECOVERY_OFFSET) " 2000" \
	";image_blks " __stringify(CONFIG_MAX_NAND_PAGE_SIZE) \
	";nand read ${loadaddr} " __stringify(CONFIG_NAND_RECOVERY_OFFSET) " ${img_align_size}\0"

#define ENV_DUIMAGE_RECOVERY \
    "duimage_recovery=serious_image_check; if test ${img_result} = bad; then run boot_rd_sec_img" \
    ";serious_image_check; if test ${img_result} = good; then reco_message; run boot_wr_img; fi; fi\0"

#define ENV_DUIMAGE_BACKUP \
   "duimage_backup=nand read ${loadaddr} " __stringify(CONFIG_NAND_RECOVERY_OFFSET) " 2000" \
   ";image_check; if test ${img_result} = bad" \
   ";then run boot_rd_img; serious_image_check; if test ${img_result} = good; then backup_message" \
   ";run boot_wr_sec_img; fi; fi\0"

#elif defined(ON_BOARD_EMMC_COMPONENT)
/*EMMC Configuration*/
/* Parallel Nor Flash */
/* Build error elimination*/
#define CONFIG_SYS_NO_FLASH
/**/


#define FEATURE_MMC_BOOT_MODE

#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_ENV_SIZE                     SZ_4K
#define CONFIG_ENV_OFFSET                   0x600 
#define CONFIG_SYS_MMC_ENV_DEV              0
#define CONFIG_SYS_MAX_FLASH_SECT           512

#define CONFIG_EMMC_UBOOT_BLOCK             0x200 
#define CONFIG_MAX_UBOOT_SIZE               0x200           // 0x200 * 512 = 256KB
#define CONFIG_EMMC_LINUX_BLOCK             0xA00
#define CONFIG_EMMC_RECOVERY_BLOCK          0x10A00

#define ENV_BOOT_WRITE_IMAGE \
    "boot_wr_img=image_check; if test ${img_result} = good; then image_blks " __stringify(CONFIG_SYS_MAX_FLASH_SECT) \
    " ${filesize};mmc device 0;mmc write ${loadaddr} " __stringify(CONFIG_EMMC_LINUX_BLOCK) " ${img_blks}; fi\0"

#define ENV_BOOT_READ_IMAGE \
    "boot_rd_img=mmc device 0;mmc read ${loadaddr} " __stringify(CONFIG_EMMC_LINUX_BLOCK) " 1" \
    ";image_blks " __stringify(CONFIG_SYS_MAX_FLASH_SECT) \
    ";mmc read ${loadaddr} " __stringify(CONFIG_EMMC_LINUX_BLOCK) " ${img_blks}\0"

#define ENV_WRITE_UBOOT \
    "wr_uboot=uboot_check;if test ${uboot_result} = good; then mmc device 0;mmc write ${loadaddr} " __stringify(CONFIG_EMMC_UBOOT_BLOCK) \
    " " __stringify(CONFIG_MAX_UBOOT_SIZE) ";reset; fi\0"

#define ENV_BOOT_WRITE_SEC_IMAGE \
    "boot_wr_sec_img=image_check; if test ${img_result} = good; then image_blks " __stringify(CONFIG_SYS_MAX_FLASH_SECT) \
    " ${filesize};mmc device 0;mmc write ${loadaddr} " __stringify(CONFIG_EMMC_RECOVERY_BLOCK) " ${img_blks}; fi\0"

#define ENV_DUIMAGE_RECOVERY \
    "duimage_recovery=serious_image_check; if test ${img_result} = bad; then run boot_rd_sec_img" \
    ";serious_image_check; if test ${img_result} = good; then reco_message; run boot_wr_img; fi; fi\0"


#define ENV_BOOT_READ_SEC_IMAGE \
    "boot_rd_sec_img=mmc device 0;mmc read ${loadaddr} " __stringify(CONFIG_EMMC_RECOVERY_BLOCK) " 1" \
    ";image_blks " __stringify(CONFIG_SYS_MAX_FLASH_SECT) \
    ";mmc read ${loadaddr} " __stringify(CONFIG_EMMC_RECOVERY_BLOCK) " ${img_blks}\0"

#define ENV_DUIMAGE_BACKUP \
   "duimage_backup=mmc device 0;mmc read ${loadaddr} " __stringify(CONFIG_EMMC_RECOVERY_BLOCK) " 1" \
   ";image_check; if test ${img_result} = bad" \
   ";then run boot_rd_img; serious_image_check; if test ${img_result} = good; then backup_message" \
   ";run boot_wr_sec_img; fi; fi\0"

#elif defined(OFF_BOARD_SD_CARD_COMPONENT)
/*SD card configuration*/
/* Build error elimination*/
#define CONFIG_SYS_NO_FLASH
/**/

#define CONFIG_ENV_IS_IN_SD
#define CONFIG_ENV_SIZE                     SZ_128K
#define CONFIG_ENV_OFFSET                   0xD2800 
#define CONFIG_SYS_MMC_ENV_DEV              1

#define ENV_BOOT_FROM_SD  \
    "boot5=mmc init; fatload mmc 0:1 0x84000000 uimage ; bootm\0"

#else
#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_SIZE                     SZ_16K
#endif

#define CONFIG_ENV_VARS_UBOOT_CONFIG

#if defined(ON_BOARD_EMMC_COMPONENT) || defined(OFF_BOARD_SD_CARD_COMPONENT) || defined(FW_UPGRADE_BY_SDXC)
/********************** MMC ***********************************/
#define PART_DEBUG
#define CONFIG_CMD_MMC
#define CONFIG_MMC
#define CONFIG_MEDIATEK_MMC

#endif

#define CONFIG_FS_FAT
#define CONFIG_CMD_FAT
#define CONFIG_DOS_PARTITION
/********************** GPIO *************************/
//#define CONFIG_MTGPIO

/********************** Watchdog *************************/
#define CONFIG_WATCHDOG_OFF

/********************** Console UART *************************/
/* Uart baudrate */
//#define CONFIG_BAUDRATE                     921600
#define CONFIG_BAUDRATE                     115200
 
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

/**********************USB**************************/
#if defined(FW_UPGRADE_BY_USB)

#define CONFIG_CMD_USB
#define CONFIG_USB_STORAGE
#define CONFIG_USB_HOST
#define CONFIG_USB_XHCI
//#define CONFIG_USB_XHCI_DWC3
//#define CONFIG_USB_XHCI_OMAP
#define CONFIG_SYS_USB_XHCI_MAX_ROOT_PORTS 2
#define CONFIG_SYS_CACHELINE_SIZE	32
#define CONFIG_FS_FAT
#define CONFIG_CMD_FAT
#define CONFIG_DOS_PARTITION
//#define PART_DEBUG

#endif
/**********************************************************************************************
 *                                       Boot Menu
 **********************************************************************************************/
#define CONFIG_BOOTDELAY                    3
#define CONFIG_BOOTCOMMAND                  "No"
#define CONFIG_CMD_BOOTMENU
#define CONFIG_MENU
#define CONFIG_MENU_SHOW
#define CONFIG_MENU_ACTIVE_ENTRY            2
#define CONFIG_UPGFILE			    "root_uImage"

#define USB_BOOT_MENU 
#define SDcard_BOOT_MENU 
#define Browser_BOOT_MENU 

#define ENV_BOOT_CMD0 \
    "boot0=tftpboot; bootm\0"

#define ENV_BOOT_CMD1 \
    "boot1=tftpboot;run boot_wr_img;run boot_rd_img;bootm\0"


#define ENV_BOOT_CMD2 \
    "boot2=run boot_rd_img;run rescue;bootm\0"
//    "boot2=run boot_rd_img;bootm\0"

#define ENV_BOOT_CMD3 \
    "boot3=tftpboot ${loadaddr} u-boot-mtk.bin;run wr_uboot\0"

#define ENV_BOOT_CMD4 \
    "boot4=loadb;run boot_wr_img;run boot_rd_img;bootm\0"

#define ENV_BOOT_CMD5 \
    "boot5=loadb;run wr_uboot\0"

#define ENV_BOOT_CMD6 \
    "boot6=usb start;run boot_wr_img;bootm\0"

#define ENV_BOOT_CMD7 \
    "boot7=http_upgrade " __stringify(CONFIG_SERVERIP) ";run boot_wr_img;run boot_rd_img;bootm\0"

#define ENV_BOOT_CMD8 \
    "boot8=mmc init; fatload mmc 0:1 ${loadaddr} " __stringify(CONFIG_UPGFILE) ";run boot_wr_img; bootm\0"
#define ENV_BOOT_CMD9 \
    "boot9=mmc init; fatload mmc 0:1 ${loadaddr} ${bpi}/${board}/${service}/${kernel}; bootm\0"
#define ENV_BOOT_CMD10 \
    "boot10=mmc init; run boot_normal; bootm\0"
#if defined(FW_UPGRADE_BY_USB)

#undef USB_BOOT_MENU
#define USB_BOOT_MENU \
   "bootmenu_6=7. System Load Linux Kernel then write to Flash via USB.=run boot6\0"
#define ENV_RESCUE_CMD \
    "rescue=serious_image_check; if test ${img_result} = bad; then usb start;run boot_wr_img; fi\0"

#elif defined(FW_UPGRADE_BY_SDXC)

#undef SDcard_BOOT_MENU
#define SDcard_BOOT_MENU \
   "bootmenu_6=7. System Load Linux Kernel then write to Flash via SDcard.=run boot8\0"
#define ENV_RESCUE_CMD \
    "rescue=serious_image_check; if test ${img_result} = bad; then mmc init; fatload mmc 0:1 ${loadaddr} " \
    __stringify(CONFIG_UPGFILE) ";run boot_wr_img; fi\0"

#elif defined(FW_UPGRADE_BY_WEBUI)

#undef Browser_BOOT_MENU
#define Browser_BOOT_MENU \
   "bootmenu_6=7. System Load Linux Kernel then write to Flash via Browser.=run boot7\0"
#define ENV_RESCUE_CMD \
    "rescue=serious_image_check; if test ${img_result} = bad; then http_upgrade " __stringify(CONFIG_SERVERIP) \
    ";run boot_wr_img;run boot_rd_img; fi\0"

#elif defined(DUAL_IMAGE_SUPPORT)

#define ENV_RESCUE_CMD \
    "rescue=serious_image_check; if test ${img_result} = bad; then run duimage_recovery;fi;run duimage_backup\0"
#else

#define ENV_RESCUE_CMD \
    "rescue=serious_image_check;\0"
#endif

#define ENV_BOOT_CMD \
    ENV_BOOT_WRITE_IMAGE \
    ENV_BOOT_READ_IMAGE \
    ENV_WRITE_UBOOT \
    ENV_DUIMAGE_RECOVERY \
    ENV_BOOT_READ_SEC_IMAGE \
    ENV_DUIMAGE_BACKUP \
    ENV_BOOT_WRITE_SEC_IMAGE \
    ENV_RESCUE_CMD \
    ENV_BOOT_CMD0 \
    ENV_BOOT_CMD1 \
    ENV_BOOT_CMD2 \
    ENV_BOOT_CMD3 \
    ENV_BOOT_CMD4 \
    ENV_BOOT_CMD5 \
    ENV_BOOT_CMD6 \
    ENV_BOOT_CMD7 \
    ENV_BOOT_CMD8 \
    ENV_BOOT_CMD9 \
    ENV_BOOT_CMD10 \
    USB_BOOT_MENU \
    Browser_BOOT_MENU \
    SDcard_BOOT_MENU


#define ENV_BOOT_MENU \
    "bootmenu_0=1. System Load Linux to SDRAM via TFTP.=run boot0\0" \
    "bootmenu_1=2. System Load Linux Kernel then write to Flash via TFTP.=run boot1\0" \
    "bootmenu_2=3. Boot Linux from SD.=run boot10\0" \
    "bootmenu_3=4. System Load Boot Loader then write to Flash via TFTP.=run boot3\0" \
    "bootmenu_4=5. System Load Linux Kernel then write to Flash via Serial.=run boot4\0" \
    "bootmenu_5=6. System Load Boot Loader then write to Flash via Serial.=run boot5\0" \
    "bootmenu_6=7. Boot system code via Flash.=run boot2\0" \
    "bpiver=1\0" \
    "bpi=bananapi\0" \
    "board=bpi-r2\0" \
    "chip=MT7623N\0" \
    "service=linux\0" \
    "scriptaddr=0x83000000\0" \
    "device=mmc\0" \
    "partition=1:1\0" \
    "kernel=uImage\0" \
    "root=/dev/mmcblk0p2\0" \
    "debug=7\0" \
    "bootenv=uEnv.txt\0" \
    "checksd=fatinfo ${device} 1:1\0" \
    "loadbootenv=fatload ${device} ${partition} ${scriptaddr} ${bpi}/${board}/${service}/${bootenv} || fatload ${device} ${partition} ${scriptaddr} ${bootenv}\0" \
    "boot_normal=if run checksd; then echo Boot from SD ; setenv partition 1:1; else echo Boot from eMMC ; mmc init 0 ; setenv partition 0:1 ; fi; if run loadbootenv; then echo Loaded environment from ${bootenv}; env import -t ${scriptaddr} ${filesize}; fi; run uenvcmd; fatload mmc 0:1 ${loadaddr} ${bpi}/${board}/${service}/${kernel}; bootm\0" \
    "bootmenu_delay=30\0" \
    ""

#define CONFIG_EXTRA_ENV_SETTINGS \
	ENV_DEVICE_SETTINGS \
	ENV_BOOT_CMD \
	ENV_BOOT_MENU


/**********************************************************************************************
 *                                       UBoot Command
 **********************************************************************************************/
/* Shell */
#define CONFIG_SYS_MAXARGS		            32
#define CONFIG_SYS_PROMPT		            "BPI-IoT> "
#define CONFIG_COMMAND_HISTORY

/* Commands */
#include <config_cmd_default.h>

/* Device tree support */
#define CONFIG_OF_BOARD_SETUP
/* ATAGs support for bootm/bootz */
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG
#ifdef CONFIG_MTK_MTD_NAND
#define CONFIG_NAND_TAG
#endif
#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_NFS

#define CONFIG_SYS_LONGHELP
#define CONFIG_CMD_PING

/* Enable Hush Parser */
#define CONFIG_SYS_HUSH_PARSER

/* Compression */
/* 
 * Iverson 20150408 :  
 *      We use kernel to decompress kernel.
 */
//#define CONFIG_LZMA                             

#endif
