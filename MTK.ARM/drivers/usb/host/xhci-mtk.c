#include <common.h>

#include "xhci-mtk.h"
#include "xhci.h"
#include "mtk-phy.h"

/* Declare global data pointer */
DECLARE_GLOBAL_DATA_PTR;

/**
 * Contains pointers to register base addresses
 * for the usb controller.
 */

void reinitIP(void)
{
	enableAllClockPower();
	mtk_xhci_scheduler_init();
}

int xhci_hcd_init(int index, struct xhci_hccr **hccr, struct xhci_hcor **hcor)
{
/*
	debug("xhci_hcd_init\n");
	u3phy_init();
	u2_slew_rate_calibration(u3phy);
	u2_slew_rate_calibration(u3phy_p1);

	debug("mt7621_phy_init\n");
	mt7623_phy_init(u3phy);

        reinitIP();

	*hccr = (uint32_t)XHC_IO_START;
	*hcor = (struct xhci_hcor *)((uint32_t) *hccr
				+ HC_LENGTH(xhci_readl(&(*hccr)->cr_capbase)));

	debug("mtk-xhci: init hccr %x and hcor %x hc_length %d\n",
		(uint32_t)*hccr, (uint32_t)*hcor,
		(uint32_t)HC_LENGTH(xhci_readl(&(*hccr)->cr_capbase)));

	return 0;
*/
    	int PhyDrv;
	int TimeDelay;

	PhyDrv = 2;
	TimeDelay = U3_PHY_PIPE_PHASE_TIME_DELAY;
	mt7623_ethifsys_init();
	//debug("*** %s ***\n", __func__);	
	u3phy_init();
	//debug("phy registers and operations initial done\n");
#ifdef CONFIG_PROJECT_7623
#if CFG_DEV_U3H0
	u2_slew_rate_calibration(u3phy);
#endif
#if CFG_DEV_U3H1
	u3phy_ops->u2_slew_rate_calibration_p1(u3phy);
#endif
#else
	if(u3phy_ops->u2_slew_rate_calibration){
		u3phy_ops->u2_slew_rate_calibration(u3phy);
	}
	else{
		debug("WARN: PHY doesn't implement port0 u2 slew rate calibration function\n");
	}

#endif

	if(u3phy_ops->init(u3phy) != PHY_TRUE){
		debug("WARN: PHY INIT FAIL\n");
		return 1;
	}

        reinitIP();

	*hccr = (uint32_t)XHC_IO_START;
	*hcor = (struct xhci_hcor *)((uint32_t) *hccr
				+ HC_LENGTH(xhci_readl(&(*hccr)->cr_capbase)));

	debug("mtk-xhci: init hccr %x and hcor %x hc_length %d\n",
		(uint32_t)*hccr, (uint32_t)*hcor,
		(uint32_t)HC_LENGTH(xhci_readl(&(*hccr)->cr_capbase)));

	if((u3phy_ops->change_pipe_phase(u3phy, PhyDrv, TimeDelay)) != PHY_TRUE){
		debug("WARN: PHY change_pipe_phase FAIL\n");
		return 1;
	}

	return 0;
}

void xhci_hcd_stop(int index)
{
	disablePortClockPower();
}

//PHY_INT32 sysRegRead(PHY_UINT32 addr)
//{
//	return os_readl(addr);
//}
//
//PHY_INT32 sysRegWrite(PHY_UINT32 addr, PHY_UINT32 data)
//{
//	os_writel(addr, data);
//
//	return 0;
//}
//
//void mt7623_ethifsys_init(void)
//{
//#define APMIXEDSYS_BASE             0xF0209000
//#define SPM_BASE                    0xF0006000
//#define TRGPLL_CON0             (APMIXEDSYS_BASE+0x280)
//#define TRGPLL_CON1             (APMIXEDSYS_BASE+0x284)
//#define TRGPLL_CON2             (APMIXEDSYS_BASE+0x288)
//#define TRGPLL_PWR_CON0         (APMIXEDSYS_BASE+0x28C)
//#define ETHPLL_CON0             (APMIXEDSYS_BASE+0x290)
//#define ETHPLL_CON1             (APMIXEDSYS_BASE+0x294)
//#define ETHPLL_CON2             (APMIXEDSYS_BASE+0x298)
//#define ETHPLL_PWR_CON0         (APMIXEDSYS_BASE+0x29C)
//#define ETH_PWR_CON             (SPM_BASE+0x2A0)
//#define HIF_PWR_CON             (SPM_BASE+0x2A4)
//
//	PHY_UINT32 temp, pwr_ack_status;
//	/*=========================================================================*/
//	/* Enable ETHPLL & TRGPLL*/
//	/*=========================================================================*/
//	/* xPLL PWR ON*/
//	temp = sysRegRead(ETHPLL_PWR_CON0);
//	sysRegWrite(ETHPLL_PWR_CON0, temp | 0x1);
//
//	temp = sysRegRead(TRGPLL_PWR_CON0);
//	sysRegWrite(TRGPLL_PWR_CON0, temp | 0x1);
//
//	udelay(5); /* wait for xPLL_PWR_ON ready (min delay is 1us)*/
//
//	/* xPLL ISO Disable*/
//	temp = sysRegRead(ETHPLL_PWR_CON0);
//	sysRegWrite(ETHPLL_PWR_CON0, temp & ~0x2);
//
//	temp = sysRegRead(TRGPLL_PWR_CON0);
//	sysRegWrite(TRGPLL_PWR_CON0, temp & ~0x2);
//
//	/* xPLL Frequency Set*/
//	temp = sysRegRead(ETHPLL_CON0);
//	sysRegWrite(ETHPLL_CON0, temp | 0x1);
//	sysRegWrite(TRGPLL_CON1, 0xCCEC4EC5);
//	sysRegWrite(TRGPLL_CON0,  0x121);
//	udelay(40); /* wait for PLL stable (min delay is 20us)*/
//
//
//	/*=========================================================================*/
//	/* Power on ETHDMASYS and HIFSYS*/
//	/*=========================================================================*/
//	/* Power on ETHDMASYS*/
//	sysRegWrite(SPM_BASE+0x000, 0x0b160001);
//	pwr_ack_status = (sysRegRead(ETH_PWR_CON) & 0x0000f000) >> 12;
//
//	if(pwr_ack_status == 0x0) {
//		debug("ETH already turn on and power on flow will be skipped...\n");
//	}else {
//		temp = sysRegRead(ETH_PWR_CON)	;
//		sysRegWrite(ETH_PWR_CON, temp | 0x4);	       /* PWR_ON*/
//		temp = sysRegRead(ETH_PWR_CON)	;
//		sysRegWrite(ETH_PWR_CON, temp | 0x8);	       /* PWR_ON_S*/
//
//		udelay(5); /* wait power settle time (min delay is 1us)*/
//
//		temp = sysRegRead(ETH_PWR_CON)	;
//		sysRegWrite(ETH_PWR_CON, temp & ~0x10);      /* PWR_CLK_DIS*/
//		temp = sysRegRead(ETH_PWR_CON)	;
//		sysRegWrite(ETH_PWR_CON, temp & ~0x2);	      /* PWR_ISO*/
//		temp = sysRegRead(ETH_PWR_CON)	;
//		sysRegWrite(ETH_PWR_CON, temp & ~0x100);   /* SRAM_PDN 0*/
//		temp = sysRegRead(ETH_PWR_CON)	;
//		sysRegWrite(ETH_PWR_CON, temp & ~0x200);   /* SRAM_PDN 1*/
//		temp = sysRegRead(ETH_PWR_CON)	;
//		sysRegWrite(ETH_PWR_CON, temp & ~0x400);   /* SRAM_PDN 2*/
//		temp = sysRegRead(ETH_PWR_CON)	;
//		sysRegWrite(ETH_PWR_CON, temp & ~0x800);   /* SRAM_PDN 3*/
//
//		udelay(5); /* wait SRAM settle time (min delay is 1Us)*/
//
//		temp = sysRegRead(ETH_PWR_CON)	;
//		sysRegWrite(ETH_PWR_CON, temp | 0x1);	       /* PWR_RST_B*/
//	}
//
//	/* Power on HIFSYS*/
//	pwr_ack_status = (sysRegRead(HIF_PWR_CON) & 0x0000f000) >> 12;
//	if(pwr_ack_status == 0x0) {
//		debug("HIF already turn on and power on flow will be skipped...\n");
//	}
//	else {
//		temp = sysRegRead(HIF_PWR_CON)  ;
//		sysRegWrite(HIF_PWR_CON, temp | 0x4);          /* PWR_ON*/
//		temp = sysRegRead(HIF_PWR_CON)  ;
//		sysRegWrite(HIF_PWR_CON, temp | 0x8);          /* PWR_ON_S*/
//
//		udelay(5); /* wait power settle time (min delay is 1us)*/
//
//		temp = sysRegRead(HIF_PWR_CON)  ;
//		sysRegWrite(HIF_PWR_CON, temp & ~0x10);      /* PWR_CLK_DIS*/
//		temp = sysRegRead(HIF_PWR_CON)  ;
//		sysRegWrite(HIF_PWR_CON, temp & ~0x2);        /* PWR_ISO*/
//		temp = sysRegRead(HIF_PWR_CON)  ;
//		sysRegWrite(HIF_PWR_CON, temp & ~0x100);   /* SRAM_PDN 0*/
//		temp = sysRegRead(HIF_PWR_CON)  ;
//		sysRegWrite(HIF_PWR_CON, temp & ~0x200);   /* SRAM_PDN 1*/
//		temp = sysRegRead(HIF_PWR_CON)  ;
//		sysRegWrite(HIF_PWR_CON, temp & ~0x400);   /* SRAM_PDN 2*/
//		temp = sysRegRead(HIF_PWR_CON)  ;
//		sysRegWrite(HIF_PWR_CON, temp & ~0x800);   /* SRAM_PDN 3*/
//
//		udelay(5); /* wait SRAM settle time (min delay is 1Us)*/
//
//		temp = sysRegRead(HIF_PWR_CON)  ;
//		sysRegWrite(HIF_PWR_CON, temp | 0x1);          /* PWR_RST_B*/
//	}
//
//}
