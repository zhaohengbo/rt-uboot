#ifndef _MT_CLKMGR_H
#define _MT_CLKMGR_H


/* clkmgr constants */

enum {
	CG_PERI0,
	CG_PERI1,
	CG_INFRA,
	CG_TOPCK,
	CG_DISP0,
	CG_DISP1,
	CG_IMAGE,
	CG_MFG,
	CG_AUDIO,
	CG_VDEC0,
	CG_VDEC1,
	NR_GRPS,
};


enum cg_clk_id {
	MT_CG_PERI_NFI,
	MT_CG_PERI_THERM,
	MT_CG_PERI_PWM1,
	MT_CG_PERI_PWM2,
	MT_CG_PERI_PWM3,
	MT_CG_PERI_PWM4,
	MT_CG_PERI_PWM5,
	MT_CG_PERI_PWM6,
	MT_CG_PERI_PWM7,
	MT_CG_PERI_PWM,
	MT_CG_PERI_USB0,
	MT_CG_PERI_USB1,
	MT_CG_PERI_AP_DMA,
	MT_CG_PERI_MSDC30_0,
	MT_CG_PERI_MSDC30_1,
	MT_CG_PERI_MSDC30_2,
	MT_CG_PERI_NLI,
	MT_CG_PERI_UART0,
	MT_CG_PERI_UART1,
	MT_CG_PERI_UART2,
	MT_CG_PERI_UART3,
	MT_CG_PERI_BTIF,
	MT_CG_PERI_I2C0,
	MT_CG_PERI_I2C1,
	MT_CG_PERI_I2C2,
	MT_CG_PERI_I2C3,
	MT_CG_PERI_AUXADC,
	MT_CG_PERI_SPI0,
	MT_CG_PERI_ETH,
	MT_CG_PERI_USB0_MCU,
	MT_CG_PERI_USB1_MCU,
	MT_CG_PERI_USB_SLV,

	MT_CG_PERI_GCPU,
	MT_CG_PERI_NFI_ECC,
	MT_CG_PERI_NFIPAD,

	MT_CG_INFRA_DBGCLK,
	MT_CG_INFRA_SMI,
	MT_CG_INFRA_AUDIO,
	MT_CG_INFRA_EFUSE,
	MT_CG_INFRA_L2C_SRAM,
	MT_CG_INFRA_M4U,
	MT_CG_INFRA_CONNMCU,
	MT_CG_INFRA_TRNG,
	MT_CG_INFRA_CPUM,
	MT_CG_INFRA_KP,
	MT_CG_INFRA_CEC,
	MT_CG_INFRA_IRRX,
	MT_CG_INFRA_PMICSPI_SHARE,
	MT_CG_INFRA_PMICWRAP,

	MT_CG_TOPCK_PMICSPI,

	MT_CG_DISP0_SMI_COMMON,
	MT_CG_DISP0_SMI_LARB0,
	MT_CG_DISP0_MM_CMDQ,
	MT_CG_DISP0_MUTEX,
	MT_CG_DISP0_DISP_COLOR,
	MT_CG_DISP0_DISP_BLS,
	MT_CG_DISP0_DISP_WDMA,
	MT_CG_DISP0_DISP_RDMA,
	MT_CG_DISP0_DISP_OVL,
	MT_CG_DISP0_MDP_TDSHP,
	MT_CG_DISP0_MDP_WROT,
	MT_CG_DISP0_MDP_WDMA,
	MT_CG_DISP0_MDP_RSZ1,
	MT_CG_DISP0_MDP_RSZ0,
	MT_CG_DISP0_MDP_RDMA,
	MT_CG_DISP0_MDP_BLS_26M,
	MT_CG_DISP0_CAM_MDP,
	MT_CG_DISP0_FAKE_ENG,
	MT_CG_DISP0_MUTEX_32K,
	MT_CG_DISP0_DISP_RMDA1,
	MT_CG_DISP0_DISP_UFOE,

	MT_CG_DISP1_DSI_ENGINE,
	MT_CG_DISP1_DSI_DIGITAL,
	MT_CG_DISP1_DPI_DIGITAL_LANE,
	MT_CG_DISP1_DPI_ENGINE,
	MT_CG_DISP1_DPI1_DIGITAL_LANE,
	MT_CG_DISP1_DPI1_ENGINE,
	MT_CG_DISP1_TVE_OUTPUT_CLOCK,
	MT_CG_DISP1_TVE_INPUT_CLOCK,
	MT_CG_DISP1_HDMI_PIXEL_CLOCK,
	MT_CG_DISP1_HDMI_PLL_CLOCK,
	MT_CG_DISP1_HDMI_AUDIO_CLOCK,
	MT_CG_DISP1_HDMI_SPDIF_CLOCK,
	MT_CG_DISP1_LVDS_PIXEL_CLOCK,
	MT_CG_DISP1_LVDS_CTS_CLOCK,

	MT_CG_IMAGE_LARB2_SMI,
	MT_CG_IMAGE_CAM_SMI,
	MT_CG_IMAGE_CAM_CAM,
	MT_CG_IMAGE_SEN_TG,
	MT_CG_IMAGE_SEN_CAM,
	MT_CG_IMAGE_VENC_JPENC,

	MT_CG_MFG_G3D,

	MT_CG_AUDIO_AFE,
	MT_CG_AUDIO_I2S,
	MT_CG_AUDIO_APLL_TUNER_CK,
	MT_CG_AUDIO_HDMI_CK,
	MT_CG_AUDIO_SPDF_CK,
	MT_CG_AUDIO_SPDF2_CK,

	MT_CG_VDEC0_VDEC,

	MT_CG_VDEC1_LARB,

	NR_CLKS,
};


enum {
	/* CLK_CFG_0 */
	MT_MUX_MM,
	MT_MUX_DDRPHYCFG,
	MT_MUX_MEM,
	MT_MUX_AXI,

	/* CLK_CFG_1 */
	MT_MUX_CAMTG,
	MT_MUX_MFG,
	MT_MUX_VDEC,
	MT_MUX_PWM,

	/* CLK_CFG_2 */
	MT_MUX_MSDC30_0,
	MT_MUX_USB20,
	MT_MUX_SPI,
	MT_MUX_UART,

	/* CLK_CFG_3 */
	MT_MUX_AUDINTBUS,
	MT_MUX_AUDIO,
	MT_MUX_MSDC30_2,
	MT_MUX_MSDC30_1,

	/* CLK_CFG_4 */
	MT_MUX_DPI1,
	MT_MUX_DPI0,
	MT_MUX_SCP,
	MT_MUX_PMICSPI,

	/* CLK_CFG_5 */
	MT_MUX_DPILVDS,
	MT_MUX_APLL,
	MT_MUX_HDMI,
	MT_MUX_TVE,

	/* CLK_CFG_6 */
	MT_MUX_ETH_50M,
	MT_MUX_NFI2X,
	MT_MUX_RTC,

	NR_MUXS,
};

enum {
	ARMPLL,
	MAINPLL,
	MSDCPLL,
	UNIVPLL,
	MMPLL,
	VENCPLL,
	TVDPLL,
	LVDSPLL,
	AUDPLL,

	NR_PLLS,
};


enum {
	SYS_CONN,
	SYS_DPY,
	SYS_DIS,
	SYS_MFG,
	SYS_ISP,
	SYS_IFR,
	SYS_VDE,

	NR_SYSS,
};


#ifdef __KERNEL__

/* the following definition / declaration are only enabled in Linux kernel */


#include <linux/list.h>
#include <asm/arch/mt_reg_base.h>
#include <asm/arch/mt_typedefs.h>


#define CLKMGR_8127             1
#define CLKMGR_EXT              0
#define CLKMGR_CLKM0            0


#define AP_PLL_CON0         (APMIXEDSYS_BASE + 0x0000)
#define AP_PLL_CON1         (APMIXEDSYS_BASE + 0x0004)
#define AP_PLL_CON2         (APMIXEDSYS_BASE + 0x0008)

#define PLL_HP_CON0         (APMIXEDSYS_BASE + 0x0014)

#define ARMPLL_CON0         (APMIXEDSYS_BASE + 0x0200)
#define ARMPLL_CON1         (APMIXEDSYS_BASE + 0x0204)
#define ARMPLL_PWR_CON0     (APMIXEDSYS_BASE + 0x020C)

#define MAINPLL_CON0        (APMIXEDSYS_BASE + 0x0210)
#define MAINPLL_CON1        (APMIXEDSYS_BASE + 0x0214)
#define MAINPLL_PWR_CON0    (APMIXEDSYS_BASE + 0x021C)

#define UNIVPLL_CON0        (APMIXEDSYS_BASE + 0x0220)
#define UNIVPLL_CON1        (APMIXEDSYS_BASE + 0x0224)
#define UNIVPLL_PWR_CON0    (APMIXEDSYS_BASE + 0x022C)

#define MMPLL_CON0          (APMIXEDSYS_BASE + 0x0230)
#define MMPLL_CON1          (APMIXEDSYS_BASE + 0x0234)
#define MMPLL_PWR_CON0      (APMIXEDSYS_BASE + 0x023C)

#define MSDCPLL_CON0        (APMIXEDSYS_BASE + 0x0240)
#define MSDCPLL_CON1        (APMIXEDSYS_BASE + 0x0244)
#define MSDCPLL_PWR_CON0    (APMIXEDSYS_BASE + 0x024C)

#define AUDPLL_CON0         (APMIXEDSYS_BASE + 0x0250)
#define AUDPLL_CON1         (APMIXEDSYS_BASE + 0x0254)
#define AUDPLL_PWR_CON0     (APMIXEDSYS_BASE + 0x025C)

#define VENCPLL_CON0        (DDRPHY_BASE + 0x800)
#define VENCPLL_CON1        (DDRPHY_BASE + 0x804)
#define VENCPLL_PWR_CON0    (DDRPHY_BASE + 0x80C)

#define TVDPLL_CON0         (APMIXEDSYS_BASE + 0x0260)
#define TVDPLL_CON1         (APMIXEDSYS_BASE + 0x0264)
#define TVDPLL_PWR_CON0     (APMIXEDSYS_BASE + 0x026C)

#define LVDSPLL_CON0        (APMIXEDSYS_BASE + 0x0270)
#define LVDSPLL_CON1        (APMIXEDSYS_BASE + 0x0274)
#define LVDSPLL_PWR_CON0    (APMIXEDSYS_BASE + 0x027C)

#define CLK_DSI_PLL_CON0    (MIPI_CONFIG_BASE + 0x50)

#define CLK_CFG_0           (INFRA_BASE + 0x0040)
#define CLK_CFG_1           (INFRA_BASE + 0x0050)
#define CLK_CFG_2           (INFRA_BASE + 0x0060)
#define CLK_CFG_3           (INFRA_BASE + 0x0070)
#define CLK_CFG_4           (INFRA_BASE + 0x0080)
#define CLK_CFG_4_SET       (INFRA_BASE + 0x0084)
#define CLK_CFG_4_CLR       (INFRA_BASE + 0x0088)
#define CLK_CFG_5           (INFRA_BASE + 0x0090)
#define CLK_CFG_6           (INFRA_BASE + 0x00A0)
#define CLK_CFG_8           (INFRA_BASE + 0x0100)
#define CLK_CFG_9           (INFRA_BASE + 0x0104)
#define CLK_CFG_10          (INFRA_BASE + 0x0108)
#define CLK_CFG_11          (INFRA_BASE + 0x010C)
#define CLK_SCP_CFG_0       (INFRA_BASE + 0x0200)
#define CLK_SCP_CFG_1       (INFRA_BASE + 0x0204)

#define INFRA_PDN_SET       (INFRACFG_AO_BASE + 0x0040)
#define INFRA_PDN_CLR       (INFRACFG_AO_BASE + 0x0044)
#define INFRA_PDN_STA       (INFRACFG_AO_BASE + 0x0048)

#define TOPAXI_PROT_EN      (INFRACFG_AO_BASE + 0x0220)
#define TOPAXI_PROT_STA1    (INFRACFG_AO_BASE + 0x0228)

#define PERI_PDN0_SET       (PERICFG_BASE + 0x0008)
#define PERI_PDN0_CLR       (PERICFG_BASE + 0x0010)
#define PERI_PDN0_STA       (PERICFG_BASE + 0x0018)

#define PERI_PDN1_SET       (PERICFG_BASE + 0x000C)
#define PERI_PDN1_CLR       (PERICFG_BASE + 0x0014)
#define PERI_PDN1_STA       (PERICFG_BASE + 0x001C)

#define AUDIO_TOP_CON0      (AUDIO_REG_BASE + 0x0000)

#define MFG_CG_CON          (G3D_CONFIG_BASE + 0x0000)
#define MFG_CG_SET          (G3D_CONFIG_BASE + 0x0004)
#define MFG_CG_CLR          (G3D_CONFIG_BASE + 0x0008)

#define DISP_CG_CON0        (DISPSYS_BASE + 0x100)
#define DISP_CG_SET0        (DISPSYS_BASE + 0x104)
#define DISP_CG_CLR0        (DISPSYS_BASE + 0x108)
#define DISP_CG_CON1        (DISPSYS_BASE + 0x110)
#define DISP_CG_SET1        (DISPSYS_BASE + 0x114)
#define DISP_CG_CLR1        (DISPSYS_BASE + 0x118)

#define IMG_CG_CON          (IMGSYS_CONFG_BASE + 0x0000)
#define IMG_CG_SET          (IMGSYS_CONFG_BASE + 0x0004)
#define IMG_CG_CLR          (IMGSYS_CONFG_BASE + 0x0008)

#define VDEC_CKEN_SET       (VDEC_GCON_BASE + 0x0000)
#define VDEC_CKEN_CLR       (VDEC_GCON_BASE + 0x0004)
#define LARB_CKEN_SET       (VDEC_GCON_BASE + 0x0008)
#define LARB_CKEN_CLR       (VDEC_GCON_BASE + 0x000C)


enum {
	MT_LARB_DISP = 0,
	MT_LARB_VDEC = 1,
	MT_LARB_IMG  = 2,
};


/* larb monitor mechanism definition */
enum {
	LARB_MONITOR_LEVEL_HIGH     = 10,
	LARB_MONITOR_LEVEL_MEDIUM   = 20,
	LARB_MONITOR_LEVEL_LOW      = 30,
};

struct larb_monitor {
	struct list_head link;
	int level;
	void (*backup)(struct larb_monitor *h, int larb_idx);       /* called before disable larb clock */
	void (*restore)(struct larb_monitor *h, int larb_idx);      /* called after enable larb clock */
};


#if CLKMGR_CLKM0


enum monitor_clk_sel_0{
	no_clk_0             = 0,
	AD_UNIV_624M_CK      = 5,
	AD_UNIV_416M_CK      = 6,
	AD_UNIV_249P6M_CK    = 7,
	AD_UNIV_178P3M_CK_0  = 8,
	AD_UNIV_48M_CK       = 9,
	AD_USB_48M_CK        = 10,
	rtc32k_ck_i_0        = 20,
	AD_SYS_26M_CK_0      = 21,
};


#endif /* CLKMGR_CLKM0 */


enum monitor_clk_sel {
	no_clk               = 0,
	AD_SYS_26M_CK        = 1,
	rtc32k_ck_i          = 2,
	clkph_MCLK_o         = 7,
	AD_DPICLK            = 8,
	AD_MSDCPLL_CK        = 9,
	AD_MMPLL_CK          = 10,
	AD_UNIV_178P3M_CK    = 11,
	AD_MAIN_H156M_CK     = 12,
	AD_VENCPLL_CK        = 13,
};


enum ckmon_sel {
#if CLKMGR_CLKM0
	clk_ckmon0           = 0,
#endif
	clk_ckmon1           = 1,
	clk_ckmon2           = 2,
	clk_ckmon3           = 3,
};


enum ABIST_CLK {
	ABIST_CLK_NULL,

	ABIST_AD_MAIN_H546M_CK            =  1,
	ABIST_AD_MAIN_H364M_CK            =  2,
	ABIST_AD_MAIN_H218P4M_CK          =  3,
	ABIST_AD_MAIN_H156M_CK            =  4,
	ABIST_AD_UNIV_624M_CK             =  5,
	ABIST_AD_UNIV_416M_CK             =  6,
	ABIST_AD_UNIV_249P6M_CK           =  7,
	ABIST_AD_UNIV_178P3M_CK           =  8,
	ABIST_AD_UNIV_48M_CK              =  9,
	ABIST_AD_USB_48M_CK               = 10,
	ABIST_AD_MMPLL_CK                 = 11,
	ABIST_AD_MSDCPLL_CK               = 12,
	ABIST_AD_DPICLK                   = 13,
	ABIST_CLKPH_MCK_O                 = 14,
	ABIST_AD_MEMPLL2_CKOUT0_PRE_ISO   = 15,
	ABIST_AD_MCUPLL1_H481M_CK         = 16,
	ABIST_AD_MDPLL1_416M_CK           = 17,
	ABIST_AD_WPLL_CK                  = 18,
	ABIST_AD_WHPLL_CK                 = 19,
	ABIST_RTC32K_CK_I                 = 20,
	ABIST_AD_SYS_26M_CK               = 21,
	ABIST_AD_VENCPLL_CK               = 22,
	ABIST_AD_MIPI_26M_CK              = 33,
	ABIST_AD_MEM_26M_CK               = 35,
	ABIST_AD_PLLGP_TST_CK             = 36,
	ABIST_AD_DSI0_LNTC_DSICLK         = 37,
	ABIST_AD_MPPLL_TST_CK             = 38,
	ABIST_ARMPLL_OCC_MON              = 39,
	ABIST_AD_MEM2MIPI_26M_CK          = 40,
	ABIST_AD_MEMPLL_MONCLK            = 41,
	ABIST_AD_MEMPLL2_MONCLK           = 42,
	ABIST_AD_MEMPLL3_MONCLK           = 43,
	ABIST_AD_MEMPLL4_MONCLK           = 44,
	ABIST_AD_MEMPLL_REFCLK            = 45,
	ABIST_AD_MEMPLL_FBCLK             = 46,
	ABIST_AD_MEMPLL2_REFCLK           = 47,
	ABIST_AD_MEMPLL2_FBCLK            = 48,
	ABIST_AD_MEMPLL3_REFCLK           = 49,
	ABIST_AD_MEMPLL3_FBCLK            = 50,
	ABIST_AD_MEMPLL4_REFCLK           = 51,
	ABIST_AD_MEMPLL4_FBCLK            = 52,
	ABIST_AD_MEMPLL_TSTDIV2_CK        = 53,
	ABIST_AD_LVDSPLL_CK               = 54,
	ABIST_AD_LVDSTX_MONCLK            = 55,
	ABIST_AD_HDMITX_MONCLK            = 56,
	ABIST_AD_USB20_C240M              = 57,
	ABIST_AD_USB20_C240M_1P           = 58,
	ABIST_AD_MONREF_CK                = 59,
	ABIST_AD_MONFBK_CK                = 60,
	ABIST_AD_TVDPLL_CK                = 61,
	ABIST_AD_AUDPLL_CK                = 62,
	ABIST_AD_LVDSPLL_ETH_CK           = 63,

	ABIST_CLK_END,
};


enum CKGEN_CLK {
	CKGEN_CLK_NULL,

	CKGEN_HF_FAXI_CK          =  1,
	CKGEN_HD_FAXI_CK          =  2,
	CKGEN_HF_FNFI2X_CK        =  3,
	CKGEN_HF_FDDRPHYCFG_CK    =  4,
	CKGEN_HF_FMM_CK           =  5,
	CKGEN_F_FPWM_CK           =  6,
	CKGEN_HF_FVDEC_CK         =  7,
	CKGEN_HF_FMFG_CK          =  8,
	CKGEN_HF_FCAMTG_CK        =  9,
	CKGEN_F_FUART_CK          = 10,
	CKGEN_HF_FSPI_CK          = 11,
	CKGEN_F_FUSB20_CK         = 12,
	CKGEN_HF_FMSDC30_0_CK     = 13,
	CKGEN_HF_FMSDC30_1_CK     = 14,
	CKGEN_HF_FMSDC30_2_CK     = 15,
	CKGEN_HF_FAUDIO_CK        = 16,
	CKGEN_HF_FAUD_INTBUS_CK   = 17,
	CKGEN_HF_FPMICSPI_CK      = 18,
	CKGEN_F_FRTC_CK           = 19,
	CKGEN_F_F26M_CK           = 20,
	CKGEN_F_F32K_MD1_CK       = 21,
	CKGEN_F_FRTC_CONN_CK      = 22,
	CKGEN_HF_FETH_50M_CK      = 23,
	CKGEN_HD_HAXI_NLI_CK      = 25,
	CKGEN_HD_QAXIDCM_CK       = 26,
	CKGEN_F_FFPC_CK           = 27,
	CKGEN_HF_FDPI0_CK         = 28,
	CKGEN_F_FCKBUS_CK_SCAN    = 29,
	CKGEN_F_FCKRTC_CK_SCAN    = 30,
	CKGEN_HF_FDPILVDS_CK      = 31,

	CKGEN_CLK_END,
};


#if CLKMGR_EXT

/* Measure clock frequency (in KHz) by frequency meter. */
extern uint32_t measure_abist_freq(enum ABIST_CLK clk);
extern uint32_t measure_ckgen_freq(enum CKGEN_CLK clk);

#endif /* CLKMGR_EXT */


extern void register_larb_monitor(struct larb_monitor *handler);
extern void unregister_larb_monitor(struct larb_monitor *handler);

/* clock API */
extern int enable_clock(enum cg_clk_id id, char *mod_name);
extern int disable_clock(enum cg_clk_id id, char *mod_name);
extern int mt_enable_clock(enum cg_clk_id id, char *mod_name);
extern int mt_disable_clock(enum cg_clk_id id, char *mod_name);

extern int clock_is_on(int id);

extern int clkmux_sel(int id, unsigned int clksrc, char *name);
extern void enable_mux(int id, char *name);
extern void disable_mux(int id, char *name);

extern void clk_set_force_on(int id);
extern void clk_clr_force_on(int id);
extern int clk_is_force_on(int id);

/* pll API */
extern int enable_pll(int id, char *mod_name);
extern int disable_pll(int id, char *mod_name);

extern int pll_hp_switch_on(int id, int hp_on);
extern int pll_hp_switch_off(int id, int hp_off);

#if CLKMGR_8127

/* set/get PLL frequency in KHz. */
extern unsigned int pll_get_freq(int id);
extern unsigned int pll_set_freq(int id, unsigned int freq_khz);

#endif /* CLKMGR_8127 */

extern int pll_fsel(int id, unsigned int value);
extern int pll_is_on(int id);

/* subsys API */
extern int enable_subsys(int id, char *mod_name);
extern int disable_subsys(int id, char *mod_name);
extern int subsys_is_on(int id);

extern bool isp_vdec_on_off(void);

extern int conn_power_on(void);
extern int conn_power_off(void);

/* other API */
const char *grp_get_name(int id);
int clk_id_to_grp_id(enum cg_clk_id id);
unsigned int clk_id_to_mask(enum cg_clk_id id);


/* init */
extern void mt_clkmgr_init(void);

extern void CLKM_32K(bool flag);
extern int CLK_Monitor(enum ckmon_sel ckmon, enum monitor_clk_sel sel, int div);

#if CLKMGR_CLKM0
extern int CLK_Monitor_0(enum ckmon_sel ckmon, enum monitor_clk_sel_0 sel, int div);
#endif /* CLKMGR_CLKM0 */


#if !CLKMGR_8127


/* deprecated or unused in MT8127 */

extern int enable_clock_ext_locked(int id, char *mod_name);
extern int disable_clock_ext_locked(int id, char *mod_name);

extern int enable_pll_spec(int id, char *mod_name);
extern int disable_pll_spec(int id, char *mod_name);

extern int md_power_on(int id);
extern int md_power_off(int id, unsigned int timeout);

extern void enable_clksq1(void);
extern void disable_clksq1(void);

extern void clksq1_sw2hw(void);
extern void clksq1_hw2sw(void);

extern int clkmgr_is_locked(void);


#endif /* !CLKMGR_8127 */


#ifdef __MT_CLKMGR_C__

/* clkmgr internal use only */

#ifdef CONFIG_MTK_MMC
extern void msdc_clk_status(int *status);
#endif

#endif /* __MT_CLKMGR_C__ */


#endif /* __KERNEL__ */


#endif
