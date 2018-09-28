#ifdef CONFIG_PROJECT_7623
#ifndef __MTK_PHY_7623_H
#define __MTK_PHY_7623_H

#include "mtk-phy.h"

#define U2_SR_COEF_7623 32 

///////////////////////////////////////////////////////////////////////////////

struct u2phy_reg {
	//0x0
	PHY_LE32 usbphyacr0;	/* 0x0 */
	PHY_LE32 usbphyacr1; 	/* 0x4 */
	PHY_LE32 usbphyacr2; 	/* 0x8 */
	PHY_LE32 reserve0;
	//0x10
	PHY_LE32 usbphyacr4; 	/* 0x10 */
	PHY_LE32 usbphyacr5; 	/* 0x14 */
	PHY_LE32 usbphyacr6; 	/* 0x18 */
	PHY_LE32 u2phyacr3;  	/* 0x1c */
	//0x20
	PHY_LE32 u2phyacr4;  	/* 0x20 */
	PHY_LE32 u2phyamon0; 	/* 0x24 */
	PHY_LE32 reserve1[2];
	//0x30~0x50
	PHY_LE32 reserve2[12];
	//0x60
	PHY_LE32 u2phydcr0;  	/* 0x60 */
	PHY_LE32 u2phydcr1;  	/* 0x64 */
	PHY_LE32 u2phydtm0;  	/* 0x68 */
	PHY_LE32 u2phydtm1;  	/* 0x6c */
	//0x70
	PHY_LE32 u2phydmon0; 	/* 0x70 */
	PHY_LE32 u2phydmon1; 	/* 0x74 */
	PHY_LE32 u2phydmon2; 	/* 0x78 */
	PHY_LE32 u2phydmon3; 	/* 0x7c */
	//0x80
	PHY_LE32 u2phybc12c;  	/* 0x80 */
	PHY_LE32 u2phybc12c1; 	/* 0x84 */
	PHY_LE32 reserve3[2];
	//0x90~0xd0
	PHY_LE32 reserve4[20];
	//0xe0
	PHY_LE32 regfppc;   	/* 0xe0 */
	PHY_LE32 reserve5[3];
	//0xf0
	PHY_LE32 versionc; 	/* 0xf0 */
	PHY_LE32 reserve6[2];
	PHY_LE32 regfcom; 	/* 0xfc */
};

/* USBPHYARC0 - 0x0 */
#define RG_USB20_MPX_OUT_SEL		(0x7 << 28) /* bit[30:28] */
#define RG_USB20_TX_PH_ROT_SE		(0x7 << 24) /* bit[26:24] */
#define RG_USB20_PLL_DIVEN		(0x7 << 20) /* bit[22:20] */
#define RG_USB20_PLL_BR			(0x1 << 18) /* bit[18] */
#define RG_USB20_PLL_BP			(0x1 << 17) /* bit[17] */
#define RG_USB20_PLL_BLP		(0x1 << 16) /* bit[16] */
#define RG_USB20_USBPLL_FORCE_ON	(0x1 << 15) /* bit[15] */
#define RG_USB20_PLL_FBDIV		(0x7f << 8) /* bit[14:8] */
#define RG_USB20_PLL_PREDIV             (0x3 << 6)  /* bit[7:6] */
#define RG_USB20_INTR_EN		(0x1 << 5)  /* bit[5] */
#define RG_USB20_REF_EN			(0x1 << 4)  /* bit[4] */
#define RG_USB20_BGR_DIV                (0x3 << 2)  /* bit[3:2] */
#define RG_SIFSLV_CHP_EN                (0x1 << 1)  /* bit[1] */
#define RG_SIFSLV_BGR_EN                (0x1 << 0)  /* bit[0] */

/* USBPHYACR1 - 0x4 */
#define RG_USB20_OTG_VBUSTH		(0x7 << 16) /* bit[18:16] */
#define RG_USB20_VRT_VREF_SEL		(0x7 << 12) /* bit[14:12] */
#define RG_USB20_TERM_VREF_SEL		(0x7 << 8)  /* bit[10:8] */
#define RG_USB20_MPX_SEL		(0xff << 0)  /* bit[7:0] */

/* USBPHYACR2 - 0x8 */
#define RG_SIFSLV_MAC_BANDGAP_EN	(0x1 << 17)  /* bit[17] */
#define RG_SIFSLV_MAC_CHOPPER_EN	(0x1 << 16)  /* bit[16] */
#define RG_USB20_CLKREF_REV		(0xffff << 0)  /* bit[15:0] */

/* USBPHYACR4 - 0x10 */
#define RG_USB20_DP_ABIST_SOURCE_EN	(0x1 << 31) /* bit[31] */
#define RG_USB20_DP_ABIST_SELE		(0xf << 24) /* bit[27:24] */
#define RG_USB20_ICUSB_EN		(0x1 << 16) /* bit[16] */
#define RG_USB20_LS_CR                  (0x7 << 12) /* bit[14:12] */
#define RG_USB20_FS_CR                  (0x7 << 8)  /* bit[10:8] */
#define RG_USB20_LS_SR                  (0x7 << 4)  /* bit[6:4] */
#define RG_USB20_FS_SR                  (0x7 << 0)  /* bit[2:0] */ 

/* USBPHYACR5 - 0x14 */
#define RG_USB20_DISC_FIT_EN		(0x1 << 28) /* bit[28] */
#define RG_USB20_INIT_SQ_EN_DG		(0x3 << 26) /* bit[27:26] */
#define RG_USB20_HSTX_TMODE_SEL		(0x3 << 24) /* bit[25:24] */
#define RG_USB20_SQD			(0x3 << 22) /* bit[23:22] */
#define RG_USB20_DISCD			(0x3 << 20) /* bit[21:20] */
#define RG_USB20_HSTX_TMODE_EN		(0x1 << 19) /* bit[19] */
#define RG_USB20_PHYD_MONEN		(0x1 << 18) /* bit[18] */
#define RG_USB20_INLPBK_EN		(0x1 << 17) /* bit[17] */
#define RG_USB20_CHIRP_EN		(0x1 << 16) /* bit[16] */
#define RG_USB20_HSTX_SRCAL_EN		(0x1 << 15) /* bit[15] */
#define RG_USB20_HSTX_SRCTRL		(0x7 << 12) /* bit[14:12] */
#define RG_USB20_HS_100U_U3_EN		(0x1 << 11) /* bit[11] */
#define RG_USB20_GBIAS_ENB		(0x1 << 10) /* bit[10] */
#define RG_USB20_DM_ABIST_SOURCE_EN	(0x1 << 7)  /* bit[7] */
#define RG_USB20_DM_ABIST_SELE		(0xf << 0)  /* bit[3:0] */

/* USBPHYACR6 - 0x18 */
#define RG_USB20_PHY_REV		(0xff << 24) /* bit[31:24] */
#define	RG_USB20_BC11_SW_EN		(0x1 << 23) /* bit[23] */
#define RG_USB20_SR_CLK_SEL		(0x1 << 22) /* bit[22] */
#define RG_USB20_OTG_VBUSCMP_EN		(0x1 << 20) /* bit[20] */
#define RG_USB20_OTG_ABIST_EN		(0x1 << 19) /* bit[19] */
#define RG_USB20_OTG_ABIST_SELE		(0x7 << 16) /* bit[18:16] */
#define RG_USB20_HSRX_MMODE_SELE	(0x3 << 12) /* bit[13:12] */
#define RG_USB20_HSRX_BIAS_EN_SEL	(0x3 << 9)  /* bit[10:9] */
#define RG_USB20_HSRX_TMODE_EN		(0x1 << 8)  /* bit[8] */
#define RG_USB20_DISCTH			(0xf << 4)  /* bit[7:4] */
#define RG_USB20_SQTH			(0xf << 0)  /* bit[3:0] */

/* U2PHYACR3 - 0x1c */
#define RG_USB20_HSTX_DBIST		(0xf << 28) /* bit[31:28] */
#define RG_USB20_HSTX_BIST_EN		(0x1 << 26) /* bit[26] */
#define RG_USB20_HSTX_I_EN_MODE		(0x3 << 24) /* bit[25:24] */ 
#define RG_USB20_USB11_TMODE_EN		(0x1 << 19) /* bit[19] */
#define RG_USB20_TMODE_FS_LS_TX_EN	(0x1 << 18) /* bit[18] */ 
#define RG_USB20_TMODE_FS_LS_RCV_EN	(0x1 << 17) /* bit[17] */
#define RG_USB20_TMODE_FS_LS_MODE	(0x1 << 16) /* bit[16] */
#define RG_USB20_HS_TERM_EN_MODE	(0x3 << 13) /* bit[14:13] */
#define RG_USB20_PUPD_BIST_EN		(0x1 << 12) /* bit[12] */
#define RG_USB20_EN_PU_DM		(0x1 << 11) /* bit[11] */
#define RG_USB20_EN_PD_DM		(0x1 << 10) /* bit[10] */
#define RG_USB20_EN_PU_DP		(0x1 << 9)  /* bit[9] */
#define RG_USB20_EN_PD_DP		(0x1 << 8)  /* bit[8] */

/* U2PHYACR4 - 0x20 */
#define	RG_USB20_DP_100K_EN		(0x1 << 18) /* bit[18] */
#define RG_USB20_DM_100K_EN		(0x1 << 17) /* bit[17] */
#define USB20_DP_100K_EN		(0x1 << 16) /* bit[16] */
#define USB20_GPIO_DM_I			(0x1 << 15) /* bit[15] */
#define USB20_GPIO_DP_I			(0x1 << 14) /* bit[14] */
#define USB20_GPIO_DM_OE		(0x1 << 13) /* bit[13] */
#define	USB20_GPIO_DP_OE		(0x1 << 12) /* bit[12] */
#define RG_USB20_GPIO_CTL		(0x1 << 9)  /* bit[9] */
#define USB20_GPIO_MODE			(0x1 << 8)  /* bit[8] */
#define RG_USB20_TX_BIAS_EN		(0x1 << 5)  /* bit[5] */
#define RG_USB20_TX_VCMPDN_EN		(0x1 << 4)  /* bit[4] */
#define RG_USB20_HS_SQ_EN_MODE		(0x3 << 2)  /* bit[3:2] */
#define RG_USB20_HS_RCV_EN_MODE		(0x3 << 0)  /* bit[1:0] */

/* U2PHYAMON0 - 0x24 */
#define RGO_USB20_GPIO_DM_O             (0x1 << 1)  /* bit[1] */
#define RGO_USB20_GPIO_DP_O             (0x1 << 0)  /* bit[0] */

/* U2PHYDCR0 - 0x60 */
#define RG_USB20_CDR_TST		(0x3 << 30)  /* bit[31:30] */
#define RG_USB20_GATED_ENB		(0x1 << 29)  /* bit[29] */ 
#define RG_USB20_TESTMODE		(0x3 << 26)  /* bit[27:26] */
#define RG_SIFSLV_USB20_PLL_STABLE	(0x1 << 25)  /* bit[25] */
#define RG_SIFSLV_USB20_PLL_FORCE_ON	(0x1 << 24)  /* bit[24] FIXME */
#define RG_USB20_PHYD_RESERVE		(0xffff << 8)  /* bit[23:8] */
#define RG_USB20_EBTHRLD		(0x1 << 7)  /* bit[7] */
#define RG_USB20_EARLY_HSTX_I		(0x1 << 6)  /* bit[6] */
#define RG_USB20_TX_TST			(0x1 << 5)  /* bit[5] */
#define RG_USB20_NEGEDGE_ENB		(0x1 << 4)  /* bit[4] */
#define RG_USB20_CDR_FILT		(0xf << 0)  /* bit[3:0] */

/* U2PHYDCR1 - 0x64 */
#define RG_USB20_PROBE_SEL              (0x1 << 24) /* bit[24] */
#define RG_USB20_DRVVBUS                (0x1 << 23) /* bit[23] */
#define RG_DEBUG_EN                     (0x1 << 22) /* bit[22] */
#define RG_USB20_OTG_PROBE              (0x3 << 20) /* bit[21:20] */
#define RG_USB20_SW_PLLMODE             (0x3 << 18) /* bit[19:18] */
#define RG_USB20_BERTH                  (0x3 << 16) /* bit[17:16] */
#define RG_USB20_LBMODE                 (0x3 << 13) /* bit[14:13] */
#define RG_USB20_FORCE_TAP              (0x1 << 12) /* bit[12] */
#define RG_USB20_TAPSEL			(0xfff << 0)  /* bit[11:0] */                 

/* U2PHYDTM0 - 0x68 */
#define RG_UART_MODE                    (0x3 << 30) /* bit[31:30] */
#define FORCE_UART_I                   	(0x1 << 29) /* bit[29] */
#define FORCE_UART_BIAS_EN             	(0x1 << 28) /* bit[28] */
#define FORCE_UART_TX_OE               	(0x1 << 27) /* bit[27] */
#define FORCE_UART_EN                  	(0x1 << 26) /* bit[26] */
#define FORCE_USB_CLKEN                	(0x1 << 25) /* bit[25] */
#define FORCE_DRVVBUS                 	(0x1 << 24) /* bit[24] */
#define FORCE_DATAIN                 	(0x1 << 23) /* bit[23] */
#define FORCE_TXVALID                	(0x1 << 22) /* bit[22] */
#define FORCE_DM_PULLDOWN             	(0x1 << 21) /* bit[21] */
#define FORCE_DP_PULLDOWN             	(0x1 << 20) /* bit[20] */
#define FORCE_XCVRSEL                 	(0x1 << 19) /* bit[19] */
#define FORCE_SUSPENDM                	(0x1 << 18) /* bit[18] */
#define FORCE_TERMSEL                 	(0x1 << 17) /* bit[17] */
#define FORCE_OPMODE                  	(0x1 << 16) /* bit[16] */
#define UTMI_MUXSEL                  	(0x1 << 15) /* bit[15] */
#define RG_RESET                    	(0x1 << 14) /* bit[14] */
#define RG_DATAIN                  	(0xf << 10) /* bit[13:10] */
#define RG_TXVALIDH                	(0x1 << 9)  /* bit[9] */
#define RG_TXVALID                 	(0x1 << 8)  /* bit[8] */
#define RG_DMPULLDOWN              	(0x1 << 7)  /* bit[7] */
#define RG_DPPULLDOWN              	(0x1 << 6)  /* bit[6] */
#define RG_XCVRSEL                 	(0x3 << 4)  /* bit[5:4] */
#define RG_SUSPENDM               	(0x1 << 3)  /* bit[3] */
#define RG_TERMSEL                	(0x1 << 2)  /* bit[2] */
#define RG_OPMODE			(0x3 << 0)  /* bit[1:0] */                 

/* U2PHYDTM1 - 0x6c */
#define RG_USB20_PRBS7_EN             	(0x1 << 31) /* bit[31] */
#define RG_USB20_PRBS7_BITCNT      	(0x3f << 24) /* bit[29:24] */
#define RG_USB20_CLK48M_EN          	(0x1 << 23) /* bit[23] */
#define RG_USB20_CLK60M_EN          	(0x1 << 22) /* bit[22] */
#define RG_UART_I                    	(0x1 << 19) /* bit[19] */
#define RG_UART_BIAS_EN               	(0x1 << 18) /* bit[18] */
#define RG_UART_TX_OE			(0x1 << 17) /* bit[17] */
#define RG_UART_EN			(0x1 << 16) /* bit[16] */
#define RG_IP_U2_PORT_POWER		(0x1 << 15) /* bit[15] */                 
#define FORCE_IP_U2_PORT_POWER          (0x1 << 14) /* bit[14] */
#define FORCE_VBUSVALID             	(0x1 << 13) /* bit[13] */ 
#define FORCE_SESSEND              	(0x1 << 12) /* bit[12] */
#define FORCE_BVALID                	(0x1 << 11) /* bit[11] */
#define FORCE_AVALID                 	(0x1 << 10) /* bit[10] */
#define FORCE_IDDIG                  	(0x1 << 9)  /* bit[9] */
#define FORCE_IDPULLUP               	(0x1 << 8)  /* bit[8] */
#define RG_VBUSVALID                  	(0x1 << 5)  /* bit[5] */
#define RG_SESSEND                    	(0x1 << 4)  /* bit[4] */
#define RG_BVALID                     	(0x1 << 3)  /* bit[3] */
#define RG_AVALID                     	(0x1 << 2)  /* bit[2] */
#define RG_IDDIG                      	(0x1 << 1)  /* bit[1] */
#define RG_IDPULLUP			(0x1 << 0)  /* bit[0] */                    

/* U2PHYDMON0 - 0x70 */
#define RG_USB20_PRBS7_BERTH            (0xff << 0)  /* bit[7:0] */

/* U2PHYDMON1 - 0x74 */
#define USB20_UART_O                   	(0x1 << 31) /* bit[31] */
#define RGO_USB20_LB_PASS             	(0x1 << 30) /* bit[30] */
#define RGO_USB20_LB_DONE            	(0x1 << 29) /* bit[29] */ 
#define AD_USB20_BVALID              	(0x1 << 28) /* bit[28] */
#define USB20_IDDIG                  	(0x1 << 27) /* bit[27] */
#define AD_USB20_VBUSVALID           	(0x1 << 26) /* bit[26] */
#define AD_USB20_SESSEND             	(0x1 << 25) /* bit[25] */
#define AD_USB20_AVALID             	(0x1 << 24) /* bit[24] */ 
#define USB20_LINE_STATE             	(0x3 << 22) /* bit[23:22] */
#define USB20_HST_DISCON             	(0x1 << 21) /* bit[21] */
#define USB20_TX_READY               	(0x1 << 20) /* bit[20] */
#define USB20_RX_ERROR               	(0x1 << 19) /* bit[19] */
#define USB20_RX_ACTIVE              	(0x1 << 18) /* bit[18] */
#define USB20_RX_VALIDH              	(0x1 << 17) /* bit[17] */
#define USB20_RX_VALID               	(0x1 << 16) /* bit[16] */
#define USB20_DATA_OUT              	(0xffff << 0)  /* bit[15:0] */ 

/* U2PHYDMON2 - 0x78 */
#define RGO_TXVALID_CNT                 (0xff << 24) /* bit[31:24] */
#define RGO_RXACTIVE_CNT               	(0xff << 16) /* bit[23:16] */
#define RGO_USB20_LB_BERCNT            	(0xff << 8)  /* bit[15:8] */
#define USB20_PROBE_OUT               	(0xff << 0)  /* bit[7:0] */  

/* U2PHYDMON3 - 0x7c */
#define RGO_USB20_PRBS7_ERRCNT         	(0xffff << 16) /* bit[31:16] */
#define RGO_USB20_PRBS7_DONE         	(0x1 << 3)  /* bit[3] */ 
#define RGO_USB20_PRBS7_LOCK           	(0x1 << 2)  /* bit[2] */
#define RGO_USB20_PRBS7_PASS            (0x1 << 1)  /* bit[1] */
#define RGO_USB20_PRBS7_PASSTH        	(0x1 << 0)  /* bit[0] */   

/* U2PHYBC12C - 0x80 */
#define RG_SIFSLV_CHGDT_DEGLCH_CNT      (0xf << 28) /* bit[31:28] */ 
#define RG_SIFSLV_CHGDT_CTRL_CNT        (0xf << 24) /* bit[27:24] */
#define RG_SIFSLV_CHGDT_FORCE_MODE      (0x1 << 16) /* bit[16] */
#define RG_CHGDT_ISRC_LEV               (0x3 << 14) /* bit[15:14] */
#define RG_CHGDT_VDATSRC              	(0x1 << 13) /* bit[13] */
#define RG_CHGDT_BGVREF_SEL           	(0x7 << 10) /* bit[12:10] */
#define RG_CHGDT_RDVREF_SEL           	(0x3 << 8)  /* bit[9:8] */
#define RG_CHGDT_ISRC_DP              	(0x1 << 7)  /* bit[7] */
#define RG_SIFSLV_CHGDT_OPOUT_DM       	(0x1 << 6)  /* bit[6] */
#define RG_CHGDT_VDAT_DM               	(0x1 << 5)  /* bit[5] */
#define RG_CHGDT_OPOUT_DP             	(0x1 << 4)  /* bit[4] */ 
#define RG_SIFSLV_CHGDT_VDAT_DP        	(0x1 << 3)  /* bit[3] */
#define RG_SIFSLV_CHGDT_COMP_EN        	(0x1 << 2)  /* bit[2] */
#define RG_SIFSLV_CHGDT_OPDRV_EN       	(0x1 << 1)  /* bit[1] */
#define RG_CHGDT_EN                   	(0x1 << 0)  /* bit[0] */ 

/* U2PHYBC12C1 - 0x84 */
#define RG_CHGDT_REV                   	(0xff << 0)  /* bit[7:0] */ 

/* REGFPPC - 0xe0 */
#define USB11_OTG_REG			(0x1 << 4)  /* bit[4] */
#define USB20_OTG_REG			(0x1 << 3)  /* bit[3] */
#define CHGDT_REG			(0x1 << 2)  /* bit[2] */
#define USB11_REG			(0x1 << 1)  /* bit[1] */
#define USB20_REG			(0x1 << 0)  /* bit[0] */

/* VERSIONC - 0xf0 */
#define VERSION_CODE_REGFILE		(0xff << 24) /* bit[31:24] */
#define USB11_VERSION_CODE		(0xff << 16) /* bit[23:16] */
#define VERSION_CODE_ANA		(0xff << 8)  /* bit[15:8] */
#define VERSION_CODE_DIG		(0xff << 0)  /* bit[7:0] */

/* REGFCOM - 0xfc */
#define RG_PAGE                       	(0xff << 24) /* bit[31:24] */
#define I2C_MODE                     	(0x1 << 16) /* bit[16] */  


/* OFFSET  */
/* USBPHYARC0 - 0x0 */
#define RG_USB20_MPX_OUT_SEL_OFST	(28) /* bit[30:28] */
#define RG_USB20_TX_PH_ROT_SE_OFST	(24) /* bit[26:24] */
#define RG_USB20_PLL_DIVEN_OFST		(20) /* bit[22:20] */
#define RG_USB20_PLL_BR_OFST		(18) /* bit[18] */
#define RG_USB20_PLL_BP_OFST		(17) /* bit[17] */
#define RG_USB20_PLL_BLP_OFST		(16) /* bit[16] */
#define RG_USB20_USBPLL_FORCE_ON_OFST	(15) /* bit[15] */
#define RG_USB20_PLL_FBDIV_OFST		(8)  /* bit[14:8] */
#define RG_USB20_PLL_PREDIV_OFST        (6)  /* bit[7:6] */
#define RG_USB20_INTR_EN_OFST		(5)  /* bit[5] */
#define RG_USB20_REF_EN_OFST		(4)  /* bit[4] */
#define RG_USB20_BGR_DIV_OFST           (2)  /* bit[3:2] */
#define RG_SIFSLV_CHP_EN_OFST           (1)  /* bit[1] */
#define RG_SIFSLV_BGR_EN_OFST           (0)  /* bit[0] */


/* USBPHYACR1 - 0x4 */
#define RG_USB20_OTG_VBUSTH_OFST	(16) /* bit[18:16] */
#define RG_USB20_VRT_VREF_SEL_OFST	(12) /* bit[14:12] */
#define RG_USB20_TERM_VREF_SEL_OFST	(8)  /* bit[10:8] */
#define RG_USB20_MPX_SEL_OFST		(0)  /* bit[7:0] */


/* USBPHYACR2 - 0x8 */
#define RG_SIFSLV_MAC_BANDGAP_EN_OFST	(17)  /* bit[17] */
#define RG_SIFSLV_MAC_CHOPPER_EN_OFST	(16)  /* bit[16] */
#define RG_USB20_CLKREF_REV_OFST	(0)  /* bit[15:0] */

/* USBPHYACR4 - 0x10 */
#define RG_USB20_DP_ABIST_SOURCE_EN_OFST	(31) /* bit[31] */
#define RG_USB20_DP_ABIST_SELE_OFST		(24) /* bit[27:24] */
#define RG_USB20_ICUSB_EN_OFST			(16) /* bit[16] */
#define RG_USB20_LS_CR_OFST                  	(12) /* bit[14:12] */
#define RG_USB20_FS_CR_OFST                  	(8)  /* bit[10:8] */
#define RG_USB20_LS_SR_OFST                  	(4)  /* bit[6:4] */
#define RG_USB20_FS_SR_OFST                  	(0)  /* bit[2:0] */ 


/* USBPHYACR5 - 0x14 */
#define RG_USB20_DISC_FIT_EN_OFST		(28) /* bit[28] */
#define RG_USB20_INIT_SQ_EN_DG_OFST		(26) /* bit[27:26] */
#define RG_USB20_HSTX_TMODE_SEL_OFST		(24) /* bit[25:24] */
#define RG_USB20_SQD_OFST			(22) /* bit[23:22] */
#define RG_USB20_DISCD_OFST			(20) /* bit[21:20] */
#define RG_USB20_HSTX_TMODE_EN_OFST		(19) /* bit[19] */
#define RG_USB20_PHYD_MONEN_OFST		(18) /* bit[18] */
#define RG_USB20_INLPBK_EN_OFST			(17) /* bit[17] */
#define RG_USB20_CHIRP_EN_OFST			(16) /* bit[16] */
#define RG_USB20_HSTX_SRCAL_EN_OFST		(15) /* bit[15] */
#define RG_USB20_HSTX_SRCTRL_OFST		(12) /* bit[14:12] */
#define RG_USB20_HS_100U_U3_EN_OFST		(11) /* bit[11] */
#define RG_USB20_GBIAS_ENB_OFST			(10) /* bit[10] */
#define RG_USB20_DM_ABIST_SOURCE_EN_OFST	(7)  /* bit[7] */
#define RG_USB20_DM_ABIST_SELE_OFST		(0)  /* bit[3:0] */


/* USBPHYACR6 - 0x18 */
#define RG_USB20_PHY_REV_OFST		(24) /* bit[31:24] */
#define	RG_USB20_BC11_SW_EN_OFST	(23) /* bit[23] */
#define RG_USB20_SR_CLK_SEL_OFST	(22) /* bit[22] */
#define RG_USB20_OTG_VBUSCMP_EN_OFST	(20) /* bit[20] */
#define RG_USB20_OTG_ABIST_EN_OFST	(19) /* bit[19] */
#define RG_USB20_OTG_ABIST_SELE_OFST	(16) /* bit[18:16] */
#define RG_USB20_HSRX_MMODE_SELE_OFST	(12) /* bit[13:12] */
#define RG_USB20_HSRX_BIAS_EN_SEL_OFST	(9)  /* bit[10:9] */
#define RG_USB20_HSRX_TMODE_EN_OFST	(8)  /* bit[8] */
#define RG_USB20_DISCTH_OFST		(4)  /* bit[7:4] */
#define RG_USB20_SQTH_OFST		(0)  /* bit[3:0] */


/* U2PHYACR3 - 0x1c */
#define RG_USB20_HSTX_DBIST_OFST		(28) /* bit[31:28] */
#define RG_USB20_HSTX_BIST_EN_OFST		(26) /* bit[26] */
#define RG_USB20_HSTX_I_EN_MODE_OFST		(24) /* bit[25:24] */ 
#define RG_USB20_USB11_TMODE_EN_OFST		(19) /* bit[19] */
#define RG_USB20_TMODE_FS_LS_TX_EN_OFST		(18) /* bit[18] */ 
#define RG_USB20_TMODE_FS_LS_RCV_EN_OFST	(17) /* bit[17] */
#define RG_USB20_TMODE_FS_LS_MODE_OFST		(16) /* bit[16] */
#define RG_USB20_HS_TERM_EN_MODE_OFST		(13) /* bit[14:13] */
#define RG_USB20_PUPD_BIST_EN_OFST		(12) /* bit[12] */
#define RG_USB20_EN_PU_DM_OFST			(11) /* bit[11] */
#define RG_USB20_EN_PD_DM_OFST			(10) /* bit[10] */
#define RG_USB20_EN_PU_DP_OFST			(9)  /* bit[9] */
#define RG_USB20_EN_PD_DP_OFST			(8)  /* bit[8] */


/* U2PHYACR4 - 0x20 */
#define	RG_USB20_DP_100K_EN_OFST	(18) /* bit[18] */
#define RG_USB20_DM_100K_EN_OFST	(17) /* bit[17] */
#define USB20_DP_100K_EN_OFST		(16) /* bit[16] */
#define USB20_GPIO_DM_I_OFST		(15) /* bit[15] */
#define USB20_GPIO_DP_I_OFST		(14) /* bit[14] */
#define USB20_GPIO_DM_OE_OFST		(13) /* bit[13] */
#define	USB20_GPIO_DP_OE_OFST		(12) /* bit[12] */
#define RG_USB20_GPIO_CTL_OFST		(9)  /* bit[9] */
#define USB20_GPIO_MODE_OFST		(8)  /* bit[8] */
#define RG_USB20_TX_BIAS_EN_OFST	(5)  /* bit[5] */
#define RG_USB20_TX_VCMPDN_EN_OFST	(4)  /* bit[4] */
#define RG_USB20_HS_SQ_EN_MODE_OFST	(2)  /* bit[3:2] */
#define RG_USB20_HS_RCV_EN_MODE_OFST	(0)  /* bit[1:0] */


/* U2PHYAMON0 - 0x24 */
#define RGO_USB20_GPIO_DM_O_OFST        (1)  /* bit[1] */
#define RGO_USB20_GPIO_DP_O_OFST        (0)  /* bit[0] */


/* U2PHYDCR0 - 0x60 */
#define RG_USB20_CDR_TST_OFST			(30)  /* bit[31:30] */
#define RG_USB20_GATED_ENB_OFST			(29)  /* bit[29] */ 
#define RG_USB20_TESTMODE_OFST			(26)  /* bit[27:26] */
#define RG_SIFSLV_USB20_PLL_STABLE_OFST		(25)  /* bit[25] */
#define RG_SIFSLV_USB20_PLL_FORCE_ON_OFST	(24)  /* bit[24] FIXME */
#define RG_USB20_PHYD_RESERVE_OFST		(8)  /* bit[23:8] */
#define RG_USB20_EBTHRLD_OFST			(7)  /* bit[7] */
#define RG_USB20_EARLY_HSTX_I_OFST		(6)  /* bit[6] */
#define RG_USB20_TX_TST_OFST			(5)  /* bit[5] */
#define RG_USB20_NEGEDGE_ENB_OFST		(4)  /* bit[4] */
#define RG_USB20_CDR_FILT_OFST			(0)  /* bit[3:0] */


/* U2PHYDCR1 - 0x64 */
#define RG_USB20_PROBE_SEL_OFST         (24) /* bit[14] */
#define RG_USB20_DRVVBUS_OFST           (23) /* bit[23] */
#define RG_DEBUG_EN_OFST                (22) /* bit[22] */
#define RG_USB20_OTG_PROBE_OFST       	(20) /* bit[21:20] */
#define RG_USB20_SW_PLLMODE_OFST        (18) /* bit[19:18] */
#define RG_USB20_BERTH_OFST             (16) /* bit[17:16] */
#define RG_USB20_LBMODE_OFST            (13) /* bit[14:13] */
#define RG_USB20_FORCE_TAP_OFST         (12) /* bit[12] */
#define RG_USB20_TAPSEL_OFST		(0)  /* bit[11:0] */                 


/* U2PHYDTM0 - 0x68 */
#define RG_UART_MODE_OFST               (30) /* bit[31:30] */
#define FORCE_UART_I_OFST               (29) /* bit[29] */
#define FORCE_UART_BIAS_EN_OFST         (28) /* bit[28] */
#define FORCE_UART_TX_OE_OFST           (27) /* bit[27] */
#define FORCE_UART_EN_OFST              (26) /* bit[26] */
#define FORCE_USB_CLKEN_OFST            (25) /* bit[25] */
#define FORCE_DRVVBUS_OFST              (24) /* bit[24] */
#define FORCE_DATAIN_OFST               (23) /* bit[23] */
#define FORCE_TXVALID_OFST              (22) /* bit[22] */
#define FORCE_DM_PULLDOWN_OFST          (21) /* bit[21] */
#define FORCE_DP_PULLDOWN_OFST          (20) /* bit[20] */
#define FORCE_XCVRSEL_OFST              (19) /* bit[19] */
#define FORCE_SUSPENDM_OFST             (18) /* bit[18] */
#define FORCE_TERMSEL_OFST              (17) /* bit[17] */
#define FORCE_OPMODE_OFST               (16) /* bit[16] */
#define UTMI_MUXSEL_OFST                (15) /* bit[15] */
#define RG_RESET_OFST                   (14) /* bit[14] */
#define RG_DATAIN_OFST                  (10) /* bit[13:10] */
#define RG_TXVALIDH_OFST               	(9)  /* bit[9] */
#define RG_TXVALID_OFST                	(8)  /* bit[8] */
#define RG_DMPULLDOWN_OFST             	(7)  /* bit[7] */
#define RG_DPPULLDOWN_OFST             	(6)  /* bit[6] */
#define RG_XCVRSEL_OFST                	(4)  /* bit[5:4] */
#define RG_SUSPENDM_OFST               	(3)  /* bit[3] */
#define RG_TERMSEL_OFST                	(2)  /* bit[2] */
#define RG_OPMODE_OFST			(0)  /* bit[1:0] */                 


/* U2PHYDTM1 - 0x6c */
#define RG_USB20_PRBS7_EN_OFST         	(31) /* bit[31] */
#define RG_USB20_PRBS7_BITCNT_OFST     	(24) /* bit[29:24] */
#define RG_USB20_CLK48M_EN_OFST        	(23) /* bit[23] */
#define RG_USB20_CLK60M_EN_OFST        	(22) /* bit[22] */
#define RG_UART_I_OFST                 	(19) /* bit[19] */
#define RG_UART_BIAS_EN_OFST           	(18) /* bit[18] */
#define RG_UART_TX_OE_OFST			(17) /* bit[17] */
#define RG_UART_EN_OFST			(16) /* bit[16] */
#define RG_IP_U2_PORT_POWER_OFST	(15) /* bit[15] */                 
#define FORCE_IP_U2_PORT_POWER_OFST     (14) /* bit[14] */
#define FORCE_VBUSVALID_OFST           	(13) /* bit[13] */ 
#define FORCE_SESSEND_OFST             	(12) /* bit[12] */
#define FORCE_BVALID_OFST              	(11) /* bit[11] */
#define FORCE_AVALID_OFST              	(10) /* bit[10] */
#define FORCE_IDDIG_OFST               	(9)  /* bit[9] */
#define FORCE_IDPULLUP_OFST            	(8)  /* bit[8] */
#define RG_VBUSVALID_OFST              	(5)  /* bit[5] */
#define RG_SESSEND_OFST                	(4)  /* bit[4] */
#define RG_BVALID_OFST                 	(3)  /* bit[3] */
#define RG_AVALID_OFST                 	(2)  /* bit[2] */
#define RG_IDDIG_OFST                  	(1)  /* bit[1] */
#define RG_IDPULLUP_OFST		(0)  /* bit[0] */                    


/* U2PHYDMON0 - 0x70 */
#define RG_USB20_PRBS7_BERTH_OFST       (0)  /* bit[7:0] */

/* U2PHYDMON1 - 0x74 */
#define USB20_UART_O_OFST               (31) /* bit[31] */
#define RGO_USB20_LB_PASS_OFST          (30) /* bit[30] */
#define RGO_USB20_LB_DONE_OFST          (29) /* bit[29] */ 
#define AD_USB20_BVALID_OFST            (28) /* bit[28] */
#define USB20_IDDIG_OFST                (27) /* bit[27] */
#define AD_USB20_VBUSVALID_OFST         (26) /* bit[26] */
#define AD_USB20_SESSEND_OFST           (25) /* bit[25] */
#define AD_USB20_AVALID_OFST            (24) /* bit[24] */ 
#define USB20_LINE_STATE_OFST           (22) /* bit[23:22] */
#define USB20_HST_DISCON_OFST           (21) /* bit[21] */
#define USB20_TX_READY_OFST             (20) /* bit[20] */
#define USB20_RX_ERROR_OFST             (19) /* bit[19] */
#define USB20_RX_ACTIVE_OFST            (18) /* bit[18] */
#define USB20_RX_VALIDH_OFST            (17) /* bit[17] */
#define USB20_RX_VALID_OFST             (16) /* bit[16] */
#define USB20_DATA_OUT_OFST             (0)  /* bit[15:0] */ 


/* U2PHYDMON2 - 0x78 */
#define RGO_TXVALID_CNT_OFST            (24) /* bit[31:24] */
#define RGO_RXACTIVE_CNT_OFST          	(16) /* bit[23:16] */
#define RGO_USB20_LB_BERCNT_OFST        (8)  /* bit[15:8] */
#define USB20_PROBE_OUT_OFST            (0)  /* bit[7:0] */  

/* U2PHYDMON3 - 0x7c */
#define RGO_USB20_PRBS7_ERRCNT_OFST     (16) /* bit[31:16] */
#define RGO_USB20_PRBS7_DONE_OFST       (3)  /* bit[3] */ 
#define RGO_USB20_PRBS7_LOCK_OFST       (2)  /* bit[2] */
#define RGO_USB20_PRBS7_PASS_OFST       (1)  /* bit[1] */
#define RGO_USB20_PRBS7_PASSTH_OFST     (0)  /* bit[0] */   


/* U2PHYBC12C - 0x80 */
#define RG_SIFSLV_CHGDT_DEGLCH_CNT_OFST  	(28) /* bit[31:28] */ 
#define RG_SIFSLV_CHGDT_CTRL_CNT_OFST        	(24) /* bit[27:24] */
#define RG_SIFSLV_CHGDT_FORCE_MODE_OFST      	(16) /* bit[16] */
#define RG_CHGDT_ISRC_LEV_OFST               	(14) /* bit[15:14] */
#define RG_CHGDT_VDATSRC_OFST              	(13) /* bit[13] */
#define RG_CHGDT_BGVREF_SEL_OFST           	(10) /* bit[12:10] */
#define RG_CHGDT_RDVREF_SEL_OFST           	(8)  /* bit[9:8] */
#define RG_CHGDT_ISRC_DP_OFST              	(7)  /* bit[7] */
#define RG_SIFSLV_CHGDT_OPOUT_DM_OFST       	(6)  /* bit[6] */
#define RG_CHGDT_VDAT_DM_OFST               	(5)  /* bit[5] */
#define RG_CHGDT_OPOUT_DP_OFST             	(4)  /* bit[4] */ 
#define RG_SIFSLV_CHGDT_VDAT_DP_OFST        	(3)  /* bit[3] */
#define RG_SIFSLV_CHGDT_COMP_EN_OFST        	(2)  /* bit[2] */
#define RG_SIFSLV_CHGDT_OPDRV_EN_OFST       	(1)  /* bit[1] */
#define RG_CHGDT_EN_OFST                   	(0)  /* bit[0] */ 

/* U2PHYBC12C1 - 0x84 */
#define RG_CHGDT_REV_OFST               (0)  /* bit[7:0] */ 

/* REGFPPC - 0xe0 */
#define USB11_OTG_REG_OFST		(4)  /* bit[4] */
#define USB20_OTG_REG_OFST		(3)  /* bit[3] */
#define CHGDT_REG_OFST			(2)  /* bit[2] */
#define USB11_REG_OFST			(1)  /* bit[1] */
#define USB20_REG_OFST			(0)  /* bit[0] */

/* VERSIONC - 0xf0 */
#define VERSION_CODE_REGFILE_OFST	(24) /* bit[31:24] */
#define USB11_VERSION_CODE_OFST		(16) /* bit[23:16] */
#define VERSION_CODE_ANA_OFST		(8)  /* bit[15:8] */
#define VERSION_CODE_DIG_OFST		(0)  /* bit[7:0] */

/* REGFCOM - 0xfc */
#define RG_PAGE_OFST                   	(24) /* bit[31:24] */
#define I2C_MODE_OFST                  	(16) /* bit[16] */  


///////////////////////////////////////////////////////////////////////////////

struct sifslv_fm_feg {
	//0x0
	PHY_LE32 fmcr0;		/* 0x0 */
	PHY_LE32 fmcr1;		/* 0x4 */
	PHY_LE32 fmcr2;		/* 0x8 */
	PHY_LE32 fmmonr0;	/* 0xc */
	//0x10
	PHY_LE32 fmmonr1;	/* 0x10 */
};

/* FMCR0 -0x0 */
#define RG_LOCKTH             	(0xf << 28) /* bit[31:28] */
#define RG_MONCLK_SEL           (0x3 << 26) /* bit[27:26] */
#define RG_FM_MODE              (0x1 << 25) /* bit[25] */
#define RG_FREQDET_EN           (0x1 << 24) /* bit[24] */
#define RG_CYCLECNT             (0xffffff << 0) /* bit[23:0] */

//U3D_FMCR1
#define RG_TARGET               (0xffffffff << 0) /* bit[31:0] */

//U3D_FMCR2
#define RG_OFFSET               (0xffffffff << 0) /* bit[31:0] */

//U3D_FMMONR0
#define USB_FM_OUT              (0xffffffff << 0) /* bit[31:0] */

//U3D_FMMONR1
#define RG_MONCLK_SEL_3         (0x1 << 9) /* bit[9] */
#define RG_FRCK_EN              (0x1 << 8) /* bit[8] */
#define USBPLL_LOCK             (0x1 << 1) /* bit[1] */
#define USB_FM_VLD              (0x1 << 0) /* bit[0] */


/* OFFSET */

//U3D_FMCR0
#define RG_LOCKTH_OFST        	(28)
#define RG_MONCLK_SEL_OFST      (26)
#define RG_FM_MODE_OFST         (25)
#define RG_FREQDET_EN_OFST      (24)
#define RG_CYCLECNT_OFST        (0)

//U3D_FMCR1
#define RG_TARGET_OFST          (0)

//U3D_FMCR2
#define RG_OFFSET_OFST          (0)

//U3D_FMMONR0
#define USB_FM_OUT_OFST         (0)

//U3D_FMMONR1
#define RG_MONCLK_SEL_3_OFST    (9)
#define RG_FRCK_EN_OFST         (8)
#define USBPLL_LOCK_OFST        (1)
#define USB_FM_VLD_OFST         (0)


///////////////////////////////////////////////////////////////////////////////
PHY_INT32 mt7623_phy_init(struct u3phy_info *info);
PHY_INT32 phy_change_pipe_phase(struct u3phy_info *info, PHY_INT32 phy_drv, PHY_INT32 pipe_phase);
PHY_INT32 eyescan_init(struct u3phy_info *info);
PHY_INT32 phy_eyescan(struct u3phy_info *info, PHY_INT32 x_t1, PHY_INT32 y_t1, PHY_INT32 x_br, PHY_INT32 y_br, PHY_INT32 delta_x, PHY_INT32 delta_y
		, PHY_INT32 eye_cnt, PHY_INT32 num_cnt, PHY_INT32 PI_cal_en, PHY_INT32 num_ignore_cnt);
PHY_INT32 u2_save_cur_en(struct u3phy_info *info);
PHY_INT32 u2_save_cur_re(struct u3phy_info *info);
PHY_INT32 u2_slew_rate_calibration(struct u3phy_info *info);
PHY_INT32 u2_slew_rate_calibration_p1(struct u3phy_info *info);

#endif
#endif
