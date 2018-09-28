/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2010
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/
#define	MSDC_DEBUG_KICKOFF

#include "cust_msdc.h"
#include "msdc.h"
#include "msdc_utils.h"
#include "mmc_core.h"
//#include "sdio.h"
//#include "api.h"
//#include "cache_api.h"

// APB Module msdc
#define MSDC0_BASE (0x11230000)

// APB Module msdc

#define MSDC1_BASE (0x11240000)

// APB Module msdc
#define MSDC2_BASE (0x11250000)

#define MSDC3_BASE (0x11260000)

#define CMD_RETRIES        (5)
#define CMD_TIMEOUT        (100) /* 100ms */

#define PERI_MSDC_SRCSEL   (0xc100000c)

/* Tuning Parameter */
#define DEFAULT_DEBOUNCE   (8)	/* 8 cycles */
#define DEFAULT_DTOC       (40)	/* data timeout counter. 65536x40 sclk. */
#define DEFAULT_WDOD       (0)	/* write data output delay. no delay. */
#define DEFAULT_BSYDLY     (8)	/* card busy delay. 8 extend sclk */

#define MAX_DMA_CNT        (32768)
#define MAX_GPD_POOL_SZ    (2)  /* include null gpd */
#define MAX_BD_POOL_SZ     (4)
#define MAX_SG_POOL_SZ     (MAX_BD_POOL_SZ)
#define MAX_SG_BUF_SZ      (MAX_DMA_CNT)
#define MAX_BD_PER_GPD     (MAX_BD_POOL_SZ/(MAX_GPD_POOL_SZ-1)) /* except null gpd */

#define MAX_DMA_TRAN_SIZE  (MAX_SG_POOL_SZ*MAX_SG_BUF_SZ)

#if MAX_SG_BUF_SZ > MAX_DMA_CNT
#error "incorrect max sg buffer size"
#endif

typedef struct {
    int    pio_bits;                   /* PIO width: 32, 16, or 8bits */
    int    autocmd;
    int    cmd23_flags;
    struct dma_config  cfg;
    struct scatterlist sg[MAX_SG_POOL_SZ];
    int    alloc_gpd;
    int    alloc_bd;
    int    rdsmpl;
    int    wdsmpl;
    int    rsmpl;
    int    start_bit;
    gpd_t *active_head;
    gpd_t *active_tail;
    gpd_t *gpd_pool;
    __bd_t  *bd_pool;
} msdc_priv_t;

static int msdc_rsp[] = {
    0,  /* RESP_NONE */
    1,  /* RESP_R1 */
    2,  /* RESP_R2 */
    3,  /* RESP_R3 */
    4,  /* RESP_R4 */
    1,  /* RESP_R5 */
    1,  /* RESP_R6 */
    1,  /* RESP_R7 */
    7,  /* RESP_R1b */
};


static msdc_priv_t msdc_priv[MSDC_MAX_NUM];
static gpd_t msdc_gpd_pool[MSDC_MAX_NUM][MAX_GPD_POOL_SZ] __attribute__ ((aligned(16))) __attribute__ ((unused)); // FOR mt6582 EMI not cross 2K boundary
static __bd_t  msdc_bd_pool[MSDC_MAX_NUM][MAX_BD_POOL_SZ] __attribute__ ((aligned(16))) __attribute__ ((unused));
static u32 hclks_msdc50[] = { 26000000, 400000000, 200000000, 156000000, 182000000,
                             156000000, 100000000, 624000000, 312000000};
static u32 hclks_msdc30[] = { 26000000, 208000000, 200000000, 156000000,
	                         182000000, 156000000, 178280000};


void msdc_dump_card_status(u32 card_status)
{
#ifndef KEEP_SLIENT_BUILD
    static char *state[] = {
        "Idle",			/* 0 */
        "Ready",		/* 1 */
        "Ident",		/* 2 */
        "Stby",			/* 3 */
        "Tran",			/* 4 */
        "Data",			/* 5 */
        "Rcv",			/* 6 */
        "Prg",			/* 7 */
        "Dis",			/* 8 */
        "Ina",             /* 9 */
        "Sleep",           /* 10 */
        "Reserved",		/* 11 */
        "Reserved",		/* 12 */
        "Reserved",		/* 13 */
        "Reserved",		/* 14 */
        "I/O mode",		/* 15 */
    };
#endif    
    if (card_status & R1_OUT_OF_RANGE)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Out of Range\n"));
    if (card_status & R1_ADDRESS_ERROR)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Address Error\n"));
    if (card_status & R1_BLOCK_LEN_ERROR)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Block Len Error\n"));
    if (card_status & R1_ERASE_SEQ_ERROR)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Erase Seq Error\n"));
    if (card_status & R1_ERASE_PARAM)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Erase Param\n"));
    if (card_status & R1_WP_VIOLATION)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] WP Violation\n"));
    if (card_status & R1_CARD_IS_LOCKED)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Card is Locked\n"));
    if (card_status & R1_LOCK_UNLOCK_FAILED)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Lock/Unlock Failed\n"));
    if (card_status & R1_COM_CRC_ERROR)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Command CRC Error\n"));
    if (card_status & R1_ILLEGAL_COMMAND)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Illegal Command\n"));
    if (card_status & R1_CARD_ECC_FAILED)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Card ECC Failed\n"));
    if (card_status & R1_CC_ERROR)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] CC Error\n"));
    if (card_status & R1_ERROR)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Error\n"));
    if (card_status & R1_UNDERRUN)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Underrun\n"));
    if (card_status & R1_OVERRUN)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Overrun\n"));
    if (card_status & R1_CID_CSD_OVERWRITE)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] CID/CSD Overwrite\n"));
    if (card_status & R1_WP_ERASE_SKIP)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] WP Eraser Skip\n"));
    if (card_status & R1_CARD_ECC_DISABLED)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Card ECC Disabled\n"));
    if (card_status & R1_ERASE_RESET)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Erase Reset\n"));
    if (card_status & R1_READY_FOR_DATA)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Ready for Data\n"));
    if (card_status & R1_SWITCH_ERROR)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Switch error\n"));
    if (card_status & R1_URGENT_BKOPS)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] Urgent background operations\n"));
    if (card_status & R1_APP_CMD)
        MSDC_TRC_PRINT(MSDC_DSTATUS,("\t[CARD_STATUS] App Command\n"));

    MSDC_TRC_PRINT(MSDC_INFO,("\t[CARD_STATUS] '%s' State\n",
    								state[R1_CURRENT_STATE(card_status)]));
}

void msdc_dump_ocr_reg(u32 resp)
{
    if (resp & (1 << 7))
        MSDC_TRC_PRINT(MSDC_DREG,("\t[OCR] Low Voltage Range\n"));
	
    if (resp & (1 << 15))
        MSDC_TRC_PRINT(MSDC_DREG,("\t[OCR] 2.7-2.8 volt\n"));
	
    if (resp & (1 << 16))
        MSDC_TRC_PRINT(MSDC_DREG,("\t[OCR] 2.8-2.9 volt\n"));
	
    if (resp & (1 << 17))
        MSDC_TRC_PRINT(MSDC_DREG,("\t[OCR] 2.9-3.0 volt\n"));
	
    if (resp & (1 << 18))
        MSDC_TRC_PRINT(MSDC_DREG,("\t[OCR] 3.0-3.1 volt\n"));
	
    if (resp & (1 << 19))
        MSDC_TRC_PRINT(MSDC_DREG,("\t[OCR] 3.1-3.2 volt\n"));
	
    if (resp & (1 << 20))
        MSDC_TRC_PRINT(MSDC_DREG,("\t[OCR] 3.2-3.3 volt\n"));
	
    if (resp & (1 << 21))
        MSDC_TRC_PRINT(MSDC_DREG,("\t[OCR] 3.3-3.4 volt\n"));
	
    if (resp & (1 << 22))
        MSDC_TRC_PRINT(MSDC_DREG,("\t[OCR] 3.4-3.5 volt\n"));
	
    if (resp & (1 << 23))
        MSDC_TRC_PRINT(MSDC_DREG,("\t[OCR] 3.5-3.6 volt\n"));
	
    if (resp & (1 << 24)){
        MSDC_TRC_PRINT(MSDC_DREG,("\t[OCR] Switching to 1.8V Accepted (S18A)\n"));
		//if(MSDC_DREG)
           MSDC_TRC_PRINT(MSDC_INFO,("\t[OCR] Switching to 1.8V Accepted (S18A)\n"));
    }
	
    if (resp & (1 << 30))
        MSDC_TRC_PRINT(MSDC_DREG,("\t[OCR] Card Capacity Status (CCS)\n"));
	
    if (resp & (1UL << 31))
        MSDC_TRC_PRINT(MSDC_DREG,("\t[OCR] Card Power Up Status (Idle)\n"));
    else
        MSDC_TRC_PRINT(MSDC_DREG,("\t[OCR] Card Power Up Status (Busy)\n"));
	
}

void msdc_dump_io_resp(u32 resp)
{
    u32 flags = (resp >> 8) & 0xFF;
#ifndef KEEP_SLIENT_BUILD    
    char *state[] = {"DIS", "CMD", "TRN", "RFU"};
#endif    
    if (flags & (1 << 7))
        MSDC_TRC_PRINT(MSDC_STACK,("\t[IO] COM_CRC_ERR\n"));
    if (flags & (1 << 6))
        MSDC_TRC_PRINT(MSDC_STACK,("\t[IO] Illgal command\n"));
    if (flags & (1 << 3))
        MSDC_TRC_PRINT(MSDC_STACK,("\t[IO] Error\n"));
    if (flags & (1 << 2))
        MSDC_TRC_PRINT(MSDC_STACK,("\t[IO] RFU\n"));
    if (flags & (1 << 1))
        MSDC_TRC_PRINT(MSDC_STACK,("\t[IO] Function number error\n"));
    if (flags & (1 << 0))
        MSDC_TRC_PRINT(MSDC_STACK,("\t[IO] Out of range\n"));

    MSDC_TRC_PRINT(MSDC_STACK,("[IO] State: %s, Data:0x%x\n", state[(resp >> 12) & 0x3], resp & 0xFF));
}

void msdc_dump_rca_resp(u32 resp)
{
    u32 card_status = (((resp >> 15) & 0x1) << 23) |
                      (((resp >> 14) & 0x1) << 22) |
                      (((resp >> 13) & 0x1) << 19) |
                        (resp & 0x1fff);

    MSDC_TRC_PRINT(MSDC_STACK,("\t[RCA] 0x%x\n", resp >> 16));
    msdc_dump_card_status(card_status);
}
static void msdc_dump_dbg_register(struct mmc_host *host)
{
    u32 base = host->base;
    u32 i;

    for (i = 0; i < 26; i++) {
        MSDC_WRITE32(MSDC_DBG_SEL, i);
        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d]SW_DBG_SEL: write reg[%x] to 0x%x\n", host->id, OFFSET_MSDC_DBG_SEL, i));
        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d]SW_DBG_OUT: read  reg[%x] to 0x%x\n", 
									host->id, OFFSET_MSDC_DBG_OUT, MSDC_READ32(MSDC_DBG_OUT)));
    }

    MSDC_WRITE32(MSDC_DBG_SEL, 0);
}

void msdc_dump_register(struct mmc_host *host)
{
#ifndef KEEP_SLIENT_BUILD
    u32 base = host->base;
#endif

#ifndef FPGA_PLATFORM
#ifdef ___MSDC_DEBUG___
		u32 u4mode, u4rdsel,u4tdsel,u4smt, u4ies, u4sr, u4rx, u4tx;
#endif /* ___MSDC_DEBUG___*/
#endif /* FPGA_PLATFORM */

    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] MSDC_CFG       = 0x%x\n", host->id, OFFSET_MSDC_CFG,       MSDC_READ32(MSDC_CFG)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] MSDC_IOCON     = 0x%x\n", host->id, OFFSET_MSDC_IOCON,     MSDC_READ32(MSDC_IOCON)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] MSDC_PS        = 0x%x\n", host->id, OFFSET_MSDC_PS,        MSDC_READ32(MSDC_PS)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] MSDC_INT       = 0x%x\n", host->id, OFFSET_MSDC_INT,       MSDC_READ32(MSDC_INT)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] MSDC_INTEN     = 0x%x\n", host->id, OFFSET_MSDC_INTEN,     MSDC_READ32(MSDC_INTEN)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] MSDC_FIFOCS    = 0x%x\n", host->id, OFFSET_MSDC_FIFOCS,    MSDC_READ32(MSDC_FIFOCS)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] MSDC_TXDATA    = not read\n", host->id, OFFSET_MSDC_TXDATA));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] MSDC_RXDATA    = not read\n", host->id, OFFSET_MSDC_RXDATA));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SDC_CFG        = 0x%x\n", host->id, OFFSET_SDC_CFG,        MSDC_READ32(SDC_CFG)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SDC_CMD        = 0x%x\n", host->id, OFFSET_SDC_CMD,        MSDC_READ32(SDC_CMD)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SDC_ARG        = 0x%x\n", host->id, OFFSET_SDC_ARG,        MSDC_READ32(SDC_ARG)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SDC_STS        = 0x%x\n", host->id, OFFSET_SDC_STS,        MSDC_READ32(SDC_STS)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SDC_RESP0      = 0x%x\n", host->id, OFFSET_SDC_RESP0,      MSDC_READ32(SDC_RESP0)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SDC_RESP1      = 0x%x\n", host->id, OFFSET_SDC_RESP1,      MSDC_READ32(SDC_RESP1)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SDC_RESP2      = 0x%x\n", host->id, OFFSET_SDC_RESP2,      MSDC_READ32(SDC_RESP2)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SDC_RESP3      = 0x%x\n", host->id, OFFSET_SDC_RESP3,      MSDC_READ32(SDC_RESP3)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SDC_BLK_NUM    = 0x%x\n", host->id, OFFSET_SDC_BLK_NUM,    MSDC_READ32(SDC_BLK_NUM)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SDC_VOL_CHG    = 0x%x\n", host->id, OFFSET_SDC_VOL_CHG,    MSDC_READ32(SDC_VOL_CHG)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SDC_CSTS       = 0x%x\n", host->id, OFFSET_SDC_CSTS,       MSDC_READ32(SDC_CSTS)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SDC_CSTS_EN    = 0x%x\n", host->id, OFFSET_SDC_CSTS_EN,    MSDC_READ32(SDC_CSTS_EN)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SDC_DATCRC_STS = 0x%x\n", host->id, OFFSET_SDC_DCRC_STS,   MSDC_READ32(SDC_DCRC_STS)));

    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC_CFG0      = 0x%x\n", host->id, OFFSET_EMMC_CFG0,      MSDC_READ32(EMMC_CFG0)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC_CFG1      = 0x%x\n", host->id, OFFSET_EMMC_CFG1,      MSDC_READ32(EMMC_CFG1)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC_STS       = 0x%x\n", host->id, OFFSET_EMMC_STS,       MSDC_READ32(EMMC_STS)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC_IOCON     = 0x%x\n", host->id, OFFSET_EMMC_IOCON,     MSDC_READ32(EMMC_IOCON)));

    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SDC_ACMD_RESP  = 0x%x\n", host->id, OFFSET_SDC_ACMD_RESP,  MSDC_READ32(SDC_ACMD_RESP)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SDC_ACMD19_TRG = 0x%x\n", host->id, OFFSET_SDC_ACMD19_TRG, MSDC_READ32(SDC_ACMD19_TRG)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SDC_ACMD19_STS = 0x%x\n", host->id, OFFSET_SDC_ACMD19_STS, MSDC_READ32(SDC_ACMD19_STS)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] MSDC_DMA_HIGH4BIT = 0x%x\n", host->id, OFFSET_MSDC_DMA_HIGH4BIT, MSDC_READ32(MSDC_DMA_HIGH4BIT)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] DMA_SA         = 0x%x\n", host->id, OFFSET_MSDC_DMA_SA,    MSDC_READ32(MSDC_DMA_SA)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] DMA_CA         = 0x%x\n", host->id, OFFSET_MSDC_DMA_CA,    MSDC_READ32(MSDC_DMA_CA)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] DMA_CTRL       = 0x%x\n", host->id, OFFSET_MSDC_DMA_CTRL,  MSDC_READ32(MSDC_DMA_CTRL)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] DMA_CFG        = 0x%x\n", host->id, OFFSET_MSDC_DMA_CFG,   MSDC_READ32(MSDC_DMA_CFG)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SW_DBG_SEL     = 0x%x\n", host->id, OFFSET_MSDC_DBG_SEL,   MSDC_READ32(MSDC_DBG_SEL)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] SW_DBG_OUT     = 0x%x\n", host->id, OFFSET_MSDC_DBG_OUT,   MSDC_READ32(MSDC_DBG_OUT)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] MSDC_DMA_LEN   = 0x%x\n", host->id, OFFSET_MSDC_DMA_LEN,   MSDC_READ32(MSDC_DMA_LEN)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] PATCH_BIT0     = 0x%x\n", host->id, OFFSET_MSDC_PATCH_BIT0,MSDC_READ32(MSDC_PATCH_BIT0)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] PATCH_BIT1     = 0x%x\n", host->id, OFFSET_MSDC_PATCH_BIT1,MSDC_READ32(MSDC_PATCH_BIT1)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] MSDC_PATCH_BIT2= 0x%x\n", host->id, OFFSET_MSDC_PATCH_BIT2,MSDC_READ32(MSDC_PATCH_BIT2)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] PAD_TUNE(0)    = 0x%x\n", host->id, OFFSET_MSDC_PAD_TUNE,  MSDC_READ32(MSDC_PAD_TUNE)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] PAD_TUNE1      = 0x%x\n", host->id, OFFSET_MSDC_PAD_TUNE1, MSDC_READ32(MSDC_PAD_TUNE1)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] DAT_RD_DLY0    = 0x%x\n", host->id, OFFSET_MSDC_DAT_RDDLY0,MSDC_READ32(MSDC_DAT_RDDLY0)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] DAT_RD_DLY1    = 0x%x\n", host->id, OFFSET_MSDC_DAT_RDDLY1,MSDC_READ32(MSDC_DAT_RDDLY1)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] DAT_RD_DLY2    = 0x%x\n", host->id, OFFSET_MSDC_DAT_RDDLY2,MSDC_READ32(MSDC_DAT_RDDLY2)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] DAT_RD_DLY3    = 0x%x\n", host->id, OFFSET_MSDC_DAT_RDDLY3,MSDC_READ32(MSDC_DAT_RDDLY3)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] HW_DBG_SEL     = 0x%x\n", host->id, OFFSET_MSDC_HW_DBG,    MSDC_READ32(MSDC_HW_DBG)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] MAIN_VER       = 0x%x\n", host->id, OFFSET_MSDC_VERSION,   MSDC_READ32(MSDC_VERSION)));
    MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] MSDC_ECO_VER   = 0x%x\n", host->id, OFFSET_MSDC_ECO_VER,   MSDC_READ32(MSDC_ECO_VER)));

    if (host->id == 3){
        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC50_PAD_CTL0          = 0x%x\n", host->id, OFFSET_EMMC50_PAD_CTL0,       MSDC_READ32(EMMC50_PAD_CTL0)));
        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC50_PAD_DS_CTL0       = 0x%x\n", host->id, OFFSET_EMMC50_PAD_DS_CTL0,    MSDC_READ32(EMMC50_PAD_DS_CTL0)));
        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC50_PAD_DS_TUNE       = 0x%x\n", host->id, OFFSET_EMMC50_PAD_DS_TUNE,    MSDC_READ32(EMMC50_PAD_DS_TUNE)));
        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC50_PAD_CMD_TUNE      = 0x%x\n", host->id, OFFSET_EMMC50_PAD_CMD_TUNE,   MSDC_READ32(EMMC50_PAD_CMD_TUNE)));
        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC50_PAD_DAT01_TUNE    = 0x%x\n", host->id, OFFSET_EMMC50_PAD_DAT01_TUNE, MSDC_READ32(EMMC50_PAD_DAT01_TUNE)));
        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC50_PAD_DAT23_TUNE    = 0x%x\n", host->id, OFFSET_EMMC50_PAD_DAT23_TUNE, MSDC_READ32(EMMC50_PAD_DAT23_TUNE)));
        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC50_PAD_DAT45_TUNE    = 0x%x\n", host->id, OFFSET_EMMC50_PAD_DAT45_TUNE, MSDC_READ32(EMMC50_PAD_DAT45_TUNE)));
        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC50_PAD_DAT67_TUNE    = 0x%x\n", host->id, OFFSET_EMMC50_PAD_DAT67_TUNE, MSDC_READ32(EMMC50_PAD_DAT67_TUNE)));

        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC51_CFG0              = 0x%x\n", host->id, OFFSET_EMMC51_CFG0,    MSDC_READ32(EMMC51_CFG0)));
        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC50_CFG0              = 0x%x\n", host->id, OFFSET_EMMC50_CFG0,    MSDC_READ32(EMMC50_CFG0)));
        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC50_CFG1              = 0x%x\n", host->id, OFFSET_EMMC50_CFG1,    MSDC_READ32(EMMC50_CFG1)));
        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC50_CFG2              = 0x%x\n", host->id, OFFSET_EMMC50_CFG2,    MSDC_READ32(EMMC50_CFG2)));
        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC50_CFG3              = 0x%x\n", host->id, OFFSET_EMMC50_CFG3,    MSDC_READ32(EMMC50_CFG3)));
        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC50_CFG4              = 0x%x\n", host->id, OFFSET_EMMC50_CFG4,    MSDC_READ32(EMMC50_CFG4)));
        MSDC_TRC_PRINT(MSDC_HREG,("[SD%d] Reg[%x] EMMC50_BLOCK_LENGTH      = 0x%x\n", host->id, OFFSET_EMMC50_BLOCK_LENGTH,    MSDC_READ32(EMMC50_BLOCK_LENGTH)));
    }

    msdc_dump_dbg_register(host);

#ifndef FPGA_PLATFORM
#ifdef ___MSDC_DEBUG___
    if(host->id == 0){
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 6), MSDC_PAD_RDSEL(MSDC0),  u4rdsel);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 6), MSDC_PAD_TDSEL(MSDC0),  u4tdsel);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%u Pad RDSEL(%u) TDSEL(%u)\r\n", host->id, u4rdsel,u4tdsel));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_CLK(MSDC0)), BIT_MODE_GPIOMASK(PIN4_CLK(MSDC0)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 0), MSDC_CLK_SR(MSDC0),    u4sr);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 0), BIT_MSDC_CLK_E8E4E2,   u4tx);	
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 0), MSDC_CLK_IES(MSDC0),   u4ies);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 0), MSDC_CLK_SMT(MSDC0),   u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 0), BIT_MSDC_CLK_R0R1PUPD, u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uCLK :GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)    \r\n",\
											host->id, PIN4_CLK(MSDC0), u4mode,u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_CMD(MSDC0)), BIT_MODE_GPIOMASK(PIN4_CMD(MSDC0)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 1), MSDC_CMD_SR(MSDC0),    u4sr);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 1), BIT_MSDC_CMD_E8E4E2,   u4tx);	
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 1), MSDC_CMD_IES(MSDC0),   u4ies);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 1), MSDC_CMD_SMT(MSDC0),   u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 1), BIT_MSDC_CMD_R0R1PUPD, u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uCMD :GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_CMD(MSDC0), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 2), MSDC_DAT_SR(MSDC0),    u4sr);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 2), BIT_MSDC_DAT_E8E4E2,   u4tx);	
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 2), MSDC_DAT_IES(MSDC0),   u4ies);
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT0)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT0)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 3), MSDC_DAT_SMT(MSDC0, DAT0),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 3), BIT_MSDC_DAT0_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT0:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC0, DAT0), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT1)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT1)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 3), MSDC_DAT_SMT(MSDC0, DAT1),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 3), BIT_MSDC_DAT1_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT1:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC0, DAT1), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));

		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT2)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT2)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 3), MSDC_DAT_SMT(MSDC0, DAT2),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 3), BIT_MSDC_DAT2_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT2:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC0, DAT2), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT3)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT3)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 3), MSDC_DAT_SMT(MSDC0, DAT3),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 3), BIT_MSDC_DAT3_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT3:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC0, DAT3), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));

		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT4)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT4)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 4), MSDC_DAT_SMT(MSDC0, DAT4),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 4), BIT_MSDC_DAT4_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT4:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC0, DAT4), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT5)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT5)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 4), MSDC_DAT_SMT(MSDC0, DAT5),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 4), BIT_MSDC_DAT5_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT5:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC0, DAT5), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));

		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT6)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT6)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 4), MSDC_DAT_SMT(MSDC0, DAT6),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 4), BIT_MSDC_DAT6_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT6:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC0, DAT6), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT7)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT7)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 4), MSDC_DAT_SMT(MSDC0, DAT7),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 4), BIT_MSDC_DAT7_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT7:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
									    host->id, PIN4_DAT(MSDC0, DAT7), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
    }
	else if(host->id == 1){
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 5), MSDC_PAD_RDSEL(MSDC1),  u4rdsel);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 5), MSDC_PAD_TDSEL(MSDC1),  u4tdsel);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%u Pad RDSEL(%u) TDSEL(%u) \r\n", host->id, u4rdsel,u4tdsel));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_CLK(MSDC1)), BIT_MODE_GPIOMASK(PIN4_CLK(MSDC1)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 0), MSDC_CLK_SR(MSDC1),    u4sr);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 0), BIT_MSDC_CLK_E8E4E2,   u4tx);	
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 0), MSDC_CLK_IES(MSDC1),   u4ies);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 0), MSDC_CLK_SMT(MSDC1),   u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 0), BIT_MSDC_CLK_R0R1PUPD, u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uCLK :GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id,PIN4_CLK(MSDC1), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_CMD(MSDC1)), BIT_MODE_GPIOMASK(PIN4_CMD(MSDC1)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 1), MSDC_CMD_SR(MSDC1),    u4sr);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 1), BIT_MSDC_CMD_E8E4E2,   u4tx);	
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 1), MSDC_CMD_IES(MSDC1),   u4ies);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 1), MSDC_CMD_SMT(MSDC1),   u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 1), BIT_MSDC_CMD_R0R1PUPD, u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uCMD :GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id,PIN4_CMD(MSDC1), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 2), MSDC_DAT_SR(MSDC1),    u4sr);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 2), BIT_MSDC_DAT_E8E4E2,   u4tx);	
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 2), MSDC_DAT_IES(MSDC1),   u4ies);
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC1,DAT0)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC1,DAT0)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 3), MSDC_DAT_SMT(MSDC1, DAT0),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 3), BIT_MSDC_DAT0_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT0:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC1, DAT0), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC1,DAT1)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC1,DAT1)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 3), MSDC_DAT_SMT(MSDC1, DAT1),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 3), BIT_MSDC_DAT1_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT1:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC1, DAT1), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));

		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC1,DAT2)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC1,DAT2)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 3), MSDC_DAT_SMT(MSDC1, DAT2),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 3), BIT_MSDC_DAT2_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT2:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC1, DAT2), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC1,DAT3)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC1,DAT3)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 3), MSDC_DAT_SMT(MSDC1, DAT3),u4smt);		
		MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 3), BIT_MSDC_DAT3_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT3:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC1, DAT3), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
    }
	else if(host->id == 2){
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 5), MSDC_PAD_RDSEL(MSDC2),  u4rdsel);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 5), MSDC_PAD_TDSEL(MSDC2),  u4tdsel);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%u Pad RDSEL(%u) TDSEL(%u) \r\n", host->id, u4rdsel,u4tdsel));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_CLK(MSDC2)), BIT_MODE_GPIOMASK(PIN4_CLK(MSDC2)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 0), MSDC_CLK_SR(MSDC2),    u4sr);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 0), BIT_MSDC_CLK_E8E4E2,   u4tx);	
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 0), MSDC_CLK_IES(MSDC2),   u4ies);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 0), MSDC_CLK_SMT(MSDC2),   u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 0), BIT_MSDC_CLK_R0R1PUPD, u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uCLK :GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id,PIN4_CLK(MSDC2), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_CMD(MSDC2)), BIT_MODE_GPIOMASK(PIN4_CMD(MSDC2)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 1), MSDC_CMD_SR(MSDC2),    u4sr);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 1), BIT_MSDC_CMD_E8E4E2,   u4tx);	
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 1), MSDC_CMD_IES(MSDC2),   u4ies);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 1), MSDC_CMD_SMT(MSDC2),   u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 1), BIT_MSDC_CMD_R0R1PUPD, u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uCMD :GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id,PIN4_CMD(MSDC2), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 2), MSDC_DAT_SR(MSDC2),    u4sr);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 2), BIT_MSDC_DAT_E8E4E2,   u4tx);	
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 2), MSDC_DAT_IES(MSDC2),   u4ies);
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC2,DAT0)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC2,DAT0)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 3), MSDC_DAT_SMT(MSDC2, DAT0),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 3), BIT_MSDC_DAT0_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT0:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC2, DAT0), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC2,DAT1)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC2,DAT1)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 3), MSDC_DAT_SMT(MSDC2, DAT1),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 3), BIT_MSDC_DAT1_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT1:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC2, DAT1), u4mode,u4sr, u4tx, u4ies, u4smt, u4rx));

		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC2,DAT2)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC2,DAT2)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 3), MSDC_DAT_SMT(MSDC2, DAT2),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 3), BIT_MSDC_DAT2_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT2:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC2, DAT2), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC2,DAT3)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC2,DAT3)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 3), MSDC_DAT_SMT(MSDC2, DAT3),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 3), BIT_MSDC_DAT3_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT3:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC2, DAT3), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
    }
	else if(host->id == 3){
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 6), MSDC_PAD_RDSEL(MSDC3),  u4rdsel);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 6), MSDC_PAD_TDSEL(MSDC3),  u4tdsel);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%u Pad RDSEL(%u) TDSEL(%u) \r\n", host->id, u4rdsel,u4tdsel));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_CLK(MSDC3)), BIT_MODE_GPIOMASK(PIN4_CLK(MSDC3)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 0), MSDC_CLK_SR(MSDC3),    u4sr);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 0), BIT_MSDC_CLK_E8E4E2,   u4tx);	
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 0), MSDC_CLK_IES(MSDC3),   u4ies);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 0), MSDC_CLK_SMT(MSDC3),   u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 0), BIT_MSDC_CLK_R0R1PUPD, u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uCLK :GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id,PIN4_CLK(MSDC3), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_CMD(MSDC3)), BIT_MODE_GPIOMASK(PIN4_CMD(MSDC3)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 1), MSDC_CMD_SR(MSDC3),    u4sr);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 1), BIT_MSDC_CMD_E8E4E2,   u4tx);	
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 1), MSDC_CMD_IES(MSDC3),   u4ies);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 1), MSDC_CMD_SMT(MSDC3),   u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 1), BIT_MSDC_CMD_R0R1PUPD, u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uCMD :GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id,PIN4_CMD(MSDC3), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 2), MSDC_DAT_SR(MSDC3),    u4sr);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 2), BIT_MSDC_DAT_E8E4E2,   u4tx);	
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 2), MSDC_DAT_IES(MSDC3),   u4ies);
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT0)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT0)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 3), MSDC_DAT_SMT(MSDC3, DAT0),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 3), BIT_MSDC_DAT0_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT0:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC3, DAT0), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT1)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT1)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 3), MSDC_DAT_SMT(MSDC3, DAT1),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 3), BIT_MSDC_DAT1_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT1:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC3, DAT1), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));

		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT2)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT2)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 3), MSDC_DAT_SMT(MSDC3, DAT2),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 3), BIT_MSDC_DAT2_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT2:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC3, DAT2), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT3)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT3)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 3), MSDC_DAT_SMT(MSDC3, DAT3),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 3), BIT_MSDC_DAT3_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT3:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC3, DAT3), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));

		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT4)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT4)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 4), MSDC_DAT_SMT(MSDC3, DAT4),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 4), BIT_MSDC_DAT4_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT4:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC3, DAT4), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT5)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT5)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 4), MSDC_DAT_SMT(MSDC3, DAT5),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 4), BIT_MSDC_DAT5_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT5:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC3, DAT5), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));

		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT6)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT6)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 4), MSDC_DAT_SMT(MSDC3, DAT6),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 4), BIT_MSDC_DAT6_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT6:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC3, DAT6), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));
		
		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT7)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT7)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 4), MSDC_DAT_SMT(MSDC3, DAT7),u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 4), BIT_MSDC_DAT7_R0R1PUPD,u4rx);
	    MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDAT7:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
										host->id, PIN4_DAT(MSDC3, DAT7), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));	

		MSDC_GET_FIELD(GPIO_MODE_PIN2REG(PIN4_DSL(MSDC3)), BIT_MODE_GPIOMASK(PIN4_DSL(MSDC3)), u4mode);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 6), MSDC_DSL_SMT(MSDC3),   u4smt);
		MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 6), BIT_MSDC_DSL_R0R1PUPD, u4rx);
#if 0
        MSDC_TRC_PRINT(MSDC_GPIO,("MSDC%uDSL :GPIO%03u:%u: SMT(%u) RX(%u)\r\n",\
										host->id,PIN4_DSL(MSDC3), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx));	
#endif
    }
#endif /* ___MSDC_DEBUG___*/
#endif /* FPGA_PLATFORM */		
	
}

static void msdc_dump_dma_desc(struct mmc_host *host) __attribute__((unused));
static void msdc_dump_dma_desc(struct mmc_host *host) 
{
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;
    int i;

#ifndef KEEP_SLIENT_BUILD
    u32 *ptr;
#else 
    u32 __attribute__ ((unused)) *ptr;
#endif

        for (i = 0; i < priv->alloc_gpd; i++) {
            ptr = (u32*)&priv->gpd_pool[i];
            MSDC_DBG_PRINT(MSDC_DMA,("[SD%d] GD[%d](0x%xh): %xh %xh %xh %xh %xh %xh %xh\n", 
                host->id, i, (u32)ptr, *ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), 
                *(ptr+5), *(ptr+6)));
        }

        for (i = 0; i < priv->alloc_bd; i++) {
            ptr = (u32*)&priv->bd_pool[i];
            MSDC_DBG_PRINT(MSDC_DMA,("[SD%d] BD[%d](0x%xh): %xh %xh %xh %xh\n", 
                host->id, i, (u32)ptr, *ptr, *(ptr+1), *(ptr+2), *(ptr+3)));
        }
}

/* add function for MSDC_PAD_CTL handle */
#ifndef FPGA_PLATFORM
void msdc_set_smt(struct mmc_host *host, int smt)
{
	switch(host->id){
		case 0:
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 1), MSDC_CLK_SMT(MSDC0),      smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 1), MSDC_CMD_SMT(MSDC0),      smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 3), MSDC_DAT_SMT(MSDC0,DAT0), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 3), MSDC_DAT_SMT(MSDC0,DAT1), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 3), MSDC_DAT_SMT(MSDC0,DAT2), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 3), MSDC_DAT_SMT(MSDC0,DAT3), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 4), MSDC_DAT_SMT(MSDC0,DAT4), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 4), MSDC_DAT_SMT(MSDC0,DAT5), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 4), MSDC_DAT_SMT(MSDC0,DAT6), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 4), MSDC_DAT_SMT(MSDC0,DAT7), smt);
			break;
		case 1:
            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 1), MSDC_CLK_SMT(MSDC1),      smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 1), MSDC_CMD_SMT(MSDC1),      smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 3), MSDC_DAT_SMT(MSDC1,DAT0), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 3), MSDC_DAT_SMT(MSDC1,DAT1), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 3), MSDC_DAT_SMT(MSDC1,DAT2), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 3), MSDC_DAT_SMT(MSDC1,DAT3), smt);
			break;
		case 2:
            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 1), MSDC_CLK_SMT(MSDC2),      smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 1), MSDC_CMD_SMT(MSDC2),      smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 3), MSDC_DAT_SMT(MSDC2,DAT0), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 3), MSDC_DAT_SMT(MSDC2,DAT1), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 3), MSDC_DAT_SMT(MSDC2,DAT2), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 3), MSDC_DAT_SMT(MSDC2,DAT3), smt);
			break;
		case 3:
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 1), MSDC_CLK_SMT(MSDC3),      smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 1), MSDC_CMD_SMT(MSDC3),      smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 3), MSDC_DAT_SMT(MSDC3,DAT0), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 3), MSDC_DAT_SMT(MSDC3,DAT1), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 3), MSDC_DAT_SMT(MSDC3,DAT2), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 3), MSDC_DAT_SMT(MSDC3,DAT3), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 4), MSDC_DAT_SMT(MSDC3,DAT4), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 4), MSDC_DAT_SMT(MSDC3,DAT5), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 4), MSDC_DAT_SMT(MSDC3,DAT6), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 4), MSDC_DAT_SMT(MSDC3,DAT7), smt);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 5), MSDC_DSL_SMT(MSDC3),   smt);
            break;		
		default:
			break;
		}

}


void msdc_set_sr(struct mmc_host *host, int clk, int cmd, int dat, int rst, int ds)
{
	switch(host->id){
		case 0:
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 1), MSDC_CLK_SR(MSDC0),  clk);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 1), MSDC_CMD_SR(MSDC0),  cmd);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 2), MSDC_DAT_SR(MSDC0),  dat);
            //MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 5), MSDC_RSTB_SR(MSDC0), rst);
			break;
		case 1:
            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 1), MSDC_CLK_SR(MSDC1),  clk);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 1), MSDC_CMD_SR(MSDC1),  cmd);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 2), MSDC_DAT_SR(MSDC1),  dat);
			break;
		case 2:
            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 1), MSDC_CLK_SR(MSDC2),  clk);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 1), MSDC_CMD_SR(MSDC2),  cmd);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 2), MSDC_DAT_SR(MSDC2),  dat);
			break;
		case 3:
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 1), MSDC_CLK_SR(MSDC3),  clk);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 1), MSDC_CMD_SR(MSDC3),  cmd);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 2), MSDC_DAT_SR(MSDC3),  dat);
            //MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 5), MSDC_RSTB_SR(MSDC3), rst);
            //MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 5), MSDC_DSL_SR(MSDC3),  ds);
            break;		
		default:
			break;
		}

}

void msdc_set_rdtdsel(struct mmc_host *host, bool sd_18)
{
    switch(host->id){
        case 0: //always 1.8V
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 6), MSDC_PAD_TDSEL(MSDC0),0x0A);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 6), MSDC_PAD_RDSEL(MSDC0),0);
            break;
        case 1:
            if (sd_18){
                MSDC_SET_FIELD(MSDC_CTRL(MSDC1,5), MSDC_PAD_TDSEL(MSDC1),0x0A);
                MSDC_SET_FIELD(MSDC_CTRL(MSDC1,5), MSDC_PAD_RDSEL(MSDC1),0);
            }
			else{
                MSDC_SET_FIELD(MSDC_CTRL(MSDC1,5), MSDC_PAD_TDSEL(MSDC1),0x0A);
                MSDC_SET_FIELD(MSDC_CTRL(MSDC1,5), MSDC_PAD_RDSEL(MSDC1),0x0C);
            }
            break;
        case 2:
            if (sd_18){
                MSDC_SET_FIELD(MSDC_CTRL(MSDC2,5), MSDC_PAD_TDSEL(MSDC2),0x0A);
                MSDC_SET_FIELD(MSDC_CTRL(MSDC2,5), MSDC_PAD_RDSEL(MSDC2),0x0);
            }
			else{
                MSDC_SET_FIELD(MSDC_CTRL(MSDC2,5), MSDC_PAD_TDSEL(MSDC2),0x0);
                MSDC_SET_FIELD(MSDC_CTRL(MSDC2,5), MSDC_PAD_RDSEL(MSDC2),0x0C);
            }
            break;
        case 3: //always 1.8V
			MSDC_SET_FIELD(MSDC_CTRL(MSDC3,6), MSDC_PAD_TDSEL(MSDC3),0x0A);
			MSDC_SET_FIELD(MSDC_CTRL(MSDC3,6), MSDC_PAD_RDSEL(MSDC3),0x0);
            break;
        default:
            break;
    }
}

/* pull up means that host driver the line to HIGH
 * pull down means that host driver the line to LOW */
static void msdc_pin_set(struct mmc_host *host, MSDC_PIN_STATE mode)
{
    /* driver CLK/DAT pin */
	MSDC_ASSERT(host);
	MSDC_ASSERT(mode < MSDC_PST_MAX);

	switch(host->id){
		case 0:
            //MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 1), BIT_MSDC_CLK_R0R1PUPD,  mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 1), BIT_MSDC_CMD_R0R1PUPD,  mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 3), BIT_MSDC_DAT0_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 3), BIT_MSDC_DAT1_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 3), BIT_MSDC_DAT2_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 3), BIT_MSDC_DAT3_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 4), BIT_MSDC_DAT4_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 4), BIT_MSDC_DAT5_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 4), BIT_MSDC_DAT6_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 4), BIT_MSDC_DAT7_R0R1PUPD, mode);
			break;
		case 1:
           // MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 1), BIT_MSDC_CLK_R0R1PUPD,  mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 1), BIT_MSDC_CMD_R0R1PUPD,  mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 3), BIT_MSDC_DAT0_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 3), BIT_MSDC_DAT1_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 3), BIT_MSDC_DAT2_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 3), BIT_MSDC_DAT3_R0R1PUPD, mode); 
			break;
		case 2:
            //MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 1), BIT_MSDC_CLK_R0R1PUPD,  mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 1), BIT_MSDC_CMD_R0R1PUPD,  mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 3), BIT_MSDC_DAT0_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 3), BIT_MSDC_DAT1_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 3), BIT_MSDC_DAT2_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 3), BIT_MSDC_DAT3_R0R1PUPD, mode);
			break;
		case 3:
            //MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 1), BIT_MSDC_CLK_R0R1PUPD,  mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 1), BIT_MSDC_CMD_R0R1PUPD,  mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 3), BIT_MSDC_DAT0_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 3), BIT_MSDC_DAT1_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 3), BIT_MSDC_DAT2_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 3), BIT_MSDC_DAT3_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 4), BIT_MSDC_DAT4_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 4), BIT_MSDC_DAT5_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 4), BIT_MSDC_DAT6_R0R1PUPD, mode);
            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 4), BIT_MSDC_DAT7_R0R1PUPD, mode);
            //MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 5), BIT_MSDC_DSL_R0R1PUPD,  mode);
            break;		
		default:
			break;
		}
}

/* host can modify from 0-7 */
void msdc_set_driving(struct mmc_host *host, struct msdc_cust *msdc_cap, bool sd_18)
{
	u32 clkdrv, cmddrv, datdrv;
	
	MSDC_ASSERT(host);
	MSDC_ASSERT(msdc_cap);
	MSDC_ASSERT(host->id < MSDC_MAX_NUM);

	if(host && msdc_cap){
		clkdrv = (sd_18) ? msdc_cap->clk_18v_drv : msdc_cap->clk_drv;
		cmddrv = (sd_18) ? msdc_cap->cmd_18v_drv : msdc_cap->cmd_drv;
		datdrv = (sd_18) ? msdc_cap->dat_18v_drv : msdc_cap->dat_drv;

		switch(host->id){
			case 0:
	            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 0), BIT_MSDC_CLK_E8E4E2, clkdrv);
	            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 1), BIT_MSDC_CMD_E8E4E2, cmddrv);
	            MSDC_SET_FIELD(MSDC_CTRL(MSDC0, 2), BIT_MSDC_DAT_E8E4E2, datdrv);
				break;
			case 1:
	            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 0), BIT_MSDC_CLK_E8E4E2, clkdrv);
	            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 1), BIT_MSDC_CMD_E8E4E2, cmddrv);
	            MSDC_SET_FIELD(MSDC_CTRL(MSDC1, 2), BIT_MSDC_DAT_E8E4E2, datdrv);
				break;
			case 2:
	            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 0), BIT_MSDC_CLK_E8E4E2, clkdrv);
	            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 1), BIT_MSDC_CMD_E8E4E2, cmddrv);
	            MSDC_SET_FIELD(MSDC_CTRL(MSDC2, 2), BIT_MSDC_DAT_E8E4E2, datdrv);
				break;
			case 3:
	            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 0), BIT_MSDC_CLK_E8E4E2, clkdrv);
	            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 1), BIT_MSDC_CMD_E8E4E2, cmddrv);
	            MSDC_SET_FIELD(MSDC_CTRL(MSDC3, 2), BIT_MSDC_DAT_E8E4E2, datdrv);
	            break;		
			default:
				break;
			}
	}
}

void msdc_get_driving(struct mmc_host *host,struct msdc_cust *msdc_cap, bool sd_18)
{
	u32 clkdrv=0, cmddrv=0, datdrv=0;
	
	MSDC_ASSERT(host);
	MSDC_ASSERT(msdc_cap);
	MSDC_ASSERT(host->id < MSDC_MAX_NUM);

	if(host && msdc_cap){
		switch(host->id){
			case 0:
	            MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 0), BIT_MSDC_CLK_E8E4E2, clkdrv);
	            MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 1), BIT_MSDC_CMD_E8E4E2, cmddrv);
	            MSDC_GET_FIELD(MSDC_CTRL(MSDC0, 2), BIT_MSDC_DAT_E8E4E2, datdrv);
				break;
			case 1:
	            MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 0), BIT_MSDC_CLK_E8E4E2, clkdrv);
	            MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 1), BIT_MSDC_CMD_E8E4E2, cmddrv);
	            MSDC_GET_FIELD(MSDC_CTRL(MSDC1, 2), BIT_MSDC_DAT_E8E4E2, datdrv);
				break;
			case 2:
	            MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 0), BIT_MSDC_CLK_E8E4E2, clkdrv);
	            MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 1), BIT_MSDC_CMD_E8E4E2, cmddrv);
	            MSDC_GET_FIELD(MSDC_CTRL(MSDC2, 2), BIT_MSDC_DAT_E8E4E2, datdrv);
				break;
			case 3:
	            MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 0), BIT_MSDC_CLK_E8E4E2, clkdrv);
	            MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 1), BIT_MSDC_CMD_E8E4E2, cmddrv);
	            MSDC_GET_FIELD(MSDC_CTRL(MSDC3, 2), BIT_MSDC_DAT_E8E4E2, datdrv);
	            break;		
			default:
				break;
			}
		
		if (sd_18) {
			msdc_cap->clk_18v_drv = clkdrv;
			msdc_cap->cmd_18v_drv = cmddrv;
			msdc_cap->dat_18v_drv = datdrv;
		}
		else {		
			msdc_cap->clk_drv = clkdrv;
			msdc_cap->cmd_drv = cmddrv;
			msdc_cap->dat_drv = datdrv;
		}
	}

}
#if 0
/* pull up means that host driver the line to HIGH
 * pull down means that host driver the line to LOW */
static void msdc_pin_pud(struct mmc_host *host, int mode)
{
    /* driver CLK/DAT pin */
	switch(host->id){
		case 0:
            switch (mode) {
                case MSDC0_PULL_NONE:
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_R0_MASK, 0);
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_R1_MASK, 0);
                    //MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_PUPD_MASK, 0);
                    
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_R0_MASK, 0);
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_R1_MASK, 0);
                    //MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_PUPD_MASK, 0);
                    break;
                case MSDC0_PU_10K:
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_R0_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_R1_MASK, 0);
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_PUPD_MASK, 0);
                    
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_R0_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_R1_MASK, 0);
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_PUPD_MASK, 0);
                    
                    /* clock pull down with 50k during card init */
                    MSDC_SET_FIELD(MSDC0_GPIO_CLK_BASE, GPIO_R0_MASK, 0);
                    MSDC_SET_FIELD(MSDC0_GPIO_CLK_BASE, GPIO_R1_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_CLK_BASE, GPIO_PUPD_MASK, 1);
                    break;
                case MSDC0_PU_50K:
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_R0_MASK, 0);
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_R1_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_PUPD_MASK, 0);
                    
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_R0_MASK, 0);
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_R1_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_PUPD_MASK, 0);
                    break;
                case MSDC0_PU_8K:
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_R0_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_R1_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_PUPD_MASK, 0);
                    
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_R0_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_R1_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_PUPD_MASK, 0);
                    break;
                case MSDC0_PD_10K:
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_R0_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_R1_MASK, 0);
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_PUPD_MASK, 1);
                    
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_R0_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_R1_MASK, 0);
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_PUPD_MASK, 1);
                    break;
                case MSDC0_PD_50K:
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_R0_MASK, 0);
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_R1_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_PUPD_MASK, 1);
                    
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_R0_MASK, 0);
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_R1_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_PUPD_MASK, 1);
                    break;
                case MSDC0_PD_8K:
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_R0_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_R1_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_PUPD_MASK, 1);
                    
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_R0_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_R1_MASK, 1);
                    MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_PUPD_MASK, 1);
                    break;
                default:
                    /* error mode! */
                    MSDC_ERR_PRINT(MSDC_ERROR,("\t[msdc_pin_pud] host0 mode error(%d)!\n", mode));
                    break;
            }
			break;
		case 1:
            if (MSDC1_PU_50K == mode){
                MSDC_SET_FIELD(MSDC1_GPIO_CMD_BASE, GPIO_PU_MASK, 1);
                MSDC_SET_FIELD(MSDC1_GPIO_CMD_BASE, GPIO_PD_MASK, 0);
             
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT0_PU_MASK, 1);
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT0_PD_MASK, 0);

                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT1_PU_MASK, 1);
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT1_PD_MASK, 0);
                
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT2_PU_MASK, 1);
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT2_PD_MASK, 0);
                
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT3_PU_MASK, 1);
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT3_PD_MASK, 0);
            } else if (MSDC1_PD_50K == mode){
                MSDC_SET_FIELD(MSDC1_GPIO_CMD_BASE, GPIO_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC1_GPIO_CMD_BASE, GPIO_PD_MASK, 1);
             
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT0_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT0_PD_MASK, 1);

                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT1_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT1_PD_MASK, 1);

                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT2_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT2_PD_MASK, 1);

                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT3_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT3_PD_MASK, 1);
            } else if (MSDC1_PULL_NONE == mode){
                MSDC_SET_FIELD(MSDC1_GPIO_CMD_BASE, GPIO_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC1_GPIO_CMD_BASE, GPIO_PD_MASK, 0);
             
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT0_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT0_PD_MASK, 0);

                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT1_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT1_PD_MASK, 0);

                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT2_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT2_PD_MASK, 0);

                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT3_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT3_PD_MASK, 0);
            } else {
                /* error mode! */
                MSDC_ERR_PRINT(MSDC_ERROR,("\t[msdc_pin_pud] host1 mode error(%d)!\n", mode));
            }
			break;
		case 2:
            if (MSDC2_PU_50K == mode){
                MSDC_SET_FIELD(MSDC2_GPIO_CMD_BASE, GPIO_PU_MASK, 1);
                MSDC_SET_FIELD(MSDC2_GPIO_CMD_BASE, GPIO_PD_MASK, 0);
             
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT0_PU_MASK, 1);
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT0_PD_MASK, 0);

                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT1_PU_MASK, 1);
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT1_PD_MASK, 0);

                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT2_PU_MASK, 1);
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT2_PD_MASK, 0);

                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT3_PU_MASK, 1);
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT3_PD_MASK, 0);
            } else if (MSDC2_PD_50K == mode){
                MSDC_SET_FIELD(MSDC2_GPIO_CMD_BASE, GPIO_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC2_GPIO_CMD_BASE, GPIO_PD_MASK, 1);
             
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT0_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT0_PD_MASK, 1);

                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT1_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT1_PD_MASK, 1);
                
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT2_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT2_PD_MASK, 1);
                
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT3_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT3_PD_MASK, 1);
            } else if (MSDC2_PULL_NONE == mode){
                MSDC_SET_FIELD(MSDC2_GPIO_CMD_BASE, GPIO_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC2_GPIO_CMD_BASE, GPIO_PD_MASK, 0);
                
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT0_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT0_PD_MASK, 0);
                
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT1_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT1_PD_MASK, 0);
                
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT2_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT2_PD_MASK, 0);
                
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT3_PU_MASK, 0);
                MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT3_PD_MASK, 0);
            } else {
                /* error mode! */
                MSDC_ERR_PRINT(MSDC_ERROR,("\t[msdc_pin_pud] host2 mode error(%d)!\n", mode));
            }
			break;
		default:
			break;
		}
}

static void msdc_pin_pnul(struct mmc_host *host, int mode)
{
	switch(host->id){
		case 1:
            MSDC_SET_FIELD(MSDC1_GPIO_CMD_BASE, GPIO_PU_MASK, 0);
            MSDC_SET_FIELD(MSDC1_GPIO_CMD_BASE, GPIO_PD_MASK, 0);

            MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT0_PU_MASK, 0);
            MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT0_PD_MASK, 0);

            MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT1_PU_MASK, 0);
            MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT1_PD_MASK, 0);

            MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT2_PU_MASK, 0);
            MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT2_PD_MASK, 0);

            MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT3_PU_MASK, 0);
            MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_DAT3_PD_MASK, 0);
			break;
		case 2:
            MSDC_SET_FIELD(MSDC2_GPIO_CMD_BASE, GPIO_PU_MASK, 0);
            MSDC_SET_FIELD(MSDC2_GPIO_CMD_BASE, GPIO_PD_MASK, 0);

            MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT0_PU_MASK, 0);
            MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT0_PD_MASK, 0);

            MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT1_PU_MASK, 0);
            MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT1_PD_MASK, 0);

            MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT2_PU_MASK, 0);
            MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT2_PD_MASK, 0);

            MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT3_PU_MASK, 0);
            MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_DAT3_PD_MASK, 0);
			break;
		default:
			break;
		}
}

static void msdc_set_smt(struct mmc_host *host,int set_smt)
{
	switch(host->id){
		case 0:
			MSDC_SET_FIELD(MSDC0_GPIO_CLK_BASE, GPIO_SMT_MASK, set_smt);
			MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_SMT_MASK, set_smt);
			MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_SMT_MASK, set_smt); 
			break;
		case 1:
			MSDC_SET_FIELD(MSDC1_GPIO_CLK_BASE, GPIO_SMT_MASK, set_smt);
			MSDC_SET_FIELD(MSDC1_GPIO_CMD_BASE, GPIO_SMT_MASK, set_smt);
			MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_SMT_MASK, set_smt); 
			break;
		case 2:
			MSDC_SET_FIELD(MSDC2_GPIO_CLK_BASE, GPIO_SMT_MASK, set_smt);
			MSDC_SET_FIELD(MSDC2_GPIO_CMD_BASE, GPIO_SMT_MASK, set_smt);
			MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_SMT_MASK, set_smt); 
			break;
		default:
			break;
	}
}

/* host can modify from 0-7 */
static void msdc_set_driving(struct mmc_host *host,u8 clkdrv, u8 cmddrv, u8 datdrv)
{
	switch(host->id){
		case 0:
			MSDC_SET_FIELD(MSDC0_GPIO_CLK_BASE, GPIO_MSDC0_DRVN, clkdrv);
			MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_MSDC0_DRVN, cmddrv);
			MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_MSDC0_DRVN, datdrv);
			break;
		case 1:
			MSDC_SET_FIELD(MSDC1_GPIO_CLK_BASE, GPIO_MSDC1_MSDC2_DRVN, clkdrv);
			MSDC_SET_FIELD(MSDC1_GPIO_CMD_BASE, GPIO_MSDC1_MSDC2_DRVN, cmddrv);
			MSDC_SET_FIELD(MSDC1_GPIO_DAT_BASE, GPIO_MSDC1_MSDC2_DRVN, datdrv);
			break;
		case 2:
			MSDC_SET_FIELD(MSDC2_GPIO_CLK_BASE, GPIO_MSDC1_MSDC2_DRVN, clkdrv);
			MSDC_SET_FIELD(MSDC2_GPIO_CMD_BASE, GPIO_MSDC1_MSDC2_DRVN, cmddrv);
			MSDC_SET_FIELD(MSDC2_GPIO_DAT_BASE, GPIO_MSDC1_MSDC2_DRVN, datdrv);
			break;
		default:
            MSDC_ERR_PRINT(MSDC_ERROR,("error...msdc_set_driving out of range!!\n"));
			break;
		}
}
#endif
#endif /* end of FPGA_PLATFORM */

#if MSDC_USE_DMA_MODE
void msdc_flush_membuf(void *buf, u32 len)
{
    //cache_clean_invalidate();
    //apmcu_clean_invalidate_dcache();
    //apmcu_clean_invalidate_dcache_ARM11();
}

u8 msdc_cal_checksum(u8 *buf, u32 len)
{
    u32 i, sum = 0;
    for (i = 0; i < len; i++) {
        sum += buf[i];
    }
    return 0xFF - (u8)sum;
}

/* allocate gpd link-list from gpd_pool */
gpd_t *msdc_alloc_gpd(struct mmc_host *host, int num)
{
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;
    gpd_t *gpd, *ptr, *prev;

    if (priv->alloc_gpd + num + 1 > MAX_GPD_POOL_SZ || num == 0)
        return NULL;

    gpd = priv->gpd_pool + priv->alloc_gpd;
    priv->alloc_gpd += (num + 1); /* include null gpd */

    memset(gpd, 0, sizeof(gpd_t) * (num + 1));

    ptr = gpd + num - 1;
    ptr->next = (void*)(gpd + num); /* pointer to null gpd */
    
    /* create link-list */
    if (ptr != gpd) {
        do {
            prev = ptr - 1;
            prev->next = ptr;
            ptr = prev;
        } while (ptr != gpd);
    }
	
	DMAGPDPTR_DBGCHK(gpd, 16);

    return gpd;
}

/* allocate bd link-list from bd_pool */
__bd_t *msdc_alloc_bd(struct mmc_host *host, int num)
{
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;
    __bd_t *bd, *ptr, *prev;

    if (priv->alloc_bd + num > MAX_BD_POOL_SZ || num == 0)
        return NULL;
    
    bd = priv->bd_pool + priv->alloc_bd;
    priv->alloc_bd += num;

    memset(bd, 0, sizeof(__bd_t) * num);

    ptr = bd + num - 1;
    ptr->eol  = 1;
    ptr->next = 0;

    /* create link-list */
    if (ptr != bd) {
        do {
            prev = ptr - 1;
            prev->next = ptr;
            prev->eol  = 0;
            ptr = prev;
        } while (ptr != bd);
    }
	
	DMABDPTR_DBGCHK(bd, 16);


    return bd;
}

/* queue bd link-list to one gpd */
void msdc_queue_bd(struct mmc_host *host, gpd_t *gpd, __bd_t *bd)
{
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;

    MSDC_ASSERT((gpd && (gpd->ptr == NULL)));
	DMAGPDPTR_DBGCHK(gpd, 16);
	DMABDPTR_DBGCHK(bd, 16);

    gpd->hwo = 1;
    gpd->bdp = 1;
    gpd->ptr = (void*)bd;

    if ((priv->cfg.flags & DMA_FLAG_EN_CHKSUM) == 0) 
        return;

    /* calculate and fill bd checksum */
    while (bd) {
        bd->chksum = msdc_cal_checksum((u8*)bd, 16);
        bd = bd->next;
    }
}

/* queue data buf to one gpd */
void msdc_queue_buf(struct mmc_host *host, gpd_t *gpd, u8 *buf)
{
    MSDC_ASSERT(gpd && gpd->ptr==NULL);
	DMAGPDPTR_DBGCHK(gpd, 16);

    gpd->hwo = 1;
    gpd->bdp = 0;
    gpd->ptr = (void*)buf;
}

/* add gpd link-list to active list */
void msdc_add_gpd(struct mmc_host *host, gpd_t *gpd, int num)
{
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;

    if (num > 0) {
        if (!priv->active_head) {
            priv->active_head = gpd;
        } else {
            priv->active_tail->next = gpd;
        }
        priv->active_tail = gpd + num - 1;

        if ((priv->cfg.flags & DMA_FLAG_EN_CHKSUM) == 0)
            return;

		DMAGPDPTR_DBGCHK(gpd, 16);
        /* calculate and fill gpd checksum */
        while (gpd) {
            gpd->chksum = msdc_cal_checksum((u8 *)gpd, 16);
            gpd = gpd->next;
        }
    }
}

void msdc_reset_gpd(struct mmc_host *host)
{
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;

    priv->alloc_bd  = 0;
    priv->alloc_gpd = 0;
    priv->active_head = NULL;
    priv->active_tail = NULL;
}
#endif

#ifndef FPGA_PLATFORM /* don't power on/off device and use power-on default volt */
void msdc_set_host_level_pwr(struct mmc_host *host, int level)
{
    unsigned int ret = 0;
    
    if (level) {
		host->cur_pwr = VOL_1800;
        
        #ifdef PMIC_SUPPORT
        ret = pmic_config_interface(0xA7,0x2,0x7,4); /* VMC=1.8V */
        #endif
    }
	else {
		host->cur_pwr = VOL_3300;
        #ifdef PMIC_SUPPORT
        ret = pmic_config_interface(0xA7,0x7,0x7,4); /* VMC=3.3V */
        #endif
    }
	
    if (ret != 0) {
            MSDC_TRC_PRINT(MSDC_PMIC,("PMIC: Set MSDC Vol Level Fail\n"));
    }
	else {
        mdelay(100); /* requires before voltage stable */
    }
    //mdelay(100);
}

#if 0
void msdc_set_card_pwr(struct mmc_host *host, int on)
{
    unsigned int ret;
    
    ret = pmic_config_interface(0xAB,0x7,0x7,4); /* VMCH=3.3V */
	host->cur_pwr = VOL_3300;

    if (ret == 0) {
		
        if(on) {            
            ret = pmic_config_interface(0xAB,0x1,0x1,0); /* VMCH_EN=1 */
        }
		else {
            ret = pmic_config_interface(0xAB,0x0,0x1,0); /* VMCH_EN=0 */
        }
    }
	
    if (ret != 0) {
         MSDC_TRC_PRINT(MSDC_PMIC,("PMIC: Set MSDC Host Power Fail\n"));
    }
	else {
         mdelay(3);
    }
}
void msdc_set_host_pwr(struct mmc_host *host, int on)
{
    unsigned int ret;

    ret = pmic_config_interface(0xA7,0x7,0x7,4); /* VMC=3.3V */

    if (ret == 0) {
        if(on) {
            ret = pmic_config_interface(0xA7,0x1,0x1,0); /* VMC_EN=1 */
        } else {
            ret = pmic_config_interface(0xA7,0x0,0x1,0); /* VMC_EN=0 */
        }
    }
    
    if (ret != 0) {
        MSDC_TRC_PRINT(MSDC_PMIC,("PMIC: Set MSDC Card Power Fail\n"));
    }
}
#else
void msdc_set_card_pwr(struct mmc_host *host, int on){}
void msdc_set_host_pwr(struct mmc_host *host, int on){}
#endif

#else
#define PWR_GPIO            (0x10001E84)
#define PWR_GPIO_EO         (0x10001E88)

#define PWR_MASK_EN         (0x1 << 8)
#define PWR_MASK_VOL_18     (0x1 << 9)
#define PWR_MASK_VOL_33     (0x1 << 10)
#define PWR_MASK_L4         (0x1 << 11)
#define PWR_MSDC_DIR        (PWR_MASK_EN | PWR_MASK_VOL_18 | PWR_MASK_VOL_33 | PWR_MASK_L4)

#define MSDC0_PWR_MASK_EN         (0x1 << 12)
#define MSDC0_PWR_MASK_VOL_18     (0x1 << 13)
#define MSDC0_PWR_MASK_VOL_33     (0x1 << 14)
#define MSDC0_PWR_MASK_L4         (0x1 << 15)
#define MSDC0_PWR_MSDC_DIR        (MSDC0_PWR_MASK_EN | MSDC0_PWR_MASK_VOL_18 | MSDC0_PWR_MASK_VOL_33 | MSDC0_PWR_MASK_L4)

static void msdc_clr_gpio(u32 bits)
{
    u32 l_val = 0;

    switch (bits){
        case MSDC0_PWR_MASK_EN:
                /* check for set before */
                MSDC_TRC_PRINT(MSDC_PMIC,("clr card pwr:\n"));
                MSDC_SET_FIELD(PWR_GPIO,    (MSDC0_PWR_MASK_EN),0);
                MSDC_SET_FIELD(PWR_GPIO_EO, (MSDC0_PWR_MASK_EN),0);
                break;
        case MSDC0_PWR_MASK_VOL_18:
                /* check for set before */
                MSDC_TRC_PRINT(MSDC_PMIC,("clr card 1.8v pwr:\n"));
                MSDC_SET_FIELD(PWR_GPIO,    (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 0);
                MSDC_SET_FIELD(PWR_GPIO_EO, (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 0);
                break;
        case MSDC0_PWR_MASK_VOL_33:
	        /* check for set before */
		MSDC_TRC_PRINT(MSDC_PMIC,("clr card 3.3v pwr:\n"));
                MSDC_SET_FIELD(PWR_GPIO,    (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 0);
                MSDC_SET_FIELD(PWR_GPIO_EO, (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 0);
                break;
        case MSDC0_PWR_MASK_L4:
		/* check for set before */
		MSDC_TRC_PRINT(MSDC_PMIC,("set l4 dir:\n"));
                MSDC_SET_FIELD(PWR_GPIO,    MSDC0_PWR_MASK_L4, 0);
                MSDC_SET_FIELD(PWR_GPIO_EO, MSDC0_PWR_MASK_L4, 0);
                break;
        case PWR_MASK_EN:
                /* check for set before */
                MSDC_TRC_PRINT(MSDC_PMIC,("clr card pwr:\n"));
                MSDC_SET_FIELD(PWR_GPIO,    PWR_MASK_EN,0);
				MSDC_SET_FIELD(PWR_GPIO_EO, PWR_MASK_EN,0);
                break;
        case PWR_MASK_VOL_18:
                /* check for set before */
                MSDC_TRC_PRINT(MSDC_PMIC,("clr card 1.8v pwr:\n"));
                MSDC_SET_FIELD(PWR_GPIO,    (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 0);
                MSDC_SET_FIELD(PWR_GPIO_EO, (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 0);
                break;
        case PWR_MASK_VOL_33:
			    /* check for set before */
			    MSDC_TRC_PRINT(MSDC_PMIC,("clr card 3.3v pwr:\n"));
                MSDC_SET_FIELD(PWR_GPIO,    (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 0);
                MSDC_SET_FIELD(PWR_GPIO_EO, (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 0);
                break;
        case PWR_MASK_L4:
			    /* check for set before */
			    MSDC_TRC_PRINT(MSDC_PMIC,("set l4 dir:\n"));
                MSDC_SET_FIELD(PWR_GPIO,    PWR_MASK_L4, 0);
                MSDC_SET_FIELD(PWR_GPIO_EO, PWR_MASK_L4, 0);
                break;
        default:
            MSDC_ERR_PRINT(MSDC_ERROR,("[%s:%d]invalid value: 0x%x\n", __FILE__, __func__, bits));
            break;
    }

#ifdef ___MSDC_DEBUG___
    MSDC_TRC_PRINT(MSDC_PMIC,("[clr]PWR_GPIO[8-16]:0x%x\n", MSDC_READ32(PWR_GPIO)));
    MSDC_TRC_PRINT(MSDC_PMIC,("[clr]GPIO_DIR[8-16]:0x%x\n", MSDC_READ32(PWR_GPIO_EO)));
#endif
}

static void msdc_set_gpio(u32 bits)
{
    u32 l_val = 0;

    switch (bits){
        case MSDC0_PWR_MASK_EN:
                /* check for set before */
                MSDC_TRC_PRINT(MSDC_PMIC,("set card pwr:\n"));
                MSDC_SET_FIELD(PWR_GPIO,    MSDC0_PWR_MASK_EN,1);
                MSDC_SET_FIELD(PWR_GPIO_EO, MSDC0_PWR_MASK_EN,1);
                break;
        case MSDC0_PWR_MASK_VOL_18:
                /* check for set before */
                MSDC_TRC_PRINT(MSDC_PMIC,("set card 1.8v pwr:\n"));
                MSDC_SET_FIELD(PWR_GPIO,    (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 1);
                MSDC_SET_FIELD(PWR_GPIO_EO, (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 1);
                break;
        case MSDC0_PWR_MASK_VOL_33:
		/* check for set before */
                MSDC_TRC_PRINT(MSDC_PMIC,("set card 3.3v pwr:\n"));
                MSDC_SET_FIELD(PWR_GPIO,    (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 2);
                MSDC_SET_FIELD(PWR_GPIO_EO, (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 2);
                break;
        case MSDC0_PWR_MASK_L4:
		/* check for set before */
                MSDC_TRC_PRINT(MSDC_PMIC,("set l4 dir:\n"));
                MSDC_SET_FIELD(PWR_GPIO,    MSDC0_PWR_MASK_L4, 1);
                MSDC_SET_FIELD(PWR_GPIO_EO, MSDC0_PWR_MASK_L4, 1);
                break;
        case PWR_MASK_EN:
                /* check for set before */
                MSDC_TRC_PRINT(MSDC_PMIC,("set card pwr:\n"));
                MSDC_SET_FIELD(PWR_GPIO,    PWR_MASK_EN,1);
				MSDC_SET_FIELD(PWR_GPIO_EO, PWR_MASK_EN,1);
                break;
        case PWR_MASK_VOL_18:
                /* check for set before */
                MSDC_TRC_PRINT(MSDC_PMIC,("set card 1.8v pwr:\n"));
                MSDC_SET_FIELD(PWR_GPIO,    (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 1);
                MSDC_SET_FIELD(PWR_GPIO_EO, (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 1);
                break;
        case PWR_MASK_VOL_33:
			    /* check for set before */
			    MSDC_TRC_PRINT(MSDC_PMIC,("set card 3.3v pwr:\n"));
                MSDC_SET_FIELD(PWR_GPIO,    (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 2);
                MSDC_SET_FIELD(PWR_GPIO_EO, (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 2);
                break;
        case PWR_MASK_L4:
			    /* check for set before */
			    MSDC_TRC_PRINT(MSDC_PMIC,("set l4 dir:\n"));
                MSDC_SET_FIELD(PWR_GPIO,    PWR_MASK_L4, 1);
                MSDC_SET_FIELD(PWR_GPIO_EO, PWR_MASK_L4, 1);
                break;
        default:
                MSDC_ERR_PRINT(MSDC_ERROR,("[%s:%s]invalid value: 0x%x\n", __FILE__, __func__, bits));
            break;
    }

#ifdef ___MSDC_DEBUG___
		MSDC_TRC_PRINT(MSDC_PMIC,("[clr]PWR_GPIO[8-16]:0x%x\n", MSDC_READ32(PWR_GPIO)));
		MSDC_TRC_PRINT(MSDC_PMIC,("[clr]GPIO_DIR[8-16]:0x%x\n", MSDC_READ32(PWR_GPIO_EO)));
#endif

}

void msdc_set_card_pwr(struct mmc_host *host, int on)
{

    if (on){
		switch(host->id){
		   case 0:
#ifndef FPGA_SDCARD_TRIALRUN		   	
			   msdc_set_gpio(MSDC0_PWR_MASK_VOL_18);
			   msdc_set_gpio(MSDC0_PWR_MASK_L4);
			   msdc_set_gpio(MSDC0_PWR_MASK_EN);
			   break;
#endif			   
		   case 1:
		   case 2:
			   msdc_set_gpio(PWR_MASK_VOL_33);
			   msdc_set_gpio(PWR_MASK_L4);
			   msdc_set_gpio(PWR_MASK_EN);
			   break;
		   case 3:
			   msdc_set_gpio(PWR_MASK_VOL_18);
			   msdc_set_gpio(PWR_MASK_L4);
			   msdc_set_gpio(PWR_MASK_EN);
		   	   break;
		   default:
		   	   MSDC_ERR_PRINT(MSDC_ERROR,("[%s:%s] power on, invalid host id: %d\n", __FILE__, __func__, host->id));
		   	   break;
		}
    }
    else {
		switch(host->id){
		   case 0:
			   msdc_clr_gpio(MSDC0_PWR_MASK_EN);
			   msdc_clr_gpio(MSDC0_PWR_MASK_VOL_18);
			   msdc_clr_gpio(MSDC0_PWR_MASK_L4);
			   break;
		   case 1:
		   case 2:
			   msdc_clr_gpio(PWR_MASK_EN);
			   msdc_clr_gpio(PWR_MASK_VOL_33);
			   msdc_clr_gpio(PWR_MASK_L4);
			   break;
		   case 3:
			   msdc_clr_gpio(PWR_MASK_EN);
			   msdc_clr_gpio(PWR_MASK_VOL_18);
			   msdc_clr_gpio(PWR_MASK_L4);
		   	   break;
		   default:
		   	   MSDC_ERR_PRINT(MSDC_ERROR,("[%s:%s] power off, invalid host id: %d\n", __FILE__, __func__, host->id));
		   	   break;
		}
    }
    mdelay(10);
}

void msdc_set_host_level_pwr(struct mmc_host *host, u32 level)
{
    msdc_clr_gpio(PWR_MASK_VOL_18);
    msdc_clr_gpio(PWR_MASK_VOL_33);

    if (level){
       msdc_set_gpio(PWR_MASK_VOL_18);
    }
	else{
       msdc_set_gpio(PWR_MASK_VOL_33);
	}
    msdc_set_gpio(PWR_MASK_L4);
}
void msdc_set_host_pwr(struct mmc_host *host, int on)
{
    msdc_set_host_level_pwr(host, 0);
}
#endif

void msdc_set_startbit(struct mmc_host *host, u8 start_bit)
{	
    u32 base = host->base;
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;

    /* set start bit */
    MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_START_BIT, start_bit);
    priv->start_bit = start_bit;
	MSDC_TRC_PRINT(MSDC_INFO,("start bit = %d, MSDC_CFG[0x%x]\n", start_bit, MSDC_READ32(MSDC_CFG))); 
}

#if 0
void msdc_set_smpl(struct mmc_host *host, u8 dsmpl, u8 rsmpl)
{
    u32 base = host->base;
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;

    /* set sampling edge */
    MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, rsmpl);
    MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DSPL, dsmpl);

    /* wait clock stable */
    while (!(MSDC_READ32(MSDC_CFG) & MSDC_CFG_CKSTB));

    priv->rsmpl = rsmpl;
    priv->rdsmpl = dsmpl;    
}
#else
#define TYPE_CMD_RESP_EDGE      (0)
#define TYPE_WRITE_CRC_EDGE     (1)
#define TYPE_READ_DATA_EDGE     (2)
#define TYPE_WRITE_DATA_EDGE    (3)

void msdc_set_smpl(struct mmc_host *host, u8 HS400, u8 mode, u8 type)
{
    u32 base = host->base;
    int i=0;
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;

    static u8 read_data_edge[8] = {MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING,
                                     MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING};
    static u8 write_data_edge[4] = {MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING};

    switch (type)
    {
        case TYPE_CMD_RESP_EDGE:
            if (HS400) {
                // eMMC5.0 only output resp at CLK pin, so no need to select DS pin
                MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_PADCMD_LATCHCK, 0); //latch cmd resp at CLK pin
                MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CMD_RESP_SEL, 0);//latch cmd resp at CLK pin
            }

            if (mode == MSDC_SMPL_RISING || mode == MSDC_SMPL_FALLING) {
              #if 0 // HS400 tune MSDC_EMMC50_CFG_CMDEDGE_SEL use DS latch, but now no DS latch
                if (HS400) {
                    MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CMDEDGE_SEL, mode);
                }
                else {
                    MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, mode);
                }
              #else
                MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, mode);
              #endif
                priv->rsmpl = mode;
            }
            else {
                MSDC_ERR_PRINT(MSDC_ERROR,("[%s]: SD%d invalid resp parameter: HS400=%d, type=%d, mode=%d\n", __func__, host->id, HS400, type, mode));
            }
            break;

        case TYPE_WRITE_CRC_EDGE:
            if (HS400) {
                MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_SEL, 1);//latch write crc status at DS pin
            }
            else {
                MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_SEL, 0);//latch write crc status at CLK pin
            }

            if (mode == MSDC_SMPL_RISING || mode == MSDC_SMPL_FALLING) {
                if (HS400) {
                    MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_EDGE, mode);
                }
                else {
                    MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, 0);
                    MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, mode);
                }
                priv->wdsmpl = mode;
            }
            else if (mode == MSDC_SMPL_SEPERATE && !HS400) {
                MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D0SPL, write_data_edge[0]); //only dat0 is for write crc status.
                priv->wdsmpl = mode;
            }
            else {
                MSDC_ERR_PRINT(MSDC_ERROR,("[%s]: SD%d invalid crc parameter: HS400=%d, type=%d, mode=%d\n", __func__, host->id, HS400, type, mode));
            }
            break;

        case TYPE_READ_DATA_EDGE:
            if (HS400) {
                msdc_set_startbit(host, START_AT_RISING_AND_FALLING); //for HS400, start bit is output both on rising and falling edge
                priv->start_bit = START_AT_RISING_AND_FALLING;
            }
            else {
                msdc_set_startbit(host, START_AT_RISING); //for the other mode, start bit is only output on rising edge. but DDR50 can try falling edge if error casued by pad delay
                priv->start_bit = START_AT_RISING;
            }
            if (mode == MSDC_SMPL_RISING || mode == MSDC_SMPL_FALLING) {
                MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL, 0);
                MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, mode);
                priv->rdsmpl = mode;
            }
            else if (mode == MSDC_SMPL_SEPERATE) {
                MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL, 1);
                for(i=0; i<8; i++);
                {
                    MSDC_SET_FIELD(MSDC_IOCON, (MSDC_IOCON_R_D0SPL << i), read_data_edge[i]);
                }
                priv->rdsmpl = mode;
            }
            else {
                MSDC_ERR_PRINT(MSDC_ERROR,("[%s]: SD%d invalid read parameter: HS400=%d, type=%d, mode=%d\n", __func__, host->id, HS400, type, mode));
            }
            break;

        case TYPE_WRITE_DATA_EDGE:
            MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_SEL, 0);//latch write crc status at CLK pin

            if (mode == MSDC_SMPL_RISING|| mode == MSDC_SMPL_FALLING) {
                MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, 0);
                MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, mode);
                priv->wdsmpl = mode;
            }
            else if (mode == MSDC_SMPL_SEPERATE) {
                MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, 1);
                for(i=0; i<4; i++);
                {
                    MSDC_SET_FIELD(MSDC_IOCON, (MSDC_IOCON_W_D0SPL << i), write_data_edge[i]);//dat0~4 is for SDIO card.
                }
                priv->wdsmpl = mode;
            } else {
                MSDC_ERR_PRINT(MSDC_ERROR,("[%s]: SD%d invalid write parameter: HS400=%d, type=%d, mode=%d\n", __func__, host->id, HS400, type, mode));
            }
            break;

        default:
            MSDC_ERR_PRINT(MSDC_ERROR,("[%s]: SD%d invalid parameter: HS400=%d, type=%d, mode=%d\n", __func__, host->id, HS400, type, mode));
            break;
    }
}

#endif

#if 0
static u32 msdc_cal_timeout(struct mmc_host *host, u32 ns, u32 clks, u32 clkunit)
{
    u32 timeout, clk_ns;

    clk_ns  = 1000000000UL / host->sclk;
    timeout = ns / clk_ns + clks;
    timeout = timeout / clkunit;
    return timeout;
}
#endif

void msdc_set_timeout(struct mmc_host *host, u32 ns, u32 clks)
{
    u32 base = host->base;
    u32 timeout, clk_ns;

    if (host->sclk == 0) {
        timeout = 0;
    }
	else {
	    clk_ns  = 1000000000UL / host->sclk;
	    timeout = ns / clk_ns + clks;
	    timeout = timeout >> 20; /* in 1048576 sclk cycle unit (MT6589 MSDC IP)*/
	    timeout = timeout > 1 ? timeout - 1 : 0;
	    timeout = timeout > 255 ? 255 : timeout;

	    MSDC_SET_FIELD(SDC_CFG, SDC_CFG_DTOC, timeout);
	}
    MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d] Set read data timeout: %dns %dclks -> %d (65536 sclk cycles)\n",
        																host->id, ns, clks, timeout + 1));
}

void msdc_clr_fifo(struct mmc_host *host)
{
    u32 base = host->base;
    MSDC_CLR_FIFO();
}

void msdc_set_blklen(struct mmc_host *host, u32 blklen)
{
    //u32 base = host->base;
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;

    host->blklen     = blklen;
    priv->cfg.blklen = blklen;
    msdc_clr_fifo(host);
}

void msdc_set_blknum(struct mmc_host *host, u32 blknum)
{
    u32 base = host->base;

    MSDC_WRITE32(SDC_BLK_NUM, blknum);
}

#if MSDC_USE_DMA_MODE
void msdc_set_dma(struct mmc_host *host, u8 burstsz, u32 flags)
{
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;
    struct dma_config *cfg = &priv->cfg;

    cfg->burstsz = burstsz;
    cfg->flags   = flags;
}

void msdc_set_autocmd(struct mmc_host *host, int cmd, int on)
{
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;

    if (on) {
        priv->autocmd |= cmd;
    } else {
        priv->autocmd &= ~cmd;
    }
}
#endif

void msdc_reset(struct mmc_host *host)
{
    u32 base = host->base;
    MSDC_RESET();
}

void msdc_abort(struct mmc_host *host)
{
    u32 base = host->base;

    MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d] Abort: MSDC_FIFOCS=%xh MSDC_PS=%xh SDC_STS=%xh\n", 
        host->id, MSDC_READ32(MSDC_FIFOCS), MSDC_READ32(MSDC_PS), MSDC_READ32(SDC_STS)));

    /* reset controller */
    msdc_reset(host);

    /* clear fifo */
    msdc_clr_fifo(host);

    /* make sure txfifo and rxfifo are empty */
    if (MSDC_TXFIFOCNT() != 0 || MSDC_RXFIFOCNT() != 0) {
        MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d] Abort: TXFIFO(%d), RXFIFO(%d) != 0\n",
            host->id, MSDC_TXFIFOCNT(), MSDC_RXFIFOCNT()));
    }

    /* clear all interrupts */
    MSDC_WRITE32(MSDC_INT, MSDC_READ32(MSDC_INT));
}

int msdc_cmd(struct mmc_host *host, struct mmc_command *cmd);
static int msdc_get_card_status(struct mmc_host *host, u32 *status)
{
	int err;
    struct mmc_command cmd;

    cmd.opcode  = MMC_CMD_SEND_STATUS;
    cmd.arg     = host->card->rca << 16;
    cmd.rsptyp  = RESP_R1;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    err = msdc_cmd(host, &cmd);

    if (err == MMC_ERR_NONE) {
        *status = cmd.resp[0];
    }
    return err;
}
int msdc_send_cmd(struct mmc_host *host, struct mmc_command *cmd);
int msdc_wait_rsp(struct mmc_host *host, struct mmc_command *cmd);
int msdc_abort_handler(struct mmc_host *host, int abort_card)
{
    //u32 base = host->base;
    struct mmc_command stop;
	u32 status = 0;    
	u32 state = 0; 
    
	while (state != 4) { // until status to "tran"
			msdc_abort(host);
			if (msdc_get_card_status(host, &status)) {
				MSDC_DBG_PRINT(MSDC_DEBUG,("Get card status failed\n"));	
				return 1;
			}	
			state = R1_CURRENT_STATE(status);
			MSDC_DBG_PRINT(MSDC_STACK,("check card state<%d>\n", state));
			if (state == 5 || state == 6) {
				MSDC_DBG_PRINT(MSDC_STACK,("state<%d> need cmd12 to stop\n", state)); 
				if (abort_card) {
        			stop.opcode  = MMC_CMD_STOP_TRANSMISSION;
       			 	stop.rsptyp  = RESP_R1B;
        			stop.arg     = 0;
        			stop.retries = CMD_RETRIES;
        			stop.timeout = CMD_TIMEOUT;
        			msdc_send_cmd(host, &stop);
        			msdc_wait_rsp(host, &stop); // don't tuning
			} else if (state == 7) {  // busy in programing 		
				MSDC_DBG_PRINT(MSDC_STACK,("state<%d> card is busy\n", state));	
				mdelay(100);
			} else if (state != 4) {
				MSDC_DBG_PRINT(MSDC_STACK,("state<%d> ??? \n", state));
				return 1;		
			}
		}
	}
	msdc_abort(host);
	return 0;
}

u32 msdc_intr_wait(struct mmc_host *host, u32 intrs)
{
    u32 base = host->base;
    u32 sts;

    /* warning that interrupts are not enabled */
    //MSDC_BUGON((MSDC_READ32(MSDC_INTEN) & intrs) != intrs);
    while (((sts = MSDC_READ32(MSDC_INT)) & intrs) == 0);
    
    MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d] INT(0x%x)\n", host->id, sts));
    MSDC_WRITE32(MSDC_INT, (sts & intrs));
    if (~intrs & sts) {
        MSDC_DBG_PRINT(MSDC_WARN,("[SD%d]<CHECKME> Unexpected INT(0x%x)\n", 
            host->id, ~intrs & sts));
    }
    return sts;
}

void msdc_intr_unmask(struct mmc_host *host, u32 bits)
{
    u32 base = host->base;
    u32 val;

    val  = MSDC_READ32(MSDC_INTEN);
    val |= bits;
    MSDC_WRITE32(MSDC_INTEN, val);    
}

void msdc_intr_mask(struct mmc_host *host, u32 bits)
{
    u32 base = host->base;
    u32 val;

    val  = MSDC_READ32(MSDC_INTEN);
    val &= ~bits;
    MSDC_WRITE32(MSDC_INTEN, val);
}

#ifdef FEATURE_MMC_SDIO
void msdc_intr_sdio(struct mmc_host *host, int enable)
{
    u32 base = host->base;

    MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d] %s SDIO INT\n", host->id, enable ? "Enable" : "Disable"));

    if (enable) {
        MSDC_SET_BIT32(SDC_CFG, SDC_CFG_SDIOIDE|SDC_CFG_SDIOINTWKUP);
        msdc_intr_unmask(host, MSDC_INT_SDIOIRQ);
    } else {
        msdc_intr_mask(host, MSDC_INT_SDIOIRQ);
        MSDC_CLR_BIT32(SDC_CFG, SDC_CFG_SDIOIDE|SDC_CFG_SDIOINTWKUP);
    }
}

void msdc_intr_sdio_gap(struct mmc_host *host, int enable)
{
    u32 base = host->base;

    MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d] %s SDIO GAP\n", host->id, enable ? "Enable" : "Disable"));

    if (enable) {
        MSDC_SET_BIT32(SDC_CFG, SDC_CFG_INTATGAP);
    } else {
        MSDC_CLR_BIT32(SDC_CFG, SDC_CFG_INTATGAP);
    }
}
#endif

int msdc_send_cmd(struct mmc_host *host, struct mmc_command *cmd)
{
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;
    u32 base   = host->base;
    u32 opcode = cmd->opcode;
    u32 rsptyp = cmd->rsptyp;
    u32 rawcmd;
    u32 timeout = cmd->timeout;
    u32 error = MMC_ERR_NONE;

    /* rawcmd :
     * vol_swt << 30 | auto_cmd << 28 | blklen << 16 | go_irq << 15 |
     * stop << 14 | rw << 13 | dtype << 11 | rsptyp << 7 | brk << 6 | opcode
     */
    rawcmd = (opcode & ~(SD_CMD_BIT | SD_CMD_APP_BIT)) |
        msdc_rsp[rsptyp] << 7 | host->blklen << 16;

    if (opcode == MMC_CMD_WRITE_MULTIPLE_BLOCK) {
        rawcmd |= ((2 << 11) | (1 << 13));
        if (priv->autocmd & MSDC_AUTOCMD12) {
            rawcmd |= (1 << 28);
        } else if (priv->autocmd & MSDC_AUTOCMD23) {
            rawcmd |= (2 << 28);
        }
    } else if (opcode == MMC_CMD_WRITE_BLOCK || opcode == MMC_CMD50) {
        rawcmd |= ((1 << 11) | (1 << 13));
    } else if (opcode == MMC_CMD_READ_MULTIPLE_BLOCK) {
        rawcmd |= (2 << 11);
        if (priv->autocmd & MSDC_AUTOCMD12) {
            rawcmd |= (1 << 28);
        } else if (priv->autocmd & MSDC_AUTOCMD23) {
            rawcmd |= (2 << 28);
        }
    } else if (opcode == MMC_CMD_READ_SINGLE_BLOCK ||
               opcode == SD_ACMD_SEND_SCR ||
               opcode == SD_CMD_SWITCH ||
               opcode == MMC_CMD_SEND_EXT_CSD ||
               opcode == MMC_CMD_SEND_WRITE_PROT ||
               opcode == MMC_CMD_SEND_WRITE_PROT_TYPE ||
               opcode == MMC_CMD21) {
        rawcmd |= (1 << 11);
    } else if (opcode == MMC_CMD_STOP_TRANSMISSION) {
        rawcmd |= (1 << 14);
        rawcmd &= ~(0x0FFF << 16);
    } else if (opcode == SD_IO_RW_EXTENDED) {
        if (cmd->arg & 0x80000000)  /* R/W flag */
            rawcmd |= (1 << 13);
        if ((cmd->arg & 0x08000000) && ((cmd->arg & 0x1FF) > 1))
            rawcmd |= (2 << 11); /* multiple block mode */
        else
            rawcmd |= (1 << 11);
    } else if (opcode == SD_IO_RW_DIRECT) {
        if ((cmd->arg & 0x80000000) && ((cmd->arg >> 9) & 0x1FFFF))/* I/O abt */
            rawcmd |= (1 << 14);
    } else if (opcode == SD_CMD_VOL_SWITCH) {
        rawcmd |= (1 << 30);
    } else if (opcode == SD_CMD_SEND_TUNING_BLOCK) {
        rawcmd |= (1 << 11); /* CHECKME */
        if (priv->autocmd & MSDC_AUTOCMD19)
            rawcmd |= (3 << 28);
    } else if (opcode == MMC_CMD_GO_IRQ_STATE) {
        rawcmd |= (1 << 15);
    } else if (opcode == MMC_CMD_WRITE_DAT_UNTIL_STOP) {
        rawcmd |= ((1<< 13) | (3 << 11));
    } else if (opcode == MMC_CMD_READ_DAT_UNTIL_STOP) {
        rawcmd |= (3 << 11);
    }
    
    MSDC_TRC_PRINT(MSDC_STACK, ("+[SD%d] CMD(%d): ARG(0x%x), RAW(0x%x), BLK_NUM(0x%x) RSP(%d)\n",
        host->id, (opcode & ~(SD_CMD_BIT | SD_CMD_APP_BIT)), cmd->arg, rawcmd, MSDC_READ32(SDC_BLK_NUM), rsptyp));

    /* FIXME. Need to check if SDC is busy before data read/write transfer */
    if (opcode == MMC_CMD_SEND_STATUS) {
        if (SDC_IS_CMD_BUSY()) {
            WAIT_COND(!SDC_IS_CMD_BUSY(), cmd->timeout, timeout);
            if (timeout == 0) {
                error = MMC_ERR_TIMEOUT;
                MSDC_ERR_PRINT(MSDC_ERROR,("-[SD%d] CMD(%d): SDC_IS_CMD_BUSY timeout\n",
                    host->id, (opcode & ~(SD_CMD_BIT | SD_CMD_APP_BIT))));
                goto end;
            }
        }
    } else {
        if (SDC_IS_BUSY()) {
            WAIT_COND(!SDC_IS_BUSY(), 1000, timeout);
            if (timeout == 0) {
                error = MMC_ERR_TIMEOUT;
                MSDC_ERR_PRINT(MSDC_ERROR,("-[SD%d] CMD(%d): SDC_IS_BUSY timeout\n",
                    host->id, (opcode & ~(SD_CMD_BIT | SD_CMD_APP_BIT))));
                goto end;
            }
        }
    }

    SDC_SEND_CMD(rawcmd, cmd->arg);

end:
    cmd->error = error;

    return error;
}

int msdc_wait_rsp(struct mmc_host *host, struct mmc_command *cmd)
{
    u32 base   = host->base;
    u32 rsptyp = cmd->rsptyp;
    u32 status;
#ifndef KEEP_SLIENT_BUILD 
    u32 opcode = (cmd->opcode & ~(SD_CMD_BIT | SD_CMD_APP_BIT));
#endif
    u32 error = MMC_ERR_NONE;
    u32 wints = MSDC_INT_CMDTMO | MSDC_INT_CMDRDY | MSDC_INT_RSPCRCERR |
        MSDC_INT_ACMDRDY | MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO |
        MSDC_INT_ACMD19_DONE;
	u32 *resp = &cmd->resp[0];

    if (cmd->opcode == MMC_CMD_GO_IRQ_STATE)
        wints |= MSDC_INT_MMCIRQ;

    status = msdc_intr_wait(host, wints);

    if (status == 0) {
        error = MMC_ERR_TIMEOUT;
        goto end;
    }
    
    if (   (status & MSDC_INT_CMDRDY) 
		|| (status & MSDC_INT_ACMDRDY) ||
           (status & MSDC_INT_ACMD19_DONE)) {
           
        switch (rsptyp) {
        case RESP_NONE:
            MSDC_TRC_PRINT(MSDC_STACK, ("-[SD%d] CMD(%d): RSP(%d)\n", host->id, opcode, rsptyp));
            break;
        case RESP_R2:
            *resp++ = MSDC_READ32(SDC_RESP3);
            *resp++ = MSDC_READ32(SDC_RESP2);
            *resp++ = MSDC_READ32(SDC_RESP1);
            *resp++ = MSDC_READ32(SDC_RESP0);
            MSDC_TRC_PRINT(MSDC_STACK, ("-[SD%d] CMD(%d): RSP(%d) = 0x%x 0x%x 0x%x 0x%x\n",
                host->id, opcode, cmd->rsptyp, cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]));
            break;
        default: /* Response types 1, 3, 4, 5, 6, 7(1b) */
            if (   (status & MSDC_INT_ACMDRDY) 
				|| (status & MSDC_INT_ACMD19_DONE)){
                cmd->resp[0] = MSDC_READ32(SDC_ACMD_RESP);
            }
			else {
                cmd->resp[0] = MSDC_READ32(SDC_RESP0);
			}
            MSDC_TRC_PRINT(MSDC_STACK, ("-[SD%d] CMD(%d): RSP(%d) = 0x%x AUTO(%d)\n", host->id, opcode,
                cmd->rsptyp, cmd->resp[0],
                ((status & MSDC_INT_ACMDRDY) || (status & MSDC_INT_ACMD19_DONE)) ? 1 : 0));
            break;
        }
    }
	else if ((status & MSDC_INT_RSPCRCERR) || (status & MSDC_INT_ACMDCRCERR)) {
        error = MMC_ERR_BADCRC;
        MSDC_TRC_PRINT(MSDC_STACK, ("-[SD%d] CMD(%d): RSP(%d) ERR(BADCRC)\n",
           												 host->id, opcode, cmd->rsptyp));
    } else if ((status & MSDC_INT_CMDTMO) || (status & MSDC_INT_ACMDTMO)) {
        error = MMC_ERR_TIMEOUT;
        MSDC_TRC_PRINT(MSDC_STACK, ("-[SD%d] CMD(%d): RSP(%d) ERR(CMDTO) AUTO(%d)\n",
            host->id, opcode, cmd->rsptyp, status & MSDC_INT_ACMDTMO ? 1: 0));
    } else {
        error = MMC_ERR_INVALID;
        MSDC_ERR_PRINT(MSDC_ERROR, ("-[SD%d] CMD(%d): RSP(%d) ERR(INVALID), Status:%x\n",
          												  host->id, opcode, cmd->rsptyp, status));
	}
end:

    if (rsptyp == RESP_R1B) {
        while ((MSDC_READ32(MSDC_PS) & MSDC_PS_DAT0) != MSDC_PS_DAT0);
    }

    if ((error == MMC_ERR_NONE) && (MSG_EVT_MASK & MSG_EVT_RSP)){
        switch(cmd->rsptyp) {
        case RESP_R1:
        case RESP_R1B:
            msdc_dump_card_status(cmd->resp[0]);
            break;
        case RESP_R3:
            msdc_dump_ocr_reg(cmd->resp[0]);
            break;
        case RESP_R5:
            msdc_dump_io_resp(cmd->resp[0]);
            break;
        case RESP_R6:
            msdc_dump_rca_resp(cmd->resp[0]);
            break;
        }
    }

    cmd->error = error;
	 if(cmd->opcode == MMC_CMD_APP_CMD && error == MMC_ERR_NONE){
			 host->app_cmd = 1;
			 host->app_cmd_arg = cmd->arg;
			 }
	 else 
	 	host->app_cmd = 0;
	 		
    return error;
}


int msdc_cmd(struct mmc_host *host, struct mmc_command *cmd)
{
    int err;

    err = msdc_send_cmd(host, cmd);
    if (err != MMC_ERR_NONE)
        return err;

    err = msdc_wait_rsp(host, cmd);

    if (err == MMC_ERR_BADCRC) {
        u32 base = host->base;
        u32 tmp = MSDC_READ32(SDC_CMD);

        /* check if data is used by the command or not */
        if (tmp & SDC_CMD_DTYP) {
            if(msdc_abort_handler(host, 1))
				MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] abort failed\n",host->id));
        }
        #ifdef FEATURE_MMC_CM_TUNING
        //Light: For CMD17/18/24/25, tuning may have been done by
        //       msdc_abort_handler()->msdc_get_card_status()->msdc_cmd() for CMD13->msdc_tune_cmdrsp().
        //       This means that 2nd invocation of msdc_tune_cmdrsp() occurs here!
        //--> To Do: consider if 2nd invocation can be avoid
        if ( host->app_cmd!=2 ) { //Light 20121225, to prevent recursive call path: msdc_tune_cmdrsp->msdc_app_cmd->msdc_cmd->msdc_tune_cmdrsp
        err = msdc_tune_cmdrsp(host, cmd);
            if (err != MMC_ERR_NONE){
                MSDC_ERR_PRINT(MSDC_ERROR,("[Err handle][%s:%d]tune cmd fail\n", __func__, __LINE__));
            }
        }
        #endif
    }
    return err;
}
static int msdc_app_cmd(struct mmc_host *host)
{
	struct mmc_command appcmd;
	int err = MMC_ERR_NONE;
	int retries = 10;
	appcmd.opcode = MMC_CMD_APP_CMD;
	appcmd.arg = host->app_cmd_arg;
	appcmd.rsptyp  = RESP_R1;
    appcmd.retries = CMD_RETRIES;
    appcmd.timeout = CMD_TIMEOUT;
	
	do {
        err = msdc_cmd(host, &appcmd);
        if (err == MMC_ERR_NONE)
            break;
    } while (retries--);
	return err;
}
#if MSDC_USE_DMA_MODE
int msdc_sg_init(struct scatterlist *sg, void *buf, u32 buflen)
{
    int i = MAX_SG_POOL_SZ;
    char *ptr = (char *)buf;

    MSDC_ASSERT(buflen <=((u64) MAX_SG_POOL_SZ * MAX_SG_BUF_SZ));
    msdc_flush_membuf(buf, buflen);
    while (i > 0) {
        if (buflen > MAX_SG_BUF_SZ) {
            sg->addr = (u32)ptr;
            sg->len  = MAX_SG_BUF_SZ;
            buflen  -= MAX_SG_BUF_SZ;
            ptr     += MAX_SG_BUF_SZ;
            sg++; i--; 
        } else {
            sg->addr = (u32)ptr;
            sg->len  = buflen;
            i--;
            break;
        }
    }

    

    return MAX_SG_POOL_SZ - i;
}

void msdc_dma_init(struct mmc_host *host, struct dma_config *cfg, void *buf, u32 buflen)
{
    u32 base = host->base;
    
    cfg->xfersz = buflen;
    
    if (cfg->mode == MSDC_MODE_DMA_BASIC) {
        cfg->sglen = 1;
        cfg->sg[0].addr = (u32)buf;
        cfg->sg[0].len = buflen;
        msdc_flush_membuf(buf, buflen);
    } else {
        cfg->sglen = msdc_sg_init(cfg->sg, buf, buflen);
    }

    msdc_clr_fifo(host);
    MSDC_DMA_ON();    
}

int msdc_dma_cmd(struct mmc_host *host, struct dma_config *cfg, struct mmc_command *cmd)
{
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;
    u32 opcode = cmd->opcode;
    u32 rsptyp = cmd->rsptyp;    
    u32 rawcmd;

    rawcmd = (opcode & ~(SD_CMD_BIT | SD_CMD_APP_BIT)) | 
        rsptyp << 7 | host->blklen << 16;

    if (opcode == MMC_CMD_WRITE_MULTIPLE_BLOCK) {
        rawcmd |= ((2 << 11) | (1 << 13));
        if (priv->autocmd & MSDC_AUTOCMD12)
            rawcmd |= (1 << 28);
        else if (priv->autocmd & MSDC_AUTOCMD23)
            rawcmd |= (2 << 28);
    } else if (opcode == MMC_CMD_WRITE_BLOCK) {
        rawcmd |= ((1 << 11) | (1 << 13));
    } else if (opcode == MMC_CMD_READ_MULTIPLE_BLOCK) {
        rawcmd |= (2 << 11);
        if (priv->autocmd & MSDC_AUTOCMD12)
            rawcmd |= (1 << 28);
        else if (priv->autocmd & MSDC_AUTOCMD23)
            rawcmd |= (2 << 28);
    } else if (opcode == MMC_CMD_READ_SINGLE_BLOCK) {
        rawcmd |= (1 << 11);
    } else {
        return -1;
    }

    MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] DMA CMD(%d), AUTOCMD12(%d), AUTOCMD23(%d)\n",
        host->id, (opcode & ~(SD_CMD_BIT | SD_CMD_APP_BIT)),
        (priv->autocmd & MSDC_AUTOCMD12) ? 1 : 0,
        (priv->autocmd & MSDC_AUTOCMD23) ? 1 : 0));
        
    cfg->cmd = rawcmd;
    cfg->arg = cmd->arg;

    return 0;
}

int msdc_dma_config(struct mmc_host *host, struct dma_config *cfg)
{
    u32 base = host->base;
    u32 sglen = cfg->sglen;
    u32 i, j, num, bdlen, arg, xfersz;
    u8  blkpad, dwpad, chksum;
    struct scatterlist *sg = cfg->sg;
    gpd_t *gpd;
    __bd_t *bd;

    switch (cfg->mode) {
    case MSDC_MODE_DMA_BASIC:
        MSDC_ASSERT(cfg->xfersz <= MAX_DMA_CNT);
        MSDC_ASSERT(cfg->sglen == 1);
        MSDC_WRITE32(MSDC_DMA_SA, sg->addr);
        MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_LASTBUF, 1);
        MSDC_WRITE32(MSDC_DMA_LEN, sg->len);
        MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_BRUSTSZ, cfg->burstsz);
        MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE, 0);
        break;
    case MSDC_MODE_DMA_DESC:
        blkpad = (cfg->flags & DMA_FLAG_PAD_BLOCK) ? 1 : 0;
        dwpad  = (cfg->flags & DMA_FLAG_PAD_DWORD) ? 1 : 0;
        chksum = (cfg->flags & DMA_FLAG_EN_CHKSUM) ? 1 : 0;

#if 0 /* YD: current design doesn't support multiple GPD in descriptor dma mode */
        /* calculate the required number of gpd */
        num = (sglen + MAX_BD_PER_GPD - 1) / MAX_BD_PER_GPD;        
        gpd = msdc_alloc_gpd(host, num);
        for (i = 0; i < num; i++) {
            gpd[i].intr = 0;
            if (sglen > MAX_BD_PER_GPD) {
                bdlen  = MAX_BD_PER_GPD;
                sglen -= MAX_BD_PER_GPD;
            } else {
                bdlen = sglen;
                sglen = 0;
            }
            bd = msdc_alloc_bd(host, bdlen);
            for (j = 0; j < bdlen; j++) {
                MSDC_INIT_BD(&bd[j], blkpad, dwpad, sg->addr, sg->len);
                sg++;
            }
            msdc_queue_bd(host, &gpd[i], bd);
            msdc_flush_membuf(bd, bdlen * sizeof(__bd_t));
        }     
        msdc_add_gpd(host, gpd, num);
        msdc_dump_dma_desc(host);
        msdc_flush_membuf(gpd, num * sizeof(gpd_t));
        MSDC_WRITE32(MSDC_DMA_SA, (u32)&gpd[0]);
        MSDC_SET_FIELD(MSDC_DMA_CFG, MSDC_DMA_CFG_DECSEN, chksum);
        MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_BRUSTSZ, cfg->burstsz);
        MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE, 1);
#else
        /* calculate the required number of gpd */
            MSDC_ASSERT(sglen <= MAX_BD_POOL_SZ);
        
        gpd = msdc_alloc_gpd(host, 1);
        gpd->intr = 0;
        
        bd = msdc_alloc_bd(host, sglen);
        for (j = 0; j < sglen; j++) {
            MSDC_INIT_BD(&bd[j], blkpad, dwpad, sg->addr, sg->len);
            sg++;
        }
        msdc_queue_bd(host, &gpd[0], bd);
        msdc_flush_membuf(bd, sglen * sizeof(__bd_t));

        msdc_add_gpd(host, gpd, 1);
        msdc_dump_dma_desc(host);
        msdc_flush_membuf(gpd, (1 + 1) * sizeof(gpd_t)); /* include null gpd */
        MSDC_WRITE32(MSDC_DMA_SA, (u32)&gpd[0]);
        MSDC_SET_FIELD(MSDC_DMA_CFG, MSDC_DMA_CFG_DECSEN, chksum);
        MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_BRUSTSZ, cfg->burstsz);
        MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE, 1);

#endif        
        break;
#ifdef FEATURE_MSDC_ENH_DMA_MODE
    case MSDC_MODE_DMA_ENHANCED:
        arg = cfg->arg;
        blkpad = (cfg->flags & DMA_FLAG_PAD_BLOCK) ? 1 : 0;
        dwpad  = (cfg->flags & DMA_FLAG_PAD_DWORD) ? 1 : 0;
        chksum = (cfg->flags & DMA_FLAG_EN_CHKSUM) ? 1 : 0;
        
        /* calculate the required number of gpd */
        num = (sglen + MAX_BD_PER_GPD - 1) / MAX_BD_PER_GPD;        
        gpd = msdc_alloc_gpd(host, num);
        for (i = 0; i < num; i++) {            
            xfersz = 0;
            if (sglen > MAX_BD_PER_GPD) {
                bdlen  = MAX_BD_PER_GPD;
                sglen -= MAX_BD_PER_GPD;
            } else {
                bdlen = sglen;
                sglen = 0;
            }
            bd = msdc_alloc_bd(host, bdlen);
            for (j = 0; j < bdlen; j++) {
                xfersz += sg->len;
                MSDC_INIT_BD(&bd[j], blkpad, dwpad, sg->addr, sg->len);
                sg++;
            }
            /* YD: 1 XFER_COMP interrupt will be triggerred by each GPD when it
             * is done. For multiple GPDs, multiple XFER_COMP interrupts will be 
             * triggerred. In such situation, it's not easy to know which 
             * interrupt indicates the transaction is done. So, we use the 
             * latest one GPD's INT as the transaction done interrupt.
             */
            //gpd[i].intr = cfg->intr;
            gpd[i].intr = (i == num - 1) ? 0 : 1;
            gpd[i].cmd  = cfg->cmd;
            gpd[i].blknum = xfersz / cfg->blklen;
            gpd[i].arg  = arg;
            gpd[i].extlen = 0xC;

            arg += xfersz;

            msdc_queue_bd(host, &gpd[i], bd);
            msdc_flush_membuf(bd, bdlen * sizeof(__bd_t));
        }     
        msdc_add_gpd(host, gpd, num);

        msdc_dump_dma_desc(host);

        msdc_flush_membuf(gpd, (num + 1) * sizeof(gpd_t)); /* include null gpd */
        MSDC_WRITE32(MSDC_DMA_SA, (u32)&gpd[0]);
        MSDC_SET_FIELD(MSDC_DMA_CFG, MSDC_DMA_CFG_DECSEN, chksum);
        MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_BRUSTSZ, cfg->burstsz);
        MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE, 1);
        break;
#endif
    default:
        break;
    }
    MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] DMA_SA   = 0x%x\n", host->id, MSDC_READ32(MSDC_DMA_SA)));
    MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] DMA_CA   = 0x%x\n", host->id, MSDC_READ32(MSDC_DMA_CA)));
    MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] DMA_CTRL = 0x%x\n", host->id, MSDC_READ32(MSDC_DMA_CTRL)));
    MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] DMA_CFG  = 0x%x\n", host->id, MSDC_READ32(MSDC_DMA_CFG)));

    return 0;
}

void msdc_dma_resume(struct mmc_host *host)
{
    u32 base = host->base;

    MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_RESUME, 1);

    MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] DMA resume\n", host->id));
}

void msdc_dma_start(struct mmc_host *host)
{
    u32 base = host->base;

    MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_START, 1);

    MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] DMA start\n", host->id));
}

void msdc_dma_stop(struct mmc_host *host)
{
    u32 base = host->base;

    MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_STOP, 1);
    while ((MSDC_READ32(MSDC_DMA_CFG) & MSDC_DMA_CFG_STS) != 0);
    MSDC_DMA_OFF();

    MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] DMA Stopped\n", host->id));
    
    msdc_reset_gpd(host);
}

int msdc_dma_wait_done(struct mmc_host *host, u32 timeout)
{
    u32 base = host->base;
    u32 tmo = timeout;
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;
    struct dma_config *cfg = &priv->cfg;
    u32 status;
    u32 error = MMC_ERR_NONE;
    u32 wints = MSDC_INT_XFER_COMPL | MSDC_INT_DATTMO | MSDC_INT_DATCRCERR |
        MSDC_INT_DXFER_DONE | MSDC_INT_DMAQ_EMPTY | 
        MSDC_INT_ACMDRDY | MSDC_INT_ACMDTMO | MSDC_INT_ACMDCRCERR | 
        MSDC_INT_CMDRDY | MSDC_INT_CMDTMO | MSDC_INT_RSPCRCERR;

    do {
        MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] DMA Curr Addr: 0x%x, Active: %d\n", host->id,
            						MSDC_READ32(MSDC_DMA_CA), MSDC_READ32(MSDC_DMA_CFG) & 0x1));

        status = msdc_intr_wait(host, wints);

        if (status == 0 || status & MSDC_INT_DATTMO) {
            MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] DMA DAT timeout(%xh)\n", host->id, status));
            error = MMC_ERR_TIMEOUT;
            goto end;
        } else if (status & MSDC_INT_DATCRCERR) {
            MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] DMA DAT CRC error(%xh)\n", host->id, status));
            error = MMC_ERR_BADCRC;
            goto end;
        } else if (status & MSDC_INT_CMDTMO) {
            MSDC_TRC_PRINT(MSDC_DMA, ("[SD%d] DMA CMD timeout(%xh)\n", host->id, status));
            error = MMC_ERR_TIMEOUT;
            goto end;        
        } else if (status & MSDC_INT_RSPCRCERR) {
            MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] DMA CMD CRC error(%xh)\n", host->id, status));
            error = MMC_ERR_BADCRC;
            goto end;    
        } else if (status & MSDC_INT_ACMDTMO) {
            MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] DMA ACMD timeout(%xh)\n", host->id, status));
            error = MMC_ERR_TIMEOUT;
            goto end;        
        } else if (status & MSDC_INT_ACMDCRCERR) {
            MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] DMA ACMD CRC error(%xh)\n", host->id, status));
            error = MMC_ERR_BADCRC;
            goto end;
        }
        else if ( host && (host->id == 3) && (status & MSDC_INT_AXI_RESP_ERR))
        {
            MSDC_DBG_PRINT(MSDC_DMA,("[SD%d]AXI BUS response error status(%xh)\n", host->id, status));
            error = MMC_ERR_AXI_RSPCRC;
            goto end;
        }

#ifdef FEATURE_MSDC_ENH_DMA_MODE
        if ((cfg->mode == MSDC_MODE_DMA_ENHANCED) && (status & MSDC_INT_CMDRDY)) {
            cfg->rsp = MSDC_READ32(SDC_RESP0);
            MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] DMA ENH CMD Rdy, Resp(%xh)\n", host->id, cfg->rsp));
            msdc_dump_card_status(cfg->rsp);
        }
#endif
        if (status & MSDC_INT_ACMDRDY) {
            cfg->autorsp = MSDC_READ32(SDC_ACMD_RESP);
            MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] DMA AUTO CMD Rdy, Resp(%xh)\n", host->id, cfg->autorsp));
            msdc_dump_card_status(cfg->autorsp);
        }
#ifdef FEATURE_MSDC_ENH_DMA_MODE
        if (cfg->mode == MSDC_MODE_DMA_ENHANCED) {
            /* YD: 1 XFER_COMP interrupt will be triggerred by each GPD when it
             * is done. For multiple GPDs, multiple XFER_COMP interrupts will be 
             * triggerred. In such situation, it's not easy to know which 
             * interrupt indicates the transaction is done. So, we use the 
             * latest one GPD's INT as the transaction done interrupt.
             */        
            if (status & MSDC_INT_DXFER_DONE)
                break;
        } else 
#endif        
        {
            if (status & MSDC_INT_XFER_COMPL)
                break;
        }
    } while (1);

    /* check dma status */
    do {
        status = MSDC_READ32(MSDC_INT);
        if (status & MSDC_INT_GPDCSERR) {
            MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] GPD checksum error\n", host->id));
            error = MMC_ERR_BADCRC;
            break;
        } else if (status & MSDC_INT_BDCSERR) {
            MSDC_DBG_PRINT(MSDC_DMA, ("[SD%d] BD checksum error\n", host->id));
            error = MMC_ERR_BADCRC;
            break;
        } else {
           status = MSDC_READ32(MSDC_DMA_CFG);
           if ((status & MSDC_DMA_CFG_STS) == 0) {
               break;
           }
        }
    } while (1);

end:
	if(error)
		MSDC_RESET();
    return error;
}

int msdc_dma_transfer(struct mmc_host *host, struct mmc_command *cmd, struct mmc_data *data)
{
    int err = MMC_ERR_NONE, derr = MMC_ERR_NONE;
    int multi;
    u32 blksz = host->blklen;
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;
    struct dma_config *cfg = &priv->cfg;
    struct mmc_command stop;
    uchar *buf = data->buf;
    ulong nblks = data->blks;

    MSDC_ASSERT(nblks <= MAX_DMA_TRAN_SIZE);

    multi = nblks > 1 ? 1 : 0;

#ifdef FEATURE_MSDC_ENH_DMA_MODE
    if (cfg->mode == MSDC_MODE_DMA_ENHANCED) {
        if (multi && (priv->autocmd == 0))
            msdc_set_autocmd(host, MSDC_AUTOCMD12, 1);
        msdc_set_blklen(host, blksz);
        msdc_set_timeout(host, data->timeout * 1000000, 0);
        msdc_dma_cmd(host, cfg, cmd);
        msdc_dma_init(host, cfg, (void*)buf, nblks * blksz);
        msdc_dma_config(host, cfg);
        msdc_dma_start(host);
        err = derr = msdc_dma_wait_done(host, 0xFFFFFFFF);
        msdc_dma_stop(host);
		msdc_flush_membuf(buf,nblks * blksz);
        if (multi && (priv->autocmd == 0))
            msdc_set_autocmd(host, MSDC_AUTOCMD12, 0);
    } else 
#endif
    {
        u32 left_sz, xfer_sz;

        msdc_set_blklen(host, blksz);
        msdc_set_timeout(host, data->timeout * 1000000, 0);

        left_sz = nblks * blksz;

        if (cfg->mode == MSDC_MODE_DMA_BASIC) {
            xfer_sz = left_sz > MAX_DMA_CNT ? MAX_DMA_CNT : left_sz;
            nblks   = xfer_sz / blksz;
        } else {
            xfer_sz = left_sz;
        }

        while (left_sz) {
#if 1
            msdc_set_blknum(host, nblks);
            msdc_dma_init(host, cfg, (void*)buf, xfer_sz);

            err = msdc_send_cmd(host, cmd);
            msdc_dma_config(host, cfg);
           	
            if (err != MMC_ERR_NONE) {
                msdc_reset_gpd(host);
                goto done;
            }

            err = msdc_wait_rsp(host, cmd);
            
            if (err == MMC_ERR_BADCRC) {
                u32 base = host->base;
                u32 tmp = MSDC_READ32(SDC_CMD);
            
                /* check if data is used by the command or not */
                if (tmp & 0x1800) {
                    if(msdc_abort_handler(host, 1))
						MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] abort failed\n",host->id));
                }
                #ifdef FEATURE_MMC_CM_TUNING
                err = msdc_tune_cmdrsp(host, cmd);
                #endif
				if(err != MMC_ERR_NONE){
					msdc_reset_gpd(host);
					goto done;
				}
            }
			msdc_dma_start(host);
            err = derr = msdc_dma_wait_done(host, 0xFFFFFFFF);
            msdc_dma_stop(host);
			msdc_flush_membuf(buf,nblks * blksz);
#else
            msdc_set_blknum(host, nblks);
            msdc_dma_init(host, cfg, (void*)buf, xfer_sz);
            msdc_dma_config(host, cfg);

            err = msdc_cmd(host, cmd);
            if (err != MMC_ERR_NONE) {
                msdc_reset_gpd(host);
                goto done;
            }

            msdc_dma_start(host);
            err = derr = msdc_dma_wait_done(host, 0xFFFFFFFF);
            msdc_dma_stop(host);
#endif

            if (multi && (priv->autocmd == 0)) {
                stop.opcode  = MMC_CMD_STOP_TRANSMISSION;
                stop.rsptyp  = RESP_R1B;
                stop.arg     = 0;
                stop.retries = CMD_RETRIES;
                stop.timeout = CMD_TIMEOUT;
                err = msdc_cmd(host, &stop) != MMC_ERR_NONE ? MMC_ERR_FAILED : err;
            }
            if (err != MMC_ERR_NONE)
                goto done;
            buf     += xfer_sz;
            left_sz -= xfer_sz;

            /* left_sz > 0 only when in basic dma mode */
            if (left_sz) {
                cmd->arg += nblks; /* update to next start address */
                xfer_sz  = (xfer_sz > left_sz) ? left_sz : xfer_sz;
                nblks    = (left_sz > xfer_sz) ? nblks : left_sz / blksz;
            }
        }
    }
done:
    if (derr != MMC_ERR_NONE) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] <CMD%d> DMA data error (%d)\n", host->id, cmd->opcode, derr));
		if(msdc_abort_handler(host, 1))
			MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] abort failed\n",host->id));
    }

    return err;
}

int msdc_dma_bread(struct mmc_host *host, u8 *dst, u32 src, u32 nblks)
{
    int multi;
    struct mmc_command cmd;    
    struct mmc_data data;

    MSDC_ASSERT(nblks <= host->max_phys_segs);

    MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d] Read data %d blks from 0x%x\n", host->id, nblks, src));

    multi = nblks > 1 ? 1 : 0;

    /* send read command */
    cmd.opcode  = multi ? MMC_CMD_READ_MULTIPLE_BLOCK : MMC_CMD_READ_SINGLE_BLOCK;
    cmd.rsptyp  = RESP_R1;
    cmd.arg     = src;
    cmd.retries = 0;
    cmd.timeout = CMD_TIMEOUT;

    data.blks    = nblks;
    data.buf     = (u8*)dst;
    data.timeout = 100; /* 100ms */
    
    return msdc_dma_transfer(host, &cmd, &data);
}

int msdc_dma_bwrite(struct mmc_host *host, u32 dst, u8 *src, u32 nblks)
{
    int multi;
    struct mmc_command cmd;
    struct mmc_data data;

    MSDC_ASSERT(nblks <= host->max_phys_segs);

    MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d] Write data %d blks to 0x%x\n", host->id, nblks, dst));

    multi = nblks > 1 ? 1 : 0;

    /* send write command */
    cmd.opcode  = multi ? MMC_CMD_WRITE_MULTIPLE_BLOCK : MMC_CMD_WRITE_BLOCK;
    cmd.rsptyp  = RESP_R1;
    cmd.arg     = dst;
    cmd.retries = 0;
    cmd.timeout = CMD_TIMEOUT;

    data.blks    = nblks;
    data.buf     = (u8*)src;
    data.timeout = 250; /* 250ms */

    return msdc_dma_transfer(host, &cmd, &data);
}
int msdc_dma_send_sandisk_fwid(struct mmc_host *host, uchar *buf,u32 opcode, ulong nblks)
{
    //int multi;
    struct mmc_command cmd;
    struct mmc_data data;

    MSDC_ASSERT(nblks <= host->max_phys_segs);

    //MSG(OPS, "[SD%d] Read data %d blks from 0x%x\n", host->id, nblks, src);

    //multi = nblks > 1 ? 1 : 0;

    /* send read command */
    cmd.opcode  = opcode;
    cmd.rsptyp  = RESP_R1;
    cmd.arg     = 0;  //src;
    cmd.retries = 0;
    cmd.timeout = CMD_TIMEOUT;

    data.blks    = nblks;
    data.buf     = (u8*)buf;
    data.timeout = 100; /* 100ms */
    
    return msdc_dma_transfer(host, &cmd, &data);
}

#endif

int msdc_pio_read_word(struct mmc_host *host, u32 *ptr, u32 size)
{
    int err = MMC_ERR_NONE;
    u32 base = host->base;
    u32 ints = MSDC_INT_DATCRCERR | MSDC_INT_DATTMO | MSDC_INT_XFER_COMPL;
    //u32 timeout = 100000;
    u32 status;
#ifndef KEEP_SLIENT_BUILD
    u32 totalsz = size;
#endif
    u8  done = 0;
    u8* u8ptr;
	u32 dcrc=0;

    while (1) {
        status = MSDC_READ32(MSDC_INT);
        MSDC_WRITE32(MSDC_INT, status);
        if (status & ~ints) {
            MSDC_DBG_PRINT(MSDC_WARN,("[SD%d]<CHECKME> Unexpected INT(0x%x)\n",
                host->id, status));
        }
        if (status & MSDC_INT_DATCRCERR) {
            MSDC_GET_FIELD(SDC_DCRC_STS, SDC_DCRC_STS_POS|SDC_DCRC_STS_NEG, dcrc);
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] DAT CRC error (0x%x), Left:%d/%d bytes, RXFIFO:%d,dcrc:0x%x\n",
                host->id, status, size, totalsz, MSDC_RXFIFOCNT(),dcrc));
            err = MMC_ERR_BADCRC;
            break;
        } else if (status & MSDC_INT_DATTMO) {
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] DAT TMO error (0x%x), Left: %d/%d bytes, RXFIFO:%d\n",
                host->id, status, size, totalsz, MSDC_RXFIFOCNT()));
            err = MMC_ERR_TIMEOUT;
            break;
        } else if (status & MSDC_INT_ACMDCRCERR) {
            MSDC_GET_FIELD(SDC_DCRC_STS, SDC_DCRC_STS_POS|SDC_DCRC_STS_NEG, dcrc);
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] AUTOCMD CRC error (0x%x), Left:%d/%d bytes, RXFIFO:%d,dcrc:0x%x\n",
                host->id, status, size, totalsz, MSDC_RXFIFOCNT(),dcrc));
            err = MMC_ERR_ACMD_RSPCRC;
            break;
        } else if (status & MSDC_INT_XFER_COMPL) {
            done = 1;
        }

        if (size == 0 && done)
            break;

        /* Note. RXFIFO count would be aligned to 4-bytes alignment size */
        if ((size >=  MSDC_FIFO_THD) && (MSDC_RXFIFOCNT() >= MSDC_FIFO_THD)) {
            int left = MSDC_FIFO_THD >> 2;
            do {
                *ptr++ = MSDC_FIFO_READ32();
            } while (--left);
            size -= MSDC_FIFO_THD;
            MSDC_DBG_PRINT(MSDC_PIO, ("[SD%d] Read %d bytes, RXFIFOCNT: %d,  Left: %d/%d\n",
                host->id, MSDC_FIFO_THD, MSDC_RXFIFOCNT(), size, totalsz));
        } else if ((size < MSDC_FIFO_THD) && MSDC_RXFIFOCNT() >= size) {
            while (size) {
                if (size > 3) {
                    *ptr++ = MSDC_FIFO_READ32();
                    size -= 4;
                } else {
                    u8ptr = (u8*)ptr;
                    while(size --)
                        *u8ptr++ = MSDC_FIFO_READ8();
                }
            }
            MSDC_DBG_PRINT(MSDC_PIO, ("[SD%d] Read left bytes, RXFIFOCNT: %d, Left: %d/%d\n",
                host->id, MSDC_RXFIFOCNT(), size, totalsz));
        }
    }

    return err;
}

int msdc_pio_read(struct mmc_host *host, u32 *ptr, u32 size)
{
    int err = msdc_pio_read_word(host, (u32*)ptr, size); 

    if (err != MMC_ERR_NONE) {
        msdc_abort(host); /* reset internal fifo and state machine */
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] %d-bit PIO Read Error (%d)\n", host->id,
            32, err));
    }
    
    return err;
}

int msdc_pio_write_word(struct mmc_host *host, u32 *ptr, u32 size)
{
    int err = MMC_ERR_NONE;
    u32 base = host->base;
    u32 ints = MSDC_INT_DATCRCERR | MSDC_INT_DATTMO | MSDC_INT_XFER_COMPL;
    //u32 timeout = 250000;
    u32 status;
    u8* u8ptr;
    //msdc_priv_t *priv = (msdc_priv_t*)host->priv;

    while (1) {
        status = MSDC_READ32(MSDC_INT);
        MSDC_WRITE32(MSDC_INT, status);
        if (status & ~ints) {
            MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d]<CHECKME> Unexpected INT(0x%x)\n",
                host->id, status));
        }
        if (status & MSDC_INT_DATCRCERR) {
            MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d] DAT CRC error (0x%x), Left DAT: %d bytes\n",
                host->id, status, size));
            err = MMC_ERR_BADCRC;
            break;
        } else if (status & MSDC_INT_DATTMO) {
            MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d] DAT TMO error (0x%x), Left DAT: %d bytes, MSDC_FIFOCS=%xh\n",
                host->id, status, size, MSDC_READ32(MSDC_FIFOCS)));
            err = MMC_ERR_TIMEOUT;
            break;
        } else if (status & MSDC_INT_ACMDCRCERR) {
            MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d] AUTO CMD CRC error (0x%x), Left DAT: %d bytes\n",
                host->id, status, size));
            err = MMC_ERR_ACMD_RSPCRC;
            break;
        } else if (status & MSDC_INT_XFER_COMPL) {
            if (size == 0) {
                MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d] all data flushed to card\n", host->id));
                break;
            } else {
                MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d]<CHECKME> XFER_COMPL before all data written\n",
                    host->id));
            }
        }

        if (size == 0)
            continue;

        if (size >= MSDC_FIFO_SZ) {
            if (MSDC_TXFIFOCNT() == 0) {
                int left = MSDC_FIFO_SZ >> 2;
                do {
                    MSDC_FIFO_WRITE32(*ptr); ptr++;
                } while (--left);
                size -= MSDC_FIFO_SZ;
            }
        } else if (size < MSDC_FIFO_SZ && MSDC_TXFIFOCNT() == 0) {            
            while (size ) {
                if (size > 3) {
                    MSDC_FIFO_WRITE32(*ptr); ptr++;
                    size -= 4;
                } else {
                    u8ptr = (u8*)ptr;
                    while(size --){
                       MSDC_FIFO_WRITE8(*u8ptr);
                       u8ptr++;
                    }
                }
            }
        }
    }

    return err;
}

int msdc_pio_write(struct mmc_host *host, u32 *ptr, u32 size)
{
    int err = msdc_pio_write_word(host, (u32*)ptr, size);

    if (err != MMC_ERR_NONE) {
        msdc_abort(host); /* reset internal fifo and state machine */
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] PIO Write Error (%d)\n", host->id, err));
    }

    return err;
}

int msdc_pio_get_sandisk_fwid(struct mmc_host *host, uchar *dst)
{
    //msdc_priv_t *priv = (msdc_priv_t*)host->priv;
    //u32 base = host->base;
    u32 blksz = host->blklen;
    int err = MMC_ERR_NONE, derr = MMC_ERR_NONE;
    //int multi;
    struct mmc_command cmd;
    
    ulong *ptr = (ulong *)dst;

    //MSG(OPS, "[SD%d] Read data %d bytes from 0x%x\n", host->id, nblks * blksz, src);

    msdc_clr_fifo(host);
    msdc_set_blknum(host, 1);
    msdc_set_blklen(host, blksz);
    msdc_set_timeout(host, 100000000, 0);

    /* send read command */
    cmd.opcode  = MMC_CMD21;
    cmd.rsptyp  = RESP_R1;
    cmd.arg     = 0;
    cmd.retries = 0;
    cmd.timeout = CMD_TIMEOUT;
    err = msdc_cmd(host, &cmd);

    if (err != MMC_ERR_NONE)
        goto done;

    err = derr = msdc_pio_read(host, (u32*)ptr, 1 * blksz);
    
    
done:
    if (err != MMC_ERR_NONE) {
        if (derr != MMC_ERR_NONE) {
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Read data error (%d)\n", host->id, derr));
            if(msdc_abort_handler(host, 1))
				MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] abort failed\n",host->id));
        } else {
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Read error (%d)\n", host->id, err));
        }
    }
    return (derr == MMC_ERR_NONE) ? err : derr;
}
int msdc_pio_send_sandisk_fwid(struct mmc_host *host,uchar *src)
{
   // msdc_priv_t *priv = (msdc_priv_t*)host->priv;
   // u32 base = host->base;
    int err = MMC_ERR_NONE, derr = MMC_ERR_NONE;
    //int multi;
    u32 blksz = host->blklen;
    struct mmc_command cmd;
   
    ulong *ptr = (ulong *)src;

    //MSG(OPS, "[SD%d] Write data %d bytes to 0x%x\n", host->id, nblks * blksz, dst);

    

    msdc_clr_fifo(host);
    msdc_set_blknum(host, 1);
    msdc_set_blklen(host, blksz);
    
    /* No need since MSDC always waits 8 cycles for write data timeout */
    
    /* send write command */
    cmd.opcode  = MMC_CMD50;
    cmd.rsptyp  = RESP_R1;
    cmd.arg     = 0;
    cmd.retries = 0;
    cmd.timeout = CMD_TIMEOUT;
    err = msdc_cmd(host, &cmd);

    if (err != MMC_ERR_NONE)
        goto done;

    err = derr = msdc_pio_write(host, (u32*)ptr, 1 * blksz);

done:
    if (err != MMC_ERR_NONE) {
        if (derr != MMC_ERR_NONE) {
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Write data error (%d)\n", host->id, derr));
            if(msdc_abort_handler(host, 1))
				MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] abort failed\n",host->id));
        } else {
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Write error (%d)\n", host->id, err));
        }
    }
    return (derr == MMC_ERR_NONE) ? err : derr;
}

int msdc_pio_bread(struct mmc_host *host, u8 *dst, u32 src, u32 nblks)
{
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;
    //u32 base = host->base;
    u32 blksz = host->blklen;
    int err = MMC_ERR_NONE, derr = MMC_ERR_NONE;
    int multi;
    struct mmc_command cmd;
    struct mmc_command stop;
    u32 *ptr = (u32 *)dst;

    MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d] Read data %d bytes from 0x%x\n", host->id, nblks * blksz, src));

    multi = nblks > 1 ? 1 : 0;

    msdc_clr_fifo(host);
    msdc_set_blknum(host, nblks);
    msdc_set_blklen(host, blksz);
    msdc_set_timeout(host, 100000000, 0);

    /* send read command */
    cmd.opcode  = multi ? MMC_CMD_READ_MULTIPLE_BLOCK : MMC_CMD_READ_SINGLE_BLOCK;
    cmd.rsptyp  = RESP_R1;
    cmd.arg     = src;
    cmd.retries = 0;
    cmd.timeout = CMD_TIMEOUT;
    err = msdc_cmd(host, &cmd);

    if (err != MMC_ERR_NONE)
        goto done;

    err = derr = msdc_pio_read(host, (u32*)ptr, nblks * blksz);
    
    if (multi && (priv->autocmd == 0)) {
        stop.opcode  = MMC_CMD_STOP_TRANSMISSION;
        stop.rsptyp  = RESP_R1B;
        stop.arg     = 0;
        stop.retries = CMD_RETRIES;
        stop.timeout = CMD_TIMEOUT;
        err = msdc_cmd(host, &stop) != MMC_ERR_NONE ? MMC_ERR_FAILED : err;
    }

done:
    if (err != MMC_ERR_NONE) {
        if (derr != MMC_ERR_NONE) {
            MSDC_ERR_PRINT(MSDC_ERROR, ("[SD%d] Read data error (%d)\n", host->id, derr));
            if(msdc_abort_handler(host, 1))
				MSDC_ERR_PRINT(MSDC_ERROR, ("[SD%d] abort failed\n",host->id));
        } else {
            MSDC_ERR_PRINT(MSDC_ERROR, ("[SD%d] Read error (%d)\n", host->id, err));
        }
    }
    return (derr == MMC_ERR_NONE) ? err : derr;
}

int msdc_pio_bwrite(struct mmc_host *host, u32 dst, u8 *src, u32 nblks)
{
    msdc_priv_t *priv = (msdc_priv_t*)host->priv;
    //u32 base = host->base;
    int err = MMC_ERR_NONE, derr = MMC_ERR_NONE;
    int multi;
    u32 blksz = host->blklen;
    struct mmc_command cmd;
    struct mmc_command stop;
    u32 *ptr = (u32 *)src;

    MSDC_DBG_PRINT(MSDC_DEBUG, ("[SD%d] Write data %d bytes to 0x%x\n", host->id, nblks * blksz, dst));

    multi = nblks > 1 ? 1 : 0;	

    msdc_clr_fifo(host);
    msdc_set_blknum(host, nblks);
    msdc_set_blklen(host, blksz);
    
    /* No need since MSDC always waits 8 cycles for write data timeout */
    
    /* send write command */
    cmd.opcode  = multi ? MMC_CMD_WRITE_MULTIPLE_BLOCK : MMC_CMD_WRITE_BLOCK;
    cmd.rsptyp  = RESP_R1;
    cmd.arg     = dst;
    cmd.retries = 0;
    cmd.timeout = CMD_TIMEOUT;
    err = msdc_cmd(host, &cmd);

    if (err != MMC_ERR_NONE)
        goto done;

    err = derr = msdc_pio_write(host, (u32*)ptr, nblks * blksz);

    if (multi && (priv->autocmd == 0)) {
        stop.opcode  = MMC_CMD_STOP_TRANSMISSION;
        stop.rsptyp  = RESP_R1B;
        stop.arg     = 0;
        stop.retries = CMD_RETRIES;
        stop.timeout = CMD_TIMEOUT;
        err = msdc_cmd(host, &stop) != MMC_ERR_NONE ? MMC_ERR_FAILED : err;
    }

done:
    if (err != MMC_ERR_NONE) {
        if (derr != MMC_ERR_NONE) {
           	MSDC_ERR_PRINT(MSDC_ERROR, ("[SD%d] Write data error (%d)\n", host->id, derr));
            if(msdc_abort_handler(host, 1))
				MSDC_ERR_PRINT(MSDC_ERROR, ("[SD%d] abort failed\n",host->id));
        } else {
            MSDC_ERR_PRINT(MSDC_ERROR, ("[SD%d] Write error (%d)\n", host->id, err));
        }
    }
    return (derr == MMC_ERR_NONE) ? err : derr;
}

void msdc_config_clksrc(struct mmc_host *host, u32 clksrc, u32 hclksrc)
{
		// modify the clock
		MSDC_ASSERT(host);
		if ( host->id == 3) {
			if ( host->card && mmc_card_hs400(host->card)) {
				/* after the card init flow, if the card support hs400 mode
						*  modify the mux channel of the msdc pll */
				//host->clksrc  = MSDC50_CLKSRC_400MHZ;
				clksrc		   = MSDC50_CLKSRC_400MHZ;
				host->hclksrc   = hclksrc;
				host->clk	   = hclks_msdc50[MSDC50_CLKSRC_400MHZ];
				MSDC_TRC_PRINT(MSDC_INIT,("[info][%s] hs400 mode, change pll mux to 400Mhz\n", __func__));
	
			}
			else {
				/* Perloader and LK use 200 is ok, no need change source */
				host->clksrc  = clksrc;
				host->hclksrc = hclksrc;
				host->clk     = hclks_msdc50[clksrc];
			}
		}
		else {
			/* Perloader and LK use 200 is ok, no need change source */
			host->clksrc  = clksrc;
			host->hclksrc = hclksrc;
			host->clk     = hclks_msdc30[clksrc];	
		}
		
#ifndef FPGA_PLATFORM
			if ( host->id == 3) {
				MSDC_SET_FIELD(TOPCKGEN_CTRL(CLKCFG6) ,MSDC_CKGENHCLK_SEL(MSDC3,CLKCFG6), hclksrc); 	
				//MSDC_SET_FIELD(TOPCKGEN_CTRL(CLKCFGUPDATE),MSDC_CKGENHCLK_UPDATE(MSDC3), 1);
				
				MSDC_SET_FIELD(TOPCKGEN_CTRL(CLKCFG14), MSDC_CKGEN_SEL(MSDC3,CLKCFG14), clksrc);		
				//MSDC_SET_FIELD(TOPCKGEN_CTRL(CLKCFGUPDATE),MSDC_CKGEN_UPDATE(MSDC3), 1);
	
			}
			else if ( host->id == 2) {
			   MSDC_SET_FIELD(TOPCKGEN_CTRL(CLKCFG3), MSDC_CKGEN_SEL(MSDC2,CLKCFG3), clksrc);	  
			   //MSDC_SET_FIELD(TOPCKGEN_CTRL(CLKCFGUPDATE),MSDC_CKGEN_UPDATE(MSDC2), 1);
			}
			else if ( host->id == 1) {
			   MSDC_SET_FIELD(TOPCKGEN_CTRL(CLKCFG3), MSDC_CKGEN_SEL(MSDC1,CLKCFG3), clksrc);	  
			  // MSDC_SET_FIELD(TOPCKGEN_CTRL(CLKCFGUPDATE),MSDC_CKGEN_UPDATE(MSDC1), 1);
			}
			else {
			   MSDC_SET_FIELD(TOPCKGEN_CTRL(CLKCFG2), MSDC_CKGEN_SEL(MSDC0,CLKCFG2), clksrc);	  
			   //MSDC_SET_FIELD(TOPCKGEN_CTRL(CLKCFGUPDATE),MSDC_CKGEN_UPDATE(MSDC0), 1);
			}
#endif		
		MSDC_TRC_PRINT(MSDC_INIT,("[info][%s] pll_clk %u (%uMHz), pll_hclk %u\n", __func__, host->clksrc, host->clk/1000000, host->hclksrc));
}

void msdc_config_clock(struct mmc_host *host, int state, u32 hz)
{
    msdc_priv_t *priv = host->priv;
    u32 base = host->base;
    u32 mode, hs400_src = 0;
    u32 div;
    u32 sclk;
    //u32 orig_clksrc = host->clksrc;
	u32 u4buswidth=0;

    if (hz >= host->f_max) {
        hz = host->f_max;
    } 
	else if (hz < host->f_min) {
        hz = host->f_min;
    }
	
    msdc_config_clksrc(host, host->clksrc, host->hclksrc);
	
#if 0
    if (state&MMC_STATE_HS400) {
        mode = 0x3; /* HS400 mode */

#if !defined(FPGA_PLATFORM)
        if (host->id == 3) {
            msdc_src_clks = hclks_msdc50;
        }
        else {
            msdc_src_clks = hclks_msdc30;
        }
#endif

      #if (1 == MTK_HS400_USED_800M)
        host->clksrc = MSDC50_CLKSRC_800MHZ;
        host->clk = msdc_src_clks[MSDC50_CLKSRC_800MHZ];
        if (hz >= (host->clk >> 2)) {
            div  = 0;               /* mean div = 1/2 */
            sclk = host->clk >> 2; /* sclk = clk/div/2. 2: internal divisor */
        } else {
            div  = (host->clk + ((hz << 2) - 1)) / (hz << 2);
            sclk = (host->clk >> 2) / div;
            div  = (div >> 1);     /* since there is 1/2 internal divisor */
        }
      #else
        host->clksrc = MSDC50_CLKSRC_400MHZ;
        host->clk = msdc_src_clks[MSDC50_CLKSRC_400MHZ];
        sclk = host->clk >> 1; // use 400Mhz source
        div  = 0;
      #endif
    }
    else
#endif		
	if (state&MMC_STATE_DDR) {
        mode = 0x2; /* ddr mode and use divisor */
        if (hz >= (host->clk >> 2)) {
            div  = 0;			   /* mean div = 1/2 */
            sclk = host->clk >> 2; /* sclk = clk/div/2. 2: internal divisor */
        } else {
            div  = (host->clk + ((hz << 2) - 1)) / (hz << 2);
            sclk = (host->clk >> 2) / div;
            div  = (div >> 1);     /* since there is 1/2 internal divisor */
        }
    } else if (hz >= host->clk) {
        mode = 0x1; /* no divisor and divisor is ignored */
        div  = 0;
        sclk = host->clk; 
    } else {
        mode = 0x0; /* use divisor */
        if (hz >= (host->clk >> 1)) {
            div  = 0;			   /* mean div = 1/2 */
            sclk = host->clk >> 1; /* sclk = clk / 2 */
        } else {
            div  = (host->clk + ((hz << 2) - 1)) / (hz << 2);
            sclk = (host->clk >> 2) / div;
        }
    }
    host->sclk = sclk;

#if 0
    if (hz > 100000000) {
        MSDC_SET_FIELD(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKDRVN, msdc_cap.clk_18v_drv);
        MSDC_SET_FIELD(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKDRVP, msdc_cap.clk_18v_drv);
        MSDC_SET_FIELD(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDDRVN, msdc_cap.cmd_18v_drv);
        MSDC_SET_FIELD(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDDRVP, msdc_cap.cmd_18v_drv);
        MSDC_SET_FIELD(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATDRVN, msdc_cap.dat_18v_drv);
        MSDC_SET_FIELD(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATDRVP, msdc_cap.dat_18v_drv);
        /* don't enable cksel for ddr mode */
        if (host->card && mmc_card_ddr(host->card) == 0)
            MSDC_SET_FIELD(MSDC_PATCH_BIT0,MSDC_CKGEN_MSDC_CK_SEL, 1);
    }
#endif

    /* set clock mode and divisor */
    MSDC_SET_FIELD(MSDC_CFG, (MSDC_CFG_CKMOD_HS400 | MSDC_CFG_CKMOD |MSDC_CFG_CKDIV),\
                                                    (hs400_src << 14) | (mode << 12) | div);
    /* wait clock stable */
    while (!(MSDC_READ32(MSDC_CFG) & MSDC_CFG_CKSTB));

#if 10
    if (mmc_card_hs400(host->card)){
        msdc_set_smpl(host, 1, priv->rsmpl, TYPE_CMD_RESP_EDGE);
        msdc_set_smpl(host, 1, priv->rdsmpl, TYPE_READ_DATA_EDGE);
        msdc_set_smpl(host, 1, priv->wdsmpl, TYPE_WRITE_CRC_EDGE);
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS,0);		
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL,1);		
    } else {
        msdc_set_smpl(host, 0, priv->rsmpl, TYPE_CMD_RESP_EDGE);
        msdc_set_smpl(host, 0, priv->rdsmpl, TYPE_READ_DATA_EDGE);
        msdc_set_smpl(host, 0, priv->wdsmpl, TYPE_WRITE_CRC_EDGE);
#ifdef MSDC_USE_PATCH_BIT2_TURNING_WITH_ASYNC
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS,1);		
#else
        MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS,0);		
#endif
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL,0);		
    }
#endif	
	MSDC_GET_FIELD(SDC_CFG,SDC_CFG_BUSWIDTH,u4buswidth);

    MSDC_TRC_PRINT(MSDC_INIT,("[SD%d] SET_CLK(%dkHz): SCLK(%dkHz) MODE(%d) DIV(%d) DS(%d) RS(%d) buswidth(%s)\n",
        host->id, hz/1000, sclk/1000, mode, div,\
        msdc_cap[host->id].data_edge, msdc_cap[host->id].cmd_edge,\
	    (u4buswidth == 0) ? "1-bit" : (u4buswidth == 1)? "4-bits" : (u4buswidth == 2)? "8-bits" : "undefined"));
}

void msdc_config_bus(struct mmc_host *host, u32 width)
{
	u32 val,mode, div;
	u32 base = host->base;

	val = (width == HOST_BUS_WIDTH_8) ? 2 :
		      (width == HOST_BUS_WIDTH_4) ? 1 : 0;
			  
	MSDC_SET_FIELD(SDC_CFG, SDC_CFG_BUSWIDTH, val);	
	MSDC_GET_FIELD(MSDC_CFG,MSDC_CFG_CKMOD,mode);
	MSDC_GET_FIELD(MSDC_CFG,MSDC_CFG_CKDIV,div);

    MSDC_TRC_PRINT(MSDC_INFO,("[MSDC%d] CLK (%dMHz), SCLK(%dkHz) MODE(%d) DIV(%d) buswidth(%u-bits)\n",
        									host->id, host->clk/1000000, host->sclk/1000, mode, div, width));	
}

void msdc_config_pin(struct mmc_host *host, int mode)
{
    //u32 base = host->base;

    MSDC_TRC_PRINT(MSDC_GPIO, ("[SD%d] Pins mode(%d), none(0), down(1), up(2), keep(3)\n",host->id, mode));

    switch (mode) {
    case MSDC_PIN_PULL_UP:
		msdc_pin_set(host,MSDC_PST_PU_10KOHM);
        break;
    case MSDC_PIN_PULL_DOWN:
		msdc_pin_set(host,MSDC_PST_PD_50KOHM);
        break;
    case MSDC_PIN_PULL_NONE:
    default:
		msdc_pin_set(host,MSDC_PST_PD_50KOHM);
        break;
    }
}

#ifdef FEATURE_MMC_UHS1

int msdc_switch_volt(struct mmc_host *host, int volt)
{
    u32 base = host->base;
    int err = MMC_ERR_FAILED;
    u32 timeout = 1000;
    u32 status;
    u32 sclk = host->sclk;
	u32 u4swvolt = 0;

    /* make sure SDC is not busy (TBC) */
    WAIT_COND(!SDC_IS_BUSY(), timeout, timeout);
    if (timeout == 0) {
        err = MMC_ERR_TIMEOUT;
        goto out;
    }

    /* check if CMD/DATA lines both 0 */
    if ((MSDC_READ32(MSDC_PS) & ((1 << 24) | (0xF << 16))) == 0) {

        /* pull up disabled in CMD and DAT[3:0] */
        msdc_config_pin(host, MSDC_PIN_PULL_NONE);

        /* change signal from 3.3v to 1.8v */
        msdc_set_host_level_pwr(host, 1);

        /* wait at least 5ms for 1.8v signal switching in card */
        mdelay(10);

		if(sclk > 65000000){
			/* config clock to 260KHz~65MHz mode for volt switch detection by host. */
			msdc_config_clock(host, 0, MSDC_MIN_SCLK);
		}
		
		 u4swvolt = host->sclk/1000;
		 MSDC_SET_FIELD(SDC_VOL_CHG,SDC_VOL_CHG_VCHGCNT, u4swvolt);
		 MSDC_DBG_PRINT(MSDC_DEBUG, ("[%s] sd%d sclk %u SDC_VOL_CHG=0x%x\n", __func__, host->id, host->sclk, u4swvolt));


        /* pull up enabled in CMD and DAT[3:0] */
        msdc_config_pin(host, MSDC_PIN_PULL_UP);
        mdelay(5);

        /* start to detect volt change by providing 1.8v signal to card */
        MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_BV18SDT);

        /* wait at max. 1ms */
        mdelay(1);

        while ((status = MSDC_READ32(MSDC_CFG)) & MSDC_CFG_BV18SDT);

        if (status & MSDC_CFG_BV18PSS)
            err = MMC_ERR_NONE;

		if(sclk > 65000000){		
             /* config clock back to init clk freq. */
             msdc_config_clock(host, 0, sclk);
		}

    }   

out:
        
    return err;
}
#endif

void msdc_clock(struct mmc_host *host, int on)
{
#ifndef FPGA_PLATFORM
		switch ( host->id){
			case 3 :
				//MSDC_SET_FIELD(TOPCKGEN_CTRL(CLKCFG6) , MSDC_CKGENHCLK_PDN(MSDC3,CLKCFG6), (on ? 0 : 1));		
				MSDC_SET_FIELD(TOPCKGEN_CTRL(CLKCFG14), MSDC_CKGEN_PDN(MSDC3,CLKCFG14), (on ? 0 : 1));		
				break;	
			case 2 : 
				MSDC_SET_FIELD(TOPCKGEN_CTRL(CLKCFG3), MSDC_CKGEN_PDN(MSDC2,CLKCFG3), (on ? 0 : 1));	  
			case 1 :
				MSDC_SET_FIELD(TOPCKGEN_CTRL(CLKCFG3), MSDC_CKGEN_PDN(MSDC1,CLKCFG3), (on ? 0 : 1));	  
			   break;
			case 0 :
				MSDC_SET_FIELD(TOPCKGEN_CTRL(CLKCFG2), MSDC_CKGEN_PDN(MSDC0,CLKCFG2), (on ? 0 : 1));	  
			   break;
			default:
			   break;
		}
#endif
    MSDC_TRC_PRINT(MSDC_INFO, ("[SD%d] Turn %s %s clock \n", host->id, on ? "on" : "off", "host"));
}

void msdc_host_power(struct mmc_host *host, int on)
{
    MSDC_TRC_PRINT(MSDC_PMIC,("[SD%d] Turn %s %s power \n", host->id, on ? "on" : "off", "host"));

    if (on) {
        msdc_config_pin(host, MSDC_PIN_PULL_UP);      
        msdc_set_host_pwr(host, 1);
        msdc_clock(host, 1);
    } else {
        msdc_clock(host, 0);
        msdc_set_host_pwr(host, 0);
        msdc_config_pin(host, MSDC_PIN_PULL_DOWN);
    }
}

void msdc_card_power(struct mmc_host *host, int on)
{
    MSDC_TRC_PRINT(MSDC_PMIC,("[SD%d] Turn %s %s power \n", host->id, on ? "on" : "off", "card"));

    if (on) {
        msdc_set_card_pwr(host, 1);
    } else {
        msdc_set_card_pwr(host, 0);
    }
}

void msdc_power(struct mmc_host *host, u8 mode)
{
    if (mode == MMC_POWER_ON || mode == MMC_POWER_UP) {
        msdc_host_power(host, 1);
        msdc_card_power(host, 1);
    } else {
        msdc_card_power(host, 0);
        msdc_host_power(host, 0);
    }
}
void msdc_reset_tune_counter(struct mmc_host *host)
{
	host->time_read = 0;
}
#ifdef FEATURE_MMC_CM_TUNING
int msdc_tune_cmdrsp(struct mmc_host *host, struct mmc_command *cmd)
{
    u32 base = host->base;
	u32 sel = 0;
    u32 rsmpl,cur_rsmpl, orig_rsmpl;
	u32 rrdly,cur_rrdly, orig_rrdly;
	u32 cntr,cur_cntr,orig_cmdrtc;
    u32 dl_cksel, cur_dl_cksel, orig_dl_cksel;
    u32 times = 0;
    int result = MMC_ERR_CMDTUNEFAIL;
	u32 tmp = 0;
	u32 t_rrdly, t_rsmpl, t_dl_cksel,t_cmdrtc;

    if (host->sclk > 100000000){
		sel = 1;
		//MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_CKGEN_RX_SDCLKO_SEL,0);
		}
		
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, orig_rsmpl);
	MSDC_GET_FIELD(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, orig_rrdly);
	MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP, orig_cmdrtc);
    MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);

	 dl_cksel = 0;
        do {
			cntr = 0;
			do{
				rrdly = 0;
             do {
				 for (rsmpl = 0; rsmpl < 2; rsmpl++) {
					 cur_rsmpl = (orig_rsmpl + rsmpl) % 2;
					 MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, cur_rsmpl);
					 if(host->sclk <= 400000) {//In sd/emmc init flow, fix rising edge for latching cmd response
						 MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, 0);
					 }
					 if (cmd->opcode != MMC_CMD_STOP_TRANSMISSION) {
						 if(host->app_cmd){
							 result = msdc_app_cmd(host);
							 if(result != MMC_ERR_NONE)
								 return MMC_ERR_CMDTUNEFAIL;
						 }
						 result = msdc_send_cmd(host, cmd);
						 if(result == MMC_ERR_TIMEOUT)
							 rsmpl--;
						 if (result != MMC_ERR_NONE && cmd->opcode != MMC_CMD_STOP_TRANSMISSION){
							 tmp = MSDC_READ32(SDC_CMD);
							 /* check if data is used by the command or not */
							 if (tmp & 0x1800) {
								 if(msdc_abort_handler(host, 1))
									 MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] abort failed\n",host->id));
							 }
							 continue;
						 }
						 result = msdc_wait_rsp(host, cmd);                        
					 } else if(cmd->opcode == MMC_CMD_STOP_TRANSMISSION){
						 result = MMC_ERR_NONE;
						 goto done;
					 }
					 else
						 result = MMC_ERR_BADCRC;
					 
					 times++;
#ifdef ___MSDC_DEBUG___
					 /* for debugging */
						 MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, t_rsmpl);
						 MSDC_GET_FIELD(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, t_rrdly);
						 //MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_CKGEN_RX_SDCLKO_SEL, t_cksel);
						 MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP, t_cmdrtc);
						 MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL, t_dl_cksel);

						 MSDC_DBG_PRINT(MSDC_TUNING,("[SD%d] <TUNE_CMD><%d><%s> CMDRRDLY=%d, RSPL=%dh\n",
								 host->id, times, (result == MMC_ERR_NONE) ?"PASS" : "FAIL", t_rrdly, t_rsmpl));
						 //MSDC_DBG_PRINT(MSDC_TUNING,("[SD%d] <TUNE_CMD><%d><%s> MSDC_CKGEN_RX_SDCLKO_SEL=%xh\n",
						 //host->id, times, (result == MMC_ERR_NONE) ?"PASS" : "FAIL", t_cksel));
						 MSDC_DBG_PRINT(MSDC_TUNING,("[SD%d] <TUNE_CMD><%d><%s> CMD_RSP_TA_CNTR=%xh\n",
								 host->id, times, (result == MMC_ERR_NONE) ?"PASS" : "FAIL", t_cmdrtc));
						 MSDC_DBG_PRINT(MSDC_TUNING,("[SD%d] <TUNE_CMD><%d><%s> INT_DAT_LATCH_CK_SEL=%xh\n",
								 host->id, times, (result == MMC_ERR_NONE) ? "PASS" : "FAIL", t_dl_cksel));
#endif
					 if (result == MMC_ERR_NONE)                        
						 goto done;
					 /*if ((result == MMC_ERR_TIMEOUT || result == MMC_ERR_BADCRC)&&  cmd->opcode == MMC_CMD_STOP_TRANSMISSION){
					   printf("[SD%d]TUNE_CMD12--failed ignore\n",host->id);
					   result = MMC_ERR_NONE;
					   goto done;
					   }*/
					 tmp = MSDC_READ32(SDC_CMD);
					 /* check if data is used by the command or not */
					 if (tmp & 0x1800) {
						 if(msdc_abort_handler(host, 1))
							 MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] abort failed\n",host->id));
					 }
				 }    
                cur_rrdly = (orig_rrdly + rrdly + 1) % 32;
                MSDC_SET_FIELD(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, cur_rrdly);
            	} while (++rrdly < 32);
			 if(!sel)
			 	break;			 	
			 	cur_cntr = (orig_cmdrtc + cntr + 1) % 8;
                MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_CMD_RSP, cur_cntr);
        	}while(++cntr < 8);
            /* no need to update data ck sel */
            if (!sel)
                break;        
            cur_dl_cksel = (orig_dl_cksel +dl_cksel+1) % 8;
            MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL, cur_dl_cksel);
            dl_cksel++;                
        } while(dl_cksel < 8);
        /* no need to update ck sel */
		if(result != MMC_ERR_NONE)
			result = MMC_ERR_CMDTUNEFAIL;
done:  

	MSDC_TRC_PRINT(MSDC_TUNING,("[SD%d] <TUNE_CMD><%d><%s>\n",host->id, times, (result == MMC_ERR_NONE) ?"PASS" : "FAIL"));	
    return result;
}
#endif

#ifdef FEATURE_MMC_RD_TUNING
int msdc_tune_bread(struct mmc_host *host, u8 *dst, u32 src, u32 nblks)
{
    u32 base = host->base;
    u32 dcrc, ddr = 0, sel = 0;
    u32 cur_rxdly0, cur_rxdly1;
    u32 dsmpl, cur_dsmpl, orig_dsmpl;
	u32 dsel,cur_dsel,orig_dsel;
	u32 dl_cksel,cur_dl_cksel,orig_dl_cksel;
	u32 rxdly;
    u32 cur_dat0, cur_dat1, cur_dat2, cur_dat3, cur_dat4, cur_dat5, 
        cur_dat6, cur_dat7;
    u32 orig_dat0, orig_dat1, orig_dat2, orig_dat3, orig_dat4, orig_dat5, 
        orig_dat6, orig_dat7;
    u32 orig_clkmode;
    u32 times = 0;
	u32 t_dspl,  t_dl_cksel;
    //u32 t_cksel;
    int result = MMC_ERR_READTUNEFAIL;

    if (host->sclk > 100000000)
        sel = 1;
	if(host->card)
    	ddr = mmc_card_ddr(host->card);
	MSDC_GET_FIELD(MSDC_CFG,MSDC_CFG_CKMOD,orig_clkmode);
	//if(orig_clkmode == 1)
		//MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_CKGEN_RX_SDCLKO_SEL, 0);
	
	MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_CKGEN_MSDC_DLY_SEL, orig_dsel);
    MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
    MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_DSPL, orig_dsmpl);
	
    /* Tune Method 2. delay each data line */
    MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
 
        dl_cksel = 0;
        do {
			dsel = 0;
			do{
            rxdly = 0;
            do {
                for (dsmpl = 0; dsmpl < 2; dsmpl++) {
                    cur_dsmpl = (orig_dsmpl + dsmpl) % 2;
                    MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DSPL, cur_dsmpl);
                    result = host->blk_read(host, dst, src, nblks);
					if(result == MMC_ERR_CMDTUNEFAIL || result == MMC_ERR_CMD_RSPCRC || result == MMC_ERR_ACMD_RSPCRC)
						goto done;
                    MSDC_GET_FIELD(SDC_DCRC_STS, SDC_DCRC_STS_POS|SDC_DCRC_STS_NEG, dcrc);
					
                    if (!ddr) dcrc &= ~SDC_DCRC_STS_NEG;

                    /* for debugging */
					times++;
#ifdef ___MSDC_DEBUG___
                        MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_DSPL, t_dspl);
                        //MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_CKGEN_RX_SDCLKO_SEL, t_cksel);
                        MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL, t_dl_cksel);

                        MSDC_DBG_PRINT(MSDC_TUNING,("[SD%d] <TUNE_BREAD_%d><%s> DCRC=%xh\n",
                            host->id, times, (result == MMC_ERR_NONE && dcrc == 0) ?"PASS" : "FAIL", dcrc));
                        MSDC_DBG_PRINT(MSDC_TUNING,("[SD%d] <TUNE_BREAD_%d><%s> DATRDDLY0=%xh, DATRDDLY1=%xh\n",
                            host->id, times, (result == MMC_ERR_NONE && dcrc == 0) ?
                            "PASS" : "FAIL", MSDC_READ32(MSDC_DAT_RDDLY0), MSDC_READ32(MSDC_DAT_RDDLY1)));
                        //MSDC_DBG_PRINT(MSDC_TUNING,("[SD%d] <TUNE_BREAD_%d><%s> MSDC_CKGEN_RX_SDCLKO_SEL=%xh\n",
                            //host->id, times, (result == MMC_ERR_NONE && dcrc == 0) ?"PASS" : "FAIL", t_cksel));
                        MSDC_DBG_PRINT(MSDC_TUNING,("[SD%d] <TUNE_BREAD_%d><%s> INT_DAT_LATCH_CK_SEL=%xh, DSMPL=%xh\n",
                            host->id, times, (result == MMC_ERR_NONE && dcrc == 0) ?
                            "PASS" : "FAIL", t_dl_cksel, t_dspl));
#endif
                    /* no cre error in this data line */
                    if (result == MMC_ERR_NONE && dcrc == 0) {
                        goto done;
                    } else {
                        result = MMC_ERR_BADCRC;
                    }      
                }
                cur_rxdly0 = MSDC_READ32(MSDC_DAT_RDDLY0);
                cur_rxdly1 = MSDC_READ32(MSDC_DAT_RDDLY1);

                
                orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
                orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
                orig_dat2 = (cur_rxdly0 >> 8) & 0x1F;
                orig_dat3 = (cur_rxdly0 >> 0) & 0x1F;
                orig_dat4 = (cur_rxdly1 >> 24) & 0x1F;
                orig_dat5 = (cur_rxdly1 >> 16) & 0x1F;
                orig_dat6 = (cur_rxdly1 >> 8) & 0x1F;
                orig_dat7 = (cur_rxdly1 >> 0) & 0x1F;
                
                if (ddr) {
                    cur_dat0 = (dcrc & (1 << 0) || dcrc & (1 <<  8)) ? ((orig_dat0 + 1) % 32) : orig_dat0;
                    cur_dat1 = (dcrc & (1 << 1) || dcrc & (1 <<  9)) ? ((orig_dat1 + 1) % 32) : orig_dat1;
                    cur_dat2 = (dcrc & (1 << 2) || dcrc & (1 << 10)) ? ((orig_dat2 + 1) % 32) : orig_dat2;
                    cur_dat3 = (dcrc & (1 << 3) || dcrc & (1 << 11)) ? ((orig_dat3 + 1) % 32) : orig_dat3;
					cur_dat4 = (dcrc & (1 << 4) || dcrc & (1 << 12)) ? ((orig_dat4 + 1) % 32) : orig_dat4;
        		    cur_dat5 = (dcrc & (1 << 5) || dcrc & (1 << 13)) ? ((orig_dat5 + 1) % 32) : orig_dat5;
            		cur_dat6 = (dcrc & (1 << 6) || dcrc & (1 << 14)) ? ((orig_dat6 + 1) % 32) : orig_dat6;
            		cur_dat7 = (dcrc & (1 << 7) || dcrc & (1 << 15)) ? ((orig_dat7 + 1) % 32) : orig_dat7;
                } else {
                    cur_dat0 = (dcrc & (1 << 0)) ? ((orig_dat0 + 1) % 32) : orig_dat0;
                    cur_dat1 = (dcrc & (1 << 1)) ? ((orig_dat1 + 1) % 32) : orig_dat1;
                    cur_dat2 = (dcrc & (1 << 2)) ? ((orig_dat2 + 1) % 32) : orig_dat2;
                    cur_dat3 = (dcrc & (1 << 3)) ? ((orig_dat3 + 1) % 32) : orig_dat3;
					cur_dat4 = (dcrc & (1 << 4)) ? ((orig_dat4 + 1) % 32) : orig_dat4;
                	cur_dat5 = (dcrc & (1 << 5)) ? ((orig_dat5 + 1) % 32) : orig_dat5;
                	cur_dat6 = (dcrc & (1 << 6)) ? ((orig_dat6 + 1) % 32) : orig_dat6;
                	cur_dat7 = (dcrc & (1 << 7)) ? ((orig_dat7 + 1) % 32) : orig_dat7;
                }
              
                cur_rxdly0 = ((cur_dat0 & 0x1F) << 24) | ((cur_dat1 & 0x1F) << 16) |
                    ((cur_dat2 & 0x1F)<< 8) | ((cur_dat3 & 0x1F) << 0);
                cur_rxdly1 = ((cur_dat4 & 0x1F) << 24) | ((cur_dat5 & 0x1F) << 16) |
                    ((cur_dat6 & 0x1F) << 8) | ((cur_dat7 & 0x1F) << 0);

                MSDC_WRITE32(MSDC_DAT_RDDLY0, cur_rxdly0);
                MSDC_WRITE32(MSDC_DAT_RDDLY1, cur_rxdly1);            
            } while (++rxdly < 32);
			if(!sel)
				break;
				cur_dsel = (orig_dsel + dsel + 1) % 32;
            	MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_CKGEN_MSDC_DLY_SEL, cur_dsel);
        	}while(++dsel < 32);
            /* no need to update data ck sel */
            if (orig_clkmode != 1)
                break;
        
            cur_dl_cksel = (orig_dl_cksel + dl_cksel + 1)% 8;
            MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL, cur_dl_cksel);
            dl_cksel++;                
        } while(dl_cksel < 8);
done:
	MSDC_TRC_PRINT(MSDC_TUNING,("[SD%d] <msdc_tune_bread<%d><%s><cmd%d>@msdc_tune_bread\n",
			host->id, times, (result == MMC_ERR_NONE && dcrc == 0) ?"PASS" : "FAIL", (nblks == 1 ? 17 : 18)));

    return result;
}
#define READ_TUNING_MAX_HS (2 * 32)
#define READ_TUNING_MAX_UHS (2 * 32 * 32)
#define READ_TUNING_MAX_UHS_CLKMOD1 (2 * 32 * 32 *8)

int msdc_tune_read(struct mmc_host *host)
{
    u32 base = host->base;
    u32 dcrc, ddr = 0, sel = 0;
    u32 cur_rxdly0 = 0 , cur_rxdly1 = 0;
    u32 cur_dsmpl = 0, orig_dsmpl;
	u32 cur_dsel = 0,orig_dsel;
	u32 cur_dl_cksel = 0,orig_dl_cksel;
    u32 cur_dat0 = 0, cur_dat1 = 0, cur_dat2 = 0, cur_dat3 = 0, cur_dat4 = 0, cur_dat5 = 0, 
        cur_dat6 = 0, cur_dat7 = 0;
    u32 orig_dat0, orig_dat1, orig_dat2, orig_dat3, orig_dat4, orig_dat5, 
        orig_dat6, orig_dat7;
    u32 orig_clkmode;
    //u32 times = 0;
    int result = MMC_ERR_NONE;

    if (host->sclk > 100000000)
        sel = 1;
	if(host->card)
	    ddr = mmc_card_ddr(host->card);
	MSDC_GET_FIELD(MSDC_CFG,MSDC_CFG_CKMOD,orig_clkmode);
	//if(orig_clkmode == 1)
		//MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_CKGEN_RX_SDCLKO_SEL, 0);
	
	MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_CKGEN_MSDC_DLY_SEL, orig_dsel);
    MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
    MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_DSPL, orig_dsmpl);
	
    /* Tune Method 2. delay each data line */
    MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);

 
      cur_dsmpl = (orig_dsmpl + 1) ;
	  MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DSPL, cur_dsmpl % 2);                    
	  if(cur_dsmpl >= 2){
		MSDC_GET_FIELD(SDC_DCRC_STS, SDC_DCRC_STS_POS|SDC_DCRC_STS_NEG, dcrc);
 		if (!ddr) dcrc &= ~SDC_DCRC_STS_NEG;		
 		
 		        cur_rxdly0 = MSDC_READ32(MSDC_DAT_RDDLY0);
                cur_rxdly1 = MSDC_READ32(MSDC_DAT_RDDLY1);
                
                orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
                orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
                orig_dat2 = (cur_rxdly0 >> 8) & 0x1F;
                orig_dat3 = (cur_rxdly0 >> 0) & 0x1F;
                orig_dat4 = (cur_rxdly1 >> 24) & 0x1F;
                orig_dat5 = (cur_rxdly1 >> 16) & 0x1F;
                orig_dat6 = (cur_rxdly1 >> 8) & 0x1F;
                orig_dat7 = (cur_rxdly1 >> 0) & 0x1F;
                
                if (ddr) {
                    cur_dat0 = (dcrc & (1 << 0) || dcrc & (1 <<  8)) ? (orig_dat0 + 1) : orig_dat0;
                    cur_dat1 = (dcrc & (1 << 1) || dcrc & (1 <<  9)) ? (orig_dat1 + 1) : orig_dat1;
                    cur_dat2 = (dcrc & (1 << 2) || dcrc & (1 << 10)) ? (orig_dat2 + 1) : orig_dat2;
                    cur_dat3 = (dcrc & (1 << 3) || dcrc & (1 << 11)) ? (orig_dat3 + 1) : orig_dat3;					
					cur_dat4 = (dcrc & (1 << 4) || dcrc & (1 << 12)) ? (orig_dat4 + 1) : orig_dat4;
        		    cur_dat5 = (dcrc & (1 << 5) || dcrc & (1 << 13)) ? (orig_dat5 + 1) : orig_dat5;
            		cur_dat6 = (dcrc & (1 << 6) || dcrc & (1 << 14)) ? (orig_dat6 + 1) : orig_dat6;
            		cur_dat7 = (dcrc & (1 << 7) || dcrc & (1 << 15)) ? (orig_dat7 + 1) : orig_dat7;
                } else {
                    cur_dat0 = (dcrc & (1 << 0)) ? (orig_dat0 + 1) : orig_dat0;
                    cur_dat1 = (dcrc & (1 << 1)) ? (orig_dat1 + 1) : orig_dat1;
                    cur_dat2 = (dcrc & (1 << 2)) ? (orig_dat2 + 1) : orig_dat2;
                    cur_dat3 = (dcrc & (1 << 3)) ? (orig_dat3 + 1) : orig_dat3;
					cur_dat4 = (dcrc & (1 << 4)) ? (orig_dat4 + 1) : orig_dat4;
                	cur_dat5 = (dcrc & (1 << 5)) ? (orig_dat5 + 1) : orig_dat5;
                	cur_dat6 = (dcrc & (1 << 6)) ? (orig_dat6 + 1) : orig_dat6;
                	cur_dat7 = (dcrc & (1 << 7)) ? (orig_dat7 + 1) : orig_dat7;
                }                

                cur_rxdly0 = ((cur_dat0 & 0x1F) << 24) | ((cur_dat1 & 0x1F) << 16) |
                    ((cur_dat2 & 0x1F) << 8) | ((cur_dat3 & 0x1F) << 0);
                cur_rxdly1 = ((cur_dat4 & 0x1F) << 24) | ((cur_dat5 & 0x1F)<< 16) |
                    ((cur_dat6 & 0x1F) << 8) | ((cur_dat7 & 0x1F) << 0);

                MSDC_WRITE32(MSDC_DAT_RDDLY0, cur_rxdly0);
                MSDC_WRITE32(MSDC_DAT_RDDLY1, cur_rxdly1); 				
            }
	  	if(cur_dat0 >= 32 || cur_dat1 >= 32 || cur_dat2 >= 32 || cur_dat3 >= 32 ||
		   cur_dat4 >= 32 || cur_dat5 >= 32 || cur_dat6 >= 32 || cur_dat7 >= 32){
			if(sel){
				
				cur_dsel = (orig_dsel + 1);
           		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_CKGEN_MSDC_DLY_SEL, cur_dsel % 32);

				}			
		}
		if(cur_dsel >= 32){
			if(orig_clkmode == 1 && sel){	
				
				cur_dl_cksel = (orig_dl_cksel + 1);
            	MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL, cur_dl_cksel % 8);
			}               
		}
		++(host->time_read);
		if((sel == 1 && orig_clkmode == 1 && host->time_read == READ_TUNING_MAX_UHS_CLKMOD1)||
           (sel == 1 && orig_clkmode != 1 && host->time_read == READ_TUNING_MAX_UHS)||
           (sel == 0 && orig_clkmode != 1 && host->time_read == READ_TUNING_MAX_HS)){
           
			result = MMC_ERR_READTUNEFAIL;
		}
		
    return result;
}
#endif

#ifdef FEATURE_MMC_WR_TUNING
int msdc_tune_bwrite(struct mmc_host *host, u32 dst, u8 *src, u32 nblks)
{
    u32 base = host->base;
    u32 orig_clkmode;
    u32 sel = 0, ddrckdly = 0;
    u32 wrrdly, cur_wrrdly, orig_wrrdly;
    u32 dsmpl, cur_dsmpl, orig_dsmpl;
	u32 d_cntr,orig_d_cntr,cur_d_cntr;
    u32 rxdly, cur_rxdly0;
    u32 orig_dat0, orig_dat1, orig_dat2, orig_dat3;
    u32 cur_dat0, cur_dat1, cur_dat2, cur_dat3;
	
    u32 times = 0;
	u32 t_dspl, t_wrrdly, t_ddrdly, t_cksel,t_d_cntr;// t_dl_cksel;
    u32 status;
    int result = MMC_ERR_WRITETUNEFAIL;  
	

    if (host->sclk > 100000000)
        sel = 1;
		

    if (host->card && mmc_card_ddr(host->card))
        ddrckdly = 1;
	
	MSDC_GET_FIELD(MSDC_CFG,MSDC_CFG_CKMOD,orig_clkmode);
		//if(orig_clkmode == 1)
			//MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_CKGEN_RX_SDCLKO_SEL, 0);
		

    MSDC_GET_FIELD(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, orig_wrrdly);
    MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, orig_dsmpl);
    //MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_DDR50CKD, orig_ddrdly);
    //MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_CKGEN_RX_SDCLKO_SEL, orig_cksel);
	MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS, orig_d_cntr);
    //MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);

    /* Tune Method 2. delay data0 line */
    MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);

    cur_rxdly0 = MSDC_READ32(MSDC_DAT_RDDLY0);


    orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
    orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
    orig_dat2 = (cur_rxdly0 >> 8) & 0x1F;
    orig_dat3 = (cur_rxdly0 >> 0) & 0x1F;

            d_cntr = 0;
            do {
                rxdly = 0;
                do {
                    wrrdly = 0;
                    do {
                        for (dsmpl = 0; dsmpl < 2; dsmpl++) {
                            cur_dsmpl = (orig_dsmpl + dsmpl) % 2;
                            MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, cur_dsmpl);
							result = host->blk_write(host, dst, src, nblks);
							if(result == MMC_ERR_CMDTUNEFAIL || result == MMC_ERR_CMD_RSPCRC || result == MMC_ERR_ACMD_RSPCRC)
									goto done;

							times++;
                            /* for debugging */
#ifdef ___MSDC_DEBUG___
                            MSDC_GET_FIELD(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, t_wrrdly);
                            MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, t_dspl);
                            //MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_CKGEN_RX_SDCLKO_SEL, t_cksel);
							MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS, t_d_cntr);
                            //MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_INT_DAT_LATCH_CK_SEL, t_dl_cksel);


                            MSDC_DBG_PRINT(MSDC_TUNING,("[SD%d] <TUNE_BWRITE_%d><%s> DSPL=%d, DATWRDLY=%d\n",
                                host->id, times, result == MMC_ERR_NONE ? "PASS" : "FAIL",t_dspl, t_wrrdly));
							MSDC_DBG_PRINT(MSDC_TUNING,("[SD%d] <TUNE_BWRITE_%d><%s> MSDC_DAT_RDDLY0=%xh\n",
                                host->id, times, (result == MMC_ERR_NONE) ? "PASS" : "FAIL", MSDC_READ32(MSDC_DAT_RDDLY0)));
                           /* MSDC_DBG_PRINT(MSDC_TUNING,("[SD%d] <TUNE_BWRITE_%d><%s> DDR50CKD=%xh\n",
                                                                                            host->id, times, (result == MMC_ERR_NONE) ? "PASS" : "FAIL", t_ddrdly));*/
                            MSDC_DBG_PRINT(MSDC_TUNING,("[SD%d] <TUNE_BWRITE_%d><%s> MSDC_PATCH_BIT1_WRDAT_CRCS=%xh\n",
                                host->id, times, (result == MMC_ERR_NONE) ? "PASS" : "FAIL",t_d_cntr));
							 //MSDC_DBG_PRINT(MSDC_TUNING,("[SD%d] <TUNE_BWRITE_%d><%s> MSDC_CKGEN_RX_SDCLKO_SEL=%xh\n",
                                //host->id, times, (result == MMC_ERR_NONE) ? "PASS" : "FAIL", t_cksel));

#endif
                            if (result == MMC_ERR_NONE) {
                                goto done;
                            }
                        }
                        cur_wrrdly = ++orig_wrrdly % 32;
                        MSDC_SET_FIELD(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, cur_wrrdly);
                    } while (++wrrdly < 32);
                    
                    cur_dat0 = ++orig_dat0 % 32; /* only adjust bit-1 for crc */
                    cur_dat1 = orig_dat1;
                    cur_dat2 = orig_dat2;
                    cur_dat3 = orig_dat3;

                    cur_rxdly0 = (cur_dat0 << 24) | (cur_dat1 << 16) |
                        (cur_dat2 << 8) | (cur_dat3 << 0);
                    
                    MSDC_WRITE32(MSDC_DAT_RDDLY0, cur_rxdly0);
                } while (++rxdly < 32);

                /* no need to update data ck sel */
                if (!sel)
                    break;

                cur_d_cntr= (orig_d_cntr + d_cntr +1 )% 8;
                MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PATCH_BIT1_WRDAT_CRCS, cur_d_cntr);
                d_cntr++;                
            } while (d_cntr < 8);
done:
	MSDC_TRC_PRINT(MSDC_TUNING,("[SD%d] <TUNE_BWRITE_%d><%s>\n",host->id, times, result == MMC_ERR_NONE ? "PASS" : "FAIL"));

    return result;
}
#endif

#ifdef FEATURE_MMC_UHS1
int msdc_tune_uhs1(struct mmc_host *host, struct mmc_card *card)
{
    u32 base = host->base;
    u32 status;
    int i;
    int err = MMC_ERR_FAILED;
    struct mmc_command cmd;

    cmd.opcode  = SD_CMD_SEND_TUNING_BLOCK;
    cmd.arg     = 0;
    cmd.rsptyp  = RESP_R1;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = 0xFFFFFFFF;

    msdc_set_timeout(host, 100000000, 0);
    msdc_set_autocmd(host, MSDC_AUTOCMD19, 1);

    for (i = 0; i < 13; i++) {
        /* Note. select a pad to be tuned. msdc only tries 32 times to tune the 
         * pad since there is only 32 tuning steps for a pad.
         */
        MSDC_SET_FIELD(SDC_ACMD19_TRG, SDC_ACMD19_TRG_TUNESEL, i);

        /* Note. autocmd19 will only trigger done interrupt and won't trigger
         * autocmd timeout and crc error interrupt. (autocmd19 is a special command
         * and is different from autocmd12 and autocmd23.
         */
        err = msdc_cmd(host, &cmd);
        if (err != MMC_ERR_NONE)
            goto out;

        /* read and check acmd19 sts. bit-1: success, bit-0: fail */
        status = MSDC_READ32(SDC_ACMD19_STS);

        if (!status) {
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] ACMD19_TRG(%d), STS(0x%x) Failed\n", host->id, i,status));
            err = MMC_ERR_FAILED;
            goto out;
        }
    }
    err = MMC_ERR_NONE;
    
out:
    msdc_set_autocmd(host, MSDC_AUTOCMD19, 0);
    return err;
}
#endif

void msdc_card_detect(struct mmc_host *host, int on)
{
    u32 base = host->base;
    
    if ((msdc_cap[host->id].flags & MSDC_CD_PIN_EN) == 0) {
        MSDC_CARD_DETECTION_OFF();
        return;
    }

    if (on) {
        MSDC_SET_FIELD(MSDC_PS, MSDC_PS_CDDEBOUNCE, DEFAULT_DEBOUNCE);   
        MSDC_CARD_DETECTION_ON();
    } else {
        MSDC_CARD_DETECTION_OFF();
        MSDC_SET_FIELD(MSDC_PS, MSDC_PS_CDDEBOUNCE, 0);
    }
}

int msdc_card_avail(struct mmc_host *host)
{
    u32 base = host->base;
    unsigned int sts, avail = 0;

    if ((msdc_cap[host->id].flags & MSDC_REMOVABLE) == 0)
        return 1;

    if (msdc_cap[host->id].flags & MSDC_CD_PIN_EN) {
        MSDC_GET_FIELD(MSDC_PS, MSDC_PS_CDSTS, sts);
        avail = sts == 0 ? 1 : 0;        
    }

    return avail;
}

#if 1
int msdc_card_protected(struct mmc_host *host)
{
    u32 base = host->base;
    u32 prot;

    if (msdc_cap[host->id].flags & MSDC_WP_PIN_EN) {
        MSDC_GET_FIELD(MSDC_PS, MSDC_PS_WP, prot);
    } else {
        prot = 0;
    }

    return prot;
}

void msdc_hard_reset(struct mmc_host *host)
{
    msdc_set_card_pwr(host,0);
    msdc_set_card_pwr(host,1);
}
#endif

#ifdef FEATURE_MMC_BOOT_MODE
int msdc_emmc_boot_start(struct mmc_host *host, u32 hz, int state, int mode, int ackdis)
{
    int err = MMC_ERR_NONE;
    u32 sts;
    u32 base = host->base;
    u32 tmo = 0xFFFFFFFF;
    u32 acktmo, dattmo;

    /* configure msdc and clock frequency */    
    msdc_reset(host);
    msdc_clr_fifo(host);
    msdc_set_blklen(host, 512);
    msdc_config_bus(host, HOST_BUS_WIDTH_1);
    msdc_config_clock(host, state, hz);

    /* requires 74 clocks/1ms before CMD0 */
    MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_CKPDN);
    mdelay(2);
    MSDC_CLR_BIT32(MSDC_CFG, MSDC_CFG_CKPDN);

    /* configure boot timeout value */
    WAIT_COND(SDC_IS_BUSY() == 0, tmo, tmo);

    acktmo = msdc_cal_timeout(host, 50 * 1000 * 1000, 0, 256);   /* 50ms */
    dattmo = msdc_cal_timeout(host, 1000 * 1000 * 1000, 0, 256); /* 1sec */

    acktmo = acktmo > 0xFFF ? 0xFFF : acktmo;
    dattmo = dattmo > 0xFFFFF ? 0xFFFFF : dattmo;

    MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d] EMMC BOOT ACK timeout: %d ms (clkcnt: %d)\n", host->id, 
        (acktmo * 256) / (host->sclk / 1000), acktmo));
    MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d] EMMC BOOT DAT timeout: %d ms (clkcnt: %d)\n", host->id,
        (dattmo * 256) / (host->sclk / 1000), dattmo));
    
    MSDC_SET_BIT32(EMMC_CFG0, EMMC_CFG0_BOOTSUPP);
    MSDC_SET_FIELD(EMMC_CFG0, EMMC_CFG0_BOOTACKDIS, ackdis);
    MSDC_SET_FIELD(EMMC_CFG0, EMMC_CFG0_BOOTMODE, mode);
    MSDC_SET_FIELD(EMMC_CFG1, EMMC_CFG1_BOOTACKTMC, acktmo);
    MSDC_SET_FIELD(EMMC_CFG1, EMMC_CFG1_BOOTDATTMC, dattmo);

    if (mode == EMMC_BOOT_RST_CMD_MODE) {
        MSDC_WRITE32(SDC_ARG, 0xFFFFFFFA);
    } else {
        MSDC_WRITE32(SDC_ARG, 0);
    }
    MSDC_WRITE32(SDC_CMD, 0x02001000); /* multiple block read */
    MSDC_SET_BIT32(EMMC_CFG0, EMMC_CFG0_BOOTSTART);

    WAIT_COND((MSDC_READ32(EMMC_STS) & EMMC_STS_BOOTUPSTATE) == 
        EMMC_STS_BOOTUPSTATE, tmo, tmo);

    if (!ackdis) {
        do {
            sts = MSDC_READ32(EMMC_STS);
            if (sts == 0)
                continue;
            MSDC_WRITE32(EMMC_STS, sts);    /* write 1 to clear */
            if (sts & EMMC_STS_BOOTACKRCV) {
                MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] EMMC_STS(%x): boot ack received\n", host->id, sts));
                break;
            } else if (sts & EMMC_STS_BOOTACKERR) {
                MSDC_ERR_PRINT(MSDC_ERROR,("[%s]: [SD%d] EMMC_STS(0x%x): boot up ack error\n", __func__, host->id, sts));
                err = MMC_ERR_BADCRC;
                goto out;
            } else if (sts & EMMC_STS_BOOTACKTMO) {
                MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] EMMC_STS(%x): boot up ack timeout\n", host->id, sts));
                err = MMC_ERR_TIMEOUT;
                goto out;
            } else if (sts & EMMC_STS_BOOTUPSTATE) {
                MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] EMMC_STS(%x): boot up mode state\n", host->id, sts));
            } else {
                MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] EMMC_STS(%x): boot up unexpected\n", host->id, sts));
            }
        } while (1);
    }

    /* check if data received */
    do {
        sts = MSDC_READ32(EMMC_STS);
        if (sts == 0)
            continue;
        if (sts & EMMC_STS_BOOTDATRCV) {
            MSDC_ERR_PRINT(MSDC_ERROR,("[%s]: [SD%d] EMMC_STS(0x%x): boot dat received\n",  __func__,host->id, sts));
            break;
        }
        if (sts & EMMC_STS_BOOTCRCERR) {
            err = MMC_ERR_BADCRC;
            goto out;
        } else if (sts & EMMC_STS_BOOTDATTMO) {
            err = MMC_ERR_TIMEOUT;
            goto out;
        }
    } while(1);
out:
    return err;    
}

void msdc_emmc_boot_stop(struct mmc_host *host, int mode)
{
    u32 base = host->base;
    u32 tmo = 0xFFFFFFFF;

    /* Step5. stop the boot mode */
    MSDC_WRITE32(SDC_ARG, 0x00000000);
    MSDC_WRITE32(SDC_CMD, 0x00001000);

    MSDC_SET_FIELD(EMMC_CFG0, EMMC_CFG0_BOOTWDLY, 2);
    MSDC_SET_BIT32(EMMC_CFG0, EMMC_CFG0_BOOTSTOP);
    WAIT_COND((MSDC_READ32(EMMC_STS) & EMMC_STS_BOOTUPSTATE) == 0, tmo, tmo);
    
    /* Step6. */
    MSDC_CLR_BIT32(EMMC_CFG0, EMMC_CFG0_BOOTSUPP);

    /* Step7. clear EMMC_STS bits */
    MSDC_WRITE32(EMMC_STS, MSDC_READ32(EMMC_STS));
}

int msdc_emmc_boot_read(struct mmc_host *host, u32 size, u32 *to)
{
    int err = MMC_ERR_NONE;
    u32 sts;
    u32 totalsz = size;
    u32 base = host->base;
  
    while (size) {
        sts = MSDC_READ32(EMMC_STS);
        if (sts & EMMC_STS_BOOTCRCERR) {
            err = MMC_ERR_BADCRC;
            goto out;
        } else if (sts & EMMC_STS_BOOTDATTMO) {
            err = MMC_ERR_TIMEOUT;
            goto out;
        }
        /* Note. RXFIFO count would be aligned to 4-bytes alignment size */
        if ((size >=  MSDC_FIFO_THD) && (MSDC_RXFIFOCNT() >= MSDC_FIFO_THD)) {
            int left = MSDC_FIFO_THD >> 2;
            do {
                *to++ = MSDC_FIFO_READ32();
            } while (--left);
            size -= MSDC_FIFO_THD;
            MSDC_DBG_PRINT(MSDC_PIO,("[SD%d] Read %d bytes, RXFIFOCNT: %d,  Left: %d/%d\n",
                host->id, MSDC_FIFO_THD, MSDC_RXFIFOCNT(), size, totalsz));
        } else if ((size < MSDC_FIFO_THD) && MSDC_RXFIFOCNT() >= size) {
            while (size) {
                if (size > 3) {
                    *to++ = MSDC_FIFO_READ32();
                    size -= 4;
                } else {
                    u32 val = MSDC_FIFO_READ32();
                    memcpy(to, &val, size);
                    size = 0;
                }
            }
            MSDC_DBG_PRINT(MSDC_PIO,("[SD%d] Read left bytes, RXFIFOCNT: %d, Left: %d/%d\n",
                host->id, MSDC_RXFIFOCNT(), size, totalsz));
        }        
    }

out:
    if (err) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] EMMC_BOOT: read boot code fail(%d), FIFOCNT=%d\n", 
            host->id, err, MSDC_RXFIFOCNT()));
    }
    return err;
}

void msdc_emmc_boot_reset(struct mmc_host *host, int reset)
{
    u32 base = host->base;

    switch (reset) {
    case EMMC_BOOT_PWR_RESET:
        msdc_hard_reset(host);
        break;
    case EMMC_BOOT_RST_N_SIG:
        if (msdc_cap[host->id].flags & MSDC_RST_PIN_EN) {
            /* set n_reset pin to low */
            MSDC_SET_BIT32(EMMC_IOCON, EMMC_IOCON_BOOTRST);

            /* tRSTW (RST_n pulse width) at least 1us */
            mdelay(1);

            /* set n_reset pin to high */
            MSDC_CLR_BIT32(EMMC_IOCON, EMMC_IOCON_BOOTRST);
            MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_CKPDN);
            /* tRSCA (RST_n to command time) at least 200us, 
               tRSTH (RST_n high period) at least 1us */
            mdelay(1);
            MSDC_CLR_BIT32(MSDC_CFG, MSDC_CFG_CKPDN);
        }
        break;
    case EMMC_BOOT_PRE_IDLE_CMD:
        /* bring emmc to pre-idle mode by software reset command. (MMCv4.41)*/
        SDC_SEND_CMD(0x0, 0xF0F0F0F0);   
        break;
    }
}
#endif

int msdc_init(struct mmc_host *host, int id)
{
    u32 baddr[] = {MSDC0_BASE, MSDC1_BASE, MSDC2_BASE, MSDC3_BASE};
    u32 base = baddr[id];
#if MSDC_USE_DMA_MODE    
    gpd_t *gpd;
    __bd_t *bd;
#endif    
    msdc_priv_t *priv;
    struct dma_config *cfg;
    int clksrc=-1;

    MSDC_TRC_PRINT(MSDC_INIT,("[%s]: msdc%d Host controller intialization start \n", __func__, id));
    clksrc = (clksrc == -1) ? msdc_cap[id].clk_src : clksrc;

#if MSDC_USE_DMA_MODE
    gpd  = &msdc_gpd_pool[id][0];
	DMAGPDPTR_DBGCHK(gpd, 16);
	
    bd   = &msdc_bd_pool[id][0];
	DMABDPTR_DBGCHK(bd, 16);
#endif    
    priv = &msdc_priv[id];
    cfg  = &priv->cfg;


#if MSDC_USE_DMA_MODE
    memset(gpd, 0, sizeof(gpd_t) * MAX_GPD_POOL_SZ);
    memset(bd,  0, sizeof(__bd_t)  * MAX_BD_POOL_SZ);
#endif
    memset(priv, 0, sizeof(msdc_priv_t));

    host->id     = id;
    host->base   = base;
	host->clksrc = msdc_cap[id].clk_src;
	host->hclksrc= msdc_cap[id].hclk_src;	
#ifndef FPGA_PLATFORM
	host->f_max  = (host->id == 3) ? hclks_msdc50[host->clksrc] : hclks_msdc30[host->clksrc];
#else
	host->f_max  = MSDC_MAX_SCLK;
#endif

    host->f_min  = MSDC_MIN_SCLK;
    host->blkbits= MMC_BLOCK_BITS;
    host->blklen = 0;
    host->priv   = (void*)priv;
        
    host->caps   = MMC_CAP_MULTIWRITE;

    if (msdc_cap[id].flags & MSDC_HIGHSPEED)
        host->caps |= (MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED);
#ifdef FEATURE_MMC_UHS1
    if (msdc_cap[id].flags & MSDC_UHS1)
        host->caps |= MMC_CAP_SD_UHS1;
#endif
    if (msdc_cap[id].flags & MSDC_DDR)
        host->caps |= MMC_CAP_DDR;
    if (msdc_cap[id].data_pins == 4)
        host->caps |= MMC_CAP_4_BIT_DATA;
    if (msdc_cap[id].data_pins == 8)
        host->caps |= MMC_CAP_8_BIT_DATA | MMC_CAP_4_BIT_DATA;
    if (msdc_cap[id].flags & MSDC_HS200)
        host->caps |= MMC_CAP_EMMC_HS200;
    if (msdc_cap[id].flags & MSDC_HS400)
        host->caps |= MMC_CAP_EMMC_HS400;

    host->ocr_avail = MMC_VDD_32_33;  /* TODO: To be customized */

    host->max_hw_segs   = MAX_DMA_TRAN_SIZE/512;
    host->max_phys_segs = MAX_DMA_TRAN_SIZE/512;
    host->max_seg_size  = MAX_DMA_TRAN_SIZE;
    host->max_blk_size  = 2048;
    host->max_blk_count = 65535;
	host->app_cmd = 0;
	host->app_cmd_arg = 0;

    priv->rdsmpl       = msdc_cap[id].data_edge;
    priv->wdsmpl       = msdc_cap[id].data_edge;
    priv->rsmpl       = msdc_cap[id].cmd_edge;

#if MSDC_USE_DMA_MODE
    host->blk_read  = msdc_dma_bread;
    host->blk_write = msdc_dma_bwrite;
    cfg->mode       = MSDC_DMA_DEFAULT_MODE;
	MSDC_TRC_PRINT(MSDC_INFO, ("Transfer method: DMA (%u) \n", MSDC_DMA_DEFAULT_MODE));	
#else
    host->blk_read  = msdc_pio_bread;
    host->blk_write = msdc_pio_bwrite;
    cfg->mode       = MSDC_MODE_PIO;
	MSDC_TRC_PRINT(MSDC_INFO, ("Transfer method: PIO \n"));
#endif

#if MSDC_USE_DMA_MODE
    priv->alloc_bd    = 0;
    priv->alloc_gpd   = 0;
    priv->bd_pool     = bd;
    priv->gpd_pool    = gpd;
    priv->active_head = NULL;
    priv->active_tail = NULL;
#endif 
    priv->rdsmpl       = msdc_cap[id].data_edge;
    priv->rsmpl       = msdc_cap[id].cmd_edge;

#if MSDC_USE_DMA_MODE
    cfg->sg      = &priv->sg[0];
    cfg->burstsz = MSDC_BRUST_64B;
    cfg->flags   = DMA_FLAG_NONE;
#endif
    msdc_power(host, MMC_POWER_OFF);
    msdc_power(host, MMC_POWER_ON);

    /* set to SD/MMC mode */
    MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_MODE, MSDC_SDMMC);
    MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_PIO);

    msdc_reset(host);
    msdc_clr_fifo(host);
    MSDC_CLR_INT();

    /* reset tuning parameter */
    //MSDC_WRITE32(MSDC_PAD_CTL0, 0x0098000);
    //MSDC_WRITE32(MSDC_PAD_CTL1, 0x00A0000);
    //MSDC_WRITE32(MSDC_PAD_CTL2, 0x00A0000);
    MSDC_WRITE32(MSDC_PAD_TUNE, 0x0000000);
    MSDC_WRITE32(MSDC_DAT_RDDLY0, 0x00000000);
    MSDC_WRITE32(MSDC_DAT_RDDLY1, 0x00000000);
    MSDC_WRITE32(MSDC_IOCON, 0x00000000);
	
#if defined(MT8127)	
    //MSDC_WRITE32(MSDC_PATCH_BIT0, 0x003C004F);
	MSDC_WRITE32(MSDC_PATCH_BIT1, 0xFFFF00C9);//High 16 bit = 0 mean Power KPI is on, enable ECO for write timeout issue 
					//MSDC_PATCH_BIT1YD:WRDAT_CRCS_TA_CNTR need fix to 3'001 by default,(<50MHz) (>=50MHz set 3'001 as initial value is OK for tunning)
					//YD:CMD_RSP_TA_CNTR need fix to 3'001 by default(<50MHz)(>=50MHz set 3'001as initial value is OK for tunning)					
#endif
    /* 2012-01-07 using internal clock instead of feedback clock */
    //MSDC_SET_BIT32(MSDC_PATCH_BIT0, MSDC_CKGEN_MSDC_CK_SEL);

    /* enable SDIO mode. it's must otherwise sdio command failed */
    MSDC_SET_BIT32(SDC_CFG, SDC_CFG_SDIO);

#ifdef MSDC_USE_PATCH_BIT2_TURNING_WITH_ASYNC
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS,1);
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP,0);
#else
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS,0);
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP,1);
#endif

    /* enable wake up events */
    //MSDC_SET_BIT32(SDC_CFG, SDC_CFG_INSWKUP);

    /* eneable SMT for glitch filter */
	#ifdef FPGA_PLATFORM
	#if 0
    MSDC_SET_BIT32(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKSMT);
    MSDC_SET_BIT32(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDSMT);
    MSDC_SET_BIT32(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATSMT);
	#endif
	#else
	msdc_set_smt(host,1);
	#endif
    /* set clk, cmd, dat pad driving */
    msdc_set_driving(host, &msdc_cap[host->id], (host->cur_pwr == VOL_1800));

    /* set sampling edge */
    MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, msdc_cap[id].cmd_edge);
    MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DSPL, msdc_cap[id].data_edge);

    /* write crc timeout detection */
    MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_DETWR_CRCTMO, 1);

    msdc_set_startbit(host, START_AT_RISING);
    
    msdc_config_bus(host, HOST_BUS_WIDTH_1);
    msdc_config_clock(host, 0, MSDC_MIN_SCLK);
    /* disable sdio interrupt by default. sdio interrupt enable upon request */
    msdc_intr_unmask(host, 0x0001FF7B);
    msdc_set_timeout(host, 100000000, 0);
    msdc_card_detect(host, 1);

    MSDC_TRC_PRINT(MSDC_INIT,("[%s]: msdc%d Host controller intialization done\n", __func__, id));
    return 0;
}

int msdc_deinit(struct mmc_host *host)
{
    u32 base = host->base;

    msdc_card_detect(host, 0);
    msdc_intr_mask(host, 0x0001FFFB);
    msdc_reset(host);
    msdc_clr_fifo(host);
    MSDC_CLR_INT();
    msdc_power(host, MMC_POWER_OFF);

    return 0;
}

int msdc_preinit(u32 id, struct mmc_host *host)
{
	/* 3 msdc host under MT6582, HOST0 for emmc 1.8v, HOST1 for T-cade(1.8v & 3.3v), HOST2 for SDIO */
	u32 baddr[] = {MSDC0_BASE, MSDC1_BASE, MSDC2_BASE, MSDC3_BASE};
	u32 base = baddr[id];
	msdc_priv_t *priv;
	struct dma_config *cfg;
	u32 hz = MSDC_MIN_SCLK;
	u32 mode, hs400_src = 0;
	u32 div;
	u32 sclk;
	u32 bus;
	//u32 clksrc;
	u32 timeout = -1;
	int error = MMC_ERR_NONE;

	MSDC_BUGON(id < MSDC_MAX_NUM);
    MSDC_TRC_PRINT(MSDC_INIT,("[%s]: msdc%d Host controller intialization start \n", __func__, id));

	do
	{
		priv = &msdc_priv[id];
		cfg  = &priv->cfg;

		memset(priv, 0, sizeof(msdc_priv_t));

		host->id     = id;
		host->base   = base;
		host->clksrc  = msdc_cap[id].clk_src;
		host->hclksrc = msdc_cap[id].hclk_src;
#ifndef FPGA_PLATFORM
		host->f_max  = (host->id == 3) ? hclks_msdc50[host->clksrc] : hclks_msdc30[host->clksrc];
#else
		host->f_max  = MSDC_MAX_SCLK;
#endif
		host->f_min  = MSDC_MIN_SCLK;
		host->blkbits= MMC_BLOCK_BITS;
		host->blklen = 0;
		host->priv   = (void*)priv;
		host->caps   = MMC_CAP_MULTIWRITE;

		if (msdc_cap[id].flags & MSDC_HIGHSPEED)
			host->caps |= (MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED);
		if (msdc_cap[id].flags & MSDC_UHS1)
			host->caps |= MMC_CAP_SD_UHS1;
		if (msdc_cap[id].flags & MSDC_DDR)
			host->caps |= MMC_CAP_DDR;
		if (msdc_cap[id].data_pins == 4)
			host->caps |= MMC_CAP_4_BIT_DATA;
		if (msdc_cap[id].data_pins == 8)
			host->caps |= MMC_CAP_8_BIT_DATA | MMC_CAP_4_BIT_DATA;
		if (msdc_cap[id].flags & MSDC_HS200)
			host->caps |= MMC_CAP_EMMC_HS200;
		if (msdc_cap[id].flags & MSDC_HS400)
			host->caps |= MMC_CAP_EMMC_HS400;
		if(host->caps & MMC_CAP_EMMC_HS200)
			host->ocr_avail = 0xff8080;//MMC_VDD_17_18 | MMC_VDD_18_19;
		else
			host->ocr_avail = MMC_VDD_30_31 | MMC_VDD_33_34;  /* TODO: To be customized */

		host->max_hw_segs   = MAX_DMA_TRAN_SIZE/512;
		host->max_phys_segs = MAX_DMA_TRAN_SIZE/512;
		host->max_seg_size  = MAX_DMA_TRAN_SIZE;
		host->max_blk_size  = 2048;
		host->max_blk_count = 65535;
		host->app_cmd = 0;
		host->app_cmd_arg = 0;

		priv->alloc_bd    = 0;
		priv->alloc_gpd   = 0;
		priv->bd_pool     = NULL;
		priv->gpd_pool    = NULL;
		priv->active_head = NULL;
		priv->active_tail = NULL;
		priv->rdsmpl      = msdc_cap[id].data_edge;
		priv->wdsmpl      = msdc_cap[id].data_edge;
		priv->rsmpl       = msdc_cap[id].cmd_edge;

		cfg->sg      = &priv->sg[0];
		cfg->burstsz = MSDC_BRUST_64B;
		cfg->flags   = DMA_FLAG_NONE;
		cfg->mode    = MSDC_MODE_PIO;
		//cfg->inboot  = 0;
		//cfg->axi_enable         = 1;
		//cfg->axi_setlen         = 15;
		//cfg->axi_rdoutstanding  = 1;
		//cfg->axi_wroutstanding  = 1;


		host->blk_read	= msdc_pio_bread;
		host->blk_write = msdc_pio_bwrite;
		cfg->mode		= MSDC_MODE_PIO;

		msdc_power(host, MMC_POWER_OFF);
		msdc_power(host, MMC_POWER_ON);

		/* set to SD/MMC mode */
		MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_MODE, MSDC_SDMMC);
		MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_PIO);

		MSDC_RESET();
		MSDC_CLR_FIFO();
		MSDC_CLR_INT();

		/* reset tuning parameter */
		MSDC_WRITE32(MSDC_PAD_TUNE, 0x0000000);
		MSDC_WRITE32(MSDC_DAT_RDDLY0, 0x00000000);
		MSDC_WRITE32(MSDC_DAT_RDDLY1, 0x00000000);
		MSDC_WRITE32(MSDC_IOCON, 0x00000000);

		/* enable SDIO mode. it's must otherwise sdio command failed */
		MSDC_SET_BIT32(SDC_CFG, SDC_CFG_SDIO);

#ifndef MSDC_USE_PATCH_BIT2_TURNING_WITH_ASYNC
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS, 1);
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP, 0);
#else
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS, 0);
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP, 1);
#endif

#ifdef ACMD23_WITH_NEW_PATH
		MSDC_SET_FIELD(MSDC_PATCH_BIT0,MSDC_PB0_MSDC_BLKNUMSEL, 0);
#else
		MSDC_SET_FIELD(MSDC_PATCH_BIT0,MSDC_PB0_MSDC_BLKNUMSEL, 1);
#if (MSDC_USE_FORCE_FLUSH || MSDC_USE_RELIABLE_WRITE || MSDC_USE_DATA_TAG || MSDC_USE_PACKED_CMD)
#error   make sure undefined anyone MSDC_USE_FORCE_FLUSH MSDC_USE_RELIABLE_WRITE MSDC_USE_DATA_TAG MSDC_USE_PACKED_CMD
#endif
#endif

		/* enable wake up events */
		//MSDC_SET_BIT32(SDC_CFG, SDC_CFG_INSWKUP);

		/* eneable SMT for glitch filter */
		msdc_set_smt(host,1);
		/* set clk, cmd, dat pad driving */
		msdc_set_driving(host, &msdc_cap[host->id],(host->cur_pwr == VOL_1800));

		/* set sampling edge */
		MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, priv->rsmpl);
		MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, priv->rdsmpl);

		/* write crc timeout detection */
		//MSDC_SET_FIELD(MSDC_PATCH_BIT0, 1 << 30, 1);

		msdc_set_startbit(host, START_AT_RISING);
		msdc_config_clksrc(host, host->clksrc, host->hclksrc);
		MSDC_SET_FIELD(SDC_CFG, SDC_CFG_BUSWIDTH, MSDC_BUS_1BITS);
		//msdc_config_clock(host, 0, MSDC_MIN_SCLK);
		if (hz >= host->clk)
		{
#ifdef FPGA_PLATFORM
			mode = 0x0; /* no divisor and divisor is ignored  fpga don't support mode = 1*/
			div  = 0;
			sclk = host->clk >> 1;
#else
			mode = 0x1; /* no divisor and divisor is ignored */
			div  = 0;
			sclk = host->clk;
#endif
		}
		else
		{
			mode = 0x0; /* use divisor */
			if (hz >= (host->clk >> 1))
			{
				div  = 0;              /* mean div = 1/2 */
				sclk = host->clk >> 1; /* sclk = clk / 2 */
			}
			else
			{
				div  = (host->clk + ((hz << 2) - 1)) / (hz << 2);
				sclk = (host->clk >> 2) / div;
			}
		}
		host->sclk = sclk;

		if (hz > 100000000){
			/* enable smt */
			msdc_set_smt(host, 1);
		}

		/* set clock mode and divisor, need wait clock stable if modify clock or clock div */
		MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD | MSDC_CFG_CKDIV, (hs400_src << 14) | (mode << 12) | div);
	
		/* wait clock stable within 20ms*/
		WAIT_COND(((MSDC_READ32(MSDC_CFG) & MSDC_CFG_CKSTB)), 20, timeout);
		if (timeout == 0)
		{
			error = MMC_ERR_INVALID;
			break;
		}

		msdc_set_smpl(host, 0, priv->rsmpl, TYPE_CMD_RESP_EDGE);
		msdc_set_smpl(host, 0, priv->rdsmpl, TYPE_READ_DATA_EDGE);
		msdc_set_smpl(host, 0, priv->wdsmpl, TYPE_WRITE_CRC_EDGE);
		
#if (MSDC_USE_PATCH_BIT2_TURNING_PAD_DELAY == 1)
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS,0);
#else
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS,1);
#endif

		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL,0);

		/* disable sdio interrupt by default. sdio interrupt enable upon request */
		msdc_intr_unmask(host, MSDC_INTEN_DFT);
		//msdc_set_dmode(host, MSDC_MODE_PIO);
		//msdc_set_pio_bits(host, 32);
		msdc_set_timeout(host, 100000000, 0);
		msdc_card_detect(host, 1);

		MSDC_GET_FIELD(SDC_CFG,SDC_CFG_BUSWIDTH,bus);
		MSDC_TRC_PRINT(MSDC_INIT,("[SD%d] SET_CLK(%dkHz): SCLK(%dkHz) MODE(%d) DDR(0) DIV(%d) DS(%d) RS(%d),Bus(%s) %u ms\n",
			host->id, hz/1000, sclk/1000, mode,\
			div, priv->rdsmpl, priv->rsmpl,\
			(bus==0)? "1-bit" : (bus==1)? "4-bits" : (bus==3)? "8-bits" : "undefined", timeout));

	}
	while(0);


	return error;
}

