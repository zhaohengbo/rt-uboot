#ifndef __MT6589_H__
#define __MT6589_H__

/* I/O mapping */
#define IO_PHYS            	0x10000000
#define IO_SIZE            	0x00100000


#define APCONFIG_BASE (IO_PHYS + 0x00026000)

/* IO register definitions */
#define EFUSE_BASE          (IO_PHYS + 0x00206000)
#define CONFIG_BASE         (IO_PHYS + 0x00001000)
#define EMI_BASE            (IO_PHYS + 0x00203000)
#define GPIO_BASE           (IO_PHYS + 0x00005000)
#define TOP_RGU_BASE        (IO_PHYS + 0x00007000)
#define PERI_CON_BASE       (IO_PHYS + 0x00003000)
#define GIC_CPU_BASE        (IO_PHYS + 0x00212000)
#define GIC_DIST_BASE       (IO_PHYS + 0x00211000)
#define SLEEP_BASE          (IO_PHYS + 0x00006000)
#define MCUSYS_CFGREG_BASE  (IO_PHYS + 0x00200000)
#define MMSYS1_CONFIG_BASE  (IO_PHYS + 0x02080000)
#define MMSYS2_CONFG_BASE   (IO_PHYS + 0x020C0000)
#define AUDIO_BASE          (IO_PHYS + 0x02071000)
#define INT_POL_CTL0        (MCUSYS_CFGREG_BASE + 0x100)
/**************************************************
 *                   F002 0000                    *
 **************************************************/

#define AP_DMA_BASE      	(IO_PHYS + 0x01000000)

/*Hong-Rong: Modified for 6575 porting*/
#define UART1_BASE 		    (IO_PHYS + 0x01002000)
#define UART2_BASE 		    (IO_PHYS + 0x01003000)
#define UART3_BASE 		    (IO_PHYS + 0x01004000)
#define UART4_BASE 		    (IO_PHYS + 0x01005000)

#define APMCU_GPTIMER_BASE  (IO_PHYS + 0x00008000)

#define KP_BASE         	(IO_PHYS + 0x00011000)
#ifdef MTK_PMIC_MT6397
#define	RTC_BASE					(0xe000)
#else
#define	RTC_BASE					(0x8000)
#endif
#define MSDC0_BASE          (IO_PHYS + 0x01230000)
#define MSDC1_BASE          (IO_PHYS + 0x01240000)
#define MSDC2_BASE          (IO_PHYS + 0x01250000)
#define MSDC3_BASE          (IO_PHYS + 0x01260000)
#define MSDC4_BASE          (IO_PHYS + 0x01270000)


/**************************************************
 *                   F003 0000                    *
 **************************************************/
#define I2C0_BASE         	(IO_PHYS + 0x01007000) 
#define I2C1_BASE         	(IO_PHYS + 0x01008000)
#define I2C2_BASE         	(IO_PHYS + 0x01009000) 
#define NFI_BASE         	(IO_PHYS + 0x0100D000) // 2011-02-16 Koshi: Modified for MT6575 NAND driver
#define NFIECC_BASE	        (IO_PHYS + 0x0100E000) // 2011-02-16 Koshi: Modified for MT6575 NAND driver
#define PWM_BASE         	(IO_PHYS + 0x01006000) // 6575
#define AUXADC_BASE	     	(IO_PHYS + 0x01001000) // (IO_PHYS + 0x00034000)
#define PWRAP_BASE              (IO_PHYS + 0xD000)
/**************************************************
 *                   F004 0000                    *
 **************************************************/

/**************************************************
 *                   F006 0000                    *
 **************************************************/

/**************************************************
 *                   F007 0000                    *
 **************************************************/
/**************************************************
 *                   F008 0000                    *
 **************************************************/

/**************************************************
 *                   F009 0000                    *
 **************************************************/

/**************************************************
 *                   F00A 0000                    *
 **************************************************/

/**************************************************
 *                   F00C 0000                    *
 **************************************************/

/**************************************************
 *                   F010 0000                    *
 **************************************************/
#define USB0_BASE           (IO_PHYS + 0x01200000)
#define USBSIF_BASE         (IO_PHYS + 0x01210000)
#define USB_BASE            (USB0_BASE)


/**************************************************
 *                   F012 0000                    *
 **************************************************/
#define LCD_BASE           	(IO_PHYS + 0x04012000) //2011-05-10, zaikuo for mt6575

/**************************************************
 *                   F013 0000                    *
 **************************************************/

/**************************************************
 *                   F014 0000                    *
 **************************************************/

#define MIPI_CONFIG_BASE 	(IO_PHYS + 0x00010000)

// APB Module LVDS ANA
#define LVDS_ANA_BASE (IO_PHYS + 0x00010400)



#define MMSYS2_CONFG_BASE   (IO_PHYS + 0x020C0000)


/* disp subsys register */
#define MMSYS_CONFIG_BASE   (IO_PHYS + 0x04000000)
#define MDP_RDMA_BASE       (IO_PHYS + 0x04001000)
#define MDP_RSZ0_BASE       (IO_PHYS + 0x04002000)
#define MDP_RSZ1_BASE       (IO_PHYS + 0x04003000)
#define MDP_WDMA_BASE       (IO_PHYS + 0x04004000)
#define MDP_WROT_BASE       (IO_PHYS + 0x04005000)
#define MDP_TDSHP_BASE      (IO_PHYS + 0x04006000)
#define DISP_OVL_BASE       (IO_PHYS + 0x04007000)
#define DISP_RDMA_BASE      (IO_PHYS + 0x04008000)
#define DISP_WDMA_BASE      (IO_PHYS + 0x04009000)
#define DISP_BLS_BASE       (IO_PHYS + 0x0400A000)
#define DISP_COLOR_BASE     (IO_PHYS + 0x0400B000)
#define DSI_BASE            (IO_PHYS + 0x0400C000)
#define DPI_BASE            (IO_PHYS + 0x0400D000)
#define DPI1_BASE            (IO_PHYS + 0x04014000)

// LVDS TX 
#define LVDS_TX_BASE (IO_PHYS + 0x04016200)

#define MM_MUTEX_BASE       (IO_PHYS + 0x0400E000)
#define MM_CMDQ_BASE        (IO_PHYS + 0x0400F000)

#define DISPSYS_BASE        (MMSYS_CONFIG_BASE)


/* hardware version register */
#define VER_BASE 0x08000000
#define APHW_CODE           (VER_BASE)
#define APHW_SUBCODE        (VER_BASE + 0x04)
#define APHW_VER            (VER_BASE + 0x08)
#define APSW_VER            (VER_BASE + 0x0C)

/* EMI Registers */
#define EMI_CON0 					(EMI_BASE+0x0000) /* Bank 0 configuration */
#define EMI_CON1 					(EMI_BASE+0x0004) /* Bank 1 configuration */
#define EMI_CON2 					(EMI_BASE+0x0008) /* Bank 2 configuration */
#define EMI_CON3 					(EMI_BASE+0x000C) /* Bank 3 configuration */
#define EMI_CON4 					(EMI_BASE+0x0010) /* Boot Mapping config  */
#define	EMI_CON5 					(EMI_BASE+0x0014)


//----------------------------------------------------------------------------
/* Powerdown module watch dog timer registers */
#define REG_RW_WATCHDOG_EN  		0x0100      // Watchdog Timer Control Register
#define REG_RW_WATCHDOG_TMR 		0x0104      // Watchdog Timer Register
#define REG_RW_WAKE_RSTCNT  		0x0108      // Wakeup Reset Counter Register




/* MT6575 EMI freq. definition */
#define EMI_52MHZ                   52000000
#define EMI_58_5MHZ                 58500000
#define EMI_104MHZ                  104000000
#define EMI_117MHZ                  117000000
#define EMI_130MHZ                  130000000

/* MT6575 storage boot type definitions */
#define NON_BOOTABLE                0
#define RAW_BOOT                    1
#define FAT_BOOT                    2

#define CONFIG_STACKSIZE	    (128*1024)	  /* regular stack */

// xuecheng, define this because we use zlib for boot logo compression
#define CONFIG_ZLIB 	1

// =======================================================================
// UBOOT DEBUG CONTROL
// =======================================================================
#define UBOOT_DEBUG_TRACER			(0)

/*MTK Memory layout configuration*/
#define MAX_NR_BANK    4

#define DRAM_PHY_ADDR   0x80000000

#define RIL_SIZE 0

#define CFG_RAMDISK_LOAD_ADDR           (DRAM_PHY_ADDR + 0x4000000)
#define CFG_BOOTIMG_LOAD_ADDR           (DRAM_PHY_ADDR + 0x8000)
#define CFG_BOOTARGS_ADDR               (DRAM_PHY_ADDR + 0x100)

/*Command passing to Kernel */
#ifdef MACH_FPGA
#define COMMANDLINE_TO_KERNEL  "console=ttyMT0,921600n1 rdinit=/init root=/dev/ram"
#else
//#define COMMANDLINE_TO_KERNEL  "console=tty0 console=ttyMT3,921600n1 root=/dev/ram"
//#define COMMANDLINE_TO_KERNEL  "console=ttyMT2,115200n1 rdinit=/init root=/dev/ram androidboot.selinux=disabled androidboot.serialno=0123456789ABCDEF printk.disable_uart=0 boot_reason=0"
//BPI
//#define COMMANDLINE_TO_KERNEL "console=tty0 console=ttyMT2,115200n1 root=/dev/ram lcm=1-hx8392a_vdo_cmd fps=4433 vram=13631488 androidboot.selinux=disabled bootprof.pl_t=1149 bootprof.lk_t=9849 printk.disable_uart=0 boot_reason=0 androidboot.serialno=0123456789ABCDEF androidboot.bootreason=power_key"
#define COMMANDLINE_TO_KERNEL "board=bpi-r2 earlyprintk console=tty1 fbcon=map:0 console=ttyS0,115200 vmalloc=496M debug=7 initcall_debug=0 root=/dev/mmcblk0p2 rootfstype=ext4 rootwait"
#endif
#define CFG_FACTORY_NAME	"factory.img"
#define HAVE_LK_TEXT_MENU

#ifdef CONFIG_MTK_USB_UNIQUE_SERIAL
#define SERIAL_KEY_HI		(EFUSE_BASE + 0x0144)
#define SERIAL_KEY_LO		(EFUSE_BASE + 0x0140)
#endif

#if 0 /* By chip and tape out process */
//ALPS00427972, implement the analog register formula
//Add here for eFuse, chip version checking -> analog register calibration
#define M_HW_RES3	                    0x10009170
//#define M_HW_RES3_PHY                   IO_PHYS+M_HW_RES3
#define RG_USB20_TERM_VREF_SEL_MASK     0xE000      //0b 1110,0000,0000,0000     15~13
#define RG_USB20_CLKREF_REF_MASK        0x1C00      //0b 0001,1100,0000,0000     12~10
#define RG_USB20_VRT_VREF_SEL_MASK      0x0380      //0b 0000,0011,1000,0000     9~7
//ALPS00427972, implement the analog register formula
#endif

#endif








