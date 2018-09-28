#include <common.h>
#include <config.h>

#include <asm/arch/typedefs.h>
#include <asm/arch/timer.h>
#include <asm/arch/wdt.h>
#include <asm/arch/mt6735.h>

DECLARE_GLOBAL_DATA_PTR;

extern int rt2880_eth_initialize(bd_t *bis);

/*
 *  Iverson 20140326 : DRAM have been initialized in preloader.
 */

int dram_init(void)
{
    /*
     *  Iverson 20140526 : Use static memory declaration.
     *      UBoot support memory auto detection. 
     *      However, we still use static declaration becasue we should modify many code if we use auto detection.
     */

    /*
     * Iverson 20150525 : Reserver 1MB memory of bottom for ATF log.
     *      It just sync from MT6735m preloader. They will reserve 1MB for ATF log.
     */

    gd->ram_size = CONFIG_SYS_SDRAM_SIZE - SZ_16M;    
    //gd->ram_size = CONFIG_SYS_SDRAM_SIZE;
    //gd->ram_size = get_ram_size((long *)CONFIG_SYS_SDRAM_BASE, CONFIG_SYS_SDRAM_SIZE);

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

    return 0;
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
#ifdef CONFIG_RT2880_ETH
    rt2880_eth_initialize(bis);
#endif
    return 0;
}

