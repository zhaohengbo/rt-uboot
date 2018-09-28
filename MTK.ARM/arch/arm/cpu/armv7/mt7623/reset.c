#include <common.h>
#include <config.h>

#include <asm/arch//mt_reg_base.h>
#include <asm/arch//mt_typedefs.h>
#include <asm/arch//mtk_wdt.h>

#define RESET_TIMEOUT 10

void reset_cpu(ulong addr)
{
    mtk_wdt_hw_reset();
}

