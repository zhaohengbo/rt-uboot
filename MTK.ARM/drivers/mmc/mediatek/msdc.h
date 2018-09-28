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
 
#ifndef _MSDC_H_
#define _MSDC_H_

#include "cust_msdc.h"
#include "msdc_debug.h"
#include "msdc_cfg.h"
#include "msdc_types.h"
#include "msdc_utils.h"
#include "mmc_core.h"
#include "mmc_test.h"

/*--------------------------------------------------------------------------*/
/* Common Macro                                                             */
/*--------------------------------------------------------------------------*/
#define REG_ADDR(x)             ((volatile uint32*)(base + OFFSET_##x))

/*--------------------------------------------------------------------------*/
/* Common Definition                                                        */
/*--------------------------------------------------------------------------*/
#define MSDC_FIFO_SZ            (128)
#define MSDC_FIFO_THD           (128)
//#define MSDC_MAX_NUM            (2)

#define MSDC_MS                 (0)
#define MSDC_SDMMC              (1)

#define MSDC_MODE_UNKNOWN       (0)
#define MSDC_MODE_PIO           (1)
#define MSDC_MODE_DMA_BASIC     (2)
#define MSDC_MODE_DMA_DESC      (3)
#define MSDC_MODE_DMA_ENHANCED  (4)
#define MSDC_MODE_MMC_STREAM    (5)

#define MSDC_BUS_1BITS          (0)
#define MSDC_BUS_4BITS          (1)
#define MSDC_BUS_8BITS          (2)

#define MSDC_BRUST_8B           (3)
#define MSDC_BRUST_16B          (4)
#define MSDC_BRUST_32B          (5)
#define MSDC_BRUST_64B          (6)

#define MSDC_PIN_PULL_NONE      (0)
#define MSDC_PIN_PULL_DOWN      (1)
#define MSDC_PIN_PULL_UP        (2)
#define MSDC_PIN_KEEP           (3)

#ifdef FPGA_PLATFORM
#define MSDC_OP_SCLK            (12000000)
#define MSDC_MAX_SCLK           MSDC_OP_SCLK
#else
#define MSDC_OP_SCLK            (200000000)
#define MSDC_MAX_SCLK           (400000000)
#endif

#define MSDC_MIN_SCLK           (260000)

#define MSDC_350K_SCLK          (350000)
#define MSDC_400K_SCLK          (400000)
#define MSDC_25M_SCLK           (25000000)
#define MSDC_26M_SCLK           (26000000)
#define MSDC_50M_SCLK           (50000000)
#define MSDC_52M_SCLK           (52000000)
#define MSDC_100M_SCLK          (100000000)
#define MSDC_179M_SCLK          (179000000)
#define MSDC_200M_SCLK          (200000000)
#define MSDC_208M_SCLK          (208000000)
#define MSDC_400M_SCLK          (400000000)
#define MSDC_800M_SCLK          (800000000)

#define MSDC_AUTOCMD12          (0x0001)
#define MSDC_AUTOCMD23          (0x0002)
#define MSDC_AUTOCMD19          (0x0003)

#define TYPE_CMD_RESP_EDGE      (0)
#define TYPE_WRITE_CRC_EDGE     (1)
#define TYPE_READ_DATA_EDGE     (2)
#define TYPE_WRITE_DATA_EDGE    (3)

#define START_AT_RISING                 (0x0)
#define START_AT_FALLING                (0x1)
#define START_AT_RISING_AND_FALLING     (0x2)
#define START_AT_RISING_OR_FALLING      (0x3)

/*--------------------------------------------------------------------------*/
/* Register Offset                                                          */
/*--------------------------------------------------------------------------*/
#define OFFSET_MSDC_CFG                  (0x00)
#define OFFSET_MSDC_IOCON                (0x04)
#define OFFSET_MSDC_PS                   (0x08)
#define OFFSET_MSDC_INT                  (0x0c)
#define OFFSET_MSDC_INTEN                (0x10)
#define OFFSET_MSDC_FIFOCS               (0x14)
#define OFFSET_MSDC_TXDATA               (0x18)
#define OFFSET_MSDC_RXDATA               (0x1c)
#define OFFSET_SDC_CFG                   (0x30)
#define OFFSET_SDC_CMD                   (0x34)
#define OFFSET_SDC_ARG                   (0x38)
#define OFFSET_SDC_STS                   (0x3c)
#define OFFSET_SDC_RESP0                 (0x40)
#define OFFSET_SDC_RESP1                 (0x44)
#define OFFSET_SDC_RESP2                 (0x48)
#define OFFSET_SDC_RESP3                 (0x4c)
#define OFFSET_SDC_BLK_NUM               (0x50)
#define OFFSET_SDC_VOL_CHG               (0x54)
#define OFFSET_SDC_CSTS                  (0x58)
#define OFFSET_SDC_CSTS_EN               (0x5c)
#define OFFSET_SDC_DCRC_STS              (0x60)

/* Only for EMMC Controller 4 registers below */
#define OFFSET_EMMC_CFG0                 (0x70)
#define OFFSET_EMMC_CFG1                 (0x74)
#define OFFSET_EMMC_STS                  (0x78)
#define OFFSET_EMMC_IOCON                (0x7c)

#define OFFSET_SDC_ACMD_RESP             (0x80)
#define OFFSET_SDC_ACMD19_TRG            (0x84)
#define OFFSET_SDC_ACMD19_STS            (0x88)
#define OFFSET_MSDC_DMA_HIGH4BIT         (0x8C)
#define OFFSET_MSDC_DMA_SA               (0x90)
#define OFFSET_MSDC_DMA_CA               (0x94)
#define OFFSET_MSDC_DMA_CTRL             (0x98)
#define OFFSET_MSDC_DMA_CFG              (0x9c)
#define OFFSET_MSDC_DBG_SEL              (0xa0)
#define OFFSET_MSDC_DBG_OUT              (0xa4)
#define OFFSET_MSDC_DMA_LEN              (0xa8)
#define OFFSET_MSDC_PATCH_BIT0           (0xb0)
#define OFFSET_MSDC_PATCH_BIT1           (0xb4)
#define OFFSET_MSDC_PATCH_BIT2           (0xb8)

/* Only for SD/SDIO Controller 6 registers below */
#define OFFSET_DAT0_TUNE_CRC             (0xc0)
#define OFFSET_DAT1_TUNE_CRC             (0xc4)
#define OFFSET_DAT2_TUNE_CRC             (0xc8)
#define OFFSET_DAT3_TUNE_CRC             (0xcc)
#define OFFSET_CMD_TUNE_CRC              (0xd0)
#define OFFSET_SDIO_TUNE_WIND            (0xd4)

#define OFFSET_MSDC_PAD_TUNE             (0xF0)
#define OFFSET_MSDC_PAD_TUNE0            (0xF0)
#define OFFSET_MSDC_PAD_TUNE1            (0xF4)
#define OFFSET_MSDC_DAT_RDDLY0           (0xF8)
#define OFFSET_MSDC_DAT_RDDLY1           (0xFC)

#define OFFSET_MSDC_DAT_RDDLY2           (0x100)
#define OFFSET_MSDC_DAT_RDDLY3           (0x104)

#define OFFSET_MSDC_HW_DBG               (0x110)
#define OFFSET_MSDC_VERSION              (0x114)
#define OFFSET_MSDC_ECO_VER              (0x118)

/* Only for EMMC 5.0 Controller 4 registers below */
#define OFFSET_EMMC50_PAD_CTL0           (0x180)
#define OFFSET_EMMC50_PAD_DS_CTL0        (0x184)
#define OFFSET_EMMC50_PAD_DS_TUNE        (0x188)
#define OFFSET_EMMC50_PAD_CMD_TUNE       (0x18c)
#define OFFSET_EMMC50_PAD_DAT01_TUNE     (0x190)
#define OFFSET_EMMC50_PAD_DAT23_TUNE     (0x194)
#define OFFSET_EMMC50_PAD_DAT45_TUNE     (0x198)
#define OFFSET_EMMC50_PAD_DAT67_TUNE     (0x19c)
#define OFFSET_EMMC51_CFG0               (0x204)
#define OFFSET_EMMC50_CFG0               (0x208)
#define OFFSET_EMMC50_CFG1               (0x20c)
#define OFFSET_EMMC50_CFG2               (0x21c)
#define OFFSET_EMMC50_CFG3               (0x220)
#define OFFSET_EMMC50_CFG4               (0x224)
#define OFFSET_EMMC50_BLOCK_LENGTH       (0x228)
/*--------------------------------------------------------------------------*/
/* Register Address                                                         */
/*--------------------------------------------------------------------------*/
/* common register */
#define MSDC_CFG                         REG_ADDR(MSDC_CFG)
#define MSDC_IOCON                       REG_ADDR(MSDC_IOCON)
#define MSDC_PS                          REG_ADDR(MSDC_PS)
#define MSDC_INT                         REG_ADDR(MSDC_INT)
#define MSDC_INTEN                       REG_ADDR(MSDC_INTEN)
#define MSDC_FIFOCS                      REG_ADDR(MSDC_FIFOCS)
#define MSDC_TXDATA                      REG_ADDR(MSDC_TXDATA)
#define MSDC_RXDATA                      REG_ADDR(MSDC_RXDATA)

/* sdmmc register */
#define SDC_CFG                          REG_ADDR(SDC_CFG)
#define SDC_CMD                          REG_ADDR(SDC_CMD)
#define SDC_ARG                          REG_ADDR(SDC_ARG)
#define SDC_STS                          REG_ADDR(SDC_STS)
#define SDC_RESP0                        REG_ADDR(SDC_RESP0)
#define SDC_RESP1                        REG_ADDR(SDC_RESP1)
#define SDC_RESP2                        REG_ADDR(SDC_RESP2)
#define SDC_RESP3                        REG_ADDR(SDC_RESP3)
#define SDC_BLK_NUM                      REG_ADDR(SDC_BLK_NUM)
#define SDC_VOL_CHG                      REG_ADDR(SDC_VOL_CHG)
#define SDC_CSTS                         REG_ADDR(SDC_CSTS)
#define SDC_CSTS_EN                      REG_ADDR(SDC_CSTS_EN)
#define SDC_DCRC_STS                     REG_ADDR(SDC_DCRC_STS)

/* emmc register*/
#define EMMC_CFG0                        REG_ADDR(EMMC_CFG0)
#define EMMC_CFG1                        REG_ADDR(EMMC_CFG1)
#define EMMC_STS                         REG_ADDR(EMMC_STS)
#define EMMC_IOCON                       REG_ADDR(EMMC_IOCON)

/* auto command register */
#define SDC_ACMD_RESP                    REG_ADDR(SDC_ACMD_RESP)
#define SDC_ACMD19_TRG                   REG_ADDR(SDC_ACMD19_TRG)
#define SDC_ACMD19_STS                   REG_ADDR(SDC_ACMD19_STS)

/* dma register */
#define MSDC_DMA_HIGH4BIT                REG_ADDR(MSDC_DMA_HIGH4BIT)
#define MSDC_DMA_SA                      REG_ADDR(MSDC_DMA_SA)
#define MSDC_DMA_CA                      REG_ADDR(MSDC_DMA_CA)
#define MSDC_DMA_CTRL                    REG_ADDR(MSDC_DMA_CTRL)
#define MSDC_DMA_CFG                     REG_ADDR(MSDC_DMA_CFG)

/* debug register */
#define MSDC_DBG_SEL                     REG_ADDR(MSDC_DBG_SEL)
#define MSDC_DBG_OUT                     REG_ADDR(MSDC_DBG_OUT)
#define MSDC_DMA_LEN                     REG_ADDR(MSDC_DMA_LEN)

/* misc register */
#define MSDC_PATCH_BIT0                  REG_ADDR(MSDC_PATCH_BIT0)
#define MSDC_PATCH_BIT1                  REG_ADDR(MSDC_PATCH_BIT1)
#define MSDC_PATCH_BIT2                  REG_ADDR(MSDC_PATCH_BIT2)
#define DAT0_TUNE_CRC                    REG_ADDR(DAT0_TUNE_CRC)
#define DAT1_TUNE_CRC                    REG_ADDR(DAT1_TUNE_CRC)
#define DAT2_TUNE_CRC                    REG_ADDR(DAT2_TUNE_CRC)
#define DAT3_TUNE_CRC                    REG_ADDR(DAT3_TUNE_CRC)
#define CMD_TUNE_CRC                     REG_ADDR(CMD_TUNE_CRC)
#define SDIO_TUNE_WIND                   REG_ADDR(SDIO_TUNE_WIND)
#define MSDC_PAD_TUNE                    REG_ADDR(MSDC_PAD_TUNE)
#define MSDC_PAD_TUNE0                   REG_ADDR(MSDC_PAD_TUNE0)
#define MSDC_PAD_TUNE1                   REG_ADDR(MSDC_PAD_TUNE1)

/* data read delay */
#define MSDC_DAT_RDDLY0                  REG_ADDR(MSDC_DAT_RDDLY0)
#define MSDC_DAT_RDDLY1                  REG_ADDR(MSDC_DAT_RDDLY1)
#define MSDC_DAT_RDDLY2                  REG_ADDR(MSDC_DAT_RDDLY2)
#define MSDC_DAT_RDDLY3                  REG_ADDR(MSDC_DAT_RDDLY3)

#define MSDC_HW_DBG                      REG_ADDR(MSDC_HW_DBG)
#define MSDC_VERSION                     REG_ADDR(MSDC_VERSION)
#define MSDC_ECO_VER                     REG_ADDR(MSDC_ECO_VER)
/* eMMC 5.0 register */
#define EMMC50_PAD_CTL0                  REG_ADDR(EMMC50_PAD_CTL0)
#define EMMC50_PAD_DS_CTL0               REG_ADDR(EMMC50_PAD_DS_CTL0)
#define EMMC50_PAD_DS_TUNE               REG_ADDR(EMMC50_PAD_DS_TUNE)
#define EMMC50_PAD_CMD_TUNE              REG_ADDR(EMMC50_PAD_CMD_TUNE)
#define EMMC50_PAD_DAT01_TUNE            REG_ADDR(EMMC50_PAD_DAT01_TUNE)
#define EMMC50_PAD_DAT23_TUNE            REG_ADDR(EMMC50_PAD_DAT23_TUNE)
#define EMMC50_PAD_DAT45_TUNE            REG_ADDR(EMMC50_PAD_DAT45_TUNE)
#define EMMC50_PAD_DAT67_TUNE            REG_ADDR(EMMC50_PAD_DAT67_TUNE)
#define EMMC51_CFG0                      REG_ADDR(EMMC51_CFG0)
#define EMMC50_CFG0                      REG_ADDR(EMMC50_CFG0)
#define EMMC50_CFG1                      REG_ADDR(EMMC50_CFG1)
#define EMMC50_CFG2                      REG_ADDR(EMMC50_CFG2)
#define EMMC50_CFG3                      REG_ADDR(EMMC50_CFG3)
#define EMMC50_CFG4                      REG_ADDR(EMMC50_CFG4)
#define EMMC50_BLOCK_LENGTH              REG_ADDR(EMMC50_BLOCK_LENGTH)

/*--------------------------------------------------------------------------*/
/* Register Mask                                                            */
/*--------------------------------------------------------------------------*/

/* MSDC_CFG mask */
#define MSDC_CFG_MODE           (0x1   <<  0)     /* RW */
#define MSDC_CFG_CKPDN          (0x1   <<  1)     /* RW */
#define MSDC_CFG_RST            (0x1   <<  2)     /* A0 */
#define MSDC_CFG_PIO            (0x1   <<  3)     /* RW */
#define MSDC_CFG_CKDRVEN        (0x1   <<  4)     /* RW */
#define MSDC_CFG_BV18SDT        (0x1   <<  5)     /* RW */
#define MSDC_CFG_BV18PSS        (0x1   <<  6)     /* R  */
#define MSDC_CFG_CKSTB          (0x1   <<  7)     /* R  */
#define MSDC_CFG_CKDIV          (0xFFF <<  8)     /* RW   !!! MT7623 change 0xFF ->0xFFF*/
#define MSDC_CFG_CKMOD          (0x3   << 20)     /* W1C  !!! MT7623 change 16 ->21 only for eMCC 5.0*/
#define MSDC_CFG_CKMOD_HS400    (0x1   << 22)     /* RW   !!! MT7623 change 18 ->22 only for eMCC 5.0*/
#define MSDC_CFG_START_BIT      (0x3   << 23)     /* RW   !!! MT7623 change 19 ->23 only for eMCC 5.0*/
#define MSDC_CFG_SCLK_STOP_DDR  (0x1   << 25)     /* RW   !!! MT7623 change 21 ->25 */

/* MSDC_IOCON mask */
#define MSDC_IOCON_SDR104CKS    (0x1   <<  0)     /* RW */
#define MSDC_IOCON_RSPL         (0x1   <<  1)     /* RW */
#define MSDC_IOCON_R_D_SMPL     (0x1   <<  2)     /* RW */
#define MSDC_IOCON_DSPL          MSDC_IOCON_R_D_SMPL /* alias */

#define MSDC_IOCON_DDLSEL       (0x1   <<  3)     /* RW */
#define MSDC_IOCON_DDR50CKD     (0x1   <<  4)     /* RW */
#define MSDC_IOCON_R_D_SMPL_SEL (0x1   <<  5)     /* RW */
#define MSDC_IOCON_W_D_SMPL     (0x1   <<  8)     /* RW */
#define MSDC_IOCON_W_D_SMPL_SEL (0x1   <<  9)     /* RW */
#define MSDC_IOCON_W_D0SPL      (0x1   << 10)     /* RW */
#define MSDC_IOCON_W_D1SPL      (0x1   << 11)     /* RW */
#define MSDC_IOCON_W_D2SPL      (0x1   << 12)     /* RW */
#define MSDC_IOCON_W_D3SPL      (0x1   << 13)     /* RW */

#define MSDC_IOCON_R_D0SPL      (0x1   << 16)     /* RW */
#define MSDC_IOCON_R_D1SPL      (0x1   << 17)     /* RW */
#define MSDC_IOCON_R_D2SPL      (0x1   << 18)     /* RW */
#define MSDC_IOCON_R_D3SPL      (0x1   << 19)     /* RW */
#define MSDC_IOCON_R_D4SPL      (0x1   << 20)     /* RW */
#define MSDC_IOCON_R_D5SPL      (0x1   << 21)     /* RW */
#define MSDC_IOCON_R_D6SPL      (0x1   << 22)     /* RW */
#define MSDC_IOCON_R_D7SPL      (0x1   << 23)     /* RW */
//#define MSDC_IOCON_RISCSZ       (0x3   << 24)     /* RW  !!! MT7623  remove*/

/* MSDC_PS mask */
#define MSDC_PS_CDEN            (0x1   <<  0)     /* RW */
#define MSDC_PS_CDSTS           (0x1   <<  1)     /* RU  */

#define MSDC_PS_CDDEBOUNCE      (0xF   << 12)     /* RW */
#define MSDC_PS_DAT             (0xFF  << 16)     /* RU */
#define MSDC_PS_DAT8PIN         (0xFF  << 16)     /* RU */
#define MSDC_PS_DAT4PIN         (0xF   << 16)     /* RU */
#define MSDC_PS_DAT0            (0x1   << 16)     /* RU */

#define MSDC_PS_CMD             (0x1   << 24)     /* RU */

#define MSDC_PS_WP              (0x1   << 31)     /* RU  */

/* MSDC_INT mask */
#define MSDC_INT_MMCIRQ         (0x1   <<  0)     /* W1C */
#define MSDC_INT_CDSC           (0x1   <<  1)     /* W1C */

#define MSDC_INT_ACMDRDY        (0x1   <<  3)     /* W1C */
#define MSDC_INT_ACMDTMO        (0x1   <<  4)     /* W1C */
#define MSDC_INT_ACMDCRCERR     (0x1   <<  5)     /* W1C */
#define MSDC_INT_DMAQ_EMPTY     (0x1   <<  6)     /* W1C */
#define MSDC_INT_SDIOIRQ        (0x1   <<  7)     /* W1C Only for SD/SDIO */
#define MSDC_INT_CMDRDY         (0x1   <<  8)     /* W1C */
#define MSDC_INT_CMDTMO         (0x1   <<  9)     /* W1C */
#define MSDC_INT_RSPCRCERR      (0x1   << 10)     /* W1C */
#define MSDC_INT_CSTA           (0x1   << 11)     /* R */
#define MSDC_INT_XFER_COMPL     (0x1   << 12)     /* W1C */
#define MSDC_INT_DXFER_DONE     (0x1   << 13)     /* W1C */
#define MSDC_INT_DATTMO         (0x1   << 14)     /* W1C */
#define MSDC_INT_DATCRCERR      (0x1   << 15)     /* W1C */
#define MSDC_INT_ACMD19_DONE    (0x1   << 16)     /* W1C */
#define MSDC_INT_BDCSERR        (0x1   << 17)     /* W1C */
#define MSDC_INT_GPDCSERR       (0x1   << 18)     /* W1C */
#define MSDC_INT_DMAPRO         (0x1   << 19)     /* W1C */
#define MSDC_INT_GOBOUND        (0x1   << 20)     /* W1C Only for SD/SDIO ACMD 53*/
#define MSDC_INT_ACMD53_DONE    (0x1   << 21)     /* W1C Only for SD/SDIO ACMD 53*/
#define MSDC_INT_ACMD53_FAIL    (0x1   << 22)     /* W1C Only for SD/SDIO ACMD 53*/
#define MSDC_INT_AXI_RESP_ERR   (0x1   << 23)     /* W1C Only for eMMC 5.0*/

/* MSDC_INTEN mask */
#define MSDC_INTEN_MMCIRQ       (0x1   <<  0)     /* RW */
#define MSDC_INTEN_CDSC         (0x1   <<  1)     /* RW */

#define MSDC_INTEN_ACMDRDY      (0x1   <<  3)     /* RW */
#define MSDC_INTEN_ACMDTMO      (0x1   <<  4)     /* RW */
#define MSDC_INTEN_ACMDCRCERR   (0x1   <<  5)     /* RW */
#define MSDC_INTEN_DMAQ_EMPTY   (0x1   <<  6)     /* RW */
#define MSDC_INTEN_SDIOIRQ      (0x1   <<  7)     /* RW Only for SDIO*/
#define MSDC_INTEN_CMDRDY       (0x1   <<  8)     /* RW */
#define MSDC_INTEN_CMDTMO       (0x1   <<  9)     /* RW */
#define MSDC_INTEN_RSPCRCERR    (0x1   << 10)     /* RW */
#define MSDC_INTEN_CSTA         (0x1   << 11)     /* RW */
#define MSDC_INTEN_XFER_COMPL   (0x1   << 12)     /* RW */
#define MSDC_INTEN_DXFER_DONE   (0x1   << 13)     /* RW */
#define MSDC_INTEN_DATTMO       (0x1   << 14)     /* RW */
#define MSDC_INTEN_DATCRCERR    (0x1   << 15)     /* RW */
#define MSDC_INTEN_ACMD19_DONE  (0x1   << 16)     /* RW */
#define MSDC_INTEN_BDCSERR      (0x1   << 17)     /* RW */
#define MSDC_INTEN_GPDCSERR     (0x1   << 18)     /* RW */
#define MSDC_INTEN_DMAPRO       (0x1   << 19)     /* RW */
#define MSDC_INTEN_GOBOUND      (0x1   << 20)     /* RW  Only for SD/SDIO ACMD 53*/
#define MSDC_INTEN_ACMD53_DONE  (0x1   << 21)     /* RW  Only for SD/SDIO ACMD 53*/
#define MSDC_INTEN_ACMD53_FAIL  (0x1   << 22)     /* RW  Only for SD/SDIO ACMD 53*/
#define MSDC_INTEN_AXI_RESP_ERR (0x1   << 23)     /* RW  Only for eMMC 5.0*/

#define MSDC_INTEN_DFT       (  MSDC_INTEN_MMCIRQ        |MSDC_INTEN_CDSC        | MSDC_INTEN_ACMDRDY\
                                |MSDC_INTEN_ACMDTMO      |MSDC_INTEN_ACMDCRCERR  | MSDC_INTEN_DMAQ_EMPTY /*|MSDC_INTEN_SDIOIRQ*/\
                                |MSDC_INTEN_CMDRDY       |MSDC_INTEN_CMDTMO      | MSDC_INTEN_RSPCRCERR   |MSDC_INTEN_CSTA\
                                |MSDC_INTEN_XFER_COMPL   |MSDC_INTEN_DXFER_DONE  | MSDC_INTEN_DATTMO      |MSDC_INTEN_DATCRCERR\
                                |MSDC_INTEN_BDCSERR      |MSDC_INTEN_ACMD19_DONE | MSDC_INTEN_GPDCSERR     /*|MSDC_INTEN_DMAPRO*/\
                                /*|MSDC_INTEN_GOBOUND   |MSDC_INTEN_ACMD53_DONE |MSDC_INTEN_ACMD53_FAIL |MSDC_INTEN_AXI_RESP_ERR*/)


/* MSDC_FIFOCS mask */
#define MSDC_FIFOCS_RXCNT       (0xFF  <<  0)     /* R */
#define MSDC_FIFOCS_TXCNT       (0xFF  << 16)     /* R */
#define MSDC_FIFOCS_CLR         (0x1   << 31)     /* RW */

/* SDC_CFG mask */
#define SDC_CFG_SDIOINTWKUP     (0x1   <<  0)     /* RW */
#define SDC_CFG_INSWKUP         (0x1   <<  1)     /* RW */
#define SDC_CFG_BUSWIDTH        (0x3   << 16)     /* RW */
#define SDC_CFG_SDIO            (0x1   << 19)     /* RW */
#define SDC_CFG_SDIOIDE         (0x1   << 20)     /* RW */
#define SDC_CFG_INTATGAP        (0x1   << 21)     /* RW */
#define SDC_CFG_DTOC            (0xFF  << 24)     /* RW */

/* SDC_CMD mask */
#define SDC_CMD_OPC             (0x3F  <<  0)     /* RW */
#define SDC_CMD_BRK             (0x1   <<  6)     /* RW */
#define SDC_CMD_RSPTYP          (0x7   <<  7)     /* RW */
#define SDC_CMD_DTYP            (0x3   << 11)     /* RW */
#define SDC_CMD_RW              (0x1   << 13)     /* RW */
#define SDC_CMD_STOP            (0x1   << 14)     /* RW */
#define SDC_CMD_GOIRQ           (0x1   << 15)     /* RW */
#define SDC_CMD_BLKLEN          (0xFFF << 16)     /* RW */
#define SDC_CMD_AUTOCMD         (0x3   << 28)     /* RW */
#define SDC_CMD_VOLSWTH         (0x1   << 30)     /* RW */
#define SDC_CMD_ACMD53          (0x1   << 31)     /* RW Only for SD/SDIO ACMD 53*/

/* SDC_STS mask */
#define SDC_STS_SDCBUSY         (0x1   <<  0)     /* RW */
#define SDC_STS_CMDBUSY         (0x1   <<  1)     /* RW */
#define SDC_STS_CMD_WR_BUSY     (0x1   << 16)     /* RW !!! MT7623  Add*/
#define SDC_STS_SWR_COMPL       (0x1   << 31)     /* RW */

/* SDC_VOL_CHG mask */
#define SDC_VOL_CHG_VCHGCNT     (0xFFFF<<  0)     /* RW  !!! MT7623  Add*/
/* SDC_DCRC_STS mask */
#define SDC_DCRC_STS_POS        (0xFF  <<  0)     /* RO */
#define SDC_DCRC_STS_NEG        (0xFF  <<  8)     /* RO */

/* EMMC_CFG0 mask */
#define EMMC_CFG0_BOOTSTART     (0x1   <<  0)     /* WO Only for eMMC */
#define EMMC_CFG0_BOOTSTOP      (0x1   <<  1)     /* WO Only for eMMC */
#define EMMC_CFG0_BOOTMODE      (0x1   <<  2)     /* RW Only for eMMC */
#define EMMC_CFG0_BOOTACKDIS    (0x1   <<  3)     /* RW Only for eMMC */

#define EMMC_CFG0_BOOTWDLY      (0x7   << 12)     /* RW Only for eMMC */
#define EMMC_CFG0_BOOTSUPP      (0x1   << 15)     /* RW Only for eMMC */

/* EMMC_CFG1 mask */
#define EMMC_CFG1_BOOTDATTMC   (0xFFFFF<<  0)     /* RW Only for eMMC */
#define EMMC_CFG1_BOOTACKTMC    (0xFFF << 20)     /* RW Only for eMMC */

/* EMMC_STS mask */
#define EMMC_STS_BOOTCRCERR     (0x1   <<  0)     /* W1C Only for eMMC */
#define EMMC_STS_BOOTACKERR     (0x1   <<  1)     /* W1C Only for eMMC */
#define EMMC_STS_BOOTDATTMO     (0x1   <<  2)     /* W1C Only for eMMC */
#define EMMC_STS_BOOTACKTMO     (0x1   <<  3)     /* W1C Only for eMMC */
#define EMMC_STS_BOOTUPSTATE    (0x1   <<  4)     /* RU Only for eMMC */
#define EMMC_STS_BOOTACKRCV     (0x1   <<  5)     /* W1C Only for eMMC */
#define EMMC_STS_BOOTDATRCV     (0x1   <<  6)     /* RU Only for eMMC */

/* EMMC_IOCON mask */
#define EMMC_IOCON_BOOTRST      (0x1   <<  0)     /* RW Only for eMMC */

/* SDC_ACMD19_TRG mask */
#define SDC_ACMD19_TRG_TUNESEL  (0xF   <<  0)     /* RW */

/* DMA_SA_HIGH4BIT mask */
#define DMA_SA_HIGH4BIT_L4BITS  (0xF   <<  0)     /* RW  !!! MT7623  Add*/
/* MSDC_DMA_CTRL mask */
#define MSDC_DMA_CTRL_START     (0x1   <<  0)     /* WO */
#define MSDC_DMA_CTRL_STOP      (0x1   <<  1)     /* AO */
#define MSDC_DMA_CTRL_RESUME    (0x1   <<  2)     /* WO */
#define MSDC_DMA_CTRL_READYM    (0x1   <<  3)     /* RO  !!! MT7623  Add*/

#define MSDC_DMA_CTRL_MODE      (0x1   <<  8)     /* RW */
#define MSDC_DMA_CTRL_ALIGN     (0x1   <<  9)     /* RW !!! MT7623  Add*/
#define MSDC_DMA_CTRL_LASTBUF   (0x1   << 10)     /* RW */
#define MSDC_DMA_CTRL_SPLIT1K   (0x1   << 11)     /* RW !!! MT7623  Add*/
#define MSDC_DMA_CTRL_BRUSTSZ   (0x7   << 12)     /* RW */
// #define MSDC_DMA_CTRL_XFERSZ    (0xffffUL << 16)/* RW */

/* MSDC_DMA_CFG mask */
#define MSDC_DMA_CFG_STS              (0x1  <<  0)     /* R */
#define MSDC_DMA_CFG_DECSEN           (0x1  <<  1)     /* RW */
#define MSDC_DMA_CFG_LOCKDISABLE      (0x1  <<  2)     /* RW !!! MT7623  Add*/
//#define MSDC_DMA_CFG_BDCSERR        (0x1  <<  4)     /* R */
//#define MSDC_DMA_CFG_GPDCSERR       (0x1  <<  5)     /* R */
#define MSDC_DMA_CFG_AHBEN            (0x3  <<  8)     /* RW */
#define MSDC_DMA_CFG_ACTEN            (0x3  << 12)     /* RW */

#define MSDC_DMA_CFG_CS12B            (0x1  << 16)     /* RW */
#define MSDC_DMA_CFG_OUTB_STOP        (0x1  << 17)     /* RW */

/* MSDC_PATCH_BIT0 mask */
//#define MSDC_PB0_RESV1              (0x1  <<  0)
#define MSDC_PB0_EN_8BITSUP           (0x1  <<  1)    /* RW */
#define MSDC_PB0_DIS_RECMDWR          (0x1  <<  2)    /* RW */
//#define MSDC_PB0_RESV2                        (0x1  <<  3)
#define MSDC_PB0_RDDATSEL             (0x1  <<  3)    /* RW !!! MT7623 Add for SD/SDIO/eMMC 4.5*/
#define MSDC_PB0_ACMD53_CRCINTR       (0x1  <<  4)    /* RW !!! MT7623 Add only for SD/SDIO */
#define MSDC_PB0_ACMD53_ONESHOT       (0x1  <<  5)    /* RW !!! MT7623 Add ony for SD/SDIO */
//#define MSDC_PB0_RESV3                        (0x1  <<  6)
#define MSDC_PB0_DESC_UP_SEL          (0x1  <<  6)    /* RW !!! MT7623  Add*/
#define MSDC_PB0_INT_DAT_LATCH_CK_SEL (0x7  <<  7)    /* RW */
#define MSDC_INT_DAT_LATCH_CK_SEL      MSDC_PB0_INT_DAT_LATCH_CK_SEL  /* alias */

#define MSDC_PB0_CKGEN_MSDC_DLY_SEL   (0x1F << 10)    /* RW */
#define MSDC_CKGEN_MSDC_DLY_SEL        MSDC_PB0_CKGEN_MSDC_DLY_SEL  /* alias */


#define MSDC_PB0_FIFORD_DIS           (0x1  << 15)    /* RW */
//#define MSDC_PB0_SDIO_DBSSEL                  (0x1  << 16)    /* RW !!! MT7623  change*/
#define MSDC_PB0_MSDC_BLKNUMSEL       (0x1  << 16)    /* RW !!! MT7623  change ACMD23*/
#define MSDC_PB0_BLKNUM_SEL           MSDC_PB0_MSDC_BLKNUMSEL /* alias */

#define MSDC_PB0_SDIO_INTCSEL         (0x1  << 17)    /* RW */
#define MSDC_PB0_SDIO_BSYDLY          (0xF  << 18)    /* RW */
#define MSDC_PB0_SDC_WDOD             (0xF  << 22)    /* RW */
#define MSDC_PB0_CMDIDRTSEL           (0x1  << 26)    /* RW */
#define MSDC_PB0_CMDFAILSEL           (0x1  << 27)    /* RW */
#define MSDC_PB0_SDIO_INTDLYSEL       (0x1  << 28)    /* RW */
#define MSDC_PB0_SPCPUSH              (0x1  << 29)    /* RW */
#define MSDC_PB0_DETWR_CRCTMO         (0x1  << 30)    /* RW */
#define MSDC_PB0_EN_DRVRSP            (0x1  << 31)    /* RW */

/* MSDC_PATCH_BIT1 mask */
#define MSDC_PB1_WRDAT_CRCS_TA_CNTR   (0x7  <<  0)    /* RW */
#define MSDC_PATCH_BIT1_WRDAT_CRCS    MSDC_PB1_WRDAT_CRCS_TA_CNTR /* alias */


#define MSDC_PB1_CMD_RSP_TA_CNTR      (0x7  <<  3)    /* RW */
#define MSDC_PATCH_BIT1_CMD_RSP       MSDC_PB1_CMD_RSP_TA_CNTR  /* alias */

//#define MSDC_PB1_RESV3                        (0x3  <<  6)
#define MSDC_PB1_GET_BUSY_MARGIN      (0x1  <<  6)    /* RW !!! MT7623  Add */
#define MSDC_PB1_GET_CRC_MARGIN       (0x1  <<  7)    /* RW !!! MT7623  Add */
#define MSDC_PB1_BIAS_TUNE_28NM       (0xF  <<  8)    /* RW */
#define MSDC_PB1_BIAS_EN18IO_28NM     (0x1  << 12)    /* RW */
#define MSDC_PB1_BIAS_EXT_28NM        (0x1  << 13)    /* RW */

//#define MSDC_PB1_RESV2                        (0x3  << 14)
#define MSDC_PB1_RESET_GDMA           (0x1  << 15)    /* RW !!! MT7623  Add */
//#define MSDC_PB1_RESV1                        (0x7F << 16)
#define MSDC_PB1_EN_SINGLE_BURST      (0x1  << 16)    /* RW !!! MT7623  Add */
#define MSDC_PB1_EN_FORCE_STOP_GDMA   (0x1  << 17)    /* RW !!! MT7623  Add  for eMMC 5.0 only*/
#define MSDC_PB1_DCM_DIV_SEL2         (0x3  << 18)    /* RW !!! MT7623  Add  for eMMC 5.0 only*/
#define MSDC_PB1_DCM_DIV_SEL1         (0x1  << 20)    /* RW !!! MT7623  Add */
#define MSDC_PB1_DCM_EN               (0x1  << 21)    /* RW !!! MT7623  Add */
#define MSDC_PB1_AXI_WRAP_CKEN        (0x1  << 22)    /* RW !!! MT7623  Add for eMMC 5.0 only*/
#define MSDC_PB1_AHBCKEN              (0x1  << 23)    /* RW */
#define MSDC_PB1_CKSPCEN              (0x1  << 24)    /* RW */
#define MSDC_PB1_CKPSCEN              (0x1  << 25)    /* RW */
#define MSDC_PB1_CKVOLDETEN           (0x1  << 26)    /* RW */
#define MSDC_PB1_CKACMDEN             (0x1  << 27)    /* RW */
#define MSDC_PB1_CKSDEN               (0x1  << 28)    /* RW */
#define MSDC_PB1_CKWCTLEN             (0x1  << 29)    /* RW */
#define MSDC_PB1_CKRCTLEN             (0x1  << 30)    /* RW */
#define MSDC_PB1_CKSHBFFEN            (0x1  << 31)    /* RW */

/* MSDC_PATCH_BIT2 mask */
#define MSDC_PB2_ENHANCEGPD           (0x1  <<  0)    /* RW !!! MT7623  Add */
#define MSDC_PB2_SUPPORT64G           (0x1  <<  1)    /* RW !!! MT7623  Add */
#define MSDC_PB2_RESPWAIT             (0x3  <<  2)    /* RW !!! MT7623  Add */
#define MSDC_PB2_CFGRDATCNT           (0x1F <<  4)    /* RW !!! MT7623  Add */
#define MSDC_PB2_CFGRDAT              (0x1  <<  9)    /* RW !!! MT7623  Add */

#define MSDC_PB2_INTCRESPSEL          (0x1  << 11)    /* RW !!! MT7623  Add */
#define MSDC_PB2_CFGRESPCNT           (0x7  << 12)    /* RW !!! MT7623  Add */
#define MSDC_PB2_CFGRESP              (0x1  << 15)    /* RW !!! MT7623  Add */
#define MSDC_PB2_RESPSTSENSEL         (0x7  << 16)    /* RW !!! MT7623  Add */

#define MSDC_PB2_POPENCNT             (0xF  << 20)    /* RW !!! MT7623  Add */
#define MSDC_PB2_CFGCRCSTSSEL         (0x1  << 24)    /* RW !!! MT7623  Add */
#define MSDC_PB2_CFGCRCSTSEDGE        (0x1  << 25)    /* RW !!! MT7623  Add */
#define MSDC_PB2_CFGCRCSTSCNT         (0x3  << 26)    /* RW !!! MT7623  Add */
#define MSDC_PB2_CFGCRCSTS            (0x1  << 28)    /* RW !!! MT7623  Add */
#define MSDC_PB2_CRCSTSENSEL          (0x7  << 29)    /* RW !!! MT7623  Add */


/* SDIO_TUNE_WIND mask */
#define SDIO_TUNE_WIND_TUNEWINDOW     (0x1F  <<  0)     /* RW !!! MT7623  Add for SD/SDIO only*/

/* MSDC_PAD_TUNE/MSDC_PAD_TUNE0 mask */
#define MSDC_PAD_TUNE_DATWRDLY        (0x1F  <<  0)     /* RW */

#define MSDC_PAD_TUNE_DELAYEN         (0x1   <<  7)     /* RW !!! MT7623  Add*/
#define MSDC_PAD_TUNE_DATRRDLY        (0x1F  <<  8)     /* RW */
#define MSDC_PAD_TUNE_DATRRDLYSEL     (0x1   << 13)     /* RW !!! MT7623  Add*/

#define MSDC_PAD_TUNE_RXDLYSEL        (0x1   << 15)     /* RW !!! MT7623  Add*/
#define MSDC_PAD_TUNE_CMDRDLY         (0x1F  << 16)     /* RW */
#define MSDC_PAD_TUNE_CMDRDLYSEL      (0x1   << 21)     /* RW !!! MT7623  Add*/
#define MSDC_PAD_TUNE_CMDRRDLY        (0x1F  << 22)     /* RW */
#define MSDC_PAD_TUNE_CLKTXDLY        (0x1F  << 27)     /* RW */

/* MSDC_PAD_TUNE1 mask */

#define MSDC_PAD_TUNE1_DATRRDLY2      (0x1F  <<  8)     /* RW  !!! MT7623  Add*/
#define MSDC_PAD_TUNE1_DATRDLY2SEL    (0x1   << 13)     /* RW  !!! MT7623  Add*/

#define MSDC_PAD_TUNE1_CMDRDLY2       (0x1F  << 16)     /* RW  !!! MT7623  Add*/
#define MSDC_PAD_TUNE1_CMDRDLY2SEL    (0x1   << 21)     /* RW  !!! MT7623  Add*/


/* MSDC_DAT_RDDLY0 mask */
#define MSDC_DAT_RDDLY0_D3            (0x1F  <<  0)     /* RW */
#define MSDC_DAT_RDDLY0_D2            (0x1F  <<  8)     /* RW */
#define MSDC_DAT_RDDLY0_D1            (0x1F  << 16)     /* RW */
#define MSDC_DAT_RDDLY0_D0            (0x1F  << 24)     /* RW */

/* MSDC_DAT_RDDLY1 mask */
#define MSDC_DAT_RDDLY1_D7            (0x1F  <<  0)     /* RW */

#define MSDC_DAT_RDDLY1_D6            (0x1F  <<  8)     /* RW */

#define MSDC_DAT_RDDLY1_D5            (0x1F  << 16)     /* RW */

#define MSDC_DAT_RDDLY1_D4            (0x1F  << 24)     /* RW */

/* MSDC_DAT_RDDLY2 mask */
#define MSDC_DAT_RDDLY2_D3            (0x1F  <<  0)     /* RW !!! MT7623  Add*/

#define MSDC_DAT_RDDLY2_D2            (0x1F  <<  8)     /* RW !!! MT7623  Add*/

#define MSDC_DAT_RDDLY2_D1            (0x1F  << 16)     /* RW !!! MT7623  Add*/

#define MSDC_DAT_RDDLY2_D0            (0x1F  << 24)     /* RW !!! MT7623  Add*/

/* MSDC_DAT_RDDLY3 mask */
#define MSDC_DAT_RDDLY3_D7            (0x1F  <<  0)     /* RW !!! MT7623  Add*/

#define MSDC_DAT_RDDLY3_D6            (0x1F  <<  8)     /* RW !!! MT7623  Add*/

#define MSDC_DAT_RDDLY3_D5            (0x1F  << 16)     /* RW !!! MT7623  Add*/

#define MSDC_DAT_RDDLY3_D4            (0x1F  << 24)     /* RW !!! MT7623  Add*/

/* MSDC_HW_DBG_SEL mask */
#define MSDC_HW_DBG0_SEL              (0xFF  <<  0)     /* RW DBG3->DBG0 !!! MT7623  Change*/
#define MSDC_HW_DBG1_SEL              (0x3F  <<  8)     /* RW DBG2->DBG1 !!! MT7623  Add*/

#define MSDC_HW_DBG2_SEL              (0xFF  << 16)     /* RW DBG1->DBG2 !!! MT7623  Add*/
//#define MSDC_HW_DBG_WRAPTYPE_SEL    (0x3   << 22)           /* RW !!! MT7623  Removed*/
#define MSDC_HW_DBG3_SEL              (0x3F  << 24)     /* RW DBG0->DBG3 !!! MT7623  Add*/
#define MSDC_HW_DBG_WRAP_SEL          (0x1   << 30)     /* RW */


/* MSDC_EMMC50_PAD_CTL0 mask*/
#define MSDC_EMMC50_PAD_CTL0_DCCSEL   (0x1  <<  0)     /* RW */
#define MSDC_EMMC50_PAD_CTL0_HLSEL    (0x1  <<  1)     /* RW */
#define MSDC_EMMC50_PAD_CTL0_DLP0     (0x3  <<  2)     /* RW */
#define MSDC_EMMC50_PAD_CTL0_DLN0     (0x3  <<  4)     /* RW */
#define MSDC_EMMC50_PAD_CTL0_DLP1     (0x3  <<  6)     /* RW */
#define MSDC_EMMC50_PAD_CTL0_DLN1     (0x3  <<  8)     /* RW */

/* MSDC_EMMC50_PAD_DS_CTL0 mask */
#define MSDC_EMMC50_PAD_DS_CTL0_SR    (0x1  <<  0)     /* RW */
#define MSDC_EMMC50_PAD_DS_CTL0_R0    (0x1  <<  1)     /* RW */
#define MSDC_EMMC50_PAD_DS_CTL0_R1    (0x1  <<  2)     /* RW */
#define MSDC_EMMC50_PAD_DS_CTL0_PUPD  (0x1  <<  3)     /* RW */
#define MSDC_EMMC50_PAD_DS_CTL0_IES   (0x1  <<  4)     /* RW */
#define MSDC_EMMC50_PAD_DS_CTL0_SMT   (0x1  <<  5)     /* RW */
#define MSDC_EMMC50_PAD_DS_CTL0_RDSEL (0x3F <<  6)     /* RW */
#define MSDC_EMMC50_PAD_DS_CTL0_TDSEL (0xF  << 12)     /* RW */
#define MSDC_EMMC50_PAD_DS_CTL0_DRV   (0x7  << 16)     /* RW */


/* EMMC50_PAD_DS_TUNE mask */
#define MSDC_EMMC50_PAD_DS_TUNE_DLYSEL  (0x1  <<  0)  /* RW */
#define MSDC_EMMC50_PAD_DS_TUNE_DLY2SEL (0x1  <<  1)  /* RW */
#define MSDC_EMMC50_PAD_DS_TUNE_DLY1    (0x1F <<  2)  /* RW */
#define MSDC_EMMC50_PAD_DS_TUNE_DLY2    (0x1F <<  7)  /* RW */
#define MSDC_EMMC50_PAD_DS_TUNE_DLY3    (0x1F << 12)  /* RW */

/* EMMC50_PAD_CMD_TUNE mask */
#define MSDC_EMMC50_PAD_CMD_TUNE_DLY3SEL (0x1  <<  0)  /* RW */
#define MSDC_EMMC50_PAD_CMD_TUNE_RXDLY3  (0x1F <<  1)  /* RW */
#define MSDC_EMMC50_PAD_CMD_TUNE_TXDLY   (0x1F <<  6)  /* RW */

/* EMMC50_PAD_DAT01_TUNE mask */
#define MSDC_EMMC50_PAD_DAT0_RXDLY3SEL   (0x1  <<  0)  /* RW */
#define MSDC_EMMC50_PAD_DAT0_RXDLY3      (0x1F <<  1)  /* RW */
#define MSDC_EMMC50_PAD_DAT0_TXDLY       (0x1F <<  6)  /* RW */
#define MSDC_EMMC50_PAD_DAT1_RXDLY3SEL   (0x1  << 16)  /* RW */
#define MSDC_EMMC50_PAD_DAT1_RXDLY3      (0x1F << 17)  /* RW */
#define MSDC_EMMC50_PAD_DAT1_TXDLY       (0x1F << 22)  /* RW */

/* EMMC50_PAD_DAT23_TUNE mask */
#define MSDC_EMMC50_PAD_DAT2_RXDLY3SEL   (0x1  <<  0)  /* RW */
#define MSDC_EMMC50_PAD_DAT2_RXDLY3      (0x1F <<  1)  /* RW */
#define MSDC_EMMC50_PAD_DAT2_TXDLY       (0x1F <<  6)  /* RW */
#define MSDC_EMMC50_PAD_DAT3_RXDLY3SEL   (0x1  << 16)  /* RW */
#define MSDC_EMMC50_PAD_DAT3_RXDLY3      (0x1F << 17)  /* RW */
#define MSDC_EMMC50_PAD_DAT3_TXDLY       (0x1F << 22)  /* RW */

/* EMMC50_PAD_DAT45_TUNE mask */
#define MSDC_EMMC50_PAD_DAT4_RXDLY3SEL   (0x1  <<  0)  /* RW */
#define MSDC_EMMC50_PAD_DAT4_RXDLY3      (0x1F <<  1)  /* RW */
#define MSDC_EMMC50_PAD_DAT4_TXDLY       (0x1F <<  6)  /* RW */
#define MSDC_EMMC50_PAD_DAT5_RXDLY3SEL   (0x1  << 16)  /* RW */
#define MSDC_EMMC50_PAD_DAT5_RXDLY3      (0x1F << 17)  /* RW */
#define MSDC_EMMC50_PAD_DAT5_TXDLY       (0x1F << 22)  /* RW */

/* EMMC50_PAD_DAT67_TUNE mask */
#define MSDC_EMMC50_PAD_DAT6_RXDLY3SEL   (0x1  <<  0)  /* RW */
#define MSDC_EMMC50_PAD_DAT6_RXDLY3      (0x1F <<  1)  /* RW */
#define MSDC_EMMC50_PAD_DAT6_TXDLY       (0x1F <<  6)  /* RW */
#define MSDC_EMMC50_PAD_DAT7_RXDLY3SEL   (0x1  << 16)  /* RW */
#define MSDC_EMMC50_PAD_DAT7_RXDLY3      (0x1F << 17)  /* RW */
#define MSDC_EMMC50_PAD_DAT7_TXDLY       (0x1F << 22)  /* RW */

/* EMMC51_CFG0 mask */
#define MSDC_EMMC51_CFG_CMDQ_EN          (0x1   <<  0) /* RW !!! MT7623  Add*/
#define MSDC_EMMC51_CFG_WDAT_CNT         (0x3FF <<  1) /* RW !!! MT7623  Add*/
#define MSDC_EMMC51_CFG_RDAT_CNT         (0x3FF << 11) /* RW !!! MT7623  Add*/
#define MSDC_EMMC51_CFG_CMDQ_CMD_EN      (0x1   << 21) /* RW !!! MT7623  Add*/


/* EMMC50_CFG0 mask */
#define MSDC_EMMC50_CFG_PADCMD_LATCHCK         (0x1  <<  0)  /* RW*/
#define MSDC_EMMC50_CFG_CRCSTS_CNT             (0x3  <<  1)  /* RW*/
#define MSDC_EMMC50_CFG_CRCSTS_EDGE            (0x1  <<  3)  /* RW*/
#define MSDC_EMMC50_CFG_CRC_STS_EDGE           MSDC_EMMC50_CFG_CRCSTS_EDGE /*alias */

#define MSDC_EMMC50_CFG_CRCSTS_SEL             (0x1  <<  4)  /* RW*/
#define MSDC_EMMC50_CFG_CRC_STS_SEL            MSDC_EMMC50_CFG_CRCSTS_SEL /*alias */

#define MSDC_EMMC50_CFG_ENDBIT_CHKCNT          (0xF  <<  5)  /* RW*/
#define MSDC_EMMC50_CFG_CMDRSP_SEL             (0x1  <<  9)  /* RW*/
#define MSDC_EMMC50_CFG_CMD_RESP_SEL           MSDC_EMMC50_CFG_CMDRSP_SEL  /*alias */

#define MSDC_EMMC50_CFG_CMDEDGE_SEL            (0x1  << 10)  /* RW*/
#define MSDC_EMMC50_CFG_ENDBIT_CNT             (0x3FF<< 11)  /* RW*/
#define MSDC_EMMC50_CFG_READDAT_CNT            (0x7  << 21)  /* RW*/
#define MSDC_EMMC50_CFG_EMMC50_MONSEL          (0x1  << 24)  /* RW*/
#define MSDC_EMMC50_CFG_MSDC_WRVALID           (0x1  << 25)  /* RW*/
#define MSDC_EMMC50_CFG_MSDC_RDVALID           (0x1  << 26)  /* RW*/
#define MSDC_EMMC50_CFG_MSDC_WRVALID_SEL       (0x1  << 27)  /* RW*/
#define MSDC_EMMC50_CFG_MSDC_RDVALID_SEL       (0x1  << 28)  /* RW*/
#define MSDC_EMMC50_CFG_MSDC_TXSKEW_SEL        (0x1  << 29)  /* RW*/
//#define MSDC_EMMC50_CFG_MSDC_GDMA_RESET      (0x1  << 31)  /* RW !!! MT7623  Removed*/

/* EMMC50_CFG1 mask */
#define MSDC_EMMC50_CFG1_WRPTR_MARGIN          (0xFF <<  0)  /* RW*/
#define MSDC_EMMC50_CFG1_CKSWITCH_CNT          (0x7  <<  8)  /* RW*/
#define MSDC_EMMC50_CFG1_RDDAT_STOP            (0x1  << 11)  /* RW*/
#define MSDC_EMMC50_CFG1_WAIT8CLK_CNT          (0xF  << 12)  /* RW*/
#define MSDC_EMMC50_CFG1_EMMC50_DBG_SEL        (0xFF << 16)  /* RW*/
#define MSDC_EMMC50_CFG1_PSH_CNT               (0x7  << 24)  /* RW !!! MT7623  Add*/
#define MSDC_EMMC50_CFG1_PSH_PS_SEL            (0x1  << 27)  /* RW !!! MT7623  Add*/
#define MSDC_EMMC50_CFG1_DS_CFG                (0x1  << 28)  /* RW !!! MT7623  Add*/

/* EMMC50_CFG2 mask */
//#define MSDC_EMMC50_CFG2_AXI_GPD_UP          (0x1  <<  0)  /* RW !!! MT7623  Removed*/
#define MSDC_EMMC50_CFG2_AXI_IOMMU_WR_EMI      (0x1  <<  1) /* RW*/
#define MSDC_EMMC50_CFG2_AXI_SHARE_EN_WR_EMI   (0x1  <<  2) /* RW*/

#define MSDC_EMMC50_CFG2_AXI_IOMMU_RD_EMI      (0x1  <<  7) /* RW*/
#define MSDC_EMMC50_CFG2_AXI_SHARE_EN_RD_EMI   (0x1  <<  8) /* RW*/

#define MSDC_EMMC50_CFG2_AXI_BOUND_128B        (0x1  << 13) /* RW*/
#define MSDC_EMMC50_CFG2_AXI_BOUND_256B        (0x1  << 14) /* RW*/
#define MSDC_EMMC50_CFG2_AXI_BOUND_512B        (0x1  << 15) /* RW*/
#define MSDC_EMMC50_CFG2_AXI_BOUND_1K          (0x1  << 16) /* RW*/
#define MSDC_EMMC50_CFG2_AXI_BOUND_2K          (0x1  << 17) /* RW*/
#define MSDC_EMMC50_CFG2_AXI_BOUND_4K          (0x1  << 18) /* RW*/
#define MSDC_EMMC50_CFG2_AXI_RD_OUTSTANDING_NUM (0x1F << 19) /* RW*/
#define MSDC_EMMC50_CFG2_AXI_RD_OUTS_NUM       MSDC_EMMC50_CFG2_AXI_RD_OUTSTANDING_NUM /*alias */

#define MSDC_EMMC50_CFG2_AXI_SET_LET           (0xF  << 24) /* RW*/
#define MSDC_EMMC50_CFG2_AXI_SET_LEN           MSDC_EMMC50_CFG2_AXI_SET_LET /*alias */

#define MSDC_EMMC50_CFG2_AXI_RESP_ERR_TYPE     (0x3  << 28) /* RW*/
#define MSDC_EMMC50_CFG2_AXI_BUSY              (0x1  << 30) /* RW*/


/* EMMC50_CFG3 mask */
#define MSDC_EMMC50_CFG3_OUTSTANDING_WR        (0x1F <<  0) /* RW*/
#define MSDC_EMMC50_CFG3_OUTS_WR               MSDC_EMMC50_CFG3_OUTSTANDING_WR /*alias */

#define MSDC_EMMC50_CFG3_ULTRA_SET_WR          (0x3F <<  5) /* RW*/
#define MSDC_EMMC50_CFG3_PREULTRA_SET_WR       (0x3F << 11) /* RW*/
#define MSDC_EMMC50_CFG3_ULTRA_SET_RD          (0x3F << 17) /* RW*/
#define MSDC_EMMC50_CFG3_PREULTRA_SET_RD       (0x3F << 23) /* RW*/

/* EMMC50_CFG4 mask */
#define MSDC_EMMC50_CFG4_IMPR_ULTRA_SET_WR     (0xFF <<  0) /* RW*/
#define MSDC_EMMC50_CFG4_IMPR_ULTRA_SET_RD     (0xFF <<  8) /* RW*/
#define MSDC_EMMC50_CFG4_ULTRA_EN              (0x3  << 16) /* RW*/
#define MSDC_EMMC50_CFG4_WRAP_SEL              (0x1F << 18) /* RW !!! MT7623  Add*/

/* EMMC50_BLOCK_LENGTH mask */
#define EMMC50_BLOCK_LENGTH_BLKLENGTH          (0xFF <<  0) /* RW !!! MT7623  Add*/

/* 
 *  This definition is used for MT7623_GPIO_RegMap.doc by candy.li on 2014-9-03 
 *  Zhanyong.Wang
 *  2014/11/28
 */
#define GPIO_PIN_MAX                          280
#ifndef GPIO_BASE
#define GPIO_BASE                             0x10005000
#endif
#if GPIO_BASE != 0x10005000
#error  Please check your GPIO_BASE definition!!!!
#endif

#define REG_GPIO_BASE(x)                     ((volatile unsigned int *)(GPIO_BASE + OFFSET_##x))

#define GPIO_DIR_ASS                         175
#define GPIO_DIR_PIN2REG(n)                  ((volatile unsigned int *)(GPIO_BASE + (((n) <= GPIO_DIR_ASS) ? 0 : 0x10) +  (((n) % GPIO_PIN_MAX) & 0x1F0))) /* 0: input, 1: output */

#define GPIO_DATOUT_PIN2REG(n)               ((volatile unsigned int *)(GPIO_BASE +  0x500 +  (((n) % GPIO_PIN_MAX) & 0x1F0)))
#define GPIO_DATIN_PIN2REG(n)                ((volatile unsigned int *)(GPIO_BASE +  0x630 +  (((n) % GPIO_PIN_MAX) & 0x1F0)))

#define GPIO_PULLEN_PIN2REG(n)               ((volatile unsigned int *)(GPIO_BASE +  0x150 +  (((n) % GPIO_PIN_MAX) & 0x1F0))) /* 0: disable, 1: enable */
#define GPIO_PULLSEL_PIN2REG(n)              ((volatile unsigned int *)(GPIO_BASE +  0x280 +  (((n) % GPIO_PIN_MAX) & 0x1F0))) /* 0: pull down, 1: pull up */

#define BIT_GPIOMASK(n)                      (1 << (((n) % GPIO_PIN_MAX) & 0x0F))                   


#define GPIO_MODE_PIN2REG(n)                 ((volatile unsigned int *)(GPIO_BASE +  0x760 +  ((((n) % GPIO_PIN_MAX)/5) << 4)))

#define BIT_MODE_GPIOMASK(n)                 (0x07 << (3*(((n) % GPIO_PIN_MAX)%5))) 
#define BV_GPIO_MSDC_MODE                    1

#define BIT_MSDC_CLK_R0R1PUPD                (0x07 << 8)
#define BIT_MSDC_CMD_R0R1PUPD                (0x07 << 8)
#define BIT_MSDC_DAT0_R0R1PUPD               (0x07 << 0)
#define BIT_MSDC_DAT1_R0R1PUPD               (0x07 << 4)
#define BIT_MSDC_DAT2_R0R1PUPD               (0x07 << 8)
#define BIT_MSDC_DAT3_R0R1PUPD               (0x07 <<12)
#define BIT_MSDC_DAT4_R0R1PUPD               (0x07 << 0)
#define BIT_MSDC_DAT5_R0R1PUPD               (0x07 << 4)
#define BIT_MSDC_DAT6_R0R1PUPD               (0x07 << 8)
#define BIT_MSDC_DAT7_R0R1PUPD               (0x07 <<12)

#define BIT_MSDC_RSTB_R0R1PUPD               (0x07 << 0)

#define BIT_MSDC_DSL_R0R1PUPD                (0x07 << 4)
#define BIT_MSDC_RCLK_R0R1PUPD               (0x07 << 8)

#define BV_MSDCRX_PUR0R1_HIGHZ                0
#define BV_MSDCRX_PUR0R1_50KOHM               2
#define BV_MSDCRX_PUR0R1_10KOHM               4
#define BV_MSDCRX_PUR0R1_08KOHM               6
#define BV_MSDCRX_PDR0R1_HIGHZ                1
#define BV_MSDCRX_PDR0R1_50KOHM               3
#define BV_MSDCRX_PDR0R1_10KOHM               5
#define BV_MSDCRX_PDR0R1_08KOHM               7

#define BIT_MSDC_CLK_E8E4E2                  (0x07 << 0)
#define BIT_MSDC_CMD_E8E4E2                  (0x07 << 0)
#define BIT_MSDC_DAT_E8E4E2                  (0x07 << 0)
#define BIT_MSDC_RCLK_E8E4E2                 (0x07 << 0)
#define BIT_GPIO_MSDC_18VE8E4E2              (0x07 << 0)
#define BV_MSDCTX_18VE8E4E2_01RD78U85MA       0
#define BV_MSDCTX_18VE8E4E2_03RD78U85MA       1
#define BV_MSDCTX_18VE8E4E2_04RD78U85MA       2
#define BV_MSDCTX_18VE8E4E2_06RD78U85MA       3
#define BV_MSDCTX_18VE8E4E2_07RD78U85MA       4
#define BV_MSDCTX_18VE8E4E2_10RD78U85MA       5
#define BV_MSDCTX_18VE8E4E2_11RD78U85MA       6
#define BV_MSDCTX_18VE8E4E2_13RD78U85MA       7

#define BIT_GPIO_MSDC_33VE8E4E2              (0x07 << 0)
#define BV_MSDCTX_33VE8E4E2_01RD14U17MA       0
#define BV_MSDCTX_33VE8E4E2_03RD14U17MA       1
#define BV_MSDCTX_33VE8E4E2_04RD14U17MA       2
#define BV_MSDCTX_33VE8E4E2_06RD14U17MA       3

/*
   MSDC Driving Strength Definition: specify as gear instead of driving
*/
#define MSDC_DRVN_GEAR0                       BV_MSDCTX_18VE8E4E2_01RD78U85MA
#define MSDC_DRVN_GEAR1                       BV_MSDCTX_18VE8E4E2_03RD78U85MA
#define MSDC_DRVN_GEAR2                       BV_MSDCTX_18VE8E4E2_04RD78U85MA
#define MSDC_DRVN_GEAR3                       BV_MSDCTX_18VE8E4E2_06RD78U85MA
#define MSDC_DRVN_GEAR4                       BV_MSDCTX_18VE8E4E2_07RD78U85MA
#define MSDC_DRVN_GEAR5                       BV_MSDCTX_18VE8E4E2_10RD78U85MA
#define MSDC_DRVN_GEAR6                       BV_MSDCTX_18VE8E4E2_11RD78U85MA
#define MSDC_DRVN_GEAR7                       BV_MSDCTX_18VE8E4E2_13RD78U85MA
#define MSDC_DRVN_DONT_CARE                   MSDC_DRVN_GEAR0

/* id valid range "MSDC[0, 3]" index"[0, 5/6]" */
#define MSDC_CTRL(id, index)                  GPIO_##id##_CTRL##index

#define PIN4_CLK(id)                          PIN4##id##_CLK
#define PIN4_CMD(id)                          PIN4##id##_CMD
#define PIN4_RSTB(id)                         PIN4##id##_RSTB
#define PIN4_DAT(id,line)                     PIN4##id##_##line
#define PIN4_DSL(id)                          PIN4##id##_DSL
#define PIN4_INS(id)                          PIN4##id##_INS
      
/* id valid range "MSDC[0, 4]"*/      
#define MSDC_CLK_SMT(id)                      B_##id##CLK_SMT
#define MSDC_CLK_R0(id)                       B_##id##CLK_R0
#define MSDC_CLK_R1(id)                       B_##id##CLK_R1
#define MSDC_CLK_PUPD(id)                     B_##id##CLK_PUPD
#define MSDC_CLK_IES(id)                      B_##id##CLK_IES
#define MSDC_CLK_SR(id)                       B_##id##CLK_SR
#define MSDC_CLK_E8(id)                       B_##id##CLK_E8
#define MSDC_CLK_E4(id)                       B_##id##CLK_E4
#define MSDC_CLK_E2(id)                       B_##id##CLK_E2
		
#define MSDC_CMD_SMT(id)                      B_##id##CMD_SMT
#define MSDC_CMD_R0(id)                       B_##id##CMD_R0
#define MSDC_CMD_R1(id)                       B_##id##CMD_R1
#define MSDC_CMD_PUPD(id)                     B_##id##CMD_PUPD
#define MSDC_CMD_IES(id)                      B_##id##CMD_IES
#define MSDC_CMD_SR(id)                       B_##id##CMD_SR
#define MSDC_CMD_E8(id)                       B_##id##CMD_E8
#define MSDC_CMD_E4(id)                       B_##id##CMD_E4
#define MSDC_CMD_E2(id)                       B_##id##CMD_E2
		
#define MSDC_DAT_IES(id)                      B_##id##DAT_IES
#define MSDC_DAT_SR(id)                       B_##id##DAT_SR
#define MSDC_DAT_E8(id)                       B_##id##DAT_E8
#define MSDC_DAT_E4(id)                       B_##id##DAT_E4
#define MSDC_DAT_E2(id)                       B_##id##DAT_E2

/* use line valid range "DAT[0, 7]" */
#define MSDC_DAT_SMT(id,line)                 B_##id##line##_SMT
#define MSDC_DAT_R0(id,line)                  B_##id##line##_R0
#define MSDC_DAT_R1(id,line)                  B_##id##line##_R1
#define MSDC_DAT_PUPD(id,line)                B_##id##line##_PUPD
		
#define MSDC_DSL_SMT(id)                      B_##id##DSL_SMT
#define MSDC_DSL_R0(id)                       B_##id##DSL_R0
#define MSDC_DSL_R1(id)                       B_##id##DSL_R1
#define MSDC_DSL_PUPD(id)                     B_##id##DSL_PUPD
		
#define MSDC_RSTB_SMT(id)                     B_##id##RSTB_SMT
#define MSDC_RSTB_R0(id)                      B_##id##RSTB_R0
#define MSDC_RSTB_R1(id)                      B_##id##RSTB_R1
#define MSDC_RSTB_PUPD(id)                    B_##id##RSTB_PUPD
		
#define MSDC_PAD_RDSEL(id)                    B_##id##PAD_RDSEL
#define MSDC_PAD_TDSEL(id)                    B_##id##PAD_TDSEL



#define MSDC_INS_SMT(id)                      B_##id##INS_SMT
/* *RCLK* Obsolete control pin by hui.zeng confirmed, fengguo.gao and zhanyong.wang present 2014/12/18*/
#define MSDC_RCLK_SMT(id)                     B_##id##RCLK_SMT           
#define MSDC_RCLK_R0(id)                      B_##id##RCLK_R0
#define MSDC_RCLK_R1(id)                      B_##id##RCLK_R1
#define MSDC_RCLK_PUPD(id)                    B_##id##RCLK_PUPD
											
#define MSDC_RCLK_IES(id)                     B_##id##RCLK_IES
#define MSDC_RCLK_SR(id)                      B_##id##RCLK_SR
#define MSDC_RCLK_E8(id)                      B_##id##RCLK_E8 
#define MSDC_RCLK_E4(id)                      B_##id##RCLK_E4 
#define MSDC_RCLK_E2(id)                      B_##id##RCLK_E2 
											
#define MSDC_RCLK_RDSEL(id)                   B_##id##RCLK_RDSEL  
#define MSDC_RCLK_TDSEL(id)                   B_##id##RCLK_TDSEL   


/* ******************************Don't modify these source code below ******************************/
/* 
 * MSDC0  GPIO Control definition 
 *  DAT7   DAT6 DAT5 DAT4 RSTB CMD CLK DAT3 DAT2 DAT1 DAT0 
 * GPIO111 112  113  114  115  116 117 118  119  120  121
 *   1      1    1    1    0    1   0   1    1    1    1  
 ***************************************************************************/
#define PIN4MSDC0_CLK                           117
#define PIN4MSDC0_CMD                           116
#define PIN4MSDC0_RSTB                          115
#define PIN4MSDC0_DAT0                          121
#define PIN4MSDC0_DAT1                          120
#define PIN4MSDC0_DAT2                          119
#define PIN4MSDC0_DAT3                          118
#define PIN4MSDC0_DAT4                          114
#define PIN4MSDC0_DAT5                          113
#define PIN4MSDC0_DAT6                          112
#define PIN4MSDC0_DAT7                          111

#define OFFSET_MSDC0_CTRL0                      0xCC0
#define OFFSET_MSDC0_CTRL1                      0xCD0
#define OFFSET_MSDC0_CTRL2                      0xCE0
#define OFFSET_MSDC0_CTRL3                      0xCF0
#define OFFSET_MSDC0_CTRL4                      0xD00
#define OFFSET_MSDC0_CTRL5                      0xD10
#define OFFSET_MSDC0_CTRL6                      0xD20

#ifdef MSDC0_CTRL0
#undef MSDC0_CTRL0
#endif
#ifdef MSDC0_CTRL1
#undef MSDC0_CTRL1
#endif
#ifdef MSDC0_CTRL2
#undef MSDC0_CTRL2
#endif
#ifdef MSDC0_CTRL3
#undef MSDC0_CTRL3
#endif
#ifdef MSDC0_CTRL4
#undef MSDC0_CTRL4
#endif
#ifdef MSDC0_CTRL5
#undef MSDC0_CTRL5
#endif
#ifdef MSDC0_CTRL6
#undef MSDC0_CTRL6
#endif

#define GPIO_MSDC0_CTRL0                        REG_GPIO_BASE(MSDC0_CTRL0)
#define GPIO_MSDC0_CTRL1                        REG_GPIO_BASE(MSDC0_CTRL1)
#define GPIO_MSDC0_CTRL2                        REG_GPIO_BASE(MSDC0_CTRL2)
#define GPIO_MSDC0_CTRL3                        REG_GPIO_BASE(MSDC0_CTRL3)
#define GPIO_MSDC0_CTRL4                        REG_GPIO_BASE(MSDC0_CTRL4)
#define GPIO_MSDC0_CTRL5                        REG_GPIO_BASE(MSDC0_CTRL5)
#define GPIO_MSDC0_CTRL6                        REG_GPIO_BASE(MSDC0_CTRL6)

/* 0x10005CC0 MSDC0_CTRL0   MSDC 0 CLK pad Control Register 0x0311 */
#define B_MSDC0CLK_BACKUP1                     (0xF << 12)
#define B_MSDC0CLK_SMT                         (  1 << 11)
#define B_MSDC0CLK_R0                          (  1 << 10)
#define B_MSDC0CLK_R1                          (  1 <<  9)
#define B_MSDC0CLK_PUPD                        (  1 <<  8)
#define B_MSDC0CLK_BACKUP0                     (0x7 <<  5)
#define B_MSDC0CLK_IES                         (  1 <<  4)
#define B_MSDC0CLK_SR                          (  1 <<  3)
#define B_MSDC0CLK_E8                          (  1 <<  2)
#define B_MSDC0CLK_E4                          (  1 <<  1)
#define B_MSDC0CLK_E2                          (  1 <<  0)

/* 0x10005CD0 MSDC0_CTRL1   MSDC 0 CMD pad Control Register 0x0411 */
#define B_MSDC0CMD_BACKUP1                     (0xF << 12)
#define B_MSDC0CMD_SMT                         (  1 << 11)
#define B_MSDC0CMD_R0                          (  1 << 10)
#define B_MSDC0CMD_R1                          (  1 <<  9)
#define B_MSDC0CMD_PUPD                        (  1 <<  8)
#define B_MSDC0CMD_BACKUP0                     (0x7 <<  5)
#define B_MSDC0CMD_IES                         (  1 <<  4)
#define B_MSDC0CMD_SR                          (  1 <<  3)
#define B_MSDC0CMD_E8                          (  1 <<  2)
#define B_MSDC0CMD_E4                          (  1 <<  1)
#define B_MSDC0CMD_E2                          (  1 <<  0)

/* 0x10005CE0 MSDC0_CTRL2   MSDC 0 DAT pad Control Register 0x0011 */
#define B_MSDC0DAT_BACKUP1                     (0x7FF<< 5)
#define B_MSDC0DAT_IES                         (  1 <<  4)
#define B_MSDC0DAT_SR                          (  1 <<  3)
#define B_MSDC0DAT_E8                          (  1 <<  2)
#define B_MSDC0DAT_E4                          (  1 <<  1)
#define B_MSDC0DAT_E2                          (  1 <<  0)


/* 0x10005CF0 MSDC0_CTRL3   MSDC 0 DAT pad Control Register 0x4444 */
#define B_MSDC0DAT3_SMT                         (  1 << 15)
#define B_MSDC0DAT3_R0                          (  1 << 14)
#define B_MSDC0DAT3_R1                          (  1 << 13)
#define B_MSDC0DAT3_PUPD                        (  1 << 12)

#define B_MSDC0DAT2_SMT                         (  1 << 11)
#define B_MSDC0DAT2_R0                          (  1 << 10)
#define B_MSDC0DAT2_R1                          (  1 <<  9)
#define B_MSDC0DAT2_PUPD                        (  1 <<  8)

#define B_MSDC0DAT1_SMT                         (  1 <<  7)
#define B_MSDC0DAT1_R0                          (  1 <<  6)
#define B_MSDC0DAT1_R1                          (  1 <<  5)
#define B_MSDC0DAT1_PUPD                        (  1 <<  4)

#define B_MSDC0DAT0_SMT                         (  1 <<  3)
#define B_MSDC0DAT0_R0                          (  1 <<  2)
#define B_MSDC0DAT0_R1                          (  1 <<  1)
#define B_MSDC0DAT0_PUPD                        (  1 <<  0)


/* 0x10005D00 MSDC0_CTRL4   MSDC 0 DAT pad Control Register 0x4444 */
#define B_MSDC0DAT7_SMT                         (  1 << 15)
#define B_MSDC0DAT7_R0                          (  1 << 14)
#define B_MSDC0DAT7_R1                          (  1 << 13)
#define B_MSDC0DAT7_PUPD                        (  1 << 12)

#define B_MSDC0DAT6_SMT                         (  1 << 11)
#define B_MSDC0DAT6_R0                          (  1 << 10)
#define B_MSDC0DAT6_R1                          (  1 <<  9)
#define B_MSDC0DAT6_PUPD                        (  1 <<  8)

#define B_MSDC0DAT5_SMT                         (  1 <<  7)
#define B_MSDC0DAT5_R0                          (  1 <<  6)
#define B_MSDC0DAT5_R1                          (  1 <<  5)
#define B_MSDC0DAT5_PUPD                        (  1 <<  4)

#define B_MSDC0DAT4_SMT                         (  1 <<  3)
#define B_MSDC0DAT4_R0                          (  1 <<  2)
#define B_MSDC0DAT4_R1                          (  1 <<  1)
#define B_MSDC0DAT4_PUPD                        (  1 <<  0)

/* 0x10005D10 MSDC0_CTRL5   MSDC 0 DAT pad Control Register  0x0004 */
#define B_MSDC0DAT_BACKUP                       (0xFFF<< 4)
#define B_MSDC0RSTB_SMT                         (  1 <<  3)
#define B_MSDC0RSTB_R0                          (  1 <<  2)
#define B_MSDC0RSTB_R1                          (  1 <<  1)
#define B_MSDC0RSTB_PUPD                        (  1 <<  0)

/* 0x10005D20 MSDC0_CTRL6   MSDC 0 pad Control Register      0x000A */
#define B_MSDC0PAD_BACKUP                       (0x3F << 10)
#define B_MSDC0PAD_RDSEL                        (0x3F <<  4)
#define B_MSDC0PAD_TDSEL                        (0x0F <<  0)

/* 
 * MSDC1  GPIO Control definition 
 *  CMD     CLK     DAT0     DAT1   DAT2    DAT3   INS
 * GPIO105  106     107      108    109     110    261  with func 1(mode 1)
 *    1      0       1        1      1       1         should be these/reset value 
 ***************************************************************************/
#define PIN4MSDC1_CLK                            106
#define PIN4MSDC1_CMD                            105
#define PIN4MSDC1_DAT0                           107
#define PIN4MSDC1_DAT1                           108
#define PIN4MSDC1_DAT2                           109
#define PIN4MSDC1_DAT3                           110
#define PIN4MSDC1_INS                            261

#define OFFSET_MSDC1_CTRL0                       0xD30
#define OFFSET_MSDC1_CTRL1                       0xD40
#define OFFSET_MSDC1_CTRL2                       0xD50
#define OFFSET_MSDC1_CTRL3                       0xD60
#define OFFSET_MSDC1_CTRL4                       0xD70
#define OFFSET_MSDC1_CTRL5                       0xD80

#define OFFSET_MSDC1_CTRL6                       0x0B0

#ifdef MSDC1_CTRL0
#undef MSDC1_CTRL0
#endif
#ifdef MSDC1_CTRL1
#undef MSDC1_CTRL1
#endif
#ifdef MSDC1_CTRL2
#undef MSDC1_CTRL2
#endif
#ifdef MSDC1_CTRL3
#undef MSDC1_CTRL3
#endif
#ifdef MSDC1_CTRL4
#undef MSDC1_CTRL4
#endif
#ifdef MSDC1_CTRL5
#undef MSDC1_CTRL5
#endif
#ifdef MSDC1_CTRL6
#undef MSDC1_CTRL6
#endif

#define GPIO_MSDC1_CTRL0                         REG_GPIO_BASE(MSDC1_CTRL0)
#define GPIO_MSDC1_CTRL1                         REG_GPIO_BASE(MSDC1_CTRL1)
#define GPIO_MSDC1_CTRL2                         REG_GPIO_BASE(MSDC1_CTRL2)
#define GPIO_MSDC1_CTRL3                         REG_GPIO_BASE(MSDC1_CTRL3)
#define GPIO_MSDC1_CTRL4                         REG_GPIO_BASE(MSDC1_CTRL4)
#define GPIO_MSDC1_CTRL5                         REG_GPIO_BASE(MSDC1_CTRL5)

#define GPIO_MSDC1_CTRL6                         REG_GPIO_BASE(MSDC1_CTRL6)

/* 0x10005D30 MSDC1_CTRL0   MSDC 1 CLK pad Control Register 0x0311 */
#define B_MSDC1CLK_BACKUP1                      (0xF << 12)
#define B_MSDC1CLK_SMT                          (  1 << 11)
#define B_MSDC1CLK_R0                           (  1 << 10)
#define B_MSDC1CLK_R1                           (  1 <<  9)
#define B_MSDC1CLK_PUPD                         (  1 <<  8)
#define B_MSDC1CLK_BACKUP0                      (0x7 <<  5)
#define B_MSDC1CLK_IES                          (  1 <<  4)
#define B_MSDC1CLK_SR                           (  1 <<  3)
#define B_MSDC1CLK_E8                           (  1 <<  2)
#define B_MSDC1CLK_E4                           (  1 <<  1)
#define B_MSDC1CLK_E2                           (  1 <<  0)

/* 0x10005D40 MSDC1_CTRL1   MSDC 1 CMD pad Control Register 0x0211 */
#define B_MSDC1CMD_BACKUP1                      (0xF << 12)
#define B_MSDC1CMD_SMT                          (  1 << 11)
#define B_MSDC1CMD_R0                           (  1 << 10)
#define B_MSDC1CMD_R1                           (  1 <<  9)
#define B_MSDC1CMD_PUPD                         (  1 <<  8)
#define B_MSDC1CMD_BACKUP0                      (0x7 <<  5)
#define B_MSDC1CMD_IES                          (  1 <<  4)
#define B_MSDC1CMD_SR                           (  1 <<  3)
#define B_MSDC1CMD_E8                           (  1 <<  2)
#define B_MSDC1CMD_E4                           (  1 <<  1)
#define B_MSDC1CMD_E2                           (  1 <<  0)

/* 0x10005D50 MSDC1_CTRL2   MSDC 1 DAT pad Control Register 0x0011 */
#define B_MSDC1DAT_BACKUP1                      (0x7FF<< 5)
#define B_MSDC1DAT_IES                          (  1 <<  4)
#define B_MSDC1DAT_SR                           (  1 <<  3)
#define B_MSDC1DAT_E8                           (  1 <<  2)
#define B_MSDC1DAT_E4                           (  1 <<  1)
#define B_MSDC1DAT_E2                           (  1 <<  0)

/* 0x10005D60 MSDC1_CTRL3   MSDC 1 DAT pad Control Register  0x2222 */
#define B_MSDC1DAT3_SMT                          (  1 << 15)
#define B_MSDC1DAT3_R0                           (  1 << 14)
#define B_MSDC1DAT3_R1                           (  1 << 13)
#define B_MSDC1DAT3_PUPD                         (  1 << 12)

#define B_MSDC1DAT2_SMT                          (  1 << 11)
#define B_MSDC1DAT2_R0                           (  1 << 10)
#define B_MSDC1DAT2_R1                           (  1 <<  9)
#define B_MSDC1DAT2_PUPD                         (  1 <<  8)

#define B_MSDC1DAT1_SMT                          (  1 <<  7)
#define B_MSDC1DAT1_R0                           (  1 <<  6)
#define B_MSDC1DAT1_R1                           (  1 <<  5)
#define B_MSDC1DAT1_PUPD                         (  1 <<  4)

#define B_MSDC1DAT0_SMT                          (  1 <<  3)
#define B_MSDC1DAT0_R0                           (  1 <<  2)
#define B_MSDC1DAT0_R1                           (  1 <<  1)
#define B_MSDC1DAT0_PUPD                         (  1 <<  0)

/* 0x10005D70 MSDC1_CTRL4   MSDC 1 DAT pad Control Register  0x0000 */
#define B_MSDC1DAT_BACKUP                        (0xFFFF<<0)

/* 0x10005D80 MSDC1_CTRL5   MSDC 1 pad Control Register      0x00CA */
#define B_MSDC1PAD_BACKUP                        (0x3F << 10)
#define B_MSDC1PAD_RDSEL                         (0x3F <<  4)
#define B_MSDC1PAD_TDSEL                         (0x0F <<  0)

/* 0x100050B0 MSDC1_CTRL6   MSDC 1 INS Pad Control Register  0x0000 */
#define B_MSDC1INS_BACKUP1                       (0xFFF<<  4)
#define B_MSDC1INS_SMT                           (  1  <<  3)
#define B_MSDC1INS_BACKUP0                       (0x07 <<  0)

/* 
 * MSDC2  GPIO Control definition 
 *  CMD     CLK   DAT0    DAT1  DAT2   DAT3
 * GPIO85   86    87      88    89     90          with func 1 (mode 1)
 *    1     0     1       1     1      1           should be these 
 ***************************************************************************/
#define PIN4MSDC2_CLK                             86
#define PIN4MSDC2_CMD                             85
#define PIN4MSDC2_DAT0                            87
#define PIN4MSDC2_DAT1                            88
#define PIN4MSDC2_DAT2                            89
#define PIN4MSDC2_DAT3                            90

#define OFFSET_MSDC2_CTRL0                        0xD90
#define OFFSET_MSDC2_CTRL1                        0xDA0
#define OFFSET_MSDC2_CTRL2                        0xDB0
#define OFFSET_MSDC2_CTRL3                        0xDC0
#define OFFSET_MSDC2_CTRL4                        0xDD0
#define OFFSET_MSDC2_CTRL5                        0xDE0

#ifdef MSDC2_CTRL0
#undef MSDC2_CTRL0
#endif
#ifdef MSDC2_CTRL1
#undef MSDC2_CTRL1
#endif
#ifdef MSDC2_CTRL2
#undef MSDC2_CTRL2
#endif
#ifdef MSDC2_CTRL3
#undef MSDC2_CTRL3
#endif
#ifdef MSDC2_CTRL4
#undef MSDC2_CTRL4
#endif
#ifdef MSDC2_CTRL5
#undef MSDC2_CTRL5
#endif
#ifdef MSDC2_CTRL6
#undef MSDC2_CTRL6
#endif


#define GPIO_MSDC2_CTRL0                          REG_GPIO_BASE(MSDC2_CTRL0)
#define GPIO_MSDC2_CTRL1                          REG_GPIO_BASE(MSDC2_CTRL1)
#define GPIO_MSDC2_CTRL2                          REG_GPIO_BASE(MSDC2_CTRL2)
#define GPIO_MSDC2_CTRL3                          REG_GPIO_BASE(MSDC2_CTRL3)
#define GPIO_MSDC2_CTRL4                          REG_GPIO_BASE(MSDC2_CTRL4)
#define GPIO_MSDC2_CTRL5                          REG_GPIO_BASE(MSDC2_CTRL5)

/* 0x10005D90 MSDC2_CTRL0   MSDC 2 CLK pad Control Register 0x0411 */
#define B_MSDC2CLK_BACKUP1                       (0xF << 12)
#define B_MSDC2CLK_SMT                           (  1 << 11)
#define B_MSDC2CLK_R0                            (  1 << 10)
#define B_MSDC2CLK_R1                            (  1 <<  9)
#define B_MSDC2CLK_PUPD                          (  1 <<  8)
#define B_MSDC2CLK_BACKUP0                       (0x7 <<  5)
#define B_MSDC2CLK_IES                           (  1 <<  4)
#define B_MSDC2CLK_SR                            (  1 <<  3)
#define B_MSDC2CLK_E8                            (  1 <<  2)
#define B_MSDC2CLK_E4                            (  1 <<  1)
#define B_MSDC2CLK_E2                            (  1 <<  0)

/* 0x10005DA0 MSDC2_CTRL1   MSDC 2 CMD pad Control Register 0x0511 */
#define B_MSDC2CMD_BACKUP1                       (0xF << 12)
#define B_MSDC2CMD_SMT                           (  1 << 11)
#define B_MSDC2CMD_R0                            (  1 << 10)
#define B_MSDC2CMD_R1                            (  1 <<  9)
#define B_MSDC2CMD_PUPD                          (  1 <<  8)
#define B_MSDC2CMD_BACKUP0                       (0x7 <<  5)
#define B_MSDC2CMD_IES                           (  1 <<  4)
#define B_MSDC2CMD_SR                            (  1 <<  3)
#define B_MSDC2CMD_E8                            (  1 <<  2)
#define B_MSDC2CMD_E4                            (  1 <<  1)
#define B_MSDC2CMD_E2                            (  1 <<  0)

/* 0x10005DB0 MSDC2_CTRL2   MSDC 2 DAT pad Control Register 0x0011 */
#define B_MSDC2DAT_BACKUP1                       (0x7FF<< 5)
#define B_MSDC2DAT_IES                           (  1 <<  4)
#define B_MSDC2DAT_SR                            (  1 <<  3)
#define B_MSDC2DAT_E8                            (  1 <<  2)
#define B_MSDC2DAT_E4                            (  1 <<  1)
#define B_MSDC2DAT_E2                            (  1 <<  0)


/* 0x10005DC0 MSDC2_CTRL3   MSDC 2 DAT pad Control Register 0x5555 */
#define B_MSDC2DAT3_SMT                          (  1 << 15)
#define B_MSDC2DAT3_R0                           (  1 << 14)
#define B_MSDC2DAT3_R1                           (  1 << 13)
#define B_MSDC2DAT3_PUPD                         (  1 << 12)

#define B_MSDC2DAT2_SMT                          (  1 << 11)
#define B_MSDC2DAT2_R0                           (  1 << 10)
#define B_MSDC2DAT2_R1                           (  1 <<  9)
#define B_MSDC2DAT2_PUPD                         (  1 <<  8)

#define B_MSDC2DAT1_SMT                          (  1 <<  7)
#define B_MSDC2DAT1_R0                           (  1 <<  6)
#define B_MSDC2DAT1_R1                           (  1 <<  5)
#define B_MSDC2DAT1_PUPD                         (  1 <<  4)

#define B_MSDC2DAT0_SMT                          (  1 <<  3)
#define B_MSDC2DAT0_R0                           (  1 <<  2)
#define B_MSDC2DAT0_R1                           (  1 <<  1)
#define B_MSDC2DAT0_PUPD                         (  1 <<  0)

/* 0x10005DD0 MSDC2_CTRL4   MSDC 2 DAT pad Control Register  0x0000 */
#define B_MSDC2DAT_BACKUP                        (0xFFFF<<0)

/* 0x10005DE0 MSDC2_CTRL5   MSDC 2 pad Control Register      0x0000 */
#define B_MSDC2PAD_BACKUP                        (0x3F << 10)
#define B_MSDC2PAD_RDSEL                         (0x3F <<  4)
#define B_MSDC2PAD_TDSEL                         (0x0F <<  0)

/*
 * MSDC3  GPIO Control definition 
 *  DAT7   DAT6 DAT5 DAT4 DAT3 DAT2 DAT1 DAT0 RSTB CMD CLK DSL
 * GPIO250 251  252  253  254  255  256  257  249  258 259 260
 *   1      1    1    1    1    1    1    1    0    1   0   1     
 ***************************************************************************/
#define PIN4MSDC3_CLK                             259
#define PIN4MSDC3_CMD                             258
#define PIN4MSDC3_RSTB                            249
#define PIN4MSDC3_DAT0                            257
#define PIN4MSDC3_DAT1                            256
#define PIN4MSDC3_DAT2                            255
#define PIN4MSDC3_DAT3                            254
#define PIN4MSDC3_DAT4                            253
#define PIN4MSDC3_DAT5                            252
#define PIN4MSDC3_DAT6                            251
#define PIN4MSDC3_DAT7                            250
#define PIN4MSDC3_DSL                             260

#define OFFSET_MSDC3_CTRL0                        0xC90
#define OFFSET_MSDC3_CTRL1                        0xCB0
#define OFFSET_MSDC3_CTRL2                        0xFC0
#define OFFSET_MSDC3_CTRL3                        0xF40
#define OFFSET_MSDC3_CTRL4                        0x130
#define OFFSET_MSDC3_CTRL5                        0x140
#define OFFSET_MSDC3_CTRL6                        0x620

#define OFFSET_MSDC3_CTRL7                        0x3A0
#define OFFSET_MSDC3_CTRL8                        0x430

#ifdef MSDC3_CTRL0
#undef MSDC3_CTRL0
#endif
#ifdef MSDC3_CTRL1
#undef MSDC3_CTRL1
#endif
#ifdef MSDC3_CTRL2
#undef MSDC3_CTRL2
#endif
#ifdef MSDC3_CTRL3
#undef MSDC3_CTRL3
#endif
#ifdef MSDC3_CTRL4
#undef MSDC3_CTRL4
#endif
#ifdef MSDC3_CTRL5
#undef MSDC3_CTRL5
#endif
#ifdef MSDC3_CTRL6
#undef MSDC3_CTRL6
#endif
#ifdef MSDC3_CTRL7
#undef MSDC3_CTRL7
#endif
#ifdef MSDC3_CTRL8
#undef MSDC3_CTRL8
#endif

#define GPIO_MSDC3_CTRL0                          REG_GPIO_BASE(MSDC3_CTRL0)
#define GPIO_MSDC3_CTRL1                          REG_GPIO_BASE(MSDC3_CTRL1)
#define GPIO_MSDC3_CTRL2                          REG_GPIO_BASE(MSDC3_CTRL2)
#define GPIO_MSDC3_CTRL3                          REG_GPIO_BASE(MSDC3_CTRL3)
#define GPIO_MSDC3_CTRL4                          REG_GPIO_BASE(MSDC3_CTRL4)
#define GPIO_MSDC3_CTRL5                          REG_GPIO_BASE(MSDC3_CTRL5)
#define GPIO_MSDC3_CTRL6                          REG_GPIO_BASE(MSDC3_CTRL6)         

#define GPIO_MSDC3_CTRL7                          REG_GPIO_BASE(MSDC3_CTRL7)
#define GPIO_MSDC3_CTRL8                          REG_GPIO_BASE(MSDC3_CTRL8)   

/* 0x10005C90 MSDC3_CTRL0   MSDC 3 CLK pad Control Register 0x0314 */
#define B_MSDC3CLK_BACKUP1                       (0xF << 12)
#define B_MSDC3CLK_SMT                           (  1 << 11)
#define B_MSDC3CLK_R0                            (  1 << 10)
#define B_MSDC3CLK_R1                            (  1 <<  9)
#define B_MSDC3CLK_PUPD                          (  1 <<  8)
#define B_MSDC3CLK_BACKUP0                       (0x7 <<  5)
#define B_MSDC3CLK_IES                           (  1 <<  4)
#define B_MSDC3CLK_SR                            (  1 <<  3)
#define B_MSDC3CLK_E8                            (  1 <<  2)
#define B_MSDC3CLK_E4                            (  1 <<  1)
#define B_MSDC3CLK_E2                            (  1 <<  0)

/* 0x10005CB0 MSDC3_CTRL1   MSDC 3 CMD pad Control Register 0x0314 */
#define B_MSDC3CMD_BACKUP1                       (0xF << 12)
#define B_MSDC3CMD_SMT                           (  1 << 11)
#define B_MSDC3CMD_R0                            (  1 << 10)
#define B_MSDC3CMD_R1                            (  1 <<  9)
#define B_MSDC3CMD_PUPD                          (  1 <<  8)
#define B_MSDC3CMD_BACKUP0                       (0x7 <<  5)
#define B_MSDC3CMD_IES                           (  1 <<  4)
#define B_MSDC3CMD_SR                            (  1 <<  3)
#define B_MSDC3CMD_E8                            (  1 <<  2)
#define B_MSDC3CMD_E4                            (  1 <<  1)
#define B_MSDC3CMD_E2                            (  1 <<  0)

/* 0x10005FC0 MSDC3_CTRL2   MSDC 3 DAT pad Control Register 0x0014 */
#define B_MSDC3DAT_BACKUP1                       (0x7FF<< 5)
#define B_MSDC3DAT_IES                           (  1 <<  4)
#define B_MSDC3DAT_SR                            (  1 <<  3)
#define B_MSDC3DAT_E8                            (  1 <<  2)
#define B_MSDC3DAT_E4                            (  1 <<  1)
#define B_MSDC3DAT_E2                            (  1 <<  0)


/* 0x10005F40 MSDC3_CTRL3   MSDC 3 DAT pad Control Register 0x4444 */
#define B_MSDC3DAT3_SMT                          (  1 << 15)
#define B_MSDC3DAT3_R0                           (  1 << 14)
#define B_MSDC3DAT3_R1                           (  1 << 13)
#define B_MSDC3DAT3_PUPD                         (  1 << 12)

#define B_MSDC3DAT2_SMT                          (  1 << 11)
#define B_MSDC3DAT2_R0                           (  1 << 10)
#define B_MSDC3DAT2_R1                           (  1 <<  9)
#define B_MSDC3DAT2_PUPD                         (  1 <<  8)

#define B_MSDC3DAT1_SMT                          (  1 <<  7)
#define B_MSDC3DAT1_R0                           (  1 <<  6)
#define B_MSDC3DAT1_R1                           (  1 <<  5)
#define B_MSDC3DAT1_PUPD                         (  1 <<  4)

#define B_MSDC3DAT0_SMT                          (  1 <<  3)
#define B_MSDC3DAT0_R0                           (  1 <<  2)
#define B_MSDC3DAT0_R1                           (  1 <<  1)
#define B_MSDC3DAT0_PUPD                         (  1 <<  0)


/* 0x10005130 MSDC3_CTRL4   MSDC 3 DAT pad Control Register 0x4444 */
#define B_MSDC3DAT7_SMT                          (  1 << 15)
#define B_MSDC3DAT7_R0                           (  1 << 14)
#define B_MSDC3DAT7_R1                           (  1 << 13)
#define B_MSDC3DAT7_PUPD                         (  1 << 12)

#define B_MSDC3DAT6_SMT                          (  1 << 11)
#define B_MSDC3DAT6_R0                           (  1 << 10)
#define B_MSDC3DAT6_R1                           (  1 <<  9)
#define B_MSDC3DAT6_PUPD                         (  1 <<  8)

#define B_MSDC3DAT5_SMT                          (  1 <<  7)
#define B_MSDC3DAT5_R0                           (  1 <<  6)
#define B_MSDC3DAT5_R1                           (  1 <<  5)
#define B_MSDC3DAT5_PUPD                         (  1 <<  4)

#define B_MSDC3DAT4_SMT                          (  1 <<  3)
#define B_MSDC3DAT4_R0                           (  1 <<  2)
#define B_MSDC3DAT4_R1                           (  1 <<  1)
#define B_MSDC3DAT4_PUPD                         (  1 <<  0)

/* 0x10005140 MSDC3_CTRL5   MSDC 3 DAT pad Control Register 0x0404 */
#define B_MSDC3DAT_BACKUP                        (  1 << 15)

#define B_MSDC3DSL_SMT                           (  1 <<  7)
#define B_MSDC3DSL_R0                            (  1 <<  6)
#define B_MSDC3DSL_R1                            (  1 <<  5)
#define B_MSDC3DSL_PUPD                          (  1 <<  4)

#define B_MSDC3RSTB_SMT                          (  1 <<  3)
#define B_MSDC3RSTB_R0                           (  1 <<  2)
#define B_MSDC3RSTB_R1                           (  1 <<  1)
#define B_MSDC3RSTB_PUPD                         (  1 <<  0)

/* 0x10005620 MSDC3_CTRL6   MSDC 3 pad Control Register      0x00AC */
#define B_MSDC3PAD_BACKUP                        (0x3F << 10)
#define B_MSDC3PAD_RDSEL                         (0x3F <<  4)
#define B_MSDC3PAD_TDSEL                         (0x0F <<  0)

/* 0x100053A0 MSDC3_CTRL7   MSDC 3 RCLK pad Control Register 0x0414 */
#define B_MSDC3RCLK_BACKUP1                      (0xF << 12)
#define B_MSDC3RCLK_SMT                          (  1 << 11)
#define B_MSDC3RCLK_R0                           (  1 << 10)
#define B_MSDC3RCLK_R1                           (  1 <<  9)
#define B_MSDC3RCLK_PUPD                         (  1 <<  8)
#define B_MSDC3RCLK_BACKUP0                      (0x7 <<  5)
#define B_MSDC3RCLK_IES                          (  1 <<  4)
#define B_MSDC3RCLK_SR                           (  1 <<  3)
#define B_MSDC3RCLK_E8                           (  1 <<  2)
#define B_MSDC3RCLK_E4                           (  1 <<  1)
#define B_MSDC3RCLK_E2                           (  1 <<  0)

/* 0x10005430 MSDC3_CTRL8   MSDC 3 RCLK SEL Control Register   0x00AC */
#define B_MSDC3RCLK_BACKUP                       (0x3F << 10)
#define B_MSDC3RCLK_RDSEL                        (0x3F <<  4)
#define B_MSDC3RCLK_TDSEL                        (0x0F <<  0)

typedef enum __MSDC_PIN_STATE {
     MSDC_PST_PU_HIGHZ = 0,
     MSDC_PST_PD_HIGHZ,				 
     MSDC_PST_PU_50KOHM,               
     MSDC_PST_PD_50KOHM,				 
     MSDC_PST_PU_10KOHM,               
     MSDC_PST_PD_10KOHM,				 
     MSDC_PST_PU_08KOHM,               
     MSDC_PST_PD_08KOHM,
     MSDC_PST_MAX
}MSDC_PIN_STATE;

/*
  *  this definition is used for MT7623_TOPCKGEN_Registers.docx by chengbin.Gao 
  *  Zhanyong.Wang
  *  2014/12/14
  */
#ifndef TOPCKGEN_BASE
#define TOPCKGEN_BASE                             0x10000000
#endif
#if TOPCKGEN_BASE != 0x10000000
#error  Please check your TOPCKGEN_BASE definition!!!!
#endif

#define REG_TOPCKGEN_BASE(x)                     ((volatile unsigned int *)(TOPCKGEN_BASE + OFFSET_##x))

/*valid reg name list: 
    CLKMODE, CLKCFGUPDATE, 
    CLKCFG2 /CLKCFG2_SET/CLKCFG2_CLR, 
    CLKCFG3 /CLKCFG3_SET/CLKCFG3_CLR
    CLKCFG14/CLKCFG14_SET/CLKCFG14_CLR
    */
#define TOPCKGEN_CTRL(topckgenreg)               TOPCKGEN_##topckgenreg

/*
  * id valid range "MSDC[0, 3]"
  * reg valid range "CLKCFG2/3/14" 
  */

#define MSDC_CKGEN_SEL(id,reg)                    B_CKGEN_##reg##_##id##_SEL
#define MSDC_CKGEN_INV(id,reg)                    B_CKGEN_##reg##_##id##_INV
#define MSDC_CKGEN_PDN(id,reg)                    B_CKGEN_##reg##_##id##_PDN
#define MSDC_CKGEN_UPDATE(id)                     B_CKGEN_CLKCFGUPDATE_##id

#define MSDC_CKGENHCLK_SEL(id,reg)                B_CKGEN_##reg##_##id##HCLK_SEL
#define MSDC_CKGENHCLK_INV(id,reg)                B_CKGEN_##reg##_##id##HCLK_INV
//#define MSDC_CKGENHCLK_PDN(id,reg)                B_CKGEN_##reg##_##id##HCLK_PDN
#define MSDC_CKGENHCLK_UPDATE(id)                 B_CKGEN_CLKCFGUPDATE_##id##HCLK

#define OFFSET_TOPCKGEN_CLK_MODE                  0x0000
#define OFFSET_TOPCKGEN_CLK_CFG_UPDATE            0x0004
#define OFFSET_TOPCKGEN_CLK_CFG2                  0x0060
#define OFFSET_TOPCKGEN_CLK_CFG2_SET              0x0064
#define OFFSET_TOPCKGEN_CLK_CFG2_CLR              0x0068
#define OFFSET_TOPCKGEN_CLK_CFG3                  0x0070
#define OFFSET_TOPCKGEN_CLK_CFG3_SET              0x0074
#define OFFSET_TOPCKGEN_CLK_CFG3_CLR              0x0078
#define OFFSET_TOPCKGEN_CLK_CFG6                 0x00A0
#define OFFSET_TOPCKGEN_CLK_CFG6_SET             0x00A4
#define OFFSET_TOPCKGEN_CLK_CFG6_CLR             0x00A8
#define OFFSET_TOPCKGEN_CLK_CFG14                 0x00E0
#define OFFSET_TOPCKGEN_CLK_CFG14_SET             0x00E4
#define OFFSET_TOPCKGEN_CLK_CFG14_CLR             0x00E8


/* 
  * 0x10000000 CLK_MODE  Clock 26MHz, 32KHz PDN Control Register  0x00000000 
  *************************************************************************/
  
#define TOPCKGEN_CLKMODE                          REG_TOPCKGEN_BASE(TOPCKGEN_CLK_MODE)
#define B_CKGEN_CLKMODE_CGDIS                     (1 <<  0)
#define B_CKGEN_CLKMODE_PDNMD32KHZ                (1 <<  8)
#define B_CKGEN_CLKMODE_PDNCONN32KHZ              (1 << 10)

/*
 *
 * 0x10000060 CLK_CFG_2  Function Clock selection Register 2  0x00000000 
 * 0x10000064 SET CLK_CFG_2  Function Clock selection Register 2  0x00000000 
 * 0x10000068 CLR CLK_CFG_2  Function Clock selection Register 2  0x00000000 
 *************************************************************************/
#define TOPCKGEN_CLKCFG2                          REG_TOPCKGEN_BASE(TOPCKGEN_CLK_CFG2)
#define TOPCKGEN_CLKCFG2_SET                      REG_TOPCKGEN_BASE(TOPCKGEN_CLK_CFG2_SET)
#define TOPCKGEN_CLKCFG2_CLR                      REG_TOPCKGEN_BASE(TOPCKGEN_CLK_CFG2_CLR)
#define B_CKGEN_CLKCFG2_MSDC0_SEL                 (0x7 << 24)
#define B_CKGEN_CLKCFG2_MSDC0_INV                 (  1 << 28)
#define B_CKGEN_CLKCFG2_MSDC0_PDN                 (  1 << 31)


/*
 *
 * 0x10000070 CLK_CFG_3  Function Clock selection Register 3  0x00000000 
 * 0x10000074 SET CLK_CFG_3  Function Clock selection Register 3  0x00000000 
 * 0x10000078 CLR CLK_CFG_3  Function Clock selection Register 3  0x00000000 
 *************************************************************************/
#define TOPCKGEN_CLKCFG3                         REG_TOPCKGEN_BASE(TOPCKGEN_CLK_CFG3)
#define TOPCKGEN_CLKCFG3_SET                     REG_TOPCKGEN_BASE(TOPCKGEN_CLK_CFG3_SET)
#define TOPCKGEN_CLKCFG3_CLR                     REG_TOPCKGEN_BASE(TOPCKGEN_CLK_CFG3_CLR)
#define B_CKGEN_CLKCFG3_MSDC1_SEL                 (0x7 <<  0)
#define B_CKGEN_CLKCFG3_MSDC1_INV                 (  1 <<  4)
#define B_CKGEN_CLKCFG3_MSDC1_PDN                 (  1 <<  7)

#define B_CKGEN_CLKCFG3_MSDC2_SEL                 (0x7 <<  8)
#define B_CKGEN_CLKCFG3_MSDC2_INV                 (  1 << 12)
#define B_CKGEN_CLKCFG3_MSDC2_PDN                 (  1 << 15)

/*
 *
 * 0x100000A0 CLK_CFG_6  Function Clock selection Register 6  0x00000000 
 * 0x100000A4 SET CLK_CFG_6  Function Clock selection Register 6  0x00000000 
 * 0x100000A8 CLR CLK_CFG_6  Function Clock selection Register 6  0x00000000 
 *************************************************************************/
#define TOPCKGEN_CLKCFG6                          REG_TOPCKGEN_BASE(TOPCKGEN_CLK_CFG6)
#define TOPCKGEN_CLKCFG6_SET                      REG_TOPCKGEN_BASE(TOPCKGEN_CLK_CFG6_SET)
#define TOPCKGEN_CLKCFG6_CLR                      REG_TOPCKGEN_BASE(TOPCKGEN_CLK_CFG6_CLR)
#define B_CKGEN_CLKCFG6_MSDC3HCLK_SEL             (0x3 << 24)
//#define B_CKGEN_CLKCFG6_MSDC3HCLK_INV                  (  1 << 28)
#define B_CKGEN_CLKCFG6_MSDC3HCLK_PDN             (  1 << 31)

/*
 *
 * 0x100000E0 CLK_CFG_14  Function Clock selection Register 14  0x00000000 
 * 0x100000E4 SET CLK_CFG_14  Function Clock selection Register 14  0x00000000 
 * 0x100000E8 CLR CLK_CFG_14  Function Clock selection Register 14  0x00000000 
 *************************************************************************/
#define TOPCKGEN_CLKCFG14                         REG_TOPCKGEN_BASE(TOPCKGEN_CLK_CFG14)
#define TOPCKGEN_CLKCFG14_SET                     REG_TOPCKGEN_BASE(TOPCKGEN_CLK_CFG14_SET)
#define TOPCKGEN_CLKCFG14_CLR                     REG_TOPCKGEN_BASE(TOPCKGEN_CLK_CFG14_CLR)
#define B_CKGEN_CLKCFG14_MSDC3_SEL                (0x7 <<  8)
#define B_CKGEN_CLKCFG14_MSDC3_INV                (  1 << 12)
#define B_CKGEN_CLKCFG14_MSDC3_PDN                (  1 << 15)


/* each PLL have different gears for select
 * software can used mux interface from clock management module to select */
enum {
    MSDC50_CLKSRC4HCLK_26MHZ  = 0,
    MSDC50_CLKSRC4HCLK_273MHZ,  
    MSDC50_CLKSRC4HCLK_182MHZ,  
    MSDC50_CLKSRC4HCLK_78MHZ,
    MSDC_DONOTCARE_HCLK,
    MSDC50_CLKSRC4HCLK_MAX
};

enum {
    MSDC50_CLKSRC_26MHZ  = 0,
    MSDC50_CLKSRC_400MHZ,  /* MSDCPLL_CK */
    MSDC50_CLKSRC_182MHZ,  /*MSDCPLL_D2 */
    MSDC50_CLKSRC_136MHZ,
    MSDC50_CLKSRC_156MHZ,
    MSDC50_CLKSRC_200MHZ,  /*MSDCPLL_D4 */
    MSDC50_CLKSRC_100MHZ,
    MSDC50_CLKSRC_50MHZ,
    MSDC50_CLKSRC_MAX
};

/* MSDC0/1/2 
     PLL MUX SEL List */
enum {
    MSDC30_CLKSRC_26MHZ   = 0,
    MSDC30_CLKSRC_200MHZ,
    MSDC30_CLKSRC_182MHZ,
    MSDC30_CLKSRC_91MHZ,
    MSDC30_CLKSRC_156MHZ,
    MSDC30_CLKSRC_104MHZ,
    MSDC30_CLKSRC_MAX
};

#define MSDC50_CLKSRC_DEFAULT     MSDC50_CLKSRC_200MHZ
#define MSDC30_CLKSRC_DEFAULT     MSDC30_CLKSRC_200MHZ

typedef enum MT65XX_POWER_VOL_TAG
{
    VOL_DEFAULT,
    VOL_0900 = 900,
    VOL_1000 = 1000,
    VOL_1100 = 1100,
    VOL_1200 = 1200,
    VOL_1300 = 1300,
    VOL_1350 = 1350,
    VOL_1500 = 1500,
    VOL_1800 = 1800,
    VOL_2000 = 2000,
    VOL_2100 = 2100,
    VOL_2500 = 2500,
    VOL_2800 = 2800,
    VOL_3000 = 3000,
    VOL_3300 = 3300,
    VOL_3400 = 3400,
    VOL_3500 = 3500,
    VOL_3600 = 3600
} MT65XX_POWER_VOLTAGE;

/*--------------------------------------------------------------------------*/
/* Descriptor Structure                                                     */
/*--------------------------------------------------------------------------*/
typedef struct {
    uint32  hwo       :1; /* could be changed by hw */
    uint32  bdp       :1;
    uint32  rsv0      :6;	
    uint32  chksum    :8;
    uint32  intr      :1;
    uint32  rsv1      :7;
    uint32  nxtgpdh4b :4; /* !!!MT7623 Add*/
    uint32  nxtbdh4b  :4; /* !!!MT7623 Add*/
	
    void   *next;
	
    void   *ptr;
	
    uint32  buflen:24;  /* bitwidth 16 -> 24 !!!MT7623 Change*/
    uint32  extlen:8;   /* bit start   8 -> 24 !!!MT7623 Change*/
    //uint32  rsv2:8;     /* bit start  24 -> 31 !!!MT7623 removed*/
    
    uint32  arg;
	
    uint32  blknum;
	
    uint32  cmd;
} gpd_t;


typedef struct {
    uint32  eol       :1;
    uint32  rsv0      :7;	
    uint32  chksum    :8;	
    uint32  rsv1      :1;
    uint32  blkpad    :1;
    uint32  dwpad     :1;	
    uint32  rsv2      :5;	
    uint32  nxtbdh4b  :4; /* !!!MT7623 Add*/
    uint32  nxtDath4b :4; /* !!!MT7623 Add*/
	
    void   *next;
	
    void   *ptr;
	
    uint32  buflen    :24; /* bitwidth 16 -> 24 !!!MT7623 Change*/
    uint32  rsv3      : 8; /* bitwidth 16 -> 8 !!!MT7623 Change*/
} __bd_t;


struct scatterlist {
    u32 addr;
    u32 len;
};

struct scatterlist_ex {
    u32 cmd;
    u32 arg;
    u32 sglen;
    struct scatterlist *sg;
};

#define DMA_FLAG_NONE       (0x00000000)
#define DMA_FLAG_EN_CHKSUM  (0x00000001)
#define DMA_FLAG_PAD_BLOCK  (0x00000002)
#define DMA_FLAG_PAD_DWORD  (0x00000004)

struct dma_config {
    u32 flags;                   /* flags */
    u32 xfersz;                  /* xfer size in bytes */
    u32 sglen;                   /* size of scatter list */
    u32 blklen;                  /* block size */
    struct scatterlist *sg;      /* I/O scatter list */
    struct scatterlist_ex *esg;  /* extended I/O scatter list */
    u8  mode;                    /* dma mode        */
    u8  burstsz;                 /* burst size      */
    u8  intr;                    /* dma done interrupt */
    u8  padding;                 /* padding */
    u32 cmd;                     /* enhanced mode command */
    u32 arg;                     /* enhanced mode arg */
    u32 rsp;                     /* enhanced mode command response */
    u32 autorsp;                 /* auto command response */
};

#if MSDC_USE_REG_OPS_DUMP
static void reg32_write(volatile uint32 *addr, uint32 data)
{
    *addr = (uint32)data;
    MSDC_DBG_PRINT(MSDC_HREG,("[WR32] %x = %x\n", addr, data));
}

static uint32 reg32_read(volatile uint32 *addr)
{
    uint32 data = *(volatile uint32*)(addr);
    MSDC_DBG_PRINT(MSDC_HREG,("[RD32] %x = %x\n", addr, data));
    return data;
}

static void reg16_write(volatile uint32 *addr, uint16 data)
{
    *(volatile uint16*)(addr) = data;
    MSDC_DBG_PRINT(MSDC_HREG,("[WR16] %x = %x\n", addr, data));
}

static uint16 reg16_read(volatile uint32 *addr)
{
    uint16 data = *(volatile uint16*)addr;
    MSDC_DBG_PRINT(MSDC_HREG,("[RD16] %x = %x\n", addr, data));
    return data;
}

static void reg8_write(volatile uint32 *addr, uint8 data)
{
    *(volatile uint8*)(addr) = data;
    MSDC_DBG_PRINT(MSDC_HREG,("[WR8] %x = %x\n", addr, data));
}

static uint8 reg8_read(volatile uint32 *addr)
{
    uint8 data = *(volatile uint8*)addr;
    MSDC_DBG_PRINT(MSDC_HREG,("[RD8] %x = %x\n", addr, data));
    return data;
}

#define MSDC_WRITE32(addr,data)		reg32_write((volatile uint32*)addr, data)
#define MSDC_READ32(addr)		    reg32_read((volatile uint32*)addr)
#define MSDC_WRITE16(addr,data)		reg16_write((volatile uint32*)addr, data)
#define MSDC_READ16(addr)		    reg16_read((volatile uint32*)addr)
#define MSDC_WRITE8(addr, data)     reg8_write((volatile uint32*)addr, data)
#define MSDC_READ8(addr)            reg8_read((volatile uint32*)addr)
#define MSDC_SET_BIT32(addr,mask)	\
    do { \
        (*(volatile uint32*)(addr) |= (mask)); \
        MSDC_DBG_PRINT(MSDC_HREG,("[SET32] %x |= %x\n", addr, mask)); \
    }while(0)
#define MSDC_CLR_BIT32(addr,mask)	\
    do { \
        (*(volatile uint32*)(addr) &= ~(mask)); \
        MSDC_DBG_PRINT(MSDC_HREG,("[CLR32] %x &= ~%x\n", addr, mask)); \
    }while(0)
#define MSDC_SET_BIT16(addr,mask)	\
    do { \
        (*(volatile uint16*)(addr) |= (mask)); \
        MSDC_DBG_PRINT(MSDC_HREG,("[SET16] %x |= %x\n", addr, mask)); \
    }while(0)
#define MSDC_CLR_BIT16(addr,mask)	\
    do { \
        (*(volatile uint16*)(addr) &= ~(mask)); \
        MSDC_DBG_PRINT(MSDC_HREG,("[CLR16] %x &= ~%x\n", addr, mask)); \
    }while(0)
#else
#if 1
#define MSDC_WRITE32(addr,data)		(*(volatile uint32*)(addr) = (uint32)(data))
#define MSDC_READ32(addr)		    (*(volatile uint32*)(addr))
#define MSDC_WRITE16(addr,data)		(*(volatile uint16*)(addr) = (uint16)(data))
#define MSDC_READ16(addr)		    (*(volatile uint16*)(addr))
#define MSDC_WRITE8(addr, data)     (*(volatile uint8*)(addr)  = (uint8)(data))
#define MSDC_READ8(addr)            (*(volatile uint8*)(addr))
#define MSDC_SET_BIT32(addr,mask)	(*(volatile uint32*)(addr) |= (mask))
#define MSDC_CLR_BIT32(addr,mask)	(*(volatile uint32*)(addr) &= ~(mask))
#define MSDC_SET_BIT16(addr,mask)	(*(volatile uint16*)(addr) |= (mask))
#define MSDC_CLR_BIT16(addr,mask)	(*(volatile uint16*)(addr) &= ~(mask))
#else
#define MSDC_WRITE32(addr,data)		mt65xx_reg_sync_writel(data,addr)
#define MSDC_READ32(addr)		    (*(volatile uint32*)(addr))
#define MSDC_WRITE16(addr,data)		mt65xx_reg_sync_writew(data,addr)
#define MSDC_READ16(addr)		    (*(volatile uint16*)(addr))
#define MSDC_WRITE8(addr, data)     mt65xx_reg_sync_writeb(data,addr)
#define MSDC_READ8(addr)            (*(volatile uint8*)(addr))
#define MSDC_SET_BIT32(addr,mask) \
	do {	\
	unsigned int tv = MSDC_READ32(addr); \
	tv |=((u32)(mask)); \
	MSDC_WRITE32(addr,tv); \
} while(0)
#define MSDC_CLR_BIT32(addr,mask) \
	do {	\
	unsigned int tv = MSDC_READ32(addr); \
	tv &= ~((u32)(mask)); \
	MSDC_WRITE32(addr,tv); \
} while(0)
#define MSDC_SET_BIT16(addr,mask) \
	do {	\
	unsigned short tv = MSDC_READ16(addr); \
	tv |= ((u16)(mask)); \
	MSDC_WRITE16(addr,tv); \
} while(0)
#define MSDC_CLR_BIT16(addr,mask) \
	do {	\
	unsigned short tv = MSDC_READ16(addr); \
	tv &= ~((u16)(mask)); \
	MSDC_WRITE16(addr,tv); \
} while(0)

#define __msdc_set_field(reg,field,val) \
			do{ \
			u32 tv = MSDC_READ32(reg); \
			tv &= ~((u32)(field)); \
			tv |= ((val) << (msdc_uffs((unsigned int)field) - 1)); \
			MSDC_WRITE32(reg,tv); \
		}while(0)

#endif
#endif
#ifdef ___MSDC_DEBUG___
#define DBGCHK(field, val)                 (((~(field)) & ((val) << (uffs(field) - 1))) ? ___printf("%s !!!WARNING Override(F.%x vs V.%x):%s@%u\n",WHEREDOESMSDCRUN,field, ((val) << (uffs(field) - 1)),__func__, __LINE__), 1 : 0)
#define DMAGPDPTR_DBGCHK(gpdaddr, align)   ((((unsigned int)(gpdaddr)) & ((align)-1)) ? ___printf("%s !!!ERROR GPD Address has required HW %uB alignment %x):%s@%u\n",WHEREDOESMSDCRUN,(gpdaddr), __func__, __LINE__), 1 : 0)
#define DMABDPTR_DBGCHK(bdaddr, align)     ((((unsigned int)(bdaddr))  & ((align)-1)) ? ___printf("%s !!!ERROR BD  Address has required HW %uB alignment %x):%s@%u\n",WHEREDOESMSDCRUN,(bdaddr) , __func__, __LINE__), 1 : 0)
#define DMAARGPTR_DBGCHK(arg, align)       (((unsigned int)((arg))     & ((align)-1)) ? ___printf("%s !!!ERROR ARG Address has required HW %uB alignment %x):%s@%u\n",WHEREDOESMSDCRUN,(arg)    , __func__, __LINE__), 1 : 0)
#else 
#define DBGCHK(field, val)              
#define DMAGPDPTR_DBGCHK(gpdaddr, align)  
#define DMABDPTR_DBGCHK(bdaddr, align)    
#define DMAARGPTR_DBGCHK(arg, align)      
#endif

#define MSDC_SET_FIELD(reg,field,val) msdc_set_field(reg,field,val)
#define MSDC_GET_FIELD(reg,field,val) get_field(reg,field,val)
#define MSDC_SET_FIELD_DISCRETE(reg,field,val) \
do {	\
	unsigned int tv = MSDC_READ32(reg); \
	tv = (val == 1) ? (tv|(field)):(tv & ~(field));\
	MSDC_WRITE32(reg,tv); \
} while(0)

#define MSDC_GET_FIELD_DISCRETE(reg,field,val) \
do {	\
	unsigned int tv = (unsigned int)(*(volatile u32*)(reg)); \
	val = tv & (field) ; \
	val = (val != 0) ? 1 :0;\
} while(0)

#define MSDC_RETRY(expr,retry,cnt) \
    do { \
        uint32 t = cnt; \
        uint32 r = retry; \
        uint32 c = cnt; \
        while (r) { \
            if (!(expr)) break; \
            if (c-- == 0) { \
                r--; udelay(200); c = t; \
            } \
        } \
        MSDC_BUGON(r); \
    } while(0)

#define MSDC_RESET() \
    do { \
        MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_RST); \
        MSDC_RETRY(MSDC_READ32(MSDC_CFG) & MSDC_CFG_RST, 5, 1000); \
    } while(0)

#define MSDC_CLR_INT() \
    do { \
        volatile uint32 val = MSDC_READ32(MSDC_INT); \
        MSDC_WRITE32(MSDC_INT, val); \
        if (MSDC_READ32(MSDC_INT)) { \
            MSDC_ERR_PRINT(MSDC_ERROR,("[ASSERT] MSDC_INT is NOT clear\n")); \
        } \
    } while(0)
        
#define MSDC_CLR_FIFO() \
    do { \
        MSDC_SET_BIT32(MSDC_FIFOCS, MSDC_FIFOCS_CLR); \
        MSDC_RETRY(MSDC_READ32(MSDC_FIFOCS) & MSDC_FIFOCS_CLR, 5, 1000); \
    } while(0)
   
#define MSDC_FIFO_WRITE32(val)  MSDC_WRITE32(MSDC_TXDATA, val)
#define MSDC_FIFO_READ32()      MSDC_READ32(MSDC_RXDATA)
#define MSDC_FIFO_WRITE16(val)  MSDC_WRITE16(MSDC_TXDATA, val)
#define MSDC_FIFO_READ16()      MSDC_READ16(MSDC_RXDATA)
#define MSDC_FIFO_WRITE8(val)   MSDC_WRITE8(MSDC_TXDATA, val)
#define MSDC_FIFO_READ8()       MSDC_READ8(MSDC_RXDATA)

#define MSDC_FIFO_WRITE(val)	MSDC_FIFO_WRITE32(val)
#define MSDC_FIFO_READ()		MSDC_FIFO_READ32()

#define MSDC_TXFIFOCNT() \
    ((MSDC_READ32(MSDC_FIFOCS) & MSDC_FIFOCS_TXCNT) >> 16)
#define MSDC_RXFIFOCNT() \
    ((MSDC_READ32(MSDC_FIFOCS) & MSDC_FIFOCS_RXCNT) >> 0)
            
#define MSDC_CARD_DETECTION_ON()  MSDC_SET_BIT32(MSDC_PS, MSDC_PS_CDEN)
#define MSDC_CARD_DETECTION_OFF() MSDC_CLR_BIT32(MSDC_PS, MSDC_PS_CDEN)	
    
#define MSDC_DMA_ON()   MSDC_CLR_BIT32(MSDC_CFG, MSDC_CFG_PIO)
#define MSDC_DMA_OFF()  MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_PIO)

#define SDC_IS_BUSY()	    (MSDC_READ32(SDC_STS) & SDC_STS_SDCBUSY)
#define SDC_IS_CMD_BUSY()	(MSDC_READ32(SDC_STS) & SDC_STS_CMDBUSY)
    
#define SDC_SEND_CMD(cmd,arg) \
    do { \
        MSDC_WRITE32(SDC_ARG, (arg)); \
        MSDC_WRITE32(SDC_CMD, (cmd)); \
    } while(0)

#define MSDC_INIT_GPD_EX(gpd,extlen,cmd,arg,blknum) \
    do { \
        ((gpd_t*)gpd)->extlen = extlen; \
        ((gpd_t*)gpd)->cmd    = cmd; \
        ((gpd_t*)gpd)->arg    = arg; \
        ((gpd_t*)gpd)->blknum = blknum; \
        DMAARGPTR_DBGCHK(arg,16);\
    }while(0)

#define MSDC_INIT_BD(bd, blkpad, dwpad, dptr, dlen) \
    do { \
        MSDC_BUGON(dlen < 0x10000UL); \
        ((__bd_t*)bd)->blkpad = blkpad; \
        ((__bd_t*)bd)->dwpad  = dwpad; \
        ((__bd_t*)bd)->ptr    = (void*)dptr; \
        ((__bd_t*)bd)->buflen = dlen; \
    }while(0)

#ifdef MMC_PROFILING
static inline void msdc_timer_init(void)
{
    /* clear. CLR[1]=1, EN[0]=0 */
    __raw_writel(0x0, GPT_BASE + 0x30);
    __raw_writel(0x2, GPT_BASE + 0x30);

    __raw_writel(0, GPT_BASE + 0x38);
    __raw_writel(32768, GPT_BASE + 0x3C);

    /* 32678HZ RTC free run */
    __raw_writel(0x30, GPT_BASE + 0x34);
    __raw_writel(0x32, GPT_BASE + 0x30);
}
static inline void msdc_timer_start(void)
{
    *(volatile unsigned int*)(GPT_BASE + 0x30) |= (1 << 0);
}
static inline void msdc_timer_stop(void)
{
    *(volatile unsigned int*)(GPT_BASE + 0x30) &= ~(1 << 0);
}
static inline void msdc_timer_stop_clear(void)
{
    *(volatile unsigned int*)(GPT_BASE + 0x30) &= ~(1 << 0); /* stop  */
    *(volatile unsigned int*)(GPT_BASE + 0x30) |= (1 << 1);  /* clear */
}
static inline unsigned int msdc_timer_get_count(void)
{
    return __raw_readl(GPT_BASE + 0x38);
}
#else
#define msdc_timer_init()       do{}while(0)
#define msdc_timer_start()      do{}while(0)
#define msdc_timer_stop()       do{}while(0)
#define msdc_timer_stop_clear() do{}while(0)
#define msdc_timer_get_count()  0
#endif

extern int msdc_reg_test(int id);
#define MSDC_RELIABLE_WRITE     (0x1 << 0)
#define MSDC_PACKED             (0x1 << 1)
#define MSDC_TAG_REQUEST        (0x1 << 2)
#define MSDC_CONTEXT_ID         (0x1 << 3)
#define MSDC_FORCED_PROG        (0x1 << 4)

#ifdef FPGA_PLATFORM
#define msdc_set_sr(...)
#define msdc_set_rdtdsel(...)
#define msdc_pin_set(...)
#define msdc_set_smt(...)
#define msdc_set_driving(...)
#endif

extern int msdc_init(struct mmc_host *host, int id);
extern int msdc_pio_bread(struct mmc_host *host, u8 *dst, u32 src, u32 nblks);
extern int msdc_pio_bwrite(struct mmc_host *host, u32 dst, u8 *src, u32 nblks);
extern int msdc_dma_bread(struct mmc_host *host, u8 *dst, u32 src, u32 nblks);
extern int msdc_dma_bwrite(struct mmc_host *host, u32 dst, u8 *src, u32 nblks);
extern int msdc_stream_bread(struct mmc_host *host, u8 *dst, u32 src, u32 nblks);
extern int msdc_stream_bwrite(struct mmc_host *host, u32 dst, u8 *src, u32 nblks);
extern int msdc_tune_bwrite(struct mmc_host *host, u32 dst, u8 *src, u32 nblks);
extern int msdc_tune_bread(struct mmc_host *host, u8 *dst, u32 src, u32 nblks);
extern int msdc_tune_cmdrsp(struct mmc_host *host, struct mmc_command *cmd);
extern void msdc_reset_tune_counter(struct mmc_host *host);
extern int msdc_abort_handler(struct mmc_host *host, int abort_card);
extern int msdc_tune_read(struct mmc_host *host);
extern void msdc_intr_sdio(struct mmc_host *host, int enable);
extern void msdc_intr_sdio_gap(struct mmc_host * host, int enable);
extern void msdc_config_clock(struct mmc_host *host, int state, u32 hz);
extern int msdc_pio_send_sandisk_fwid(struct mmc_host *host,uchar *src);
extern int msdc_pio_get_sandisk_fwid(struct mmc_host * host,uchar * src);
extern int msdc_dma_send_sandisk_fwid(struct mmc_host *host, uchar *buf,u32 opcode, ulong nblks);
extern void msdc_set_startbit(struct mmc_host *host, u8 start_bit);

#endif /* end of _MSDC_H_ */

