/*
 * (C) Copyright 2002-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
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

/*
 * To match the U-Boot user interface on ARM platforms to the U-Boot
 * standard (as on PPC platforms), some messages with debug character
 * are removed from the default U-Boot build.
 *
 * Define DEBUG here if you want additional info as shown below
 * printed upon startup:
 *
 * U-Boot code: 00F00000 -> 00F3C774  BSS: -> 00FC3274
 * IRQ Stack: 00ebff7c
 * FIQ Stack: 00ebef7c
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <stdio_dev.h>
#include <version.h>
#include <net.h>
#include <serial.h>
#include <nand.h>
#include <onenand_uboot.h>
#include <mmc.h>
#include <libfdt.h>
#include <fdtdec.h>
#include <post.h>
#include <logbuff.h>
#ifdef CONFIG_SYS_HUSH_PARSER
#include <hush.h>
#endif

#if defined(ASUS_PRODUCT)
#include <gpio.h>
#include <replace.h>
#include <flash_wrapper.h>
#include <cmd_tftpServer.h>
#if defined(CONFIG_UBI_SUPPORT)
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <ubi_uboot.h>
#endif
#endif

#ifdef CONFIG_BITBANGMII
#include <miiphy.h>
#endif

#ifdef CONFIG_DRIVER_SMC91111
#include "../drivers/net/smc91111.h"
#endif
#ifdef CONFIG_DRIVER_LAN91C96
#include "../drivers/net/lan91c96.h"
#endif

#if defined(ASUS_PRODUCT) && defined(CONFIG_IPQ40XX)
extern void sw_gpio_init(void);
#endif

DECLARE_GLOBAL_DATA_PTR;

#ifdef crc32
#undef crc32
#endif

#if defined(ASUS_PRODUCT)
extern int do_bootm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
extern int do_load_serial_bin(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
extern int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
extern int do_tftpb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
extern int do_tftpd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
extern int do_source (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

#if defined(UBOOT_STAGE1)
#define BOOTFILENAME	"uboot_stage1.img"
#elif defined(UBOOT_STAGE2)
#define BOOTFILENAME	"uboot_stage2.img"
#else
#define BOOTFILENAME	"u-boot_" CONFIG_FLASH_TYPE ".img"
#endif

#define SEL_LOAD_LINUX_WRITE_FLASH_BY_SERIAL	'0'
#define SEL_LOAD_LINUX_SDRAM			'1'
#define SEL_LOAD_LINUX_WRITE_FLASH		'2'
#define SEL_BOOT_FLASH				'3'
#define SEL_ENTER_CLI				'4'
#define SEL_LOAD_BOOT_SDRAM_VIA_SERIAL		'5'
#define SEL_LOAD_BOOT_WRITE_FLASH_BY_SERIAL	'7'
#define SEL_LOAD_BOOT_SDRAM			'8'
#define SEL_LOAD_BOOT_WRITE_FLASH		'9'

#if defined(UBOOT_STAGE1)
extern struct stage2_loc g_s2_loc;

#define BOOT_IMAGE_NAME	"Bootloader stage1 code"
#define SYS_IMAGE_NAME	"Bootloader stage2 code"
#elif defined(UBOOT_STAGE2)
#define BOOT_IMAGE_NAME "Bootloader stage1/2 code"
#define SYS_IMAGE_NAME  "System code"
#else
#define BOOT_IMAGE_NAME "Boot Loader code"
#define SYS_IMAGE_NAME  "System code"
#endif

int modifies = 0;

#define ARGV_LEN	128
static char file_name_space[ARGV_LEN];
#endif	/* ASUS_PRODUCT */

ulong monitor_flash_len;

#ifdef CONFIG_HAS_DATAFLASH
extern int  AT91F_DataflashInit(void);
extern void dataflash_print_info(void);
#endif

#if defined(CONFIG_HARD_I2C) || \
    defined(CONFIG_SOFT_I2C)
#include <i2c.h>
#endif

/************************************************************************
 * Coloured LED functionality
 ************************************************************************
 * May be supplied by boards if desired
 */
void __coloured_LED_init(void) {}
void coloured_LED_init(void)
	__attribute__((weak, alias("__coloured_LED_init")));
void __red_led_on(void) {}
void red_led_on(void) __attribute__((weak, alias("__red_led_on")));
void __red_led_off(void) {}
void red_led_off(void) __attribute__((weak, alias("__red_led_off")));
void __green_led_on(void) {}
void green_led_on(void) __attribute__((weak, alias("__green_led_on")));
void __green_led_off(void) {}
void green_led_off(void) __attribute__((weak, alias("__green_led_off")));
void __yellow_led_on(void) {}
void yellow_led_on(void) __attribute__((weak, alias("__yellow_led_on")));
void __yellow_led_off(void) {}
void yellow_led_off(void) __attribute__((weak, alias("__yellow_led_off")));
void __blue_led_on(void) {}
void blue_led_on(void) __attribute__((weak, alias("__blue_led_on")));
void __blue_led_off(void) {}
void blue_led_off(void) __attribute__((weak, alias("__blue_led_off")));

/*
 ************************************************************************
 * Init Utilities							*
 ************************************************************************
 * Some of this code should be moved into the core functions,
 * or dropped completely,
 * but let's get it working (again) first...
 */

#if defined(CONFIG_ARM_DCC) && !defined(CONFIG_BAUDRATE)
#define CONFIG_BAUDRATE 115200
#endif

static int init_baudrate(void)
{
	gd->baudrate = getenv_ulong("baudrate", 10, CONFIG_BAUDRATE);
	return 0;
}

static int display_banner(void)
{
#if defined(ASUS_PRODUCT)
	printf("\n\n%s\n%s bootloader%s version: %c.%c.%c.%c\n\n",
		version_string, model, bl_stage, blver[0], blver[1], blver[2], blver[3]);
#else
	printf("\n\n%s\n\n", version_string);
#endif

	debug("U-Boot code: %08lX -> %08lX  BSS: -> %08lX\n",
	       _TEXT_BASE,
	       _bss_start_ofs + _TEXT_BASE, _bss_end_ofs + _TEXT_BASE);
#ifdef CONFIG_MODEM_SUPPORT
	debug("Modem Support enabled\n");
#endif
#ifdef CONFIG_USE_IRQ
	debug("IRQ Stack: %08lx\n", IRQ_STACK_START);
	debug("FIQ Stack: %08lx\n", FIQ_STACK_START);
#endif

	return (0);
}

/*
 * WARNING: this code looks "cleaner" than the PowerPC version, but
 * has the disadvantage that you either get nothing, or everything.
 * On PowerPC, you might see "DRAM: " before the system hangs - which
 * gives a simple yet clear indication which part of the
 * initialization if failing.
 */
static int display_dram_config(void)
{
	int i;

#ifdef DEBUG
	puts("RAM Configuration:\n");

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		printf("Bank #%d: %08lx ", i, gd->bd->bi_dram[i].start);
		print_size(gd->bd->bi_dram[i].size, "\n");
	}
#else
	ulong size = 0;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++)
		size += gd->bd->bi_dram[i].size;

	puts("DRAM:  ");
	print_size(size, "\n");
#endif

	return (0);
}

#if defined(CONFIG_HARD_I2C) || defined(CONFIG_SOFT_I2C)
static int init_func_i2c(void)
{
	puts("I2C:   ");
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	puts("ready\n");
	return (0);
}
#endif

#if defined(CONFIG_CMD_PCI) || defined (CONFIG_PCI)
#include <pci.h>
static int arm_pci_init(void)
{
	pci_init();
	return 0;
}
#endif /* CONFIG_CMD_PCI || CONFIG_PCI */

/*
 * Breathe some life into the board...
 *
 * Initialize a serial port as console, and carry out some hardware
 * tests.
 *
 * The first part of initialization is running from Flash memory;
 * its main purpose is to initialize the RAM so that we
 * can relocate the monitor code to RAM.
 */

/*
 * All attempts to come up with a "common" initialization sequence
 * that works for all boards and architectures failed: some of the
 * requirements are just _too_ different. To get rid of the resulting
 * mess of board dependent #ifdef'ed code we now make the whole
 * initialization sequence configurable to the user.
 *
 * The requirements for any new initalization function is simple: it
 * receives a pointer to the "global data" structure as it's only
 * argument, and returns an integer return code, where 0 means
 * "continue" and != 0 means "fatal error, hang the system".
 */
typedef int (init_fnc_t) (void);

int print_cpuinfo(void);

void __dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size =  gd->ram_size;
}
void dram_init_banksize(void)
	__attribute__((weak, alias("__dram_init_banksize")));

int __arch_cpu_init(void)
{
	return 0;
}
int arch_cpu_init(void)
	__attribute__((weak, alias("__arch_cpu_init")));

init_fnc_t *init_sequence[] = {
	arch_cpu_init,		/* basic arch cpu dependent setup */

#if defined(CONFIG_BOARD_EARLY_INIT_F)
	board_early_init_f,
#endif
#ifdef CONFIG_OF_CONTROL
	fdtdec_check_fdt,
#endif
	timer_init,		/* initialize timer */
#ifdef CONFIG_FSL_ESDHC
	get_clocks,
#endif
	env_init,		/* initialize environment */
	init_baudrate,		/* initialze baudrate settings */
	serial_init,		/* serial communications setup */
	console_init_f,		/* stage 1 init of console */
	display_banner,		/* say that we are here */
#if defined(CONFIG_DISPLAY_CPUINFO)
	print_cpuinfo,		/* display cpu info (and speed) */
#endif
#if defined(CONFIG_DISPLAY_BOARDINFO)
	checkboard,		/* display board info */
#endif
#if defined(CONFIG_HARD_I2C) || defined(CONFIG_SOFT_I2C)
	init_func_i2c,
#endif
	dram_init,		/* configure available RAM banks */
	NULL,
};

void board_init_f(ulong bootflag)
{
	bd_t *bd;
	init_fnc_t **init_fnc_ptr;
	gd_t *id;
	ulong addr, addr_sp;
#ifdef CONFIG_PRAM
	ulong reg;
#endif

	bootstage_mark_name(BOOTSTAGE_ID_START_UBOOT_F, "board_init_f");

	/* Pointer is writable since we allocated a register for it */
	gd = (gd_t *) ((CONFIG_SYS_INIT_SP_ADDR) & ~0x07);
	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("": : :"memory");

	memset((void *)gd, 0, sizeof(gd_t));

	gd->mon_len = _bss_end_ofs;
#ifdef CONFIG_OF_EMBED
	/* Get a pointer to the FDT */
	gd->fdt_blob = _binary_dt_dtb_start;
#elif defined CONFIG_OF_SEPARATE
	/* FDT is at end of image */
	gd->fdt_blob = (void *)(_end_ofs + _TEXT_BASE);
#endif
	/* Allow the early environment to override the fdt address */
	gd->fdt_blob = (void *)getenv_ulong("fdtcontroladdr", 16,
						(uintptr_t)gd->fdt_blob);

	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr)() != 0) {
			hang ();
		}
	}

#if defined(ASUS_PRODUCT)
	led_init(); // turn_on_led
	gpio_init();
#endif

#ifdef CONFIG_OF_CONTROL
	/* For now, put this check after the console is ready */
	if (fdtdec_prepare_fdt()) {
		panic("** CONFIG_OF_CONTROL defined but no FDT - please see "
			"doc/README.fdt-control");
	}
#endif

	debug("monitor len: %08lX\n", gd->mon_len);
	/*
	 * Ram is setup, size stored in gd !!
	 */
	debug("ramsize: %08lX\n", gd->ram_size);
#if defined(CONFIG_SYS_MEM_TOP_HIDE)
	/*
	 * Subtract specified amount of memory to hide so that it won't
	 * get "touched" at all by U-Boot. By fixing up gd->ram_size
	 * the Linux kernel should now get passed the now "corrected"
	 * memory size and won't touch it either. This should work
	 * for arch/ppc and arch/powerpc. Only Linux board ports in
	 * arch/powerpc with bootwrapper support, that recalculate the
	 * memory size from the SDRAM controller setup will have to
	 * get fixed.
	 */
	gd->ram_size -= CONFIG_SYS_MEM_TOP_HIDE;
#endif
#ifdef CONFIG_IPQ40XX_XIP
	addr= _TEXT_BASE;
#else
	addr = CONFIG_SYS_SDRAM_BASE + gd->ram_size;
#endif

#ifdef CONFIG_IPQ_APPSBL_DLOAD
	/* We reserve 2MB of memory when built with crashdump enabled */
	gd->ram_size -= (2 * 1024 * 1024);
#endif

#ifdef CONFIG_LOGBUFFER
#ifndef CONFIG_ALT_LB_ADDR
	/* reserve kernel log buffer */
	addr -= (LOGBUFF_RESERVE);
	debug("Reserving %dk for kernel logbuffer at %08lx\n", LOGBUFF_LEN,
		addr);
#endif
#endif

#ifdef CONFIG_PRAM
	/*
	 * reserve protected RAM
	 */
	reg = getenv_ulong("pram", 10, CONFIG_PRAM);
	addr -= (reg << 10);		/* size is in kB */
	debug("Reserving %ldk for protected RAM at %08lx\n", reg, addr);
#endif /* CONFIG_PRAM */

#if !(defined(CONFIG_SYS_ICACHE_OFF) && defined(CONFIG_SYS_DCACHE_OFF))
	/* reserve TLB table */
	addr -= (4096 * 4);

	/* round down to next 64 kB limit */
	addr &= ~(0x10000 - 1);

	gd->tlb_addr = addr;
	debug("TLB table at: %08lx\n", addr);
#endif

	/* round down to next 4 kB limit */
	addr &= ~(4096 - 1);
	debug("Top of RAM usable for U-Boot at: %08lx\n", addr);

#ifdef CONFIG_LCD
#ifdef CONFIG_FB_ADDR
	gd->fb_base = CONFIG_FB_ADDR;
#else
	/* reserve memory for LCD display (always full pages) */
	addr = lcd_setmem(addr);
	gd->fb_base = addr;
#endif /* CONFIG_FB_ADDR */
#endif /* CONFIG_LCD */

#ifndef CONFIG_IPQ40XX_XIP
	/*
	 * reserve memory for U-Boot code, data & bss
	 * round down to next 4 kB limit
	 */
	addr -= gd->mon_len;
	addr &= ~(4096 - 1);

	debug("Reserving %ldk for U-Boot at: %08lx\n", gd->mon_len >> 10, addr);
#endif

#ifndef CONFIG_SPL_BUILD
	/*
	 * reserve memory for malloc() arena
	 */
	addr_sp = addr - TOTAL_MALLOC_LEN;
	debug("Reserving %dk for malloc() at: %08lx\n",
			TOTAL_MALLOC_LEN >> 10, addr_sp);
	/*
	 * (permanently) allocate a Board Info struct
	 * and a permanent copy of the "global" data
	 */
	addr_sp -= sizeof (bd_t);
	bd = (bd_t *) addr_sp;
	gd->bd = bd;
	debug("Reserving %zu Bytes for Board Info at: %08lx\n",
			sizeof (bd_t), addr_sp);

#ifdef CONFIG_MACH_TYPE
	gd->bd->bi_arch_number = CONFIG_MACH_TYPE; /* board id for Linux */
#endif

	addr_sp -= sizeof (gd_t);
	id = (gd_t *) addr_sp;
	debug("Reserving %zu Bytes for Global Data at: %08lx\n",
			sizeof (gd_t), addr_sp);

	/* setup stackpointer for exeptions */
	gd->irq_sp = addr_sp;
#ifdef CONFIG_USE_IRQ
	addr_sp -= (CONFIG_STACKSIZE_IRQ+CONFIG_STACKSIZE_FIQ);
	debug("Reserving %zu Bytes for IRQ stack at: %08lx\n",
		CONFIG_STACKSIZE_IRQ+CONFIG_STACKSIZE_FIQ, addr_sp);
#endif
	/* leave 3 words for abort-stack    */
	addr_sp -= 12;

	/* 8-byte alignment for ABI compliance */
	addr_sp &= ~0x07;
#else
	addr_sp += 128;	/* leave 32 words for abort-stack   */
	gd->irq_sp = addr_sp;
#endif

	debug("New Stack Pointer is: %08lx\n", addr_sp);

#ifdef CONFIG_POST
	post_bootmode_init();
	post_run(NULL, POST_ROM | post_bootmode_get(0));
#endif

	gd->bd->bi_baudrate = gd->baudrate;
	/* Ram ist board specific, so move it to board code ... */
	dram_init_banksize();
	display_dram_config();	/* and display it */

#ifdef CONFIG_IPQ40XX_XIP
	gd->malloc_end = addr;
	gd->relocaddr = _TEXT_BASE;
	gd->start_addr_sp = addr_sp;
	gd->reloc_off = 0;
#else
	gd->relocaddr = addr;
	gd->start_addr_sp = addr_sp;
	gd->reloc_off = addr - _TEXT_BASE;
#endif

	debug("relocation Offset is: %08lx\n", gd->reloc_off);
	memcpy(id, (void *)gd, sizeof(gd_t));

#ifdef CONFIG_IPQ40XX_XIP
	relocate_code(addr_sp, id, _TEXT_BASE);
#else
	relocate_code(addr_sp, id, addr);
#endif

	/* NOTREACHED - relocate_code() does not return */
}

#if defined(ASUS_PRODUCT)
#if !defined(UBOOT_STAGE1)
void set_ver(void)
{
	int rc;

	rc = replace(0xd18a, (unsigned char*)blver, 4);
	if (rc)
		printf("\n### [set boot ver] flash write fail\n");
}

static void __call_replace(unsigned long addr, unsigned char *ptr, int len, char *msg)
{
	int rc;
	char *status = "ok";

	if (!ptr || len <= 0)
		return;

	if (!msg)
		msg = "";

	rc = replace(addr, ptr, len);
	if (rc)
		status = "fail";

	printf("\n### [%s] flash writs %s\n", msg, status);
}

void init_mac(void)
{
	int i;
	unsigned char mac[6];
	char *tmp, *end, *default_ethaddr_str = MK_STR(CONFIG_ETHADDR);

	printf("\ninit mac\n");
	for (i = 0, tmp = default_ethaddr_str; i < 6; ++i) {
		mac[i] = tmp? simple_strtoul(tmp, &end, 16):0;
		if (tmp)
			tmp = (*end)? end+1:end;
	}
	mac[5] &= 0xFC;	/* align with 4 */
	__call_replace(RAMAC0_OFFSET, mac, sizeof(mac), "init mac");

#if defined(CONFIG_DUAL_BAND)
	mac[5] += 4;
	__call_replace(RAMAC1_OFFSET, mac, sizeof(mac), "init mac2");
#endif

	__call_replace(0xd188, (unsigned char*) "DB", 2, "init countrycode");
	__call_replace(0xd180, (unsigned char*) "12345670", 8, "init pincode");
}

extern int nand_eraseenv(void);

/* Restore to default. */
int reset_to_default(void)
{
	ulong addr, size;

#if defined(CONFIG_UBI_SUPPORT)
	unsigned char *p;

	addr = CFG_NVRAM_ADDR;
	size = CFG_NVRAM_SIZE;
	p = malloc(CFG_NVRAM_SIZE);
	if (!p)
		p = (unsigned char*) CONFIG_SYS_LOAD_ADDR;

	memset(p, 0xFF, CFG_NVRAM_SIZE);
	ra_flash_erase_write(p, addr, size, 0);

	if (p != (unsigned char*) CONFIG_SYS_LOAD_ADDR)
		free(p);

	nand_eraseenv();
#else
	/* erase U-Boot environment whether it shared same block with nvram or not. */
	addr = CONFIG_ENV_ADDR;
	size = CONFIG_ENV_SIZE;
	printf("Erase 0x%08lx size 0x%lx\n", addr, size);
	ranand_set_sbb_max_addr(addr + size);
	ra_flash_erase(addr, size);
	ranand_set_sbb_max_addr(0);
#endif

	return 0;
}
#endif	/* !UBOOT_STAGE1 */

static void input_value(char *str)
{
	if (str)
		strcpy(console_buffer, str);
	else
		console_buffer[0] = '\0';

	while(1) {
		if (__readline ("==:", 1) > 0) {
			strcpy (str, console_buffer);
			break;
		}
		else
			break;
	}
}

int tftp_config(int type, char *argv[])
{
	char *s;
	char default_file[ARGV_LEN], file[ARGV_LEN], devip[ARGV_LEN], srvip[ARGV_LEN], default_ip[ARGV_LEN];
	static char buf_addr[] = "0x80060000XXX";

	printf(" Please Input new ones /or Ctrl-C to discard\n");

	memset(default_file, 0, ARGV_LEN);
	memset(file, 0, ARGV_LEN);
	memset(devip, 0, ARGV_LEN);
	memset(srvip, 0, ARGV_LEN);
	memset(default_ip, 0, ARGV_LEN);

	printf("\tInput device IP ");
	s = getenv("ipaddr");
	memcpy(devip, s, strlen(s));
	memcpy(default_ip, s, strlen(s));

	printf("(%s) ", devip);
	input_value(devip);
	setenv("ipaddr", devip);
	if (strcmp(default_ip, devip) != 0)
		modifies++;

	printf("\tInput server IP ");
	s = getenv("serverip");
	memcpy(srvip, s, strlen(s));
	memset(default_ip, 0, ARGV_LEN);
	memcpy(default_ip, s, strlen(s));

	printf("(%s) ", srvip);
	input_value(srvip);
	setenv("serverip", srvip);
	if (strcmp(default_ip, srvip) != 0)
		modifies++;

	sprintf(buf_addr, "0x%x", CONFIG_SYS_LOAD_ADDR);
	argv[1] = buf_addr;

	switch (type) {
	case SEL_LOAD_BOOT_SDRAM:	/* fall through */
	case SEL_LOAD_BOOT_WRITE_FLASH:	/* fall through */
	case SEL_LOAD_BOOT_WRITE_FLASH_BY_SERIAL:
		printf("\tInput Uboot filename ");
		strncpy(argv[2], BOOTFILENAME, ARGV_LEN);
		break;
	case SEL_LOAD_LINUX_WRITE_FLASH:/* fall through */
	case SEL_LOAD_LINUX_SDRAM:
		printf("\tInput Linux Kernel filename ");
		strncpy(argv[2], "uImage", ARGV_LEN);
		break;
	default:
		printf("%s: Unknown type %d\n", __func__, type);
	}

	s = getenv("bootfile");
	if (s != NULL) {
		memcpy(file, s, strlen(s));
		memcpy(default_file, s, strlen(s));
	}
	printf("(%s) ", file);
	input_value(file);
	if (file == NULL)
		return 1;
	copy_filename(argv[2], file, sizeof(file));
	setenv("bootfile", file);
	if (strcmp(default_file, file) != 0)
		modifies++;

	return 0;
}

#if defined(CONFIG_CMD_SOURCE)
/** Program FIT image bootloader.
 * @addr:
 * @return:
 * 	0:	success
 *  otherwise:	fail
 */
int program_bootloader_fit_image(ulong addr, ulong size)
{
	int rcode;
	char *vb, *vr;
	static char tmp[] = "$imgaddr:scriptXXXXXXXXXX";
	char *run_fit_img_script[] = { "source", tmp };
#if defined(CONFIG_UBI_SUPPORT)
#ifdef UBOOT_ON_NAND
	const struct ubi_device *ubi = get_ubi_device();
	char *ubi_detach[] = { "ubi", "detach"};
#endif /* UBOOT_ON_NAND */
#endif

	if (!addr)
		return -1;

#if defined(CONFIG_UBI_SUPPORT)
#ifdef UBOOT_ON_NAND
	/* detach UBI_DEV */
	if (ubi)
		do_ubi(NULL, 0, ARRAY_SIZE(ubi_detach), ubi_detach);
#endif /* UBOOT_ON_NAND */
#endif

#ifdef CONFIG_SYS_HUSH_PARSER
	u_boot_hush_start ();
#endif

#if defined(CONFIG_HUSH_INIT_VAR)
	hush_init_var ();
#endif

	vb = getenv("verbose");
	vr = getenv("verify");
	setenv("verbose", NULL);
	setenv("verify", NULL);

	sprintf(tmp, "%lx", addr);
	setenv("imgaddr", tmp);
	sprintf(tmp, "%lx:script", addr);
#ifdef UBOOT_ON_NAND
	ranand_set_sbb_max_addr(CFG_BOOTLOADER_SIZE);
#endif
	rcode = do_source(NULL, 0, ARRAY_SIZE(run_fit_img_script), run_fit_img_script);
#ifdef UBOOT_ON_NAND
	ranand_set_sbb_max_addr(0);
#endif
	setenv("verbose", vb);
	setenv("verify", vr);

	return rcode;
}
#endif

#if !defined(UBOOT_STAGE1)
/* System Load %s to SDRAM via TFTP */
static void handle_boottype_1(void)
{
	int argc= 3;
	char *argv[4];
	cmd_tbl_t c, *cmdtp = &c;

	argv[2] = &file_name_space[0];
	memset(file_name_space, 0, sizeof(file_name_space));

	tftp_config(SEL_LOAD_LINUX_SDRAM, argv);
	argc= 3;
	setenv("autostart", "yes");
	do_tftpb(cmdtp, 0, argc, argv);
}
#endif

/* System Load %s then write to Flash via TFTP */
static void handle_boottype_2(void)
{
	int argc= 3, confirm = 0;
	char *argv[4];
	cmd_tbl_t c, *cmdtp = &c;
	char addr_str[11];
	unsigned int load_address;

	argv[2] = &file_name_space[0];
	memset(file_name_space, 0, sizeof(file_name_space));

	printf(" Warning!! Erase Linux in Flash then burn new one. Are you sure?(Y/N)\n");
	confirm = getc();
	if (confirm != 'y' && confirm != 'Y') {
		printf(" Operation terminated\n");
		return;
	}
#if defined(UBOOT_STAGE1)
	setenv("bootfile", "uboot_stage2.img");
#endif
	tftp_config(SEL_LOAD_LINUX_WRITE_FLASH, argv);
	argc= 3;
	setenv("autostart", "no");
	do_tftpb(cmdtp, 0, argc, argv);

	load_address = simple_strtoul(argv[1], NULL, 16);
	{
#if defined(UBOOT_STAGE1)
		struct stage2_loc *s2 = &g_s2_loc;

		ranand_write_stage2(load_address, NetBootFileXferSize);
		ranand_locate_stage2(s2);
		sprintf(addr_str, "0x%X", s2->good->code);
#else
		ra_flash_erase_write((uchar*)load_address+sizeof(image_header_t), CFG_KERN_ADDR, NetBootFileXferSize-sizeof(image_header_t), 0);
#if defined(CONFIG_DUAL_TRX)
		ra_flash_erase_write((uchar*)load_address+sizeof(image_header_t), CFG_KERN2_ADDR, NetBootFileXferSize-sizeof(image_header_t), 0);
#endif
#endif
	}

	argc= 2;
#if !defined(UBOOT_STAGE1)
	sprintf(addr_str, "0x%X#config@1", load_address+sizeof(image_header_t));
#endif
	argv[1] = &addr_str[0];
	do_bootm(cmdtp, 0, argc, argv);
}

/* System Boot Linux via Flash */
static void handle_boottype_3(void)
{
	char *argv[2] = {"", ""};
	char addr_str[30];
	cmd_tbl_t c, *cmdtp = &c;

#if !defined(UBOOT_STAGE1)
	sprintf(addr_str, "0x%X#config@1", CFG_KERN_ADDR);
	argv[1] = &addr_str[0];
#endif

#if !defined(UBOOT_STAGE1)
	if (!chkVer())
	       set_ver();

	if ((chkMAC()) < 0)
	       init_mac();
#endif /* ! UBOOT_STAGE1 */

	/* eth_initialize(gd->bd); */
#if defined(ASUS_PRODUCT) && defined(CONFIG_IPQ40XX)
	sw_gpio_init();
#endif

	do_tftpd(cmdtp, 0, 2, argv);
#if defined(ASUS_PRODUCT)
	leds_off();
	power_led_on();
#endif
}

/* System Enter Boot Command Line Interface */
static void handle_boottype_4(void)
{
	printf ("\n%s\n", version_string);
	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;) {
		main_loop ();
	}
}

#if 0 //!defined(UBOOT_STAGE1)
/* System Load %s to SDRAM via Serial (*.bin) */
static void handle_boottype_5(void)
{
	int my_tmp, argc= 3;
	char *argv[4];
	char tftp_load_addr[] = "0x81000000XXX";
	cmd_tbl_t c, *cmdtp = &c;

	argv[2] = &file_name_space[0];
	memset(file_name_space, 0, sizeof(file_name_space));
	sprintf(tftp_load_addr, "0x%x", CONFIG_SYS_LOAD_ADDR);

	argc= 4;
	argv[1] = tftp_load_addr;
	setenv("autostart", "yes");
	my_tmp = do_load_serial_bin(cmdtp, 0, argc, argv);
	NetBootFileXferSize=simple_strtoul(getenv("filesize"), NULL, 16);

	if (NetBootFileXferSize > CFG_BOOTLOADER_SIZE || my_tmp == 1)
		printf("Abort: Bootloader is too big or download aborted!\n");
}
#endif

/* System Load %s then write to Flash via Serial */
static void handle_boottype_7(void)
{
	int my_tmp, argc= 3;
	cmd_tbl_t c, *cmdtp = &c;
	char tftp_load_addr[] = "0x81000000XXX";
	char *argv[4] = { "loadb", tftp_load_addr, NULL, NULL };
	unsigned int addr = CONFIG_SYS_LOAD_ADDR;

	sprintf(tftp_load_addr, "0x%x", addr);
	argv[1] = tftp_load_addr;
	argc=2;
	setenv("autostart", "no");
	my_tmp = do_load_serial_bin(cmdtp, 0, argc, argv);
	if (my_tmp == 1) {
		printf("Abort: Load bootloader from serial fail!\n");
		return;
	}

	NetBootFileXferSize = simple_strtoul(getenv("filesize"), NULL, 16);
	if (NetBootFileXferSize > 0 && NetBootFileXferSize <= CONFIG_MAX_BL_BINARY_SIZE)
		program_bootloader(addr, NetBootFileXferSize);

	//reset
	do_reset(cmdtp, 0, argc, argv);
}

#if 0 //!defined(UBOOT_STAGE1)
/* System Load %s to SDRAM via TFTP.(*.bin) */
static void handle_boottype_8(void)
{
	int argc= 3;
	char *argv[4];
	cmd_tbl_t c, *cmdtp = &c;

	argv[2] = &file_name_space[0];
	memset(file_name_space, 0, sizeof(file_name_space));

	tftp_config(SEL_LOAD_BOOT_SDRAM, argv);
	argc= 5;
	setenv("autostart", "yes");
	do_tftpb(cmdtp, 0, argc, argv);
}
#endif

/* System Load %s then write to Flash via TFTP. (.bin) */
static void handle_boottype_9(void)
{
	int argc= 3, confirm = 0;
	char *argv[4];
	cmd_tbl_t c, *cmdtp = &c;

	argv[2] = &file_name_space[0];
	memset(file_name_space, 0, sizeof(file_name_space));

	printf(" Warning!! Erase %s in Flash then burn new one. Are you sure?(Y/N)\n", BOOT_IMAGE_NAME);
	confirm = getc();
	if (confirm != 'y' && confirm != 'Y') {
		printf(" Operation terminated\n");
		return;
	}
	setenv("bootfile", BOOTFILENAME);
	tftp_config(SEL_LOAD_BOOT_WRITE_FLASH, argv);
	argc= 3;
	setenv("autostart", "no");
	do_tftpb(cmdtp, 0, argc, argv);
	if (NetBootFileXferSize > 0 && NetBootFileXferSize <= CONFIG_MAX_BL_BINARY_SIZE)
		program_bootloader(CONFIG_SYS_LOAD_ADDR, NetBootFileXferSize);

	//reset
	do_reset(cmdtp, 0, argc, argv);
}

#if 0 //defined (CONFIG_ENV_IS_IN_NAND)
/* System Load %s then write to Flash via Serial */
static void handle_boottype_0(void)
{
	int argc= 3;
	char *argv[4];
	cmd_tbl_t c, *cmdtp = &c;

	argc= 1;
	setenv("autostart", "no");
	do_load_serial_bin(cmdtp, 0, argc, argv);
	NetBootFileXferSize=simple_strtoul(getenv("filesize"), NULL, 16);
#if defined(UBOOT_STAGE1)
	ranand_write_stage2(CONFIG_SYS_LOAD_ADDR, NetBootFileXferSize);
#else
	ra_flash_erase_write((uchar*) CONFIG_SYS_LOAD_ADDR, CFG_KERN_ADDR, NetBootFileXferSize, 0);
#endif

	//reset
	do_reset(cmdtp, 0, argc, argv);
}
#endif

static struct boot_menu_s {
	char type;
	void (*func)(void);
	char *msg;
	const char *param1;
} boot_menu[] = {
#if 0 //defined(CONFIG_ENV_IS_IN_NAND)
	{ SEL_LOAD_LINUX_WRITE_FLASH_BY_SERIAL,	handle_boottype_0, "Load %s then write to Flash via Serial.", SYS_IMAGE_NAME },
#endif
#if !defined(UBOOT_STAGE1)
	{ SEL_LOAD_LINUX_SDRAM,			handle_boottype_1, "Load %s to SDRAM via TFTP.", SYS_IMAGE_NAME },
#endif
	{ SEL_LOAD_LINUX_WRITE_FLASH,		handle_boottype_2, "Load %s then write to Flash via TFTP.", SYS_IMAGE_NAME },
	{ SEL_BOOT_FLASH,			handle_boottype_3, "Boot %s via Flash (default).", SYS_IMAGE_NAME },
	{ SEL_ENTER_CLI,			handle_boottype_4, "Entr boot command line interface.", NULL },
#if 0 //!defined(UBOOT_STAGE1)
	{ SEL_LOAD_BOOT_SDRAM_VIA_SERIAL,	handle_boottype_5, "Load %s to SDRAM via Serial.", BOOT_IMAGE_NAME },
#endif
	{ SEL_LOAD_BOOT_WRITE_FLASH_BY_SERIAL,	handle_boottype_7, "Load %s then write to Flash via Serial.", BOOT_IMAGE_NAME },
#if 0 //!defined(UBOOT_STAGE1)
	{ SEL_LOAD_BOOT_SDRAM,			handle_boottype_8, "Load %s to SDRAM via TFTP.", BOOT_IMAGE_NAME },
#endif
	{ SEL_LOAD_BOOT_WRITE_FLASH,		handle_boottype_9, "Load %s then write to Flash via TFTP.", BOOT_IMAGE_NAME },

	{ 0, NULL, NULL, NULL },
};

static int OperationSelect(void)
{
	char valid_boot_type[16];
	char msg[256];
	struct boot_menu_s *p = &boot_menu[0];
	char *s = getenv ("bootdelay"), *q = &valid_boot_type[0];
	int my_tmp, BootType = '3', timer1 = s ? (int)simple_strtol(s, NULL, 10) : CONFIG_BOOTDELAY;

	memset(valid_boot_type, 0, sizeof(valid_boot_type));
	printf("\nPlease choose the operation: \n");
	while (p->func) {
		*q++ = p->type;
		sprintf(msg, "   %c: %s\n", p->type, p->msg);
		if (p->param1)
			printf(msg, p->param1);
		else
			printf(msg);

		p++;
	}
	*q = '\0';

	if (timer1 > 5)
		timer1 = 5;
#if defined(UBOOT_STAGE1) || defined(UBOOT_STAGE2)
	if (timer1 <= CONFIG_BOOTDELAY)
		timer1 = 0;
#endif

	timer1 *= 100;
	if (!timer1)
		timer1 = 20;
	while (timer1 > 0) {
		--timer1;
		/* delay 10ms */
		if ((my_tmp = tstc()) != 0) {	/* we got a key press	*/
			timer1 = 0;	/* no more delay	*/
			BootType = getc();
			if (!strchr(valid_boot_type, BootType))
				BootType = '3';
			printf("\n\rYou choosed %c\n\n", BootType);
			break;
		}
		if (DETECT() || DETECT_WPS()) {
			BootType = '3';
			break;
		}
		udelay (10000);
		if ((timer1 / 100 * 100) == timer1)
			printf ("\b\b\b%2d ", timer1 / 100);
	}
	putc ('\n');

	return BootType;
}

#endif /* ASUS_PRODUCT */

#if !defined(CONFIG_SYS_NO_FLASH)
static char *failed = "*** failed ***\n";
#endif

/*
 ************************************************************************
 *
 * This is the next part if the initialization sequence: we are now
 * running from RAM and have a "normal" C environment, i. e. global
 * data can be written, BSS has been cleared, the stack size in not
 * that critical any more, etc.
 *
 ************************************************************************
 */

void board_init_r(gd_t *id, ulong dest_addr)
{
	ulong malloc_start;
#if !defined(CONFIG_SYS_NO_FLASH)
	ulong flash_size;
#endif
#ifdef CONFIG_IPQ_APPSBL_DLOAD
	unsigned long * dmagic1 = (unsigned long *) 0x2A03F000;
	unsigned long * dmagic2 = (unsigned long *) 0x2A03F004;
#endif
#if defined(ASUS_PRODUCT)
	cmd_tbl_t *cmdtp = NULL;
	char *argv[4], msg[256];
	int argc = 3, BootType = '3';
	struct boot_menu_s *p = &boot_menu[0];

	argv[2] = &file_name_space[0];
	file_name_space[0] = '\0';
#endif

	gd = id;

	gd->flags |= GD_FLG_RELOC;	/* tell others: relocation done */
	bootstage_mark_name(BOOTSTAGE_ID_START_UBOOT_R, "board_init_r");

	monitor_flash_len = _end_ofs;

	/* Enable caches */
	enable_caches();

	debug("monitor flash len: %08lX\n", monitor_flash_len);
	board_init();	/* Setup chipselects */
	/*
	 * TODO: printing of the clock inforamtion of the board is now
	 * implemented as part of bdinfo command. Currently only support for
	 * davinci SOC's is added. Remove this check once all the board
	 * implement this.
	 */
#ifdef CONFIG_CLOCKS
	set_cpu_clk_info(); /* Setup clock information */
#endif
#ifdef CONFIG_SERIAL_MULTI
	serial_initialize();
#endif

	debug("Now running in RAM - U-Boot at: %08lx\n", dest_addr);

#ifdef CONFIG_LOGBUFFER
	logbuff_init_ptrs();
#endif
#ifdef CONFIG_POST
	post_output_backlog();
#endif

	/* The Malloc area is immediately below the monitor copy in DRAM */
#ifdef CONFIG_IPQ40XX_XIP
	malloc_start = gd->malloc_end - TOTAL_MALLOC_LEN;
#else
	malloc_start = dest_addr - TOTAL_MALLOC_LEN;
#endif
	mem_malloc_init (malloc_start, TOTAL_MALLOC_LEN);

	printf("Maximum malloc length: %d KBytes\n", CONFIG_SYS_MALLOC_LEN >> 10);
	printf("mem_malloc_start/brk/end: 0x%lx/%lx/%lx\n",
		mem_malloc_start, mem_malloc_brk, mem_malloc_end);
	printf("Relocation offset: %lx\n", gd->reloc_off);

#ifdef CONFIG_ARCH_EARLY_INIT_R
	arch_early_init_r();
#endif

#if !defined(CONFIG_SYS_NO_FLASH)
	puts("Flash: ");

	flash_size = flash_init();
	if (flash_size > 0) {
# ifdef CONFIG_SYS_FLASH_CHECKSUM
		char *s = getenv("flashchecksum");

		print_size(flash_size, "");
		/*
		 * Compute and print flash CRC if flashchecksum is set to 'y'
		 *
		 * NOTE: Maybe we should add some WATCHDOG_RESET()? XXX
		 */
		if (s && (*s == 'y')) {
			printf("  CRC: %08X", crc32(0,
				(const unsigned char *) CONFIG_SYS_FLASH_BASE,
				flash_size));
		}
		putc('\n');
# else	/* !CONFIG_SYS_FLASH_CHECKSUM */
		print_size(flash_size, "\n");
# endif /* CONFIG_SYS_FLASH_CHECKSUM */
	} else {
		puts(failed);
		hang();
	}
#endif

#if defined(CONFIG_CMD_NAND)
	puts("NAND:  ");
	nand_init();		/* go init the NAND */
#endif

#if defined(CONFIG_CMD_ONENAND)
	onenand_init();
#endif

#ifdef CONFIG_GENERIC_MMC
       puts("MMC:   ");
       mmc_initialize(gd->bd);
#endif

#ifdef CONFIG_HAS_DATAFLASH
	AT91F_DataflashInit();
	dataflash_print_info();
#endif

	/* initialize environment */
	env_relocate();

#if defined(CONFIG_CMD_PCI) || defined(CONFIG_PCI)
	arm_pci_init();
#endif

	stdio_init();	/* get the devices list going. */

	jumptable_init();

#if defined(CONFIG_API)
	/* Initialize API */
	api_init();
#endif

	console_init_r();	/* fully init console as a device */

#if defined(CONFIG_ARCH_MISC_INIT)
	/* miscellaneous arch dependent initialisations */
	arch_misc_init();
#endif
#if defined(CONFIG_MISC_INIT_R)
	/* miscellaneous platform dependent initialisations */
	misc_init_r();
#endif

#if defined(CONFIG_UBI_SUPPORT)
#ifdef UBOOT_ON_NAND
	ranand_check_and_fix_bootloader();
#endif /* UBOOT_ON_NAND */
#endif

	 /* set up exceptions */
	interrupt_init();
	/* enable exceptions */
	enable_interrupts();

#ifdef CONFIG_IPQ_APPSBL_DLOAD
	/* check if we are in download mode */
	if (*dmagic1 == 0xE47B337D && *dmagic2 == 0x0501CAB0) {
		/* clear the magic and run the dump command */
		*dmagic1 = 0;
		*dmagic2 = 0;
		printf("\nCrashdump magic found, clear it and reboot!\n");
		/* reset the system, some images might not be loaded
		 * when crashmagic is found
		 */
		run_command("reset", 0);
	}
#endif

	/* Perform network card initialisation if necessary */
#if defined(CONFIG_DRIVER_SMC91111) || defined (CONFIG_DRIVER_LAN91C96)
	/* XXX: this needs to be moved to board init */
	if (getenv("ethaddr")) {
		uchar enetaddr[6];
		eth_getenv_enetaddr("ethaddr", enetaddr);
		smc_set_mac_addr(enetaddr);
	}
#endif /* CONFIG_DRIVER_SMC91111 || CONFIG_DRIVER_LAN91C96 */

	/* Initialize from environment */
	load_addr = getenv_ulong("loadaddr", 16, load_addr);

#ifdef CONFIG_BOARD_LATE_INIT
	board_late_init();
#endif

#ifdef CONFIG_BITBANGMII
	bb_miiphy_init();
#endif
#if defined(CONFIG_CMD_NET)
#if !defined(CONFIG_IPQ_ETH_INIT_DEFER)
	puts("Net:   ");
	eth_initialize(gd->bd);
#endif
#if defined(CONFIG_RESET_PHY_R)
	debug("Reset Ethernet PHY\n");
	reset_phy();
#endif
#endif

#ifdef CONFIG_POST
	post_run(NULL, POST_RAM | post_bootmode_get(0));
#endif

#if defined(CONFIG_PRAM) || defined(CONFIG_LOGBUFFER)
	/*
	 * Export available size of memory for Linux,
	 * taking into account the protected RAM at top of memory
	 */
	{
		ulong pram = 0;
		uchar memsz[32];

#ifdef CONFIG_PRAM
		pram = getenv_ulong("pram", 10, CONFIG_PRAM);
#endif
#ifdef CONFIG_LOGBUFFER
#ifndef CONFIG_ALT_LB_ADDR
		/* Also take the logbuffer into account (pram is in kB) */
		pram += (LOGBUFF_LEN + LOGBUFF_OVERHEAD) / 1024;
#endif
#endif
		sprintf((char *)memsz, "%ldk", (gd->ram_size / 1024) - pram);
		setenv("mem", (char *)memsz);
	}
#endif

#if defined(ASUS_PRODUCT)
	/* Boot Loader Menu */

	//LANWANPartition();	/* FIXME */
	disable_all_leds();	/* Inhibit ALL LED, except PWR LED. */
	leds_off();
	power_led_on();

	ra_flash_init_layout();

	BootType = OperationSelect();
	for (p = &boot_menu[0]; p->func; ++p ) {
		if (p->type != BootType) {
			continue;
		}

		sprintf(msg, "   %c: %s\n", p->type, p->msg);
		if (p->param1)
			printf(msg, p->param1);
		else
			printf(msg);

		p->func();
		break;
	}

	if (!p->func) {
		printf("   \nSystem Boot Linux via Flash.\n");
		do_bootm(cmdtp, 0, 1, argv);
	}

	for (;;) {
		do_reset(cmdtp, 0, argc, argv);
	}
#else	/* !ASUS_PRODUCT */
	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;) {
		main_loop();
	}
#endif	/* ASUS_PRODUCT */

	/* NOTREACHED - no way out of command loop except booting */
}

void hang(void)
{
	puts("### ERROR ### Please RESET the board ###\n");
	for (;;);
}
