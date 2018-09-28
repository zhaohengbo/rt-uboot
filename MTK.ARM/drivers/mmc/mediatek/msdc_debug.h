#ifndef __MSDC_DEBUG_H__
#define __MSDC_DEBUG_H__
/*  zhanyong.wang  2014/11/11 */   
#include "msdc_cfg.h"

#ifndef ___printf
int ___printf(const char *fmt, ...);
#endif 


#define WHEREDOESMSDCRUN           "MSDC@PL" 

typedef struct MSDCDBGZNHELPER {
  const char *cszWhere2BeRun; 
        char  szZnFiler[32][10];
unsigned int  u4MsdcDbgZoneMask;
}MSDCDBGZNHELPER;

#ifdef KEEP_SLIENT_BUILD

#define MSDCDBGZONE(znbval)     0

#define MSDC_ERR_PRINT(...)
#define MSDC_TRC_PRINT(...)
#define MSDC_DBG_PRINT(...)
#define MSDC_DBGCHK(...)
#define MSDC_BUGON(exp)

#define MSDC_ASSERT(exp)

#define MSDC_RAW_PRINT(...)


#else 

#define MSDCDBGZONE(znbval)        (g_MsdcZnHelper.u4MsdcDbgZoneMask&(znbval))

#define MSDC_DBGZONE_INIT          (1 << 0)   
#define MSDC_DBGZONE_WARNING       (1 << 1) 
#define MSDC_DBGZONE_ERROR         (1 << 2) 
#define MSDC_DBGZONE_FATAL         (1 << 3)
#define MSDC_DBGZONE_INFO          (1 << 4) 
#define MSDC_DBGZONE_PERF          (1 << 5) 
#define MSDC_DBGZONE_DEBUG         (1 << 6) 
#define MSDC_DBGZONE_FUNC          (1 << 7) 

#define MSDC_DBGZONE_HREG          (1 << 8) 
#define MSDC_DBGZONE_DREG          (1 << 9)
#define MSDC_DBGZONE_TUNING        (1 << 10)
#define MSDC_DBGZONE_GPIO          (1 << 11)
#define MSDC_DBGZONE_PMIC          (1 << 12)
#define MSDC_DBGZONE_PLL           (1 << 13)
#define MSDC_DBGZONE_DMA           (1 << 14)
#define MSDC_DBGZONE_PIO           (1 << 15)
#define MSDC_DBGZONE_IRQ           (1 << 16)

#define MSDC_DBGZONE_STACK         (1 << 20)
#define MSDC_DBGZONE_HSTATE        (1 << 21)
#define MSDC_DBGZONE_DSTATUS       (1 << 22)

#define STR_MSDC_INIT               "INIT" 
#define STR_MSDC_WARNING            "WARN"  
#define STR_MSDC_ERROR              "ERROR" 
#define STR_MSDC_FATAL              "FATAL" 
#define STR_MSDC_INFO               "INFO" 
#define STR_MSDC_PERF               "PERF" 
#define STR_MSDC_DEBUG              "DEBUG" 
#define STR_MSDC_FUNC               "FUNC" 

#define STR_MSDC_HREG               "HREG"
#define STR_MSDC_DREG               "DREG"
#define STR_MSDC_TUNING             "TUNING"
#define STR_MSDC_GPIO               "GPIO"
#define STR_MSDC_PMIC               "PMIC"
#define STR_MSDC_PLL                "PLL"
#define STR_MSDC_DMA                "DMA"
#define STR_MSDC_PIO                "PIO"
#define STR_MSDC_IRQ                "IRQ"

#define STR_MSDC_STACK              "STACK"
#define STR_MSDC_HSTATE             "HSTATE"
#define STR_MSDC_DSTATUS            "DSTATUS"

#define STR_MSDC_UNUSED             "UNUSED"

#define MSDC_FILTER_NAME(cond)      g_MsdcZnHelper.szZnFiler[POWER2ROOT(cond)]


#define MSDC_INIT                  MSDCDBGZONE(MSDC_DBGZONE_INIT) 
#define MSDC_WARN                  MSDCDBGZONE(MSDC_DBGZONE_WARNING)   
#define MSDC_ERROR                 MSDCDBGZONE(MSDC_DBGZONE_ERROR) 
#define MSDC_FATAL                 MSDCDBGZONE(MSDC_DBGZONE_FATAL) 
#define MSDC_INFO                  MSDCDBGZONE(MSDC_DBGZONE_INFO) 
#define MSDC_PERF                  MSDCDBGZONE(MSDC_DBGZONE_PERF) 
#define MSDC_DEBUG                 MSDCDBGZONE(MSDC_DBGZONE_DEBUG) 
#define MSDC_FUNC                  MSDCDBGZONE(MSDC_DBGZONE_FUNC) 

#define MSDC_HREG                  MSDCDBGZONE(MSDC_DBGZONE_HREG)
#define MSDC_DREG                  MSDCDBGZONE(MSDC_DBGZONE_DREG)
#define MSDC_TUNING                MSDCDBGZONE(MSDC_DBGZONE_TUNING)
#define MSDC_GPIO                  MSDCDBGZONE(MSDC_DBGZONE_GPIO)
#define MSDC_PMIC                  MSDCDBGZONE(MSDC_DBGZONE_PMIC)
#define MSDC_PLL                   MSDCDBGZONE(MSDC_DBGZONE_PLL)
#define MSDC_DMA                   MSDCDBGZONE(MSDC_DBGZONE_DMA)
#define MSDC_PIO                   MSDCDBGZONE(MSDC_DBGZONE_PIO)
#define MSDC_IRQ                   MSDCDBGZONE(MSDC_DBGZONE_IRQ)

#define MSDC_STACK                 MSDCDBGZONE(MSDC_DBGZONE_STACK)
#define MSDC_HSTATE                MSDCDBGZONE(MSDC_DBGZONE_HSTATE)
#define MSDC_DSTATUS               MSDCDBGZONE(MSDC_DBGZONE_DSTATUS)

#define MSDC_DBGZONE_RELEASE       MSDC_DBGZONE_INIT|MSDC_DBGZONE_ERROR|MSDC_DBGZONE_FATAL\
										|MSDC_DBGZONE_INFO|MSDC_DBGZONE_PMIC             

#ifdef ___MSDC_DEBUG___
#include <common.h>
#define ___printf(fmt, ...) printf(fmt, ## __VA_ARGS__)

#define MSDC_DBG_PRINT(cond,printf_exp)   \
   ((void)((cond) ? ___printf("%s:%s ",MSDC_FILTER_NAME(cond),WHEREDOESMSDCRUN),\
   				  ( ___printf printf_exp),1:0))
   				  
#define MSDC_DBGCHK(friendname,exp) \
   ((void)((exp)?1:(          \
       ___printf ( "%s: DBGCHK failed in file %s at line %d \r\n", \
                 friendname, __FILE__ ,__LINE__ ),    \
	   0  \
   )))

#define MSDC_BUGON(exp) MSDC_DBGCHK(WHEREDOESMSDCRUN, exp)
#ifndef MSDC_DEBUG_KICKOFF
extern  MSDCDBGZNHELPER  g_MsdcZnHelper;
unsigned int  POWER2ROOT(unsigned int u4Power);
int MUSTBE(void);
#else
MSDCDBGZNHELPER  g_MsdcZnHelper = {
	WHEREDOESMSDCRUN,
	{  STR_MSDC_INIT,    STR_MSDC_WARNING,   STR_MSDC_ERROR,   STR_MSDC_FATAL,
	   STR_MSDC_INFO,    STR_MSDC_PERF,      STR_MSDC_DEBUG,   STR_MSDC_FUNC,

       STR_MSDC_HREG,    STR_MSDC_DREG,      STR_MSDC_TUNING,  STR_MSDC_GPIO,    
       STR_MSDC_PMIC,    STR_MSDC_PLL,       STR_MSDC_DMA,     STR_MSDC_PIO, 
       
       STR_MSDC_IRQ,     STR_MSDC_UNUSED,    STR_MSDC_UNUSED,  STR_MSDC_UNUSED,
       STR_MSDC_STACK,   STR_MSDC_HSTATE,    STR_MSDC_DSTATUS, STR_MSDC_UNUSED,

	   STR_MSDC_UNUSED,  STR_MSDC_UNUSED,    STR_MSDC_UNUSED,  STR_MSDC_UNUSED,
	   STR_MSDC_UNUSED,  STR_MSDC_UNUSED,    STR_MSDC_UNUSED,  STR_MSDC_UNUSED	
	},
	MSDC_DBGZONE_RELEASE
};

unsigned int  POWER2ROOT(unsigned int u4Power)
{
    unsigned int u4Exponent = 0;
	
	while(u4Power > 1){
       u4Power >>= 1;
	   u4Exponent ++;
	}

	return u4Exponent;
}

int MUSTBE(void)
{
       while(1);
	   return 0;
}

#endif

#else // ___MSDC_DEBUG___

#define MSDC_DBG_PRINT(cond,printf_exp) ((void)0)
#define MSDC_DBGCHK(module,exp) ((void)0)
#define MSDC_BUGON(exp) ((void)0)

#endif // ___MSDC_DEBUG___



#define MSDC_ASSERT(exp) \
   ((void)((exp)?1:(          \
       ___printf( "%s: MUSTBE: failed in file %s at line %d \r\n", \
                 WHEREDOESMSDCRUN,__FILE__ ,__LINE__ ),    \
       MUSTBE(), \
       0  \
   )))

#define MSDC_TRC_PRINT(cond,printf_exp)   \
   ((cond)? ___printf("%s:%s ",MSDC_FILTER_NAME(cond),WHEREDOESMSDCRUN),\
           (___printf printf_exp),1:0)


#define MSDC_ERR_PRINT(cond,printf_exp)	 \
   ((cond)?( ___printf("%s:%s : ",MSDC_FILTER_NAME(cond),WHEREDOESMSDCRUN),\
             ___printf("***ERROR: %s line %d: ",__FILE__,__LINE__), printf printf_exp),1:0)

#define MSDC_RAW_PRINT(cond,printf_exp)   \
   ((cond)?(___printf printf_exp),1:0)

#endif     
#endif

