#include <common.h>
#include <config.h>

#include <asm/arch/mt6735.h>
#include <asm/arch/typedefs.h>
#include <asm/arch/wdt.h>

#define RESET_TIMEOUT 10

void reset_cpu(ulong addr)
{
    mtk_arch_reset(0);
}

