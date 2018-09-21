#undef CONFIG_BOOTCOMMAND
#undef CONFIG_IPQ_ATAG_PART_LIST
#ifndef CONFIG_SYS_FLASH_BASE
#define CONFIG_SYS_FLASH_BASE	0xC0000000	/* define fake flash address. */
#endif
#define CONFIG_SYS_BOOTM_LEN	(32 << 20)	/* 32 MB */

/*
 * ASUS configuration.
 * All CONFIG_XXX will be copied to include/autoconf.mk automatically.
 */
#define CONFIG_ASUS_PRODUCT
#define UBOOT_ON_NAND
#define CONFIG_MODEL		"RT-AC82U"
#define CONFIG_FLASH_TYPE	"nand"
#define CONFIG_BLS_FIT_IMAGE
#define CONFIG_UBI_SUPPORT
#define CONFIG_DUAL_TRX
#define CONFIG_BLVER		"1009"
#define CONFIG_DUAL_BAND
//#define CONFIG_UART_GSBI2
//#define CONFIG_SWITCH_RTL8370M_PHY_QCA8033_X2
//#define CONFIG_HAVE_WAN_RED_LED
#define CONFIG_HAVE_MULTI_LAN_LED

#define CONFIG_SYS_LOAD_ADDR	0x84000000			/* (CONFIG_SYS_SDRAM_BASE + (32 << 20))	*/
#define CONFIG_SYS_LONGHELP
#define CONFIG_LZMA

#define XMK_STR(x)	#x
#define MK_STR(x)	XMK_STR(x)

/*
 * Environment variables.
 */
#define CONFIG_IPADDR		192.168.1.1
#define CONFIG_SERVERIP		192.168.1.70
#define CONFIG_NETMASK		255.255.255.0
#define CONFIG_BOOTFILE		CONFIG_MODEL ".trx"		/* RT-AC88Q.trx */
#define CONFIG_BOOTCOMMAND	"tftp"
/* the default ethernet address is hardcoded at ipq40xx_def_enetaddr in drivers/net/ipq40xx/ipq40xx_edma_eth.c
#define CONFIG_ETHADDR		00:aa:bb:cc:dd:e0
*/
#define CONFIG_EXTRA_ENV_SETTINGS	\
	"imgaddr="MK_STR(CONFIG_SYS_LOAD_ADDR)"\0" \
	"preferred_nic=eth0\0"

/*
 * Enable commands
 */
#define CONFIG_CMD_LOADB

#ifndef __ASSEMBLY__
/* ipq806x_cdp.c */
extern const char *model;
extern const char *blver;
extern const char *bl_stage;

/* board.c */
extern int modifies;
#endif

/*-----------------------------------------------------------------------
 * Factory
 */
#define RAMAC0_OFFSET	0x1006	/* 2G EEPROM */
#define RAMAC1_OFFSET	0x9006	/* 5G EEPROM */

/*-----------------------------------------------------------------------
 * Bootloader size and Config size definitions
 */
#define CONFIG_MAX_BL_BINARY_SIZE	0x3C0000			/* 4MiB - (128KiB*2) */
#define CFG_BOOTLOADER_SIZE		CONFIG_MAX_BL_BINARY_SIZE
#define CFG_MAX_BOOTLOADER_BINARY_SIZE	CONFIG_MAX_BL_BINARY_SIZE
#define CONFIG_ENV_SIZE			0x20000
#define	CONFIG_ENV_RESERVED_BLOCK	2
#define CONFIG_ENV_SIZE_MAX		CONFIG_ENV_SIZE
#define CONFIG_SYS_MALLOC_LEN           (4 << 20)


#define MTDIDS				"nand0=nand0"
/* Keep Bootloader size and environment size equal to CFG_BOOTLOADER_SIZE and CONFIG_ENV_SIZE respectively. */
#define MTDPARTS			"mtdparts=nand0:3840k(Bootloader),256k(environment),-(UBI_DEV)"
#define BOOT_MTDPARTS			"mtdparts=nand0:4096k(Bootloader),-(UBI_DEV)"

/*
 * UBI volume size definitions
 * Don't define size for tailed reserved space due to it's size varies.
 */
#define PEB_SIZE			(128 * 1024)
#define LEB_SIZE			(PEB_SIZE - (2 * 2 * 1024))
#define CFG_UBI_NVRAM_NR_LEB		3
#define CFG_UBI_FACTORY_NR_LEB		1
#define CFG_UBI_FIRMWARE_NR_LEB		397	/* 124KB x 397 = 48.074MB */
#define CFG_UBI_FIRMWARE2_NR_LEB	397
#define CFG_UBI_APP_NR_LEB		219	/* 124KB x 219 = 27.37MB. This volume size cannot reach requested size due to UBI's overhead. */

#define CFG_UBI_NVRAM_SIZE		(LEB_SIZE * CFG_UBI_NVRAM_NR_LEB)
#define CFG_UBI_FACTORY_SIZE		(LEB_SIZE * CFG_UBI_FACTORY_NR_LEB)
#define CFG_UBI_FACTORY2_SIZE		(LEB_SIZE * CFG_UBI_FACTORY_NR_LEB)
#define CFG_UBI_FIRMWARE_SIZE		(LEB_SIZE * CFG_UBI_FIRMWARE_NR_LEB)
#define CFG_UBI_FIRMWARE2_SIZE		(LEB_SIZE * CFG_UBI_FIRMWARE2_NR_LEB)
#define CFG_UBI_APP_SIZE		(LEB_SIZE * CFG_UBI_APP_NR_LEB)

#define CFG_NVRAM_SIZE			CFG_UBI_NVRAM_SIZE

#define CFG_FACTORY_SIZE		(CFG_UBI_FACTORY_SIZE + CFG_UBI_FACTORY2_SIZE)

#define CFG_UBI_DEV_OFFSET		(CFG_BOOTLOADER_SIZE + (CONFIG_ENV_SIZE * CONFIG_ENV_RESERVED_BLOCK))

/* Environment address, factory address, and firmware address definitions */
/* Basically, CFG_FACTORY_ADDR and CFG_KERN_ADDR are used to compatible to original code infrastructure.
 * Real nvram area would be moved into the nvram volume of UBI device.
 * Real Factory area would be moved into the Factory volume of UBI device.
 * Real firmware area would be moved into the linux and linux2 volume of UBI device.
 */
#define CONFIG_ENV_ADDR			(CONFIG_SYS_FLASH_BASE + CFG_BOOTLOADER_SIZE)
#define CFG_NVRAM_ADDR			(CONFIG_ENV_ADDR + (CONFIG_ENV_SIZE * CONFIG_ENV_RESERVED_BLOCK))
#define CFG_FACTORY_ADDR		(CFG_NVRAM_ADDR + CFG_NVRAM_SIZE)
#define CFG_KERN_ADDR			(CFG_FACTORY_ADDR + CFG_FACTORY_SIZE)
#define CFG_KERN2_ADDR			(CFG_KERN_ADDR + CFG_UBI_FIRMWARE_SIZE)
/*-----------------------------------------------------------------------*/


/* Include header files of demo board. */
#include <configs/ipq40xx_cdp.h>
