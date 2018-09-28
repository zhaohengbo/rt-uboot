#include <common.h>
#include <config.h>

#include <asm/arch/mt_gpt.h>
#include <asm/arch/mtk_wdt.h>
#include <asm/arch/mt_typedefs.h>
#include <asm/arch/mt_reg_base.h>
#include <asm/arch/boot_mode.h>

DECLARE_GLOBAL_DATA_PTR;

extern int rt2880_eth_initialize(bd_t *bis);
int g_nr_bank;
int g_rank_size[4];
int g_total_rank_size;
BOOT_ARGUMENT *g_boot_arg;
BOOT_ARGUMENT boot_addr;

/*
 *  Iverson 20140326 : DRAM have been initialized in preloader.
 */

int dram_init(void)
{
    /*
     * Iverson 20150909 : Use bootarg from preloader as ram size
     */ 

	int i;
	unsigned int dram_rank_num;

    /* Get parameters from pre-loader. Get as early as possible
     * The address of BOOT_ARGUMENT_LOCATION will be used by Linux later
     * So copy the parameters from BOOT_ARGUMENT_LOCATION to LK's memory region
     */
    g_boot_arg = &boot_addr;
    memcpy(g_boot_arg, (void *)BOOT_ARGUMENT_LOCATION, sizeof(BOOT_ARGUMENT));

    printf("================== Iverson debug. ===========================\n");

	dram_rank_num = g_boot_arg->dram_rank_num;
	g_nr_bank = dram_rank_num;

    printf("g_nr_bank = %d.\n", g_nr_bank);

    g_total_rank_size = 0;
	for (i = 0; i < g_nr_bank; i++) {
		g_rank_size[i] = g_boot_arg->dram_rank_size[i];
        g_total_rank_size += g_rank_size[i];
	}
    printf("g_total_rank_size = 0x%8X\n", g_total_rank_size);

    gd->ram_size = g_total_rank_size;

	return 0;
}

int board_init(void)
{
    mtk_timer_init();
    
#ifdef CONFIG_WATCHDOG_OFF
    mtk_wdt_disable();
#endif

    /* Nelson: address of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;

    return 0;
}

int board_late_init (void)
{
    gd->env_valid = 1; //to load environment variable from persistent store
    env_relocate();
#ifdef CONFIG_CMDLINE_TAG
    char *commandline;

    /* Nelson: set linux kernel boot arguments */
    setenv("bootargs", COMMANDLINE_TO_KERNEL);
    commandline = getenv("bootargs");
    printf("bootargs = %s\n", commandline);

    return 0;
#endif
}

/*
 *  Iverson todo
 */

void ft_board_setup(void *blob, bd_t *bd)
{
}

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
    /* Enable D-cache. I-cache is already enabled in start.S */
    dcache_enable();
}
#endif

int board_eth_init(bd_t *bis)
{
    rt2880_eth_initialize(bis);

    return 0;
}

