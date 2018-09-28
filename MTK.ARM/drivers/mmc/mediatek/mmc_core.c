/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * 
 * MediaTek Inc. (C) 2010. All rights reserved.
 * 
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

/*=======================================================================*/
/* HEADER FILES                                                          */
/*=======================================================================*/

#include "msdc.h"
#include "msdc_utils.h"
#include "mmc_core.h"
#include "mmc_test.h"
#include "mmc_hal.h"
#include "emmc_device_list.h"

#define NR_MMC             (MSDC_MAX_NUM)
#define CMD_RETRIES        (5)
#define CMD_TIMEOUT        (100)    /* 100ms */
#define ARRAY_SIZE(x)      (sizeof(x) / sizeof((x)[0]))

static struct mmc_host sd_host[NR_MMC];
static struct mmc_card sd_card[NR_MMC];
//static block_dev_desc_t sd_dev[NR_MMC];
//static int boot_dev_found = 0;
//static part_dev_t boot_dev;

static const unsigned int tran_exp[] = {
    10000,      100000,     1000000,    10000000,
    0,      0,      0,      0
};

static const unsigned char tran_mant[] = {
    0,  10, 12, 13, 15, 20, 25, 30,
    35, 40, 45, 50, 55, 60, 70, 80,
};

static const unsigned char mmc_tran_mant[] = {
    0,  10, 12, 13, 15, 20, 26, 30,
    35, 40, 45, 52, 55, 60, 70, 80,
};

static const unsigned int tacc_exp[] = {
    1,  10, 100,    1000,   10000,  100000, 1000000, 10000000,
};

static const unsigned int tacc_mant[] = {
    0,  10, 12, 13, 15, 20, 25, 30,
    35, 40, 45, 50, 55, 60, 70, 80,
};

static u32 unstuff_bits(u32 *resp, u32 start, u32 size)
{
    const u32 __mask = (1 << (size)) - 1;
    const int __off = 3 - ((start) / 32);
    const int __shft = (start) & 31;
    u32 __res;

    __res = resp[__off] >> __shft;
    if ((size) + __shft >= 32)
        __res |= resp[__off-1] << (32 - __shft);
    return __res & __mask;
}

#define UNSTUFF_BITS(r,s,sz)    unstuff_bits(r,s,sz)

#ifdef MMC_PROFILING
static void mmc_prof_card_init(void *data, u32 id, u32 counts)
{
    int err = (int)data;    
    if (!err) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Init Card, %d counts, %d us\n",
            id, counts, counts * 30 + counts * 16960 / 32768));
    }
}

static void mmc_prof_read(void *data, u32 id, u32 counts)
{
    struct mmc_op_perf *perf = (struct mmc_op_perf *)data;
    struct mmc_op_report *rpt;
    u32 blksz = perf->host->blklen;
    u32 blkcnt = (u32)id;

    if (blkcnt > 1)
        rpt = &perf->multi_blks_read;
    else
        rpt = &perf->single_blk_read;

    rpt->count++;
    rpt->total_size += blkcnt * blksz;
    rpt->total_time += counts;
    if ((counts < rpt->min_time) || (rpt->min_time == 0))
        rpt->min_time = counts;
    if ((counts > rpt->max_time) || (rpt->max_time == 0))
        rpt->max_time = counts;

    MSDC_TRC_PRINT(MSDC_PERF,("[SD%d] Read %d bytes, %d counts, %d us, %d KB/s, Avg: %d KB/s\n",
        perf->host->id, blkcnt * blksz, counts,
        counts * 30 + counts * 16960 / 32768,
        blkcnt * blksz * 32 / (counts ? counts : 1),
        ((rpt->total_size / 1024) * 32768) / rpt->total_time));
}

static void mmc_prof_write(void *data, u32 id, u32 counts)
{
    struct mmc_op_perf *perf = (struct mmc_op_perf *)data;
    struct mmc_op_report *rpt;
    u32 blksz = perf->host->blklen;
    u32 blkcnt = (u32)id;

    if (blkcnt > 1)
        rpt = &perf->multi_blks_write;
    else
        rpt = &perf->single_blk_write;

    rpt->count++;
    rpt->total_size += blkcnt * blksz;
    rpt->total_time += counts;
    if ((counts < rpt->min_time) || (rpt->min_time == 0))
        rpt->min_time = counts;
    if ((counts > rpt->max_time) || (rpt->max_time == 0))
        rpt->max_time = counts;

    MSDC_TRC_PRINT(MSDC_PERF,("[SD%d] Write %d bytes, %d counts, %d us, %d KB/s, Avg: %d KB/s\n",
        perf->host->id, blkcnt * blksz, counts,
        counts * 30 + counts * 16960 / 32768,
        blkcnt * blksz * 32 / (counts ? counts : 1),
        ((rpt->total_size / 1024) * 32768) / rpt->total_time));
}
#endif

#ifdef FEATURE_MMC_SDIO
static int cistpl_vers_1(struct mmc_card *card, struct sdio_func *func,
    const unsigned char *buf, unsigned size)
{
    unsigned i, nr_strings;
    char **buffer, *string;

    buf += 2;
    size -= 2;

    nr_strings = 0;
    for (i = 0; i < size; i++) {
        if (buf[i] == 0xff)
            break;
        if (buf[i] == 0)
            nr_strings++;
    }

    if (buf[i-1] != '\0') {
        MSDC_DBG_PRINT(MSDC_DEBUG,("SDIO: ignoring broken CISTPL_VERS_1\n"));
        return 0;
    }

    size = i;

    buffer = malloc(sizeof(char*) * nr_strings + size);
    if (!buffer)
        return -1;

    string = (char*)(buffer + nr_strings);

    for (i = 0; i < nr_strings; i++) {
        buffer[i] = string;
        strcpy(string, buf);
        string += strlen(string) + 1;
        buf += strlen(buf) + 1;
    }

    if (func) {
        func->num_info = nr_strings;
        func->info = (const char**)buffer;
    } else {
        card->num_info = nr_strings;
        card->info = (const char**)buffer;
    }

    return 0;
}

static int cistpl_manfid(struct mmc_card *card, struct sdio_func *func,
    const unsigned char *buf, unsigned size)
{
    unsigned int vendor, device;

    /* TPLMID_MANF */
    vendor = buf[0] | (buf[1] << 8);

    /* TPLMID_CARD */
    device = buf[2] | (buf[3] << 8);

    if (func) {
        func->vendor = vendor;
        func->device = device;
    } else {
        card->cis.vendor = vendor;
        card->cis.device = device;
    }

    return 0;
}

static const unsigned char speed_val[16] =
	{ 0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80 };
static const unsigned int speed_unit[8] =
	{ 10000, 100000, 1000000, 10000000, 0, 0, 0, 0 };

static int cistpl_funce_common(struct mmc_card *card,
    const unsigned char *buf, unsigned size)
{
	if (size < 0x04 || buf[0] != 0)
        return MMC_ERR_INVALID;

    /* TPLFE_FN0_BLK_SIZE */
    card->cis.blksize = buf[1] | (buf[2] << 8);

    /* TPLFE_MAX_TRAN_SPEED */
    card->cis.max_dtr = speed_val[(buf[3] >> 3) & 15] *
        speed_unit[buf[3] & 7];

	return 0;
}

static int cistpl_funce_func(struct sdio_func *func,
			     const unsigned char *buf, unsigned size)
{
    unsigned vsn;
    unsigned min_size;

    vsn = func->card->cccr.sdio_vsn;
    min_size = (vsn == SDIO_SDIO_REV_1_00) ? 28 : 42;

    if (size < min_size || buf[0] != 1)
        return MMC_ERR_INVALID;

    /* TPLFE_MAX_BLK_SIZE */
    func->max_blksize = buf[12] | (buf[13] << 8);

    /* TPLFE_ENABLE_TIMEOUT_VAL, present in ver 1.1 and above */
    if (vsn > SDIO_SDIO_REV_1_00)
        func->enable_timeout = (buf[28] | (buf[29] << 8)) * 10;
    else
        func->enable_timeout = 10;

    return 0;
}

static int cistpl_funce(struct mmc_card *card, struct sdio_func *func,
    const unsigned char *buf, unsigned size)
{
	int ret;

	/*
	 * There should be two versions of the CISTPL_FUNCE tuple,
	 * one for the common CIS (function 0) and a version used by
	 * the individual function's CIS (1-7). Yet, the later has a
	 * different length depending on the SDIO spec version.
	 */
	if (func)
		ret = cistpl_funce_func(func, buf, size);
	else
		ret = cistpl_funce_common(card, buf, size);

	if (ret) {
		MSDC_DBG_PRINT(MSDC_DEBUG,("bad CISTPL_FUNCE size %u type %u\n", size, buf[0]));
        return ret;
	}

	return 0;
}

typedef int (tpl_parse_t)(struct mmc_card *, struct sdio_func *,
    const unsigned char *, unsigned);

struct cis_tpl {
    unsigned char code;
    unsigned char min_size;
    tpl_parse_t *parse;
};

static const struct cis_tpl cis_tpl_list[] = {
    {   0x15,   3,  cistpl_vers_1       },
    {   0x20,   4,  cistpl_manfid       },
    {   0x21,   2,  NULL /* cistpl_funcid */ },
    {   0x22,   0,  cistpl_funce        },
};
#endif

void msdc_dump_card_status(u32 card_status);
void mmc_dump_card_status(u32 card_status)
{
    msdc_dump_card_status(card_status);
}

#if 0
void msdc_dump_ocr_reg(u32 resp);
static void mmc_dump_ocr_reg(u32 resp)
{
    msdc_dump_ocr_reg(resp);
}
#endif

#if 0
void msdc_dump_rca_resp(u32 resp);
static void mmc_dump_rca_resp(u32 resp)
{
    msdc_dump_rca_resp(resp);
}
#endif

#if 0
static void mmc_dump_tuning_blk(u8 *buf)
{
    int i;
    for (i = 0; i < 16; i++) {
        MSDC_DBG_PRINT(MSDC_DEBUG,("[TBLK%d] %x%x%x%x%x%x%x%x\n", i,
            (buf[(i<<2)] >> 4) & 0xF, buf[(i<<2)] & 0xF,
            (buf[(i<<2)+1] >> 4) & 0xF, buf[(i<<2)+1] & 0xF,
            (buf[(i<<2)+2] >> 4) & 0xF, buf[(i<<2)+2] & 0xF,
            (buf[(i<<2)+3] >> 4) & 0xF, buf[(i<<2)+3] & 0xF));
    }
}
#endif

#ifndef FEATURE_MMC_STRIPPED
static void mmc_dump_csd(struct mmc_card *card)
{
    struct mmc_csd *csd = &card->csd;
    u32 *resp = card->raw_csd;
    int i;
    unsigned int csd_struct;
 #ifndef KEEP_SLIENT_BUILD 
    static char *sd_csd_ver[] = {"v1.0", "v2.0"};
    static char *mmc_csd_ver[] = {"v1.0", "v1.1", "v1.2", "Ver. in EXT_CSD"};
    static char *mmc_cmd_cls[] = {"basic", "stream read", "block read",
        "stream write", "block write", "erase", "write prot", "lock card",
        "app-spec", "I/O", "rsv.", "rsv."};  
    static char *sd_cmd_cls[] = {"basic", "rsv.", "block read",
        "rsv.", "block write", "erase", "write prot", "lock card",
        "app-spec", "I/O", "switch", "rsv."};
#endif

    if (mmc_card_sd(card)) {
        csd_struct = UNSTUFF_BITS(resp, 126, 2);
        MSDC_TRC_PRINT(MSDC_DREG,("[CSD] CSD %s\n", sd_csd_ver[csd_struct]));
        MSDC_TRC_PRINT(MSDC_DREG,("[CSD] TACC_NS: %d ns, TACC_CLKS: %d clks\n", csd->tacc_ns, csd->tacc_clks));
        if (csd_struct == 1) {
            MSDC_TRC_PRINT(MSDC_DREG,("[CSD] Read/Write Blk Len = 512bytes\n"));
        } else {
            MSDC_TRC_PRINT(MSDC_DREG,("[CSD] Read Blk Len = %d, Write Blk Len = %d\n",
                1 << csd->read_blkbits, 1 << csd->write_blkbits));
        }
        MSDC_TRC_PRINT(MSDC_DREG,("[CSD] CMD Class:"));
        for (i = 0; i < 12; i++) {
            if ((csd->cmdclass >> i) & 0x1)
                MSDC_TRC_PRINT(MSDC_DREG,("'%s' ", sd_cmd_cls[i]));
        }
        MSDC_TRC_PRINT(MSDC_DREG,("\n"));
    } else {
        csd_struct = UNSTUFF_BITS(resp, 126, 2);
        MSDC_TRC_PRINT(MSDC_DREG,("[CSD] CSD %s\n", mmc_csd_ver[csd_struct]));
        MSDC_TRC_PRINT(MSDC_DREG,("[CSD] MMCA Spec v%d\n", csd->mmca_vsn));
        MSDC_TRC_PRINT(MSDC_DREG,("[CSD] TACC_NS: %d ns, TACC_CLKS: %d clks\n", csd->tacc_ns, csd->tacc_clks));
        MSDC_TRC_PRINT(MSDC_DREG,("[CSD] Read Blk Len = %d, Write Blk Len = %d\n",
            1 << csd->read_blkbits, 1 << csd->write_blkbits));
        MSDC_TRC_PRINT(MSDC_DREG,("[CSD] CMD Class:"));
        for (i = 0; i < 12; i++) {
            if ((csd->cmdclass >> i) & 0x1)
                MSDC_TRC_PRINT(MSDC_DREG,("'%s' ", mmc_cmd_cls[i]));
        }
        MSDC_TRC_PRINT(MSDC_DREG,("\n"));
    }
}
#endif

void mmc_dump_ext_csd(struct mmc_card *card)
{
    u8 *ext_csd = &card->raw_ext_csd[0];
    u32 tmp;
#ifndef KEEP_SLIENT_BUILD
    char *rev[] = {"4.0", "4.1", "4.2", "4.3", "Obsolete", "4.41", "4.5", "5.0", "5.1"};
#endif
    MSDC_TRC_PRINT(MSDC_DREG,("===========================================================\n"));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] EXT_CSD rev.              : v1.%d (MMCv%s)\n",
        ext_csd[EXT_CSD_REV], rev[ext_csd[EXT_CSD_REV]]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] CSD struct rev.           : v1.%d\n", ext_csd[EXT_CSD_STRUCT]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Supported command sets    : %xh\n", ext_csd[EXT_CSD_S_CMD_SET]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] HPI features              : %xh\n", ext_csd[EXT_CSD_HPI_FEATURE]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] BG operations support     : %xh\n", ext_csd[EXT_CSD_BKOPS_SUPP]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] BG operations status      : %xh\n", ext_csd[EXT_CSD_BKOPS_STATUS]));
    memcpy(&tmp, &ext_csd[EXT_CSD_CORRECT_PRG_SECTS_NUM], 4);
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Correct prg. sectors      : %xh\n", tmp));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] 1st init time after part. : %d ms\n", ext_csd[EXT_CSD_INI_TIMEOUT_AP] * 100));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Min. write perf.(DDR,52MH,8b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_DDR_W_8_52]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Min. read perf. (DDR,52MH,8b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_DDR_R_8_52]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] TRIM timeout: %d ms\n", ext_csd[EXT_CSD_TRIM_MULT] & 0xFF * 300));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Secure feature support: %xh\n", ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Secure erase timeout  : %d ms\n", 300 *
        ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT] * ext_csd[EXT_CSD_SEC_ERASE_MULT]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Secure trim timeout   : %d ms\n", 300 *
        ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT] * ext_csd[EXT_CSD_SEC_TRIM_MULT]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Access size           : %d bytes\n", ext_csd[EXT_CSD_ACC_SIZE] * 512));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] HC erase unit size    : %d kbytes\n", ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * 512));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] HC erase timeout      : %d ms\n", ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT] * 300));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] HC write prot grp size: %d kbytes\n", 512 *
        ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * ext_csd[EXT_CSD_HC_WP_GPR_SIZE]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] HC erase grp def.     : %xh\n", ext_csd[EXT_CSD_ERASE_GRP_DEF]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Reliable write sect count: %xh\n", ext_csd[EXT_CSD_REL_WR_SEC_C]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Sleep current (VCC) : %xh\n", ext_csd[EXT_CSD_S_C_VCC]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Sleep current (VCCQ): %xh\n", ext_csd[EXT_CSD_S_C_VCCQ]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Sleep/awake timeout : %d ns\n",
        100 * (2 << ext_csd[EXT_CSD_S_A_TIMEOUT])));
    memcpy(&tmp, &ext_csd[EXT_CSD_SEC_CNT], 4);
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Sector count : %xh\n", tmp));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Min. WR Perf.  (52MH,8b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_W_8_52]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Min. Read Perf.(52MH,8b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_R_8_52]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Min. WR Perf.  (26MH,8b,52MH,4b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_W_8_26_4_25]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Min. Read Perf.(26MH,8b,52MH,4b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_R_8_26_4_25]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Min. WR Perf.  (26MH,4b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_W_4_26]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Min. Read Perf.(26MH,4b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_R_4_26]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Power class: %x\n", ext_csd[EXT_CSD_PWR_CLASS]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Power class(DDR,52MH,3.6V): %xh\n", ext_csd[EXT_CSD_PWR_CL_DDR_52_360]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Power class(DDR,52MH,1.9V): %xh\n", ext_csd[EXT_CSD_PWR_CL_DDR_52_195]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Power class(26MH,3.6V)    : %xh\n", ext_csd[EXT_CSD_PWR_CL_26_360]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Power class(52MH,3.6V)    : %xh\n", ext_csd[EXT_CSD_PWR_CL_52_360]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Power class(26MH,1.9V)    : %xh\n", ext_csd[EXT_CSD_PWR_CL_26_195]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Power class(52MH,1.9V)    : %xh\n", ext_csd[EXT_CSD_PWR_CL_52_195]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Part. switch timing    : %xh\n", ext_csd[EXT_CSD_PART_SWITCH_TIME]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Out-of-INTR busy timing: %xh\n", ext_csd[EXT_CSD_OUT_OF_INTR_TIME]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Card type       : %xh\n", ext_csd[EXT_CSD_CARD_TYPE]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Command set     : %xh\n", ext_csd[EXT_CSD_CMD_SET]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Command set rev.: %xh\n", ext_csd[EXT_CSD_CMD_SET_REV]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] HS timing       : %xh\n", ext_csd[EXT_CSD_HS_TIMING]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Bus width       : %xh\n", ext_csd[EXT_CSD_BUS_WIDTH]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Erase memory content : %xh\n", ext_csd[EXT_CSD_ERASED_MEM_CONT]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Partition config      : %xh\n", ext_csd[EXT_CSD_PART_CFG]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Boot partition size   : %d kbytes\n", ext_csd[EXT_CSD_BOOT_SIZE_MULT] * 128));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Boot information      : %xh\n", ext_csd[EXT_CSD_BOOT_INFO]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Boot config protection: %xh\n", ext_csd[EXT_CSD_BOOT_CONFIG_PROT]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Boot bus width        : %xh\n", ext_csd[EXT_CSD_BOOT_BUS_WIDTH]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Boot area write prot  : %xh\n", ext_csd[EXT_CSD_BOOT_WP]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] User area write prot  : %xh\n", ext_csd[EXT_CSD_USR_WP]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] FW configuration      : %xh\n", ext_csd[EXT_CSD_FW_CONFIG]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] RPMB size : %d kbytes\n", ext_csd[EXT_CSD_RPMB_SIZE_MULT] * 128));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Write rel. setting  : %xh\n", ext_csd[EXT_CSD_WR_REL_SET]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Write rel. parameter: %xh\n", ext_csd[EXT_CSD_WR_REL_PARAM]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Start background ops : %xh\n", ext_csd[EXT_CSD_BKOPS_START]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Enable background ops: %xh\n", ext_csd[EXT_CSD_BKOPS_EN]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] H/W reset function   : %xh\n", ext_csd[EXT_CSD_RST_N_FUNC]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] HPI management       : %xh\n", ext_csd[EXT_CSD_HPI_MGMT]));
    memcpy(&tmp, &ext_csd[EXT_CSD_MAX_ENH_SIZE_MULT], 4);
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Max. enhanced area size : %xh (%d kbytes)\n",
        tmp & 0x00FFFFFF, (tmp & 0x00FFFFFF) * 512 *
        ext_csd[EXT_CSD_HC_WP_GPR_SIZE] * ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Part. support  : %xh\n", ext_csd[EXT_CSD_PART_SUPPORT]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Part. attribute: %xh\n", ext_csd[EXT_CSD_PART_ATTR]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Part. setting  : %xh\n", ext_csd[EXT_CSD_PART_SET_COMPL]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] General purpose 1 size : %xh (%d kbytes)\n",
        (ext_csd[EXT_CSD_GP1_SIZE_MULT + 0] | 
         ext_csd[EXT_CSD_GP1_SIZE_MULT + 1] << 8 | 
         ext_csd[EXT_CSD_GP1_SIZE_MULT + 2] << 16),
        (ext_csd[EXT_CSD_GP1_SIZE_MULT + 0] | 
         ext_csd[EXT_CSD_GP1_SIZE_MULT + 1] << 8 | 
         ext_csd[EXT_CSD_GP1_SIZE_MULT + 2] << 16) * 512 *
         ext_csd[EXT_CSD_HC_WP_GPR_SIZE] * 
         ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] General purpose 2 size : %xh (%d kbytes)\n",
        (ext_csd[EXT_CSD_GP2_SIZE_MULT + 0] | 
         ext_csd[EXT_CSD_GP2_SIZE_MULT + 1] << 8 | 
         ext_csd[EXT_CSD_GP2_SIZE_MULT + 2] << 16),
        (ext_csd[EXT_CSD_GP2_SIZE_MULT + 0] | 
         ext_csd[EXT_CSD_GP2_SIZE_MULT + 1] << 8 | 
         ext_csd[EXT_CSD_GP2_SIZE_MULT + 2] << 16) * 512 *
         ext_csd[EXT_CSD_HC_WP_GPR_SIZE] * 
         ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] General purpose 3 size : %xh (%d kbytes)\n",
        (ext_csd[EXT_CSD_GP3_SIZE_MULT + 0] | 
         ext_csd[EXT_CSD_GP3_SIZE_MULT + 1] << 8 | 
         ext_csd[EXT_CSD_GP3_SIZE_MULT + 2] << 16),
        (ext_csd[EXT_CSD_GP3_SIZE_MULT + 0] | 
         ext_csd[EXT_CSD_GP3_SIZE_MULT + 1] << 8 | 
         ext_csd[EXT_CSD_GP3_SIZE_MULT + 2] << 16) * 512 *
         ext_csd[EXT_CSD_HC_WP_GPR_SIZE] * 
         ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] General purpose 4 size : %xh (%d kbytes)\n",
        (ext_csd[EXT_CSD_GP4_SIZE_MULT + 0] | 
         ext_csd[EXT_CSD_GP4_SIZE_MULT + 1] << 8 | 
         ext_csd[EXT_CSD_GP4_SIZE_MULT + 2] << 16),
        (ext_csd[EXT_CSD_GP4_SIZE_MULT + 0] | 
         ext_csd[EXT_CSD_GP4_SIZE_MULT + 1] << 8 | 
         ext_csd[EXT_CSD_GP4_SIZE_MULT + 2] << 16) * 512 *
         ext_csd[EXT_CSD_HC_WP_GPR_SIZE] * 
         ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Enh. user area size : %xh (%d kbytes)\n",
        (ext_csd[EXT_CSD_ENH_SIZE_MULT + 0] | 
         ext_csd[EXT_CSD_ENH_SIZE_MULT + 1] << 8 | 
         ext_csd[EXT_CSD_ENH_SIZE_MULT + 2] << 16),
        (ext_csd[EXT_CSD_ENH_SIZE_MULT + 0] | 
         ext_csd[EXT_CSD_ENH_SIZE_MULT + 1] << 8 | 
         ext_csd[EXT_CSD_ENH_SIZE_MULT + 2] << 16) * 512 *
         ext_csd[EXT_CSD_HC_WP_GPR_SIZE] * 
         ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Enh. user area start: %xh\n",
        (ext_csd[EXT_CSD_ENH_START_ADDR + 0] |
         ext_csd[EXT_CSD_ENH_START_ADDR + 1] << 8 |
         ext_csd[EXT_CSD_ENH_START_ADDR + 2] << 16 |
         ext_csd[EXT_CSD_ENH_START_ADDR + 3]) << 24));
    MSDC_TRC_PRINT(MSDC_DREG,("[EXT_CSD] Bad block mgmt mode: %xh\n", ext_csd[EXT_CSD_BADBLK_MGMT]));
    MSDC_TRC_PRINT(MSDC_DREG,("===========================================================\n"));
}


void mmc_dump_ext_csd_p(struct mmc_card *card)
{
	u8 *ext_csd = &card->raw_ext_csd[0];
	u32 tmp;
	char *rev[] = {"4.0", "4.1", "4.2", "4.3", "Obsolete", "4.41", "4.5", "5.0", "5.1"};

	printf("===========================================================\n");
	printf("[EXT_CSD] EXT_CSD rev.              : v1.%d (MMCv%s)\n",
				   ext_csd[EXT_CSD_REV], rev[ext_csd[EXT_CSD_REV]]);
	printf("[EXT_CSD] CSD struct rev.           : v1.%d\n", ext_csd[EXT_CSD_STRUCT]);
	printf("[EXT_CSD] Supported command sets    : %xh\n", ext_csd[EXT_CSD_S_CMD_SET]);
	printf("[EXT_CSD] HPI features              : %xh\n", ext_csd[EXT_CSD_HPI_FEATURE]);
	printf("[EXT_CSD] BG operations support     : %xh\n", ext_csd[EXT_CSD_BKOPS_SUPP]);
	printf("[EXT_CSD] BG operations status      : %xh\n", ext_csd[EXT_CSD_BKOPS_STATUS]);
	memcpy(&tmp, &ext_csd[EXT_CSD_CORRECT_PRG_SECTS_NUM], 4);
	printf("[EXT_CSD] Correct prg. sectors      : %xh\n", tmp);
	printf("[EXT_CSD] 1st init time after part. : %d ms\n", ext_csd[EXT_CSD_INI_TIMEOUT_AP] * 100);
	printf("[EXT_CSD] Min. write perf.(DDR,52MH,8b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_DDR_W_8_52]);
	printf("[EXT_CSD] Min. read perf. (DDR,52MH,8b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_DDR_R_8_52]);
	printf("[EXT_CSD] TRIM timeout: %d ms\n", ext_csd[EXT_CSD_TRIM_MULT] & 0xFF * 300);
	printf("[EXT_CSD] Secure feature support: %xh\n", ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT]);
	printf("[EXT_CSD] Secure erase timeout  : %d ms\n", 300 *
				   ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT] * ext_csd[EXT_CSD_SEC_ERASE_MULT]);
	printf("[EXT_CSD] Secure trim timeout   : %d ms\n", 300 *
				   ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT] * ext_csd[EXT_CSD_SEC_TRIM_MULT]);
	printf("[EXT_CSD] Access size           : %d bytes\n", ext_csd[EXT_CSD_ACC_SIZE] * 512);
	printf("[EXT_CSD] HC erase unit size    : %d kbytes\n", ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * 512);
	printf("[EXT_CSD] HC erase timeout      : %d ms\n", ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT] * 300);
	printf("[EXT_CSD] HC write prot grp size: %d kbytes\n", 512 *
				   ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * ext_csd[EXT_CSD_HC_WP_GPR_SIZE]);
	printf("[EXT_CSD] HC erase grp def.     : %xh\n", ext_csd[EXT_CSD_ERASE_GRP_DEF]);
	printf("[EXT_CSD] Reliable write sect count: %xh\n", ext_csd[EXT_CSD_REL_WR_SEC_C]);
	printf("[EXT_CSD] Sleep current (VCC) : %xh\n", ext_csd[EXT_CSD_S_C_VCC]);
	printf("[EXT_CSD] Sleep current (VCCQ): %xh\n", ext_csd[EXT_CSD_S_C_VCCQ]);
	printf("[EXT_CSD] Sleep/awake timeout : %d ns\n",
				   100 *(2 << ext_csd[EXT_CSD_S_A_TIMEOUT]));
	memcpy(&tmp, &ext_csd[EXT_CSD_SEC_CNT], 4);
	printf("[EXT_CSD] Sector count : %xh\n", tmp);
	printf("[EXT_CSD] Min. WR Perf.  (52MH,8b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_W_8_52]);
	printf("[EXT_CSD] Min. Read Perf.(52MH,8b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_R_8_52]);
	printf("[EXT_CSD] Min. WR Perf.  (26MH,8b,52MH,4b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_W_8_26_4_25]);
	printf("[EXT_CSD] Min. Read Perf.(26MH,8b,52MH,4b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_R_8_26_4_25]);
	printf("[EXT_CSD] Min. WR Perf.  (26MH,4b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_W_4_26]);
	printf("[EXT_CSD] Min. Read Perf.(26MH,4b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_R_4_26]);
	printf("[EXT_CSD] Power class: %x\n", ext_csd[EXT_CSD_PWR_CLASS]);
	printf("[EXT_CSD] Power class(DDR,52MH,3.6V): %xh\n", ext_csd[EXT_CSD_PWR_CL_DDR_52_360]);
	printf("[EXT_CSD] Power class(DDR,52MH,1.9V): %xh\n", ext_csd[EXT_CSD_PWR_CL_DDR_52_195]);
	printf("[EXT_CSD] Power class(26MH,3.6V)    : %xh\n", ext_csd[EXT_CSD_PWR_CL_26_360]);
	printf("[EXT_CSD] Power class(52MH,3.6V)    : %xh\n", ext_csd[EXT_CSD_PWR_CL_52_360]);
	printf("[EXT_CSD] Power class(26MH,1.9V)    : %xh\n", ext_csd[EXT_CSD_PWR_CL_26_195]);
	printf("[EXT_CSD] Power class(52MH,1.9V)    : %xh\n", ext_csd[EXT_CSD_PWR_CL_52_195]);
	printf("[EXT_CSD] Part. switch timing    : %xh\n", ext_csd[EXT_CSD_PART_SWITCH_TIME]);
	printf("[EXT_CSD] Out-of-INTR busy timing: %xh\n", ext_csd[EXT_CSD_OUT_OF_INTR_TIME]);
	printf("[EXT_CSD] Card type       : %xh\n", ext_csd[EXT_CSD_CARD_TYPE]);
	printf("[EXT_CSD] Command set     : %xh\n", ext_csd[EXT_CSD_CMD_SET]);
	printf("[EXT_CSD] Command set rev.: %xh\n", ext_csd[EXT_CSD_CMD_SET_REV]);
	printf("[EXT_CSD] HS timing       : %xh\n", ext_csd[EXT_CSD_HS_TIMING]);
	printf("[EXT_CSD] Bus width       : %xh\n", ext_csd[EXT_CSD_BUS_WIDTH]);
	printf("[EXT_CSD] Erase memory content : %xh\n", ext_csd[EXT_CSD_ERASED_MEM_CONT]);
	printf("[EXT_CSD] Partition config      : %xh\n", ext_csd[EXT_CSD_PART_CFG]);
	printf("[EXT_CSD] Boot partition size   : %d kbytes\n", ext_csd[EXT_CSD_BOOT_SIZE_MULT] * 128);
	printf("[EXT_CSD] Boot information      : %xh\n", ext_csd[EXT_CSD_BOOT_INFO]);
	printf("[EXT_CSD] Boot config protection: %xh\n", ext_csd[EXT_CSD_BOOT_CONFIG_PROT]);
	printf("[EXT_CSD] Boot bus width        : %xh\n", ext_csd[EXT_CSD_BOOT_BUS_WIDTH]);
	printf("[EXT_CSD] Boot area write prot  : %xh\n", ext_csd[EXT_CSD_BOOT_WP]);
	printf("[EXT_CSD] User area write prot  : %xh\n", ext_csd[EXT_CSD_USR_WP]);
	printf("[EXT_CSD] FW configuration      : %xh\n", ext_csd[EXT_CSD_FW_CONFIG]);
	printf("[EXT_CSD] RPMB size : %d kbytes\n", ext_csd[EXT_CSD_RPMB_SIZE_MULT] * 128);
	printf("[EXT_CSD] Write rel. setting  : %xh\n", ext_csd[EXT_CSD_WR_REL_SET]);
	printf("[EXT_CSD] Write rel. parameter: %xh\n", ext_csd[EXT_CSD_WR_REL_PARAM]);
	printf("[EXT_CSD] Start background ops : %xh\n", ext_csd[EXT_CSD_BKOPS_START]);
	printf("[EXT_CSD] Enable background ops: %xh\n", ext_csd[EXT_CSD_BKOPS_EN]);
	printf("[EXT_CSD] H/W reset function   : %xh\n", ext_csd[EXT_CSD_RST_N_FUNC]);
	printf("[EXT_CSD] HPI management       : %xh\n", ext_csd[EXT_CSD_HPI_MGMT]);
	memcpy(&tmp, &ext_csd[EXT_CSD_MAX_ENH_SIZE_MULT], 4);
	printf("[EXT_CSD] Max. enhanced area size : %xh (%d kbytes)\n",
				   tmp & 0x00FFFFFF, (tmp & 0x00FFFFFF) * 512 *
				   ext_csd[EXT_CSD_HC_WP_GPR_SIZE] * ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]);
	printf("[EXT_CSD] Part. support  : %xh\n", ext_csd[EXT_CSD_PART_SUPPORT]);
	printf("[EXT_CSD] Part. attribute: %xh\n", ext_csd[EXT_CSD_PART_ATTR]);
	printf("[EXT_CSD] Part. setting  : %xh\n", ext_csd[EXT_CSD_PART_SET_COMPL]);
	printf("[EXT_CSD] General purpose 1 size : %xh (%d kbytes)\n",
				   (ext_csd[EXT_CSD_GP1_SIZE_MULT + 0] |
				    ext_csd[EXT_CSD_GP1_SIZE_MULT + 1] << 8 |
				    ext_csd[EXT_CSD_GP1_SIZE_MULT + 2] << 16),
				   (ext_csd[EXT_CSD_GP1_SIZE_MULT + 0] |
				    ext_csd[EXT_CSD_GP1_SIZE_MULT + 1] << 8 |
				    ext_csd[EXT_CSD_GP1_SIZE_MULT + 2] << 16) * 512 *
				   ext_csd[EXT_CSD_HC_WP_GPR_SIZE] *
				   ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]);
	printf("[EXT_CSD] General purpose 2 size : %xh (%d kbytes)\n",
				   (ext_csd[EXT_CSD_GP2_SIZE_MULT + 0] |
				    ext_csd[EXT_CSD_GP2_SIZE_MULT + 1] << 8 |
				    ext_csd[EXT_CSD_GP2_SIZE_MULT + 2] << 16),
				   (ext_csd[EXT_CSD_GP2_SIZE_MULT + 0] |
				    ext_csd[EXT_CSD_GP2_SIZE_MULT + 1] << 8 |
				    ext_csd[EXT_CSD_GP2_SIZE_MULT + 2] << 16) * 512 *
				   ext_csd[EXT_CSD_HC_WP_GPR_SIZE] *
				   ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]);
	printf("[EXT_CSD] General purpose 3 size : %xh (%d kbytes)\n",
				   (ext_csd[EXT_CSD_GP3_SIZE_MULT + 0] |
				    ext_csd[EXT_CSD_GP3_SIZE_MULT + 1] << 8 |
				    ext_csd[EXT_CSD_GP3_SIZE_MULT + 2] << 16),
				   (ext_csd[EXT_CSD_GP3_SIZE_MULT + 0] |
				    ext_csd[EXT_CSD_GP3_SIZE_MULT + 1] << 8 |
				    ext_csd[EXT_CSD_GP3_SIZE_MULT + 2] << 16) * 512 *
				   ext_csd[EXT_CSD_HC_WP_GPR_SIZE] *
				   ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]);
	printf("[EXT_CSD] General purpose 4 size : %xh (%d kbytes)\n",
				   (ext_csd[EXT_CSD_GP4_SIZE_MULT + 0] |
				    ext_csd[EXT_CSD_GP4_SIZE_MULT + 1] << 8 |
				    ext_csd[EXT_CSD_GP4_SIZE_MULT + 2] << 16),
				   (ext_csd[EXT_CSD_GP4_SIZE_MULT + 0] |
				    ext_csd[EXT_CSD_GP4_SIZE_MULT + 1] << 8 |
				    ext_csd[EXT_CSD_GP4_SIZE_MULT + 2] << 16) * 512 *
				   ext_csd[EXT_CSD_HC_WP_GPR_SIZE] *
				   ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]);
	printf("[EXT_CSD] Enh. user area size : %xh (%d kbytes)\n",
				   (ext_csd[EXT_CSD_ENH_SIZE_MULT + 0] |
				    ext_csd[EXT_CSD_ENH_SIZE_MULT + 1] << 8 |
				    ext_csd[EXT_CSD_ENH_SIZE_MULT + 2] << 16),
				   (ext_csd[EXT_CSD_ENH_SIZE_MULT + 0] |
				    ext_csd[EXT_CSD_ENH_SIZE_MULT + 1] << 8 |
				    ext_csd[EXT_CSD_ENH_SIZE_MULT + 2] << 16) * 512 *
				   ext_csd[EXT_CSD_HC_WP_GPR_SIZE] *
				   ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]);
	printf("[EXT_CSD] Enh. user area start: %xh\n",
				   (ext_csd[EXT_CSD_ENH_START_ADDR + 0] |
				    ext_csd[EXT_CSD_ENH_START_ADDR + 1] << 8 |
				    ext_csd[EXT_CSD_ENH_START_ADDR + 2] << 16 |
				    ext_csd[EXT_CSD_ENH_START_ADDR + 3]) << 24);
	printf("[EXT_CSD] Bad block mgmt mode: %xh\n", ext_csd[EXT_CSD_BADBLK_MGMT]);
	printf("===========================================================\n");
}



int mmc_card_avail(struct mmc_host *host)
{
    return msdc_card_avail(host);
}

#if 0
int mmc_card_protected(struct mmc_host *host)
{
    return msdc_card_protected(host);
}
#endif

struct mmc_host *mmc_get_host(int id)
{
    return &sd_host[id];
}

struct mmc_card *mmc_get_card(int id)
{
    return &sd_card[id];
}

static int mmc_cmd(struct mmc_host *host, struct mmc_command *cmd)
{
    int err;
    int retry = cmd->retries;

    do {
        err = msdc_cmd(host, cmd);
        if (err == MMC_ERR_NONE)
            break;    
    } while(retry--);

    return err;
}

static int mmc_app_cmd(struct mmc_host *host, struct mmc_command *cmd,
                       u32 rca, int retries)
{
    int err = MMC_ERR_FAILED;
    struct mmc_command appcmd;

    appcmd.opcode  = MMC_CMD_APP_CMD;
    appcmd.arg     = rca << 16;
    appcmd.rsptyp  = RESP_R1;
    appcmd.retries = CMD_RETRIES;
    appcmd.timeout = CMD_TIMEOUT;

    do {
        err = mmc_cmd(host, &appcmd);

        if (err == MMC_ERR_NONE)
            err = mmc_cmd(host, cmd);
        if (err == MMC_ERR_NONE)
            break;
    } while (retries--);

    return err;
}

static u32 mmc_select_voltage(struct mmc_host *host, u32 ocr)
{
    int bit;

    ocr &= host->ocr_avail;

    bit = uffs(ocr);
    if (bit) {
        bit -= 1;
        ocr &= 3 << bit;
    } else {
        ocr = 0;
    }
    return ocr;
}

int mmc_go_idle(struct mmc_host *host)
{
    struct mmc_command cmd;

    cmd.opcode  = MMC_CMD_GO_IDLE_STATE;
    cmd.rsptyp  = RESP_NONE;
    cmd.arg     = 0;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    return mmc_cmd(host, &cmd); 
}

#if 0
int mmc_go_irq_state(struct mmc_host *host, struct mmc_card *card)
{
    struct mmc_command cmd;

    if (!(card->csd.cmdclass & CCC_IO_MODE)) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Card doesn't support I/O mode for IRQ state\n", host->id));
        return MMC_ERR_FAILED;
    }

    cmd.opcode  = MMC_CMD_GO_IRQ_STATE;
    cmd.rsptyp  = RESP_R5;
    cmd.arg     = 0;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    return mmc_cmd(host, &cmd);
}

static int mmc_go_inactive(struct mmc_host *host, struct mmc_card *card)
{
    struct mmc_command cmd;

    cmd.opcode  = MMC_CMD_GO_INACTIVE_STATE;
    cmd.rsptyp  = RESP_NONE;
    cmd.arg     = 0;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    return mmc_cmd(host, &cmd);
}

static int mmc_go_pre_idle(struct mmc_host *host, struct mmc_card *card)
{
    struct mmc_command cmd;

    cmd.opcode  = MMC_CMD_GO_IDLE_STATE;
    cmd.rsptyp  = RESP_NONE;
    cmd.arg     = 0xF0F0F0F0;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    return mmc_cmd(host, &cmd);
}

int mmc_sleep_awake(struct mmc_host *host, struct mmc_card *card, int sleep)
{
    struct mmc_command cmd;
    u32 timeout;

    if (card->raw_ext_csd[EXT_CSD_S_A_TIMEOUT]) {
        timeout = ((1 << card->raw_ext_csd[EXT_CSD_S_A_TIMEOUT]) * 100) / 1000000;
    } else {
        timeout = CMD_TIMEOUT;
    }

    cmd.opcode  = MMC_CMD_SLEEP_AWAKE;
    cmd.rsptyp  = RESP_R1B;
    cmd.arg     = (card->rca << 16) | (sleep << 15);
    cmd.retries = CMD_RETRIES;
    cmd.timeout = timeout;

    return mmc_cmd(host, &cmd);    
}
#endif

int mmc_send_status(struct mmc_host *host, struct mmc_card *card, u32 *status)
{
    int err;
    struct mmc_command cmd;

    cmd.opcode  = MMC_CMD_SEND_STATUS;
    cmd.arg     = card->rca << 16;
    cmd.rsptyp  = RESP_R1;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    err = mmc_cmd(host, &cmd);

    if (err == MMC_ERR_NONE) {
        *status = cmd.resp[0];
        mmc_dump_card_status(*status);
    }
    return err;
}

static int mmc_send_io_op_cond(struct mmc_host *host, u32 ocr, u32 *rocr)
{
    struct mmc_command cmd;
    int i, err = 0;

    MSDC_ASSERT(host);

    memset(&cmd, 0, sizeof(struct mmc_command));

    cmd.opcode = SD_IO_SEND_OP_COND;
    cmd.arg    = ocr;
    cmd.rsptyp = RESP_R4;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    for (i = 100; i; i--) {
        err = mmc_cmd(host, &cmd);
        if (err)
        	break;

        /* if we're just probing, do a single pass */
        if (ocr == 0)
        	break;

        if (cmd.resp[0] & MMC_CARD_BUSY)
        	break;

        err = MMC_ERR_TIMEOUT;

        mdelay(10);
    }

    if (rocr)
    	*rocr = cmd.resp[0];

    return err;
}

static int mmc_send_if_cond(struct mmc_host *host, u32 ocr)
{
    struct mmc_command cmd;
    int err;
    static const u8 test_pattern = 0xAA;
    u8 result_pattern;

    /*
     * To support SD 2.0 cards, we must always invoke SD_SEND_IF_COND
     * before SD_APP_OP_COND. This command will harmlessly fail for
     * SD 1.0 cards.
     */

    cmd.opcode  = SD_CMD_SEND_IF_COND;
    cmd.arg     = ((ocr & 0xFF8000) != 0) << 8 | test_pattern;
    cmd.rsptyp  = RESP_R1;
    cmd.retries = 0;
    cmd.timeout = CMD_TIMEOUT;

    err = mmc_cmd(host, &cmd);

    if (err != MMC_ERR_NONE)
        return err;

    result_pattern = cmd.resp[0] & 0xFF;

    if (result_pattern != test_pattern)
        return MMC_ERR_INVALID;

    return MMC_ERR_NONE;
}

static int mmc_send_op_cond(struct mmc_host *host, u32 ocr, u32 *rocr)
{
    struct mmc_command cmd;
    int i, err = 0;

    cmd.opcode  = MMC_CMD_SEND_OP_COND;
    cmd.arg     = ocr;
    cmd.rsptyp  = RESP_R3;
    cmd.retries = 0;
    cmd.timeout = CMD_TIMEOUT;

    for (i = 100; i; i--) {
        err = mmc_cmd(host, &cmd);
        if (err)
            break;

        /* if we're just probing, do a single pass */
        if (ocr == 0)
            break;

        if (cmd.resp[0] & MMC_CARD_BUSY)
            break;

        err = MMC_ERR_TIMEOUT;

        mdelay(10);
    }

    if (!err && rocr)
        *rocr = cmd.resp[0];

    return err;
}

static int mmc_send_app_op_cond(struct mmc_host *host, u32 ocr, u32 *rocr)
{
    struct mmc_command cmd;
    int i, err = 0;

    cmd.opcode  = SD_ACMD_SEND_OP_COND;
    cmd.arg     = ocr;
    cmd.rsptyp  = RESP_R3;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    for (i = 100; i; i--) {
        err = mmc_app_cmd(host, &cmd, 0, CMD_RETRIES);
        if (err != MMC_ERR_NONE)
            break;

        if (cmd.resp[0] & MMC_CARD_BUSY || ocr == 0)
            break;

        err = MMC_ERR_TIMEOUT;

        mdelay(10);
    }

    if (rocr)
        *rocr = cmd.resp[0];

    return err;
}

static int mmc_all_send_cid(struct mmc_host *host, u32 *cid)
{
    int err;
    struct mmc_command cmd;

    /* send cid */
    cmd.opcode  = MMC_CMD_ALL_SEND_CID;
    cmd.arg     = 0;
    cmd.rsptyp  = RESP_R2;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    err = mmc_cmd(host, &cmd);

    if (err != MMC_ERR_NONE)
        return err;

    memcpy(cid, cmd.resp, sizeof(u32) * 4);

    return MMC_ERR_NONE;
}

/* code size add 1KB*/
static void mmc_decode_cid(struct mmc_card *card)
{
    u32 *resp = card->raw_cid;

    memset(&card->cid, 0, sizeof(struct mmc_cid));

    if (mmc_card_sd(card)) {
    	/*
    	 * SD doesn't currently have a version field so we will
    	 * have to assume we can parse this.
    	 */
    	card->cid.manfid		= UNSTUFF_BITS(resp, 120, 8);
    	card->cid.oemid			= UNSTUFF_BITS(resp, 104, 16);
    	card->cid.prod_name[0]	= UNSTUFF_BITS(resp, 96, 8);
    	card->cid.prod_name[1]	= UNSTUFF_BITS(resp, 88, 8);
    	card->cid.prod_name[2]	= UNSTUFF_BITS(resp, 80, 8);
    	card->cid.prod_name[3]	= UNSTUFF_BITS(resp, 72, 8);
    	card->cid.prod_name[4]	= UNSTUFF_BITS(resp, 64, 8);
    	card->cid.hwrev			= UNSTUFF_BITS(resp, 60, 4);
    	card->cid.fwrev			= UNSTUFF_BITS(resp, 56, 4);
    	card->cid.serial		= UNSTUFF_BITS(resp, 24, 32);
    	card->cid.year			= UNSTUFF_BITS(resp, 12, 8);
    	card->cid.month			= UNSTUFF_BITS(resp, 8, 4);

    	card->cid.year += 2000; /* SD cards year offset */
    } else {
        /*
         * The selection of the format here is based upon published
         * specs from sandisk and from what people have reported.
         */
        switch (card->csd.mmca_vsn) {
        case 0: /* MMC v1.0 - v1.2 */
        case 1: /* MMC v1.4 */
            card->cid.manfid        = UNSTUFF_BITS(resp, 104, 24);
            card->cid.prod_name[0]  = UNSTUFF_BITS(resp, 96, 8);
            card->cid.prod_name[1]  = UNSTUFF_BITS(resp, 88, 8);
            card->cid.prod_name[2]  = UNSTUFF_BITS(resp, 80, 8);
            card->cid.prod_name[3]  = UNSTUFF_BITS(resp, 72, 8);
            card->cid.prod_name[4]  = UNSTUFF_BITS(resp, 64, 8);
            card->cid.prod_name[5]  = UNSTUFF_BITS(resp, 56, 8);
            card->cid.prod_name[6]  = UNSTUFF_BITS(resp, 48, 8);
            card->cid.hwrev         = UNSTUFF_BITS(resp, 44, 4);
            card->cid.fwrev         = UNSTUFF_BITS(resp, 40, 4);
            card->cid.serial        = UNSTUFF_BITS(resp, 16, 24);
            card->cid.month         = UNSTUFF_BITS(resp, 12, 4);
            card->cid.year          = UNSTUFF_BITS(resp, 8, 4) + 1997;
            break;

        case 2: /* MMC v2.0 - v2.2 */
        case 3: /* MMC v3.1 - v3.3 */
        case 4: /* MMC v4 */
            card->cid.manfid        = UNSTUFF_BITS(resp, 120, 8);
            //card->cid.cbx           = UNSTUFF_BITS(resp, 112, 2);
            card->cid.oemid         = UNSTUFF_BITS(resp, 104, 16);
            card->cid.prod_name[0]  = UNSTUFF_BITS(resp, 96, 8);
            card->cid.prod_name[1]  = UNSTUFF_BITS(resp, 88, 8);
            card->cid.prod_name[2]  = UNSTUFF_BITS(resp, 80, 8);
            card->cid.prod_name[3]  = UNSTUFF_BITS(resp, 72, 8);
            card->cid.prod_name[4]  = UNSTUFF_BITS(resp, 64, 8);
            card->cid.prod_name[5]  = UNSTUFF_BITS(resp, 56, 8);
            card->cid.serial        = UNSTUFF_BITS(resp, 16, 32);
            card->cid.month         = UNSTUFF_BITS(resp, 12, 4);
            card->cid.year          = UNSTUFF_BITS(resp, 8, 4) + 1997;
            break;

        default:
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Unknown MMCA version %d\n",
                mmc_card_id(card), card->csd.mmca_vsn));
            break;
        }
    }
}

#ifndef FEATURE_MMC_STRIPPED

static int mmc_decode_csd(struct mmc_card *card)
{
    struct mmc_csd *csd = &card->csd;
    unsigned int e, m, csd_struct;
    u32 *resp = card->raw_csd;

    /* common part */
    csd_struct = UNSTUFF_BITS(resp, 126, 2);
    csd->csd_struct = csd_struct;

    m = UNSTUFF_BITS(resp, 99, 4);
    e = UNSTUFF_BITS(resp, 96, 3);
    csd->max_dtr      = tran_exp[e] * tran_mant[m];
    csd->cmdclass     = UNSTUFF_BITS(resp, 84, 12);

    csd->write_prot_grpsz = UNSTUFF_BITS(resp, 32, 7);
    csd->write_prot_grp = UNSTUFF_BITS(resp, 31, 1);
    csd->perm_wr_prot = UNSTUFF_BITS(resp, 13, 1);
    csd->tmp_wr_prot = UNSTUFF_BITS(resp, 12, 1);
    csd->copy = UNSTUFF_BITS(resp, 14, 1);
    csd->dsr = UNSTUFF_BITS(resp, 76, 1);

    /* update later according to spec. */
    csd->read_blkbits = UNSTUFF_BITS(resp, 80, 4);
    csd->read_partial = UNSTUFF_BITS(resp, 79, 1);
    csd->write_misalign = UNSTUFF_BITS(resp, 78, 1);
    csd->read_misalign = UNSTUFF_BITS(resp, 77, 1);
    csd->r2w_factor = UNSTUFF_BITS(resp, 26, 3);
    csd->write_blkbits = UNSTUFF_BITS(resp, 22, 4);
    csd->write_partial = UNSTUFF_BITS(resp, 21, 1);

    m = UNSTUFF_BITS(resp, 115, 4);
    e = UNSTUFF_BITS(resp, 112, 3);
    csd->tacc_ns     = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
    csd->tacc_clks   = UNSTUFF_BITS(resp, 104, 8) * 100;
    
    e = UNSTUFF_BITS(resp, 47, 3);
    m = UNSTUFF_BITS(resp, 62, 12);
    csd->capacity     = (1 + m) << (e + 2);

    if (mmc_card_sd(card)) {
        csd->erase_blk_en = UNSTUFF_BITS(resp, 46, 1);
        csd->erase_sctsz = UNSTUFF_BITS(resp, 39, 7) + 1;

        switch (csd_struct) {
        case 0:
            break;
        case 1:
            /*
             * This is a block-addressed SDHC card. Most
             * interesting fields are unused and have fixed
             * values. To avoid getting tripped by buggy cards,
             * we assume those fixed values ourselves.
             */
            mmc_card_set_blockaddr(card);

            csd->tacc_ns	 = 0; /* Unused */
            csd->tacc_clks	 = 0; /* Unused */

            m = UNSTUFF_BITS(resp, 48, 22);
            csd->capacity     = (1 + m) << 10;

            csd->read_blkbits = 9;
            csd->read_partial = 0;
            csd->write_misalign = 0;
            csd->read_misalign = 0;
            csd->r2w_factor = 4; /* Unused */
            csd->write_blkbits = 9;
            csd->write_partial = 0;
            break;
        default:
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Unknown CSD ver %d\n", mmc_card_id(card), csd_struct));
            return MMC_ERR_INVALID;
        }
    } else {
        /*
         * We only understand CSD structure v1.1 and v1.2.
         * v1.2 has extra information in bits 15, 11 and 10.
         */
        if (csd_struct != CSD_STRUCT_VER_1_0 && csd_struct != CSD_STRUCT_VER_1_1 
            && csd_struct != CSD_STRUCT_VER_1_2 && csd_struct != CSD_STRUCT_EXT_CSD) {
            MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] Unknown CSD ver %d\n", mmc_card_id(card), csd_struct));
            return MMC_ERR_INVALID;
        }

        csd->mmca_vsn    = UNSTUFF_BITS(resp, 122, 4);
        csd->erase_sctsz = (UNSTUFF_BITS(resp, 42, 5) + 1) * (UNSTUFF_BITS(resp, 37, 5) + 1);
    }

    mmc_dump_csd(card);

    return 0;
}

#else
static int mmc_decode_csd(struct mmc_card *card)
{
    struct mmc_csd *csd = &card->csd;
    unsigned int e, m, csd_struct;
    u32 *resp = card->raw_csd;

    /* common part */
    csd_struct = UNSTUFF_BITS(resp, 126, 2);
    csd->csd_struct = csd_struct;

    /* update later according to spec. */
    m = UNSTUFF_BITS(resp, 99, 4);
    e = UNSTUFF_BITS(resp, 96, 3);
    csd->max_dtr      = tran_exp[e] * tran_mant[m];

    e = UNSTUFF_BITS(resp, 47, 3);
    m = UNSTUFF_BITS(resp, 62, 12);
    csd->capacity     = (1 + m) << (e + 2);
    csd->read_blkbits = UNSTUFF_BITS(resp, 80, 4);

    if (mmc_card_sd(card)) {
        switch (csd_struct) {
        case 0:
            break;
        case 1:
            /*
             * This is a block-addressed SDHC card. Most
             * interesting fields are unused and have fixed
             * values. To avoid getting tripped by buggy cards,
             * we assume those fixed values ourselves.
             */
            mmc_card_set_blockaddr(card);

            m = UNSTUFF_BITS(resp, 48, 22);
            csd->capacity     = (1 + m) << 10;
            csd->read_blkbits = 9;            
            break;
        default:
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Unknown CSD ver %d\n", mmc_card_id(card), csd_struct));
            return MMC_ERR_INVALID;
        }

    } else {
        /*
         * We only understand CSD structure v1.1 and v1.2.
         * v1.2 has extra information in bits 15, 11 and 10.
         */
        if (csd_struct != CSD_STRUCT_VER_1_0 && csd_struct != CSD_STRUCT_VER_1_1 
            && csd_struct != CSD_STRUCT_VER_1_2 && csd_struct != CSD_STRUCT_EXT_CSD) {
            MSDC_DBG_PRINT(MSDC_DEBUG,("[SD%d] Unknown CSD ver %d\n", mmc_card_id(card), csd_struct));
            return MMC_ERR_INVALID;
        }

        csd->mmca_vsn    = UNSTUFF_BITS(resp, 122, 4);
    }

    return 0;
}
#endif

static void mmc_decode_ext_csd(struct mmc_card *card)
{
    u8 *ext_csd = &card->raw_ext_csd[0];    

    card->ext_csd.sectors =
       	ext_csd[EXT_CSD_SEC_CNT + 0] << 0 |
    	ext_csd[EXT_CSD_SEC_CNT + 1] << 8 |
    	ext_csd[EXT_CSD_SEC_CNT + 2] << 16 |
    	ext_csd[EXT_CSD_SEC_CNT + 3] << 24;

#ifndef FEATURE_MMC_STRIPPED
    card->ext_csd.rev = ext_csd[EXT_CSD_REV];
    card->ext_csd.hc_erase_grp_sz = ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * 512 * 1024;
    card->ext_csd.hc_wp_grp_sz = ext_csd[EXT_CSD_HC_WP_GPR_SIZE] * ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * 512 * 1024;
    card->ext_csd.trim_tmo_ms = ext_csd[EXT_CSD_TRIM_MULT] * 300;
    card->ext_csd.boot_info   = ext_csd[EXT_CSD_BOOT_INFO];
    card->ext_csd.boot_part_sz = ext_csd[EXT_CSD_BOOT_SIZE_MULT] * 128 * 1024;
    card->ext_csd.access_sz = (ext_csd[EXT_CSD_ACC_SIZE] & 0xf) * 512;
    card->ext_csd.rpmb_sz = ext_csd[EXT_CSD_RPMB_SIZE_MULT] * 128 * 1024;
    card->ext_csd.erased_mem_cont = ext_csd[EXT_CSD_ERASED_MEM_CONT];
    card->ext_csd.part_en = ext_csd[EXT_CSD_PART_SUPPORT] & EXT_CSD_PART_SUPPORT_PART_EN ? 1 : 0;
    card->ext_csd.enh_attr_en = ext_csd[EXT_CSD_PART_SUPPORT] & EXT_CSD_PART_SUPPORT_ENH_ATTR_EN ? 1 : 0;
    card->ext_csd.enh_start_addr = 
        (ext_csd[EXT_CSD_ENH_START_ADDR + 0] |
         ext_csd[EXT_CSD_ENH_START_ADDR + 1] << 8 |
         ext_csd[EXT_CSD_ENH_START_ADDR + 2] << 16 |
         ext_csd[EXT_CSD_ENH_START_ADDR + 3] << 24);
    card->ext_csd.enh_sz = 
        (ext_csd[EXT_CSD_ENH_SIZE_MULT + 0] | 
         ext_csd[EXT_CSD_ENH_SIZE_MULT + 1] << 8 | 
         ext_csd[EXT_CSD_ENH_SIZE_MULT + 2] << 16) * 512 * 1024 *
         ext_csd[EXT_CSD_HC_WP_GPR_SIZE] * ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE];
#endif

    if (card->ext_csd.sectors)
       	mmc_card_set_blockaddr(card);

    if ((ext_csd[EXT_CSD_CARD_TYPE] & EXT_CSD_CARD_TYPE_DDR_52_1_2V) ||
        (ext_csd[EXT_CSD_CARD_TYPE] & EXT_CSD_CARD_TYPE_DDR_52)) {
        card->ext_csd.ddr_support = 1;
        card->ext_csd.hs_max_dtr = 52000000;
    } else if (ext_csd[EXT_CSD_CARD_TYPE] & EXT_CSD_CARD_TYPE_52) {
        card->ext_csd.hs_max_dtr = 52000000;
    } else if ((ext_csd[EXT_CSD_CARD_TYPE] & EXT_CSD_CARD_TYPE_26)) {
        card->ext_csd.hs_max_dtr = 26000000;
    } else {
        /* MMC v4 spec says this cannot happen */
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] MMCv4 but HS unsupported\n",card->host->id));
    }

    mmc_dump_ext_csd(card);
    return;
}

#if 0
int mmc_deselect_all_card(struct mmc_host *host)
{
    int err;
    struct mmc_command cmd;

    cmd.opcode  = MMC_CMD_SELECT_CARD;
    cmd.arg     = 0;
    cmd.rsptyp  = RESP_NONE;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    err = mmc_cmd(host, &cmd);

    return err;
}
#endif

int mmc_select_card(struct mmc_host *host, struct mmc_card *card)
{
    int err;
    struct mmc_command cmd;

    cmd.opcode  = MMC_CMD_SELECT_CARD;
    cmd.arg     = card->rca << 16;
    cmd.rsptyp  = RESP_R1B;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    err = mmc_cmd(host, &cmd);

    return err;
}

static int mmc_send_relative_addr(struct mmc_host *host, struct mmc_card *card, unsigned int *rca)
{
    int err;
    struct mmc_command cmd;

    memset(&cmd, 0, sizeof(struct mmc_command));

    if (mmc_card_mmc(card)) { /* set rca */
        cmd.opcode  = MMC_CMD_SET_RELATIVE_ADDR;
        cmd.arg     = *rca << 16;
        cmd.rsptyp  = RESP_R1;
        cmd.retries = CMD_RETRIES;
        cmd.timeout = CMD_TIMEOUT;
    } else {  /* send rca */
        cmd.opcode  = SD_CMD_SEND_RELATIVE_ADDR;
        cmd.arg     = 0;
        cmd.rsptyp  = RESP_R6;
        cmd.retries = CMD_RETRIES;
        cmd.timeout = CMD_TIMEOUT;
    }
    err = mmc_cmd(host, &cmd);
    if ((err == MMC_ERR_NONE) && !mmc_card_mmc(card))
        *rca = cmd.resp[0] >> 16;

    return err;
}

#if 0
int mmc_send_tuning_blk(struct mmc_host *host, struct mmc_card *card, u32 *buf)
{
    int err;
	struct mmc_command cmd;

    cmd.opcode  = SD_CMD_SEND_TUNING_BLOCK;
    cmd.arg     = 0;
    cmd.rsptyp  = RESP_R1;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    msdc_set_blknum(host, 1);
    msdc_set_blklen(host, 64);
    msdc_set_timeout(host, 100000000, 0);
    err = mmc_cmd(host, &cmd);
    if (err != MMC_ERR_NONE)
        goto out;
    
    err = msdc_pio_read(host, buf, 64);
    if (err != MMC_ERR_NONE)
        goto out;

    mmc_dump_tuning_blk((u8*)buf);

out:
    return err;
}
#endif

int mmc_switch(struct mmc_host *host, struct mmc_card *card, 
               u8 set, u8 index, u8 value)
{
    int err;
    u32 status = 0;
    uint count = 0;
    struct mmc_command cmd;

    cmd.opcode = MMC_CMD_SWITCH;
    cmd.arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
        (index << 16) | (value << 8) | set;
    cmd.rsptyp = RESP_R1B;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    err = mmc_cmd(host, &cmd);

    if (err != MMC_ERR_NONE)
        return err;

    do {
        err = mmc_send_status(host, card, &status);
        if (err) {
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Fail to send status %d\n", host->id, err));
            break;
        }
        if (status & R1_SWITCH_ERROR) {
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] switch error. arg(0x%x)\n", host->id, cmd.arg));
            return MMC_ERR_FAILED;
        }
        if (count++ >= 600000)
        {
            MSDC_ERR_PRINT(MSDC_ERROR,("[%s]: timeout happend, count=%d, status=0x%x\n", __func__, count, status));
            break;
        }

    } while (!(status & R1_READY_FOR_DATA) || (R1_CURRENT_STATE(status) == 7));

    return err;
}

static int mmc_sd_switch(struct mmc_host *host, 
                         struct mmc_card *card, 
                         int mode, int group, u8 value, mmc_switch_t *resp)
{
    int err = MMC_ERR_FAILED;
	int result = 0;
    struct mmc_command cmd;
    u32 *sts = (u32 *)resp;
    int retries __attribute__((unused));

    mode = !!mode;
    value &= 0xF;

    /* argument: mode[31]= 0 (for check func.) and 1 (for switch func) */
    cmd.opcode = SD_CMD_SWITCH;
    cmd.arg = mode << 31 | 0x00FFFFFF;
    cmd.arg &= ~(0xF << (group * 4));
    cmd.arg |= value << (group * 4);
    cmd.rsptyp = RESP_R1;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = 100;  /* 100ms */
	msdc_reset_tune_counter(host);
	do{
    msdc_set_blknum(host, 1);
    msdc_set_blklen(host, 64);
    msdc_set_timeout(host, 100000000, 0);
    err = mmc_cmd(host, &cmd);

    if (err != MMC_ERR_NONE)
        goto out;

    retries = 50000;

    /* 512 bits = 64 bytes = 16 words */
    err = msdc_pio_read(host, sts, 64);
    if (err != MMC_ERR_NONE){
			if(msdc_abort_handler(host, 1))
				MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] data abort failed\n",host->id));
			result = msdc_tune_read(host);
		}
	}while(err && result != MMC_ERR_READTUNEFAIL);
	msdc_reset_tune_counter(host);

    {
        int i;
 #ifndef KEEP_SLIENT_BUILD        
        u8 *byte = (u8*)&sts[0];
#endif
        /* Status:   B0      B1    ...
         * Bits  : 511-504 503-495 ...
         */

        for (i = 0; i < 4; i++) {
            MSDC_DBG_PRINT(MSDC_DEBUG, ("  [%d-%d] %xh %xh %xh %xh\n",
                ((3 - i + 1) << 7) - 1, (3 - i) << 7, 
                sts[(i << 2) + 0], sts[(i << 2) + 1],
                sts[(i << 2) + 2], sts[(i << 2) + 3]));
        }
        for (i = 0; i < 8; i++) {
            MSDC_DBG_PRINT(MSDC_DEBUG, ("  [%d-%d] %xh %xh %xh %xh %xh %xh %xh %xh\n",
                ((8 - i) << 6) - 1, (8 - i - 1) << 6, 
                byte[(i << 3) + 0], byte[(i << 3) + 1], 
                byte[(i << 3) + 2], byte[(i << 3) + 3],
                byte[(i << 3) + 4], byte[(i << 3) + 5], 
                byte[(i << 3) + 6], byte[(i << 3) + 7]));
        }
    }

out:
    return err;
}

#ifdef FEATURE_MMC_UHS1
int mmc_ctrl_speed_class(struct mmc_host *host, u32 scc)
{
	struct mmc_command cmd;

    cmd.opcode  = SD_CMD_SPEED_CLASS_CTRL;
    cmd.arg     = scc << 28;
    cmd.rsptyp  = RESP_R1B;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    return mmc_cmd(host, &cmd);
}

static int mmc_switch_volt(struct mmc_host *host, struct mmc_card *card)
{
    int err;
	struct mmc_command cmd;

    
    cmd.opcode  = SD_CMD_VOL_SWITCH;
    cmd.arg     = 0;
    cmd.rsptyp  = RESP_R1;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    err = mmc_cmd(host, &cmd);

    if (err == MMC_ERR_NONE)
        err = msdc_switch_volt(host, MMC_VDD_18_19);

    return err;
}
#endif

int mmc_switch_hs(struct mmc_host *host, struct mmc_card *card)
{
    int err;
    u8  status[64];
    int val = MMC_SWITCH_MODE_SDR25;

    err = mmc_sd_switch(host, card, 1, 0, val, (mmc_switch_t*)&status[0]);

    if (err != MMC_ERR_NONE)
        goto out;

    if ((status[16] & 0xF) != 1) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] HS mode not supported!\n", host->id));
        err = MMC_ERR_FAILED;
    } else {
        MSDC_TRC_PRINT(MSDC_INFO,("[SD%d] Switch to HS mode!\n", host->id));
        mmc_card_set_highspeed(card);
    }

out:
    return err;
}

#ifdef FEATURE_MMC_UHS1
int mmc_switch_uhs1(struct mmc_host *host, struct mmc_card *card, unsigned int mode)
{
    int err;
    u8  status[64];
    int val;
    const char *smode[] = { "SDR12", "SDR25", "SDR50", "SDR104", "DDR50" };
    
    err = mmc_sd_switch(host, card, 1, 0, mode, (mmc_switch_t*)&status[0]);

    if (err != MMC_ERR_NONE)
       	goto out;

    if ((status[16] & 0xF) != mode) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] UHS-1 %s mode not supported!\n", host->id, smode[mode]));
        err = MMC_ERR_FAILED;
    } else {
        card->uhs_mode = mode;
        mmc_card_set_uhs1(card);
        MSDC_TRC_PRINT(MSDC_INFO,("[SD%d] Switch to UHS-1 %s mode!\n",host->id, smode[mode]));
        if (mode == MMC_SWITCH_MODE_DDR50) {
            mmc_card_set_ddr(card);
        }
    }

out:
    return err;
}

int mmc_switch_drv_type(struct mmc_host *host, struct mmc_card *card, int val)
{
    int err;
    u8  status[64];
    const char *type[] = { "TYPE-B", "TYPE-A", "TYPE-C", "TYPE-D" };
    
    err = mmc_sd_switch(host, card, 1, 2, val, (mmc_switch_t*)&status[0]);

    if (err != MMC_ERR_NONE)
       	goto out;

    if ((status[15] & 0xF) != val) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] UHS-1 %s drv not supported!\n", host->id, type[val]));
        err = MMC_ERR_FAILED;
    } else {
        MSDC_TRC_PRINT(MSDC_INFO,("[SD%d] Switch to UHS-1 %s drv!\n", host->id, type[val]));
    }

out:
    return err;
}

int mmc_switch_max_cur(struct mmc_host *host, struct mmc_card *card, int val)
{
    int err;
    u8  status[64];
    const char *curr[] = { "200mA", "400mA", "600mA", "800mA" };
    
    err = mmc_sd_switch(host, card, 1, 3, val, (mmc_switch_t*)&status[0]);

    if (err != MMC_ERR_NONE)
       	goto out;

    if (((status[15] >> 4) & 0xF) != val) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] UHS-1 %s max. current not supported!\n",host->id, curr[val]));
        err = MMC_ERR_FAILED;
    } else {
        MSDC_TRC_PRINT(MSDC_INFO,("[SD%d] Switch to UHS-1 %s max. current!\n",host->id, curr[val]));
    }

out:
    return err;
}
#endif

static int mmc_read_csds(struct mmc_host *host, struct mmc_card *card)
{
    int err;
    struct mmc_command cmd;

    cmd.opcode  = MMC_CMD_SEND_CSD;
    cmd.arg     = card->rca << 16;
    cmd.rsptyp  = RESP_R2;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT * 100;

    err = mmc_cmd(host, &cmd);
    if (err == MMC_ERR_NONE)
        memcpy(&card->raw_csd, &cmd.resp[0], sizeof(u32) * 4);
    return err;
}

#ifndef FEATURE_MMC_STRIPPED
static int mmc_read_scrs(struct mmc_host *host, struct mmc_card *card)
{
    int err = MMC_ERR_NONE; 
	int result; 
    int retries;
    struct mmc_command cmd;
    struct sd_scr *scr = &card->scr;
    u32 resp[4];
    u32 tmp;

    msdc_set_blknum(host, 1);
    msdc_set_blklen(host, 8);
    msdc_set_timeout(host, 100000000, 0);

    cmd.opcode  = SD_ACMD_SEND_SCR;
    cmd.arg     = 0;
    cmd.rsptyp  = RESP_R1;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;
	msdc_reset_tune_counter(host);
	do{
    mmc_app_cmd(host, &cmd, card->rca, CMD_RETRIES);
    if ((err != MMC_ERR_NONE) || !(cmd.resp[0] & R1_APP_CMD))
        return MMC_ERR_FAILED;

    retries = 50000;

    /* 8 bytes = 2 words */
    err = msdc_pio_read(host, card->raw_scr, 8);
    if (err != MMC_ERR_NONE){
			if(msdc_abort_handler(host, 1))
				MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] data abort failed\n",host->id));
			result = msdc_tune_read(host);
		}
	}while(err && result != MMC_ERR_READTUNEFAIL);
	msdc_reset_tune_counter(host);
    MSDC_TRC_PRINT(MSDC_DREG,("[SD%d] SCR: %x %x (raw)\n", host->id, card->raw_scr[0], card->raw_scr[1]));

    tmp = ntohl(card->raw_scr[0]);
    card->raw_scr[0] = ntohl(card->raw_scr[1]);
    card->raw_scr[1] = tmp;   

    MSDC_TRC_PRINT(MSDC_DREG,("[SD%d] SCR: %x %x (ntohl)\n", host->id, card->raw_scr[0], card->raw_scr[1]));

    resp[2] = card->raw_scr[1];
    resp[3] = card->raw_scr[0];

    if (UNSTUFF_BITS(resp, 60, 4) != 0) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Unknown SCR ver %d\n",
            mmc_card_id(card), unstuff_bits(resp, 60, 4)));
        return MMC_ERR_INVALID;
    }

    scr->scr_struct = UNSTUFF_BITS(resp, 60, 4);
    scr->sda_vsn = UNSTUFF_BITS(resp, 56, 4);
    scr->data_bit_after_erase = UNSTUFF_BITS(resp, 55, 1);
    scr->security = UNSTUFF_BITS(resp, 52, 3);
    scr->bus_widths = UNSTUFF_BITS(resp, 48, 4);
    scr->sda_vsn3 = UNSTUFF_BITS(resp, 47, 1);
    scr->ex_security = UNSTUFF_BITS(resp, 43, 4);
    scr->cmd_support = UNSTUFF_BITS(resp, 32, 2);
    MSDC_TRC_PRINT(MSDC_INFO,("[SD%d] SD_SPEC(%d) SD_SPEC3(%d) SD_BUS_WIDTH=%d\n",
        							mmc_card_id(card), scr->sda_vsn, scr->sda_vsn3, scr->bus_widths));
	
    MSDC_TRC_PRINT(MSDC_INFO,("[SD%d] SD_SECU(%d) EX_SECU(%d), CMD_SUPP(%d): CMD23(%d), CMD20(%d)\n",
       								 mmc_card_id(card), scr->security, scr->ex_security, scr->cmd_support,
        												(scr->cmd_support >> 1) & 0x1, scr->cmd_support & 0x1));
    return err;
}
#endif

/* Read and decode extended CSD. */
int mmc_read_ext_csd(struct mmc_host *host, struct mmc_card *card)
{
    int err = MMC_ERR_NONE;
    u32 *ptr;
	int result = MMC_ERR_NONE;
    struct mmc_command cmd;

    if (card->csd.mmca_vsn < CSD_SPEC_VER_4) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] MMCA_VSN: %d. Skip EXT_CSD\n",host->id, card->csd.mmca_vsn));
        return MMC_ERR_NONE;
    }

    /*
     * As the ext_csd is so large and mostly unused, we don't store the
     * raw block in mmc_card.
     */
    memset(&card->raw_ext_csd[0], 0, 512);
    ptr = (u32*)&card->raw_ext_csd[0];

    cmd.opcode  = MMC_CMD_SEND_EXT_CSD;
    cmd.arg     = 0;
    cmd.rsptyp  = RESP_R1;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;
	msdc_reset_tune_counter(host);
do{
    msdc_set_blknum(host, 1);
    msdc_set_blklen(host, 512);
    msdc_set_timeout(host, 100000000, 0);
    err = mmc_cmd(host, &cmd);
    if (err != MMC_ERR_NONE)
        goto out;

    err = msdc_pio_read(host, ptr, 512);
    if (err != MMC_ERR_NONE){
       	if(msdc_abort_handler(host, 1))
			MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] data abort failed\n",host->id));
		result = msdc_tune_read(host);
	}
}while(err && result != MMC_ERR_READTUNEFAIL);
	msdc_reset_tune_counter(host);
    mmc_decode_ext_csd(card);

out:
    return err;
}

/* Fetches and decodes switch information */
static int mmc_read_switch(struct mmc_host *host, struct mmc_card *card)
{
    int err;
    u8  status[64];

    err = mmc_sd_switch(host, card, 0, 0, 1, (mmc_switch_t*)&status[0]);
    if (err != MMC_ERR_NONE) {
        /* Card not supporting high-speed will ignore the command. */
        err = MMC_ERR_NONE;
        goto out;
    }

    /* bit 511:480 in status[0]. bit 415:400 in status[13] */
    if (status[13] & 0x01) {
        MSDC_TRC_PRINT(MSDC_DREG,("[SD%d] Support: Default/SDR12\n", host->id));
        card->sw_caps.hs_max_dtr = 25000000;  /* default or sdr12 */
    }
    if (status[13] & 0x02) {
        MSDC_TRC_PRINT(MSDC_DREG,("[SD%d] Support: HS/SDR25\n", host->id));
        card->sw_caps.hs_max_dtr = 50000000;  /* high-speed or sdr25 */
    } 
    if (status[13] & 0x10) {
        MSDC_TRC_PRINT(MSDC_DREG,("[SD%d] Support: DDR50\n", host->id));
        card->sw_caps.hs_max_dtr = 50000000;  /* ddr50 */
        card->sw_caps.ddr = 1;
    }
#ifdef FEATURE_MMC_UHS1
    if (status[13] & 0x04) {
        MSDC_TRC_PRINT(MSDC_DREG,("[SD%d] Support: SDR50\n", host->id));
        card->sw_caps.hs_max_dtr = 100000000; /* sdr50 */    
    }    
    if (status[13] & 0x08) {
        MSDC_TRC_PRINT(MSDC_DREG,("[SD%d] Support: SDR104\n", host->id));
        card->sw_caps.hs_max_dtr = 208000000; /* sdr104 */
    }    
    if (status[9] & 0x01) {
        MSDC_TRC_PRINT(MSDC_DREG,("[SD%d] Support: Type-B Drv\n", host->id));
    }    
    if (status[9] & 0x02) {
        MSDC_TRC_PRINT(MSDC_DREG,("[SD%d] Support: Type-A Drv\n", host->id));
    } 
    if (status[9] & 0x04) {
        MSDC_TRC_PRINT(MSDC_DREG,("[SD%d] Support: Type-C Drv\n", host->id));
    }    
    if (status[9] & 0x08) {
        MSDC_TRC_PRINT(MSDC_DREG,("[SD%d] Support: Type-D Drv\n", host->id));
    }
    if (status[7] & 0x01) {
        MSDC_TRC_PRINT(MSDC_DREG,("[SD%d] Support: 200mA current limit\n", host->id));
    }    
    if (status[7] & 0x02) {
        MSDC_TRC_PRINT(MSDC_DREG,("[SD%d] Support: 400mA current limit\n", host->id));
    } 
    if (status[7] & 0x04) {
        MSDC_TRC_PRINT(MSDC_DREG,("[SD%d] Support: 600mA current limit\n", host->id));
    }    
    if (status[7] & 0x08) {
        MSDC_TRC_PRINT(MSDC_DREG,("[SD%d] Support: 800mA current limit\n", host->id));
    }
#endif

out:
    return err;
}

#ifdef FEATURE_MMC_SDIO
int mmc_io_rw_direct(struct mmc_card *card, int write, unsigned fn,	
    unsigned addr, u8 in, u8* out)
{
    struct mmc_command cmd;
    int err;

    MSDC_BUGON(!card);
    MSDC_BUGON(fn > 7);

    memset(&cmd, 0, sizeof(struct mmc_command));

    cmd.opcode = SD_IO_RW_DIRECT;
    cmd.arg = write ? 0x80000000 : 0x00000000;
    cmd.arg |= fn << 28;
    cmd.arg |= (write && out) ? 0x08000000 : 0x00000000;
    cmd.arg |= addr << 9;
    cmd.arg |= in;
    cmd.rsptyp  = RESP_R5;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;	

    err = mmc_cmd(card->host, &cmd);

    if (err)
        return err;

    if (cmd.resp[0] & R5_ERROR)
        return MMC_ERR_FAILED;
    if (cmd.resp[0] & R5_FUNCTION_NUMBER)
        return MMC_ERR_INVALID;
    if (cmd.resp[0] & R5_OUT_OF_RANGE)
        return MMC_ERR_INVALID;

    if (out)
        *out = cmd.resp[0] & 0xFF;

    return MMC_ERR_NONE;
}

int mmc_io_rw_extended(struct mmc_card *card, int write, unsigned fn,
	unsigned addr, int incr_addr, u8 *buf, unsigned blocks, unsigned blksz)
{
    int err;

    MSDC_BUGON(!card);
    MSDC_BUGON(fn > 7);
    MSDC_BUGON(blocks == 1 && blksz > 512);
    MSDC_BUGON(blocks == 0);
    MSDC_BUGON(blksz == 0);

    /* sanity check */
    if (addr & ~0x1FFFF)
        return MMC_ERR_INVALID;

    err = msdc_iorw(card, write, fn, addr, incr_addr, buf, blocks, blksz);

    return err;
}

static unsigned int mmc_sdio_max_byte_size(struct sdio_func *func)
{
    unsigned mval =	min(func->card->host->max_seg_size,
        func->card->host->max_blk_size);
    mval = min(mval, func->max_blksize);
    return min(mval, 512u); /* maximum size for byte mode */
}

/* Split an arbitrarily sized data transfer into several IO_RW_EXTENDED commands. */
static int mmc_sdio_io_rw_ext_helper(struct sdio_func *func, int write,
	unsigned addr, int incr_addr, u8 *buf, unsigned size)
{
	unsigned remainder = size;
	unsigned max_blocks;
	int ret;

	/* Do the bulk of the transfer using block mode (if supported). */
	if (func->card->cccr.multi_block && (size > mmc_sdio_max_byte_size(func))) {
		/* Blocks per command is limited by host count, host transfer
		 * size (we only use a single sg entry) and the maximum for
		 * IO_RW_EXTENDED of 511 blocks. */
		max_blocks = min(func->card->host->max_blk_count,
			func->card->host->max_seg_size / func->cur_blksize);
		max_blocks = min(max_blocks, 511u);

		while (remainder > func->cur_blksize) {
			unsigned blocks;

			blocks = remainder / func->cur_blksize;
			if (blocks > max_blocks)
				blocks = max_blocks;
			size = blocks * func->cur_blksize;

			ret = mmc_io_rw_extended(func->card, write,
				func->num, addr, incr_addr, buf,
				blocks, func->cur_blksize);
			if (ret)
				return ret;

			remainder -= size;
			buf += size;
			if (incr_addr)
				addr += size;
		}
	}

	/* Write the remainder using byte mode. */
	while (remainder > 0) {
		size = min(remainder, mmc_sdio_max_byte_size(func));

		ret = mmc_io_rw_extended(func->card, write, func->num, addr,
			 incr_addr, buf, 1, size);
		if (ret)
			return ret;

		remainder -= size;
		buf += size;
		if (incr_addr)
			addr += size;
	}
	return 0;
}

int mmc_sdio_memcpy_fromio(struct sdio_func *func, void *dst,
	unsigned int addr, int count)
{
	return mmc_sdio_io_rw_ext_helper(func, 0, addr, 1, dst, count);
}

int mmc_sdio_memcpy_toio(struct sdio_func *func, unsigned int addr,
	void *src, int count)
{
	return mmc_sdio_io_rw_ext_helper(func, 1, addr, 1, src, count);
}

int mmc_sdio_readsb(struct sdio_func *func, void *dst, unsigned int addr,
	int count)
{
	return mmc_sdio_io_rw_ext_helper(func, 0, addr, 0, dst, count);
}

int mmc_sdio_writesb(struct sdio_func *func, unsigned int addr, void *src,
	int count)
{
	return mmc_sdio_io_rw_ext_helper(func, 1, addr, 0, src, count);
}

u16 mmc_sdio_readw(struct sdio_func *func, unsigned int addr, int *err_ret)
{
	int ret;
	u16 tmp;

	if (err_ret)
		*err_ret = 0;

	ret = mmc_sdio_memcpy_fromio(func, func->tmpbuf, addr, 2);
	if (ret) {
		if (err_ret)
			*err_ret = ret;
		return 0xFFFF;
	}

	memcpy(&tmp, func->tmpbuf, sizeof(u16));

	return tmp;
}

void mmc_sdio_writew(struct sdio_func *func, u16 b, unsigned int addr, int *err_ret)
{
	int ret;

    memcpy(&func->tmpbuf[0], &b, sizeof(u16));

	ret = mmc_sdio_memcpy_toio(func, addr, func->tmpbuf, 2);
	if (err_ret)
		*err_ret = ret;
}

u32 mmc_sdio_readl(struct sdio_func *func, unsigned int addr, int *err_ret)
{
	int ret;
	u32 tmp;

	if (err_ret)
		*err_ret = 0;

	ret = mmc_sdio_memcpy_fromio(func, func->tmpbuf, addr, 4);
	if (ret) {
		if (err_ret)
			*err_ret = ret;
		return 0xFFFFFFFF;
	}

    memcpy(&tmp, func->tmpbuf, sizeof(u32));
    
	return tmp;
}

void mmc_sdio_writel(struct sdio_func *func, u32 b, unsigned int addr, int *err_ret)
{
    int ret;

    memcpy(&func->tmpbuf[0], &b, sizeof(u32));

    ret = mmc_sdio_memcpy_toio(func, addr, func->tmpbuf, 4);
    if (err_ret)
        *err_ret = ret;
}

unsigned char mmc_sdio_f0_readb(struct sdio_func *func, unsigned int addr,
	int *err_ret)
{
	int ret;
	unsigned char val;

	MSDC_BUGON(!func);

	if (err_ret)
		*err_ret = 0;

	ret = mmc_io_rw_direct(func->card, 0, 0, addr, 0, &val);
	if (ret) {
		if (err_ret)
			*err_ret = ret;
		return 0xFF;
	}

	return val;
}
#if 0
void mmc_sdio_f0_writeb(struct sdio_func *func, unsigned char b, unsigned int addr,
	int *err_ret)
{
	int ret;

	MSDC_BUGON(!func);

	if ((addr < 0xF0 || addr > 0xFF) && (!mmc_card_lenient_fn0(func->card))) {
		if (err_ret)
			*err_ret = MMC_ERR_INVALID;
		return;
	}

	ret = mmc_io_rw_direct(func->card, 1, 0, addr, b, NULL);
	if (err_ret)
		*err_ret = ret;
}
#endif

int mmc_sdio_enable_func(struct sdio_func *func)
{
	int ret;
	unsigned char reg;
	unsigned long timeout;

	MSDC_BUGON(!func);
	MSDC_BUGON(!func->card);

	MSDC_DBG_PRINT(MSDC_DEBUG,("SDIO: Enabling device ...\n"));

	ret = mmc_io_rw_direct(func->card, 0, 0, SDIO_CCCR_IOEx, 0, &reg);
	if (ret)
		goto err;

	reg |= 1 << func->num;

	ret = mmc_io_rw_direct(func->card, 1, 0, SDIO_CCCR_IOEx, reg, NULL);
	if (ret)
		goto err;

	while (1) {
		ret = mmc_io_rw_direct(func->card, 0, 0, SDIO_CCCR_IORx, 0, &reg);
		if (ret)
			goto err;
		if (reg & (1 << func->num))
			break;
	}

	MSDC_DBG_PRINT(MSDC_DEBUG,("SDIO: Enabled device\n"));

	return 0;

err:
	MSDC_DBG_PRINT(MSDC_DEBUG,("SDIO: Failed to enable device\n"));
	return ret;
}

int mmc_sdio_set_block_size(struct sdio_func *func, unsigned blksz)
{
	int ret;

	if (blksz > func->card->host->max_blk_size)
		return MMC_ERR_INVALID;

	if (blksz == 0) {
		blksz = min(func->max_blksize, func->card->host->max_blk_size);
		blksz = min(blksz, 512u);
	}

	ret = mmc_io_rw_direct(func->card, 1, 0,
		SDIO_FBR_BASE(func->num) + SDIO_FBR_BLKSIZE,
		blksz & 0xff, NULL);
	if (ret)
		return ret;
	ret = mmc_io_rw_direct(func->card, 1, 0,
		SDIO_FBR_BASE(func->num) + SDIO_FBR_BLKSIZE + 1,
		(blksz >> 8) & 0xff, NULL);
	if (ret)
		return ret;
	func->cur_blksize = blksz;
	return 0;
}

int mmc_sdio_disable_func(struct sdio_func *func)
{
	int ret;
	unsigned char reg;

	MSDC_BUGON(!func);
	MSDC_BUGON(!func->card);

	MSDC_DBG_PRINT(MSDC_DEBUG,("SDIO: Disabling device...\n"));

	ret = mmc_io_rw_direct(func->card, 0, 0, SDIO_CCCR_IOEx, 0, &reg);
	if (ret)
		goto err;

	reg &= ~(1 << func->num);

	ret = mmc_io_rw_direct(func->card, 1, 0, SDIO_CCCR_IOEx, reg, NULL);
	if (ret)
		goto err;

	MSDC_DBG_PRINT(MSDC_DEBUG,("SDIO: Disabled device\n"));

	return 0;

err:
	MSDC_DBG_PRINT(MSDC_DEBUG,("SDIO: Failed to disable device %s\n"));
	return MMC_ERR_FAILED;
}

static int mmc_sdio_read_cis(struct mmc_card *card, struct sdio_func *func)
{
    int ret;
    struct sdio_func_tuple *this, **prev;
    unsigned i, ptr = 0;

    /*
    * Note that this works for the common CIS (function number 0) as
    * well as a function's CIS * since SDIO_CCCR_CIS and SDIO_FBR_CIS
    * have the same offset.
    */
    for (i = 0; i < 3; i++) {
        unsigned char x, fn;

        if (func)
            fn = func->num;
        else
            fn = 0;

        ret = mmc_io_rw_direct(card, 0, 0,
            SDIO_FBR_BASE(fn) + SDIO_FBR_CIS + i, 0, &x);
        if (ret)
            return ret;
        ptr |= x << (i * 8);
    }

    if (func)
        prev = &func->tuples;
    else
        prev = &card->tuples;

    MSDC_BUGON(*prev);

    do {
        unsigned char tpl_code, tpl_link;

        ret = mmc_io_rw_direct(card, 0, 0, ptr++, 0, &tpl_code);
        if (ret)
            break;

        /* 0xff means we're done */
        if (tpl_code == 0xff)
            break;

        ret = mmc_io_rw_direct(card, 0, 0, ptr++, 0, &tpl_link);
        if (ret)
            break;

        this = malloc(sizeof(*this) + tpl_link);
        if (!this)
            return -__LINE__;

        for (i = 0; i < tpl_link; i++) {
            ret = mmc_io_rw_direct(card, 0, 0,
                ptr + i, 0, &this->data[i]);
            if (ret)
                break;
        }
        if (ret) {
            free(this);
            break;
        }

        for (i = 0; i < ARRAY_SIZE(cis_tpl_list); i++)
            if (cis_tpl_list[i].code == tpl_code)
                break;

        if (i >= ARRAY_SIZE(cis_tpl_list)) {
            /* this tuple is unknown to the core */
            this->next = NULL;
            this->code = tpl_code;
            this->size = tpl_link;
            *prev = this;
            prev = &this->next;
            MSDC_DBG_PRINT(MSDC_DEBUG,("queuing CIS tuple 0x%02x length %u\n", tpl_code, tpl_link));
        } else {
            const struct cis_tpl *tpl = cis_tpl_list + i;
            if (tpl_link < tpl->min_size) {
            MSDC_DBG_PRINT(MSDC_DEBUG,("bad CIS tuple 0x%02x (length = %u, expected >= %u)\n",
                tpl_code, tpl_link, tpl->min_size));
            ret = -__LINE__;
            } else if (tpl->parse) {
                ret = tpl->parse(card, func, this->data, tpl_link);
            }
            free(this);
        }

        ptr += tpl_link;
    } while (!ret);

    /*
    * Link in all unknown tuples found in the common CIS so that
    * drivers don't have to go digging in two places.
    */
    if (func)
        *prev = card->tuples;

    return ret;
}

int mmc_sdio_read_common_cis(struct mmc_card *card)
{
	return mmc_sdio_read_cis(card, NULL);
}

void mmc_sdio_free_common_cis(struct mmc_card *card)
{
    struct sdio_func_tuple *tuple, *victim;

    tuple = card->tuples;

    while (tuple) {
        victim = tuple;
        tuple = tuple->next;
        free(victim);
    }

    card->tuples = NULL;
}

int mmc_sdio_read_func_cis(struct sdio_func *func)
{
    int ret;

    ret = mmc_sdio_read_cis(func->card, func);
    if (ret)
        return ret;

    /*
    * Vendor/device id is optional for function CIS, so
    * copy it from the card structure as needed.
    */
    if (func->vendor == 0) {
        func->vendor = func->card->cis.vendor;
        func->device = func->card->cis.device;
    }

    return 0;
}

void mmc_sdio_free_func_cis(struct sdio_func *func)
{
    struct sdio_func_tuple *tuple, *victim;

    tuple = func->tuples;

    while (tuple && tuple != func->card->tuples) {
    	victim = tuple;
    	tuple = tuple->next;
    	free(victim);
    }

    func->tuples = NULL;
}


int mmc_sdio_read_fbr(struct sdio_func *func)
{
	int ret;
	unsigned char data;

	ret = mmc_io_rw_direct(func->card, 0, 0,
		SDIO_FBR_BASE(func->num) + SDIO_FBR_STD_IF, 0, &data);
	if (ret)
		goto out;

	data &= 0x0f;

	if (data == 0x0f) {
		ret = mmc_io_rw_direct(func->card, 0, 0,
			SDIO_FBR_BASE(func->num) + SDIO_FBR_STD_IF_EXT, 0, &data);
		if (ret)
			goto out;
	}

	func->class = data;

out:
	return ret;
}

int mmc_sdio_init_func(struct mmc_card *card, unsigned int fn)
{
    int ret;
    struct sdio_func *func;

    MSDC_BUGON(fn > SDIO_MAX_FUNCS);

    func = malloc(sizeof(struct sdio_func));
    if (!func)
    	return -1;

    memset(func, 0, sizeof(struct sdio_func));

    func->card = card;
    func->num  = fn;

    ret = mmc_sdio_read_fbr(func);
    if (ret)
        goto fail;

    ret = mmc_sdio_read_func_cis(func);
    if (ret)
        goto fail;

    card->io_func[fn - 1] = func;

    return 0;

fail:

    return ret;
}

int mmc_sdio_read_cccr(struct mmc_card *card)
{
    int ret;
    int cccr_vsn;
    unsigned char data;

    memset(&card->cccr, 0, sizeof(struct sdio_cccr));

    ret = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_CCCR, 0, &data);
    if (ret)
        goto out;

    cccr_vsn = data & 0x0f;

    if (cccr_vsn > SDIO_CCCR_REV_1_20) {
        MSDC_TRC_PRINT(MSDC_STACK,("unrecognised CCCR structure version %d\n", cccr_vsn));
        return MMC_ERR_INVALID;
    }

    card->cccr.sdio_vsn = (data & 0xf0) >> 4;

    ret = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_CAPS, 0, &data);
    if (ret)
        goto out;

    if (data & SDIO_CCCR_CAP_SMB)
        card->cccr.multi_block = 1;
    if (data & SDIO_CCCR_CAP_LSC)
        card->cccr.low_speed = 1;
    if (data & SDIO_CCCR_CAP_4BLS)
        card->cccr.wide_bus = 1;
    if (data & SDIO_CCCR_CAP_S4MI)
        card->cccr.intr_multi_block = 1;

    if (cccr_vsn >= SDIO_CCCR_REV_1_10) {
        ret = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_POWER, 0, &data);
        if (ret)
            goto out;

        if (data & SDIO_POWER_SMPC)
            card->cccr.high_power = 1;
    }

    if (cccr_vsn >= SDIO_CCCR_REV_1_20) {
        ret = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_SPEED, 0, &data);
        if (ret)
            goto out;

        if (data & SDIO_SPEED_SHS)
            card->cccr.high_speed = 1;
    }

out:
    return ret;
}

int mmc_sdio_proc_pending_irqs(struct mmc_card *card)
{
    int id = card->host->id;
    int i, ret, count;
    unsigned char pending;

    ret = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_INTx, 0, &pending);
    if (ret) {
        MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] error %d reading SDIO_CCCR_INTx\n", id, ret));
        return ret;
    }

    count = 0;
    for (i = 1; i <= 7; i++) {
        if (pending & (1 << i)) {
            struct sdio_func *func = card->io_func[i - 1];
            if (!func) {
                MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] pending IRQ for non-existant function\n", id));
                ret = MMC_ERR_INVALID;
            } else if (func->irq_handler) {
                func->irq_handler(func);
                count++;
            } else {
                MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] pending IRQ with no handler\n", id));
                ret = MMC_ERR_INVALID;
            }
        }
    }

    if (count)
        return count;

    return ret;
}

static int mmc_sdio_card_irq_get(struct mmc_card *card)
{
    struct mmc_host *host = card->host;

    msdc_intr_sdio(card->host, 1);
    
    return 0;
}

static int mmc_sdio_card_irq_put(struct mmc_card *card)
{
    struct mmc_host *host = card->host;

    msdc_intr_sdio(card->host, 0);

    return 0;
}

int mmc_sdio_register_irq(struct mmc_card *card, hw_irq_handler_t handler)
{
    msdc_register_hwirq(card->host, handler);

    return 0;
}

int mmc_sdio_claim_irq(struct sdio_func *func, sdio_irq_handler_t *handler)
{
    int ret;
    unsigned char reg;

    MSDC_BUGON(!func);
    MSDC_BUGON(!func->card);

    MSDC_TRC_PRINT(MSDC_STACK,("SDIO: Enabling IRQ...\n"));

    if (func->irq_handler) {
        MSDC_TRC_PRINT(MSDC_STACK,("SDIO: IRQ already in use.\n"));
        return MMC_ERR_FAILED;
    }

    ret = mmc_io_rw_direct(func->card, 0, 0, SDIO_CCCR_IENx, 0, &reg);
    if (ret)
        return ret;

    reg |= 1 << func->num;

    reg |= 1; /* Master interrupt enable */

    ret = mmc_io_rw_direct(func->card, 1, 0, SDIO_CCCR_IENx, reg, NULL);
    if (ret)
        return ret;

    func->irq_handler = handler;
    ret = mmc_sdio_card_irq_get(func->card);	
    if (ret)
        func->irq_handler = NULL;

    return ret;
}

int mmc_sdio_release_irq(struct sdio_func *func)
{
    int ret;
    unsigned char reg;

    MSDC_BUGON(!func);
    MSDC_BUGON(!func->card);

    MSDC_TRC_PRINT(MSDC_STACK,("SDIO: Disabling IRQ...\n"));

    if (func->irq_handler) {
        func->irq_handler = NULL;
        mmc_sdio_card_irq_put(func->card);
    }
    ret = mmc_io_rw_direct(func->card, 0, 0, SDIO_CCCR_IENx, 0, &reg);
    if (ret)
        return ret;

    reg &= ~(1 << func->num);

    /* Disable master interrupt with the last function interrupt */
    if (!(reg & 0xFE))
        reg = 0;

    ret = mmc_io_rw_direct(func->card, 1, 0, SDIO_CCCR_IENx, reg, NULL);
    if (ret)
        return ret;

    return 0;
}

int mmc_sdio_enable_irq_gap(struct mmc_card *card, int enable)
{
    int ret;
    u8 ctrl;

    if (!(card->host->caps & MMC_CAP_4_BIT_DATA))
        return -1;

    if (!card->cccr.intr_multi_block)
        return -1;

    ret = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_CAPS, 0, &ctrl);
    if (ret)
        return ret;

    if (enable) {
        ctrl |= SDIO_CCCR_CAP_E4MI;
    } else {
        ctrl &= ~SDIO_CCCR_CAP_E4MI;
    }

    ret = mmc_io_rw_direct(card, 1, 0, SDIO_CCCR_CAPS, ctrl, NULL);
    if (ret)
        return ret;

    msdc_intr_sdio_gap(card->host, enable);

    return 0;
}

int mmc_sdio_enable_wide(struct mmc_card *card)
{
    int ret;
    u8 ctrl;

    if (!(card->host->caps & MMC_CAP_4_BIT_DATA))
        return 0;

    if (card->cccr.low_speed && !card->cccr.wide_bus)
        return 0;

    ret = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_IF, 0, &ctrl);
    if (ret)
        return ret;

    ctrl |= SDIO_BUS_WIDTH_4BIT;

    ret = mmc_io_rw_direct(card, 1, 0, SDIO_CCCR_IF, ctrl, NULL);
    if (ret)
        return ret;

    msdc_config_bus(card->host, HOST_BUS_WIDTH_4);

    return 0;
}

/*
 * Test if the card supports high-speed mode and, if so, switch to it.
 */
static int mmc_sdio_enable_hs(struct mmc_card *card)
{
    int ret;
    u8 speed;

    if (!(card->host->caps & MMC_CAP_SD_HIGHSPEED))
        return 0;

    if (!card->cccr.high_speed)
        return 0;

    ret = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_SPEED, 0, &speed);
    if (ret)
        return ret;

    speed |= SDIO_SPEED_EHS;

    ret = mmc_io_rw_direct(card, 1, 0, SDIO_CCCR_SPEED, speed, NULL);
    if (ret)
        return ret;

    mmc_card_set_highspeed(card);

    return 0;
}
#endif

#if 0
static int mmc_deselect_cards(struct mmc_host *host)
{
    struct mmc_command cmd;

    cmd.opcode  = MMC_CMD_SELECT_CARD;
    cmd.arg     = 0;
    cmd.rsptyp  = RESP_NONE;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    return mmc_cmd(host, &cmd);
}

int mmc_lock_unlock(struct mmc_host *host)
{
    struct mmc_command cmd;

    cmd.opcode  = MMC_CMD_LOCK_UNLOCK;
    cmd.rsptyp  = RESP_R1;
    cmd.arg     = 0;
    cmd.retries = 3;
    cmd.timeout = CMD_TIMEOUT;

    return mmc_cmd(host, &cmd);
}
#endif

int mmc_set_write_prot(struct mmc_card *card, u32 addr)
{
    struct mmc_command cmd;

    if (!(card->csd.cmdclass & CCC_WRITE_PROT))
        return MMC_ERR_INVALID;

    cmd.opcode  = MMC_CMD_SET_WRITE_PROT;
    cmd.rsptyp  = RESP_R1B;
    cmd.arg     = addr;
    cmd.retries = 3;
    cmd.timeout = CMD_TIMEOUT;

    return mmc_cmd(card->host, &cmd);
}

int mmc_clr_write_prot(struct mmc_card *card, u32 addr)
{
    struct mmc_command cmd;

    if (!(card->csd.cmdclass & CCC_WRITE_PROT))
        return MMC_ERR_INVALID;

    cmd.opcode  = MMC_CMD_CLR_WRITE_PROT;
    cmd.rsptyp  = RESP_R1B;
    cmd.arg     = addr;
    cmd.retries = 3;
    cmd.timeout = CMD_TIMEOUT;

    return mmc_cmd(card->host, &cmd);
}

int mmc_send_write_prot(struct mmc_card *card, u32 wp_addr, u32 *wp_status)
{
    int err;
    struct mmc_command cmd;
    struct mmc_host *host = card->host;
    u8 *buf = (u8*)wp_status;

    if (!(card->csd.cmdclass & CCC_WRITE_PROT))
        return MMC_ERR_INVALID;

    cmd.opcode  = MMC_CMD_SEND_WRITE_PROT;
    cmd.rsptyp  = RESP_R1;
    cmd.arg     = wp_addr;
    cmd.retries = 3;
    cmd.timeout = CMD_TIMEOUT;

    msdc_set_blknum(host, 1);
    msdc_set_blklen(host, 4);
    msdc_set_timeout(host, 100000000, 0);
    err = mmc_cmd(host, &cmd);
    if (err != MMC_ERR_NONE)
        goto out;
    
    err = msdc_pio_read(host, (u32*)buf, 4);
    if (err != MMC_ERR_NONE)
        goto out;

out:
    return err;    
}

int mmc_send_write_prot_type(struct mmc_card *card, u32 wp_addr, u32 *wp_type)
{
    int err;
    struct mmc_command cmd;
    struct mmc_host *host = card->host;
    u8 *buf = (u8*)wp_type;

    if (!(card->csd.cmdclass & CCC_WRITE_PROT))
        return MMC_ERR_INVALID;

    cmd.opcode  = MMC_CMD_SEND_WRITE_PROT_TYPE;
    cmd.rsptyp  = RESP_R1;
    cmd.arg     = wp_addr;
    cmd.retries = 3;
    cmd.timeout = CMD_TIMEOUT;

    msdc_set_blknum(host, 1);
    msdc_set_blklen(host, 8);
    msdc_set_timeout(host, 100000000, 0);
    err = mmc_cmd(host, &cmd);
    if (err != MMC_ERR_NONE)
        goto out;
    
    err = msdc_pio_read(host, (u32*)buf, 8);
    if (err != MMC_ERR_NONE)
        goto out;

out:
    return err;    
}

int mmc_erase_start(struct mmc_card *card, u32 addr)
{
    struct mmc_command cmd;

    if (!(card->csd.cmdclass & CCC_ERASE)) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Card doesn't support Erase commands\n", card->host->id));
        return MMC_ERR_INVALID;
    }

    if (mmc_card_highcaps(card))
        addr /= MMC_BLOCK_SIZE; /* in sector unit */

    if (mmc_card_mmc(card)) {
        cmd.opcode = MMC_CMD_ERASE_GROUP_START;
    } else {
        cmd.opcode = MMC_CMD_ERASE_WR_BLK_START;
    }

    cmd.rsptyp  = RESP_R1;
    cmd.arg     = addr;
    cmd.retries = 3;
    cmd.timeout = CMD_TIMEOUT;

    return mmc_cmd(card->host, &cmd);
}

int mmc_erase_end(struct mmc_card *card, u32 addr)
{
    struct mmc_command cmd;

    if (!(card->csd.cmdclass & CCC_ERASE)) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Erase isn't supported\n", card->host->id));
        return MMC_ERR_INVALID;
    }

    if (mmc_card_highcaps(card))
        addr /= MMC_BLOCK_SIZE; /* in sector unit */

    if (mmc_card_mmc(card)) {
        cmd.opcode = MMC_CMD_ERASE_GROUP_END;
    } else {
        cmd.opcode = MMC_CMD_ERASE_WR_BLK_END;
    }

    cmd.rsptyp  = RESP_R1;
    cmd.arg     = addr;
    cmd.retries = 3;
    cmd.timeout = CMD_TIMEOUT;

    return mmc_cmd(card->host, &cmd);
}

int mmc_erase_block_start(struct mmc_card *card, u32 blkno)
{
    struct mmc_command cmd;

    if (!(card->csd.cmdclass & CCC_ERASE)) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Card doesn't support Erase commands\n", card->host->id));
        return MMC_ERR_INVALID;
    }

    if (mmc_card_mmc(card)) {
        cmd.opcode = MMC_CMD_ERASE_GROUP_START;
    } else {
        cmd.opcode = MMC_CMD_ERASE_WR_BLK_START;
    }

    cmd.rsptyp  = RESP_R1;
    cmd.arg     = blkno;
    cmd.retries = 3;
    cmd.timeout = CMD_TIMEOUT;

    return mmc_cmd(card->host, &cmd);
}

int mmc_erase_block_end(struct mmc_card *card, u32 blkno)
{
    struct mmc_command cmd;

    if (!(card->csd.cmdclass & CCC_ERASE)) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Erase isn't supported\n", card->host->id));
        return MMC_ERR_INVALID;
    }

    if (mmc_card_mmc(card)) {
        cmd.opcode = MMC_CMD_ERASE_GROUP_END;
    } else {
        cmd.opcode = MMC_CMD_ERASE_WR_BLK_END;
    }

    cmd.rsptyp  = RESP_R1;
    cmd.arg     = blkno;
    cmd.retries = 3;
    cmd.timeout = CMD_TIMEOUT;

    return mmc_cmd(card->host, &cmd);
}

int mmc_erase(struct mmc_card *card, u32 arg)
{
    int err;
    u32 status;
    struct mmc_command cmd;

    if (!(card->csd.cmdclass & CCC_ERASE)) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Erase isn't supported\n", card->host->id));
        return MMC_ERR_INVALID;
    }

    if (arg & MMC_ERASE_SECURE_REQ) {
        if (!(card->raw_ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT] & 
            EXT_CSD_SEC_FEATURE_ER_EN)) {
            return MMC_ERR_INVALID;
        }
    }
    if ((arg & MMC_ERASE_GC_REQ) || (arg & MMC_ERASE_TRIM)) {
        if (!(card->raw_ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT] & 
            EXT_CSD_SEC_FEATURE_GB_CL_EN)) {
            return MMC_ERR_INVALID;
        }        
    }

    cmd.opcode  = MMC_CMD_ERASE;
    cmd.rsptyp  = RESP_R1B;
    cmd.arg     = arg;
    cmd.retries = 3;
    cmd.timeout = CMD_TIMEOUT;

    err = mmc_cmd(card->host, &cmd);

    if (!err) {
        do {
            err = mmc_send_status(card->host, card, &status);
            if (err) break;
            mmc_dump_card_status(status);
            if (R1_STATUS(status) != 0) break;
        } while (R1_CURRENT_STATE(status) == 7);        
    }    
    return err;
}

#ifdef FEATURE_MMC_UHS1
int mmc_tune_timing(struct mmc_host *host, struct mmc_card *card)
{
    int err = MMC_ERR_NONE;

    if (mmc_card_sd(card) && mmc_card_uhs1(card) && !mmc_card_ddr(card)) {
        err = msdc_tune_uhs1(host, card);
    }
    return err;
}
#endif

u32 mmc_get_wpg_size(struct mmc_card *card)
{
    u32 size;
    u8 *ext_csd;

    if (mmc_card_mmc(card)) {
        ext_csd = &card->raw_ext_csd[0];

        if ((ext_csd[EXT_CSD_ERASE_GRP_DEF] & EXT_CSD_ERASE_GRP_DEF_EN)
            && (ext_csd[EXT_CSD_HC_WP_GPR_SIZE] > 0)) {
            size = 512 * 1024 * ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * 
                ext_csd[EXT_CSD_HC_WP_GPR_SIZE];
        } else {
            size = card->csd.write_prot_grpsz;
        }
    } else {
        if (card->csd.write_prot_grp) {
            /* SDSC could support write protect group */
            size = (card->csd.write_prot_grpsz + 1) * (1 << card->csd.write_blkbits);
        } else {
            /* SDHC and SDXC don't support write protect group */
            size = 0;
        }
    }

    return size;
}

void mmc_set_clock(struct mmc_host *host, int state, unsigned int hz)
{
    if (hz >= host->f_max) {
        hz = host->f_max;
    } else if (hz < host->f_min) {
        hz = host->f_min;
    }
    msdc_config_clock(host, state, hz);
}

int mmc_set_ext_csd(struct mmc_card *card, uint8 addr, uint8 value)
{
    int err;
    u8 *ext_csd;

    /* can't write */
    if (192 <= addr || !card || !mmc_card_mmc(card))
        return MMC_ERR_INVALID;

    err = mmc_switch(card->host, card, EXT_CSD_CMD_SET_NORMAL, addr, value);

    if (err == MMC_ERR_NONE) {
        err = mmc_read_ext_csd(card->host, card);
        if (err == MMC_ERR_NONE) {
            ext_csd = &card->raw_ext_csd[0];
            if (ext_csd[addr] != value)
                err = MMC_ERR_FAILED;
        }
    }

    return err;
}

int mmc_set_card_detect(struct mmc_host *host, struct mmc_card *card, int connect)
{
    int err;
    struct mmc_command cmd;

    cmd.opcode  = SD_ACMD_SET_CLR_CD;
    cmd.arg     = connect;
    cmd.rsptyp  = RESP_R1; /* CHECKME */
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    err = mmc_app_cmd(host, &cmd, card->rca, CMD_RETRIES);
    return err;
}

int mmc_set_blk_length(struct mmc_host *host, u32 blklen)
{
    int err;
    struct mmc_command cmd;

    /* set block len */
    cmd.opcode  = MMC_CMD_SET_BLOCKLEN;
    cmd.rsptyp  = RESP_R1;
    cmd.arg     = blklen;
    cmd.retries = 3;
    cmd.timeout = CMD_TIMEOUT;
    err = mmc_cmd(host, &cmd);

    if (err == MMC_ERR_NONE)
        msdc_set_blklen(host, blklen);

    return err;
}

int mmc_set_blk_count(struct mmc_host *host, u32 blkcnt)
{
    int err;
    struct mmc_command cmd;

    /* set block count */
    cmd.opcode  = MMC_CMD_SET_BLOCK_COUNT;
    cmd.rsptyp  = RESP_R1;
    cmd.arg     = blkcnt; /* bit31 is for reliable write request */
    cmd.retries = 3;
    cmd.timeout = CMD_TIMEOUT;
    err = mmc_cmd(host, &cmd);

    return err;
}

int mmc_set_bus_width(struct mmc_host *host, struct mmc_card *card, int width)
{
    int err = MMC_ERR_NONE;
    u32 arg = 0;
    struct mmc_command cmd;

    if (mmc_card_sd(card)) {
        if (width == HOST_BUS_WIDTH_8) {
            arg = SD_BUS_WIDTH_4;
            width = HOST_BUS_WIDTH_4;
        }        

        if ((width == HOST_BUS_WIDTH_4) && (host->caps & MMC_CAP_4_BIT_DATA)) {
            arg = SD_BUS_WIDTH_4;
        } else {
            arg = SD_BUS_WIDTH_1;
            width = HOST_BUS_WIDTH_1;
        }

        cmd.opcode  = SD_ACMD_SET_BUSWIDTH;
        cmd.arg     = arg;
        cmd.rsptyp  = RESP_R1;
        cmd.retries = CMD_RETRIES;
        cmd.timeout = CMD_TIMEOUT;

        err = mmc_app_cmd(host, &cmd, card->rca, 0);
        if (err != MMC_ERR_NONE)
            goto out;

        msdc_config_bus(host, width);
    } else if (mmc_card_mmc(card)) {
        if (card->csd.mmca_vsn < CSD_SPEC_VER_4)
            goto out;

        if (width == HOST_BUS_WIDTH_8) {
            if (host->caps & MMC_CAP_8_BIT_DATA) {
                arg = ((host->caps & MMC_CAP_DDR) && card->ext_csd.ddr_support) ? 
                    EXT_CSD_BUS_WIDTH_8_DDR : EXT_CSD_BUS_WIDTH_8;
            } else {
                width = HOST_BUS_WIDTH_4;
            }
        } 
        if (width == HOST_BUS_WIDTH_4) {
            if (host->caps & MMC_CAP_4_BIT_DATA) {
                arg = ((host->caps & MMC_CAP_DDR) && card->ext_csd.ddr_support) ? 
                    EXT_CSD_BUS_WIDTH_4_DDR : EXT_CSD_BUS_WIDTH_4; 
            } else {
                width = HOST_BUS_WIDTH_1;
            }
        }        
        if (width == HOST_BUS_WIDTH_1)
            arg = EXT_CSD_BUS_WIDTH_1;

        err = mmc_switch(host, card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BUS_WIDTH, arg);
        if (err != MMC_ERR_NONE) {
            MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] Switch to bus width(%d) failed\n", host->id, arg));
            goto out;
        }
        if (arg == EXT_CSD_BUS_WIDTH_8_DDR || arg == EXT_CSD_BUS_WIDTH_4_DDR) {
            mmc_card_set_ddr(card);
        } else {
            mmc_card_clr_ddr(card);
        }
        mmc_set_clock(host, card->state, host->sclk);

        msdc_config_bus(host, width);
    } else {
        MSDC_BUGON(1); /* card is not recognized */
    }
out:    
    return err;
}

int mmc_set_erase_grp_def(struct mmc_card *card, int enable)
{
    int err = MMC_ERR_FAILED;

    if (mmc_card_sd(card) || !mmc_card_highcaps(card))
        goto out;

    if (card->csd.mmca_vsn < CSD_SPEC_VER_4)
        goto out;

    err = mmc_set_ext_csd(card, EXT_CSD_ERASE_GRP_DEF, 
        EXT_CSD_ERASE_GRP_DEF_EN & enable);

out:
    return err;
}

int mmc_set_gp_size(struct mmc_card *card, u8 id, u32 size)
{
    int i;
    int err = MMC_ERR_FAILED;
    u8 gp[] = { EXT_CSD_GP1_SIZE_MULT, EXT_CSD_GP2_SIZE_MULT, 
        EXT_CSD_GP3_SIZE_MULT, EXT_CSD_GP4_SIZE_MULT };
    u8 arg;
    u8 *ext_csd = &card->raw_ext_csd[0]; 

    if (mmc_card_sd(card) || !mmc_card_highcaps(card))
        goto out;

    if (card->csd.mmca_vsn < CSD_SPEC_VER_4)
        goto out;

    id--;
    size /= 512 * 1024;
    size /= (ext_csd[EXT_CSD_HC_WP_GPR_SIZE] * ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]);

    /* 143-144: GP_SIZE_MULT_X_0-GP_SIZE_MULT_X_2 */
    for (i = 0; i < 3; i++) {
        arg  = (u8)(size & 0xFF);
        size = size >> 8;
        err = mmc_set_ext_csd(card, gp[id] + i, arg);
        if (err)
            goto out;
    }

out:
    return err;
}

int mmc_set_enh_size(struct mmc_card *card, u32 size)
{
    int i;
    int err = MMC_ERR_FAILED;
    u8 arg;
    u8 *ext_csd = &card->raw_ext_csd[0]; 

    if (mmc_card_sd(card) || !mmc_card_highcaps(card))
        goto out;

    if (card->csd.mmca_vsn < CSD_SPEC_VER_4)
        goto out;

    /* need to set ERASE_GRP_DEF first?? */
    if (0 == (card->raw_ext_csd[EXT_CSD_ERASE_GRP_DEF] & EXT_CSD_ERASE_GRP_DEF_EN))
        goto out;

    size /= (512 * 1024);
    size /= (ext_csd[EXT_CSD_HC_WP_GPR_SIZE] * ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]);

    /* 140-142: ENH_SIZE_MULT0-ENH_SIZE_MULT2 */
    for (i = 0; i < 3; i++) {
        arg  = (u8)(size & 0xFF);
        size = size >> 8;
        err = mmc_set_ext_csd(card, EXT_CSD_ENH_SIZE_MULT + i, arg);
        if (err)
            goto out;
    }

out:
    return err;
}

int mmc_set_enh_start_addr(struct mmc_card *card, u32 addr)
{
    int i;
    int err = MMC_ERR_FAILED;
    u8 arg;

    if (mmc_card_sd(card))
        goto out;

    if (card->csd.mmca_vsn < CSD_SPEC_VER_4)
        goto out;

    /* need to set ERASE_GRP_DEF first?? */
    if (0 == (card->raw_ext_csd[EXT_CSD_ERASE_GRP_DEF] & EXT_CSD_ERASE_GRP_DEF_EN))
        goto out;

    /* start address would be round to protect group aligned. */
    if (mmc_card_highcaps(card))
        addr = addr / 512; /* in sector unit. otherwise in byte unit */

    /* 136-139: ENH_START_ADDR0-ENH_START_ADDR3 */
    for (i = 0; i < 4; i++) {
        arg  = (u8)(addr & 0xFF);
        addr = addr >> 8;
        err = mmc_set_ext_csd(card, EXT_CSD_ENH_START_ADDR + i, arg);
        if (err)
            goto out;
    }

out:
    return err;
}

int mmc_set_boot_bus(struct mmc_card *card, u8 rst_bwidth, u8 mode, u8 bwidth)
{
    int err = MMC_ERR_FAILED;
    u8 arg;

    if (mmc_card_sd(card))
        goto out;

    if (card->csd.mmca_vsn < CSD_SPEC_VER_4)
        goto out;

    arg = mode | rst_bwidth | bwidth;

    err = mmc_set_ext_csd(card, EXT_CSD_BOOT_BUS_WIDTH, arg);

out:
    return err;
}

int mmc_set_part_config(struct mmc_card *card, u8 cfg)
{
    int err = MMC_ERR_FAILED;

    if (mmc_card_sd(card))
        goto out;

    if (card->csd.mmca_vsn < CSD_SPEC_VER_4)
        goto out;

    err = mmc_set_ext_csd(card, EXT_CSD_PART_CFG, cfg);

out:
    return err;
}

int mmc_set_part_attr(struct mmc_card *card, u8 attr)
{
    int err = MMC_ERR_FAILED;

    if (mmc_card_sd(card))
        goto out;

    if (card->csd.mmca_vsn < CSD_SPEC_VER_4)
        goto out;

    if (!card->ext_csd.enh_attr_en) {
        err = MMC_ERR_INVALID;
        goto out;
    }

    attr &= 0x1F;
    attr |= (card->raw_ext_csd[EXT_CSD_PART_ATTR] & 0x1F);

    err = mmc_set_ext_csd(card, EXT_CSD_PART_ATTR, attr);

out:
    return err;
}

int mmc_set_part_compl(struct mmc_card *card)
{
    int err = MMC_ERR_FAILED;

    if (mmc_card_sd(card))
        goto out;

    if (card->csd.mmca_vsn < CSD_SPEC_VER_4)
        goto out;

    err = mmc_set_ext_csd(card, EXT_CSD_PART_SET_COMPL, 
        EXT_CSD_PART_SET_COMPL_BIT);

out:
    return err;
}

#ifdef FEATURE_MMC_BOOT_MODE
int mmc_set_reset_func(struct mmc_card *card, u8 enable)
{
    int err = MMC_ERR_FAILED;
    u8 *ext_csd = &card->raw_ext_csd[0];

    if (mmc_card_sd(card))
        goto out;

    if (card->csd.mmca_vsn < CSD_SPEC_VER_4)
        goto out;

    if (ext_csd[EXT_CSD_RST_N_FUNC] == 0) {
        err = mmc_set_ext_csd(card, EXT_CSD_RST_N_FUNC, enable);
    }
out:
    return err;
}

int mmc_boot_config(struct mmc_card *card, u8 acken, u8 enpart, u8 buswidth, u8 busmode)
{
    int err = MMC_ERR_FAILED;
    u8 val;
    u8 rst_bwidth = 0;
    u8 *ext_csd = &card->raw_ext_csd[0];

    if (mmc_card_sd(card) || card->csd.mmca_vsn < CSD_SPEC_VER_4 || 
        !card->ext_csd.boot_info || card->ext_csd.rev < 3)
        goto out;

    if (card->ext_csd.rev > 3 && !card->ext_csd.part_en)
        goto out;

    /* configure boot partition */
    val = acken | enpart | (ext_csd[EXT_CSD_PART_CFG] & 0x7);
    err = mmc_set_part_config(card, val);
    if (err != MMC_ERR_NONE)
        goto out;

    /* update ext_csd information */
    ext_csd[EXT_CSD_PART_CFG] = val;

    /* configure boot bus mode and width */
    rst_bwidth = (buswidth != EXT_CSD_BOOT_BUS_WIDTH_1 ? 1 : 0) << 2;
    MSDC_TRC_PRINT(MSDC_INFO,("%s =====Set boot Bus Width<%d>=======\n",buswidth));
    MSDC_TRC_PRINT(MSDC_INFO,("%s =====Set boot Bus mode<%d>=======\n",busmode));
    err = mmc_set_boot_bus(card, rst_bwidth, busmode, buswidth);       

out:

    return err;
}

int mmc_part_read(struct mmc_card *card, u8 partno, unsigned long blknr, u32 blkcnt, unsigned long *dst)
{
    int err = MMC_ERR_FAILED;
    u8 val;
    u8 *ext_csd = &card->raw_ext_csd[0];
    struct mmc_host *host = card->host;

    if (mmc_card_sd(card) || card->csd.mmca_vsn < CSD_SPEC_VER_4 || 
        !card->ext_csd.boot_info || card->ext_csd.rev < 3)
        goto out;

    if (card->ext_csd.rev > 3 && !card->ext_csd.part_en)
        goto out; 

    /* configure to specified partition */
    val = (ext_csd[EXT_CSD_PART_CFG] & ~0x7) | (partno & 0x7);
    err = mmc_set_part_config(card, val);
    if (err != MMC_ERR_NONE)
        goto out;

    /* write block to this partition */
    err = mmc_block_read(host->id, blknr, blkcnt, dst);

out:
    /* configure to user partition */
    val = (ext_csd[EXT_CSD_PART_CFG] & ~0x7) | EXT_CSD_PART_CFG_DEFT_PART;
    mmc_set_part_config(card, val);

    return err;
}

int mmc_part_write(struct mmc_card *card, u8 partno, unsigned long blknr, u32 blkcnt, unsigned long *src)
{
    int err = MMC_ERR_FAILED;
    u8 val;
    u8 *ext_csd = &card->raw_ext_csd[0];
    struct mmc_host *host = card->host;

    if (mmc_card_sd(card) || card->csd.mmca_vsn < CSD_SPEC_VER_4 || 
        !card->ext_csd.boot_info || card->ext_csd.rev < 3)
        goto out;

    if (card->ext_csd.rev > 3 && !card->ext_csd.part_en)
        goto out; 

    /* configure to specified partition */
    val = (ext_csd[EXT_CSD_PART_CFG] & ~0x7) | (partno & 0x7);
    err = mmc_set_part_config(card, val);
    if (err != MMC_ERR_NONE)
        goto out;  

    /* write block to this partition */
    err = mmc_block_write(host->id, blknr, blkcnt, src);

out:
    /* configure to user partition */
    val = (ext_csd[EXT_CSD_PART_CFG] & ~0x7) | EXT_CSD_PART_CFG_DEFT_PART;
    mmc_set_part_config(card, val);

    return err;
}

int mmc_set_boot_prot(struct mmc_card *card, u8 prot)
{
    int err = MMC_ERR_FAILED;

    if (mmc_card_sd(card))
        goto out;

    MSDC_BUGON(card->csd.mmca_vsn >= CSD_SPEC_VER_4);
    if (card->csd.mmca_vsn < CSD_SPEC_VER_4)
        goto out;

    err = mmc_switch(card->host, card, EXT_CSD_CMD_SET_NORMAL, 
        EXT_CSD_BOOT_CONFIG_PROT, prot);

out:
    return err;
}

int mmc_set_boot_wp(struct mmc_card *card, u8 wp)
{
    int err = MMC_ERR_FAILED;

    if (mmc_card_sd(card))
        goto out;

    MSDC_BUGON(card->csd.mmca_vsn >= CSD_SPEC_VER_4);
    if (card->csd.mmca_vsn < CSD_SPEC_VER_4)
        goto out;

    err = mmc_switch(card->host, card, EXT_CSD_CMD_SET_NORMAL, 
        EXT_CSD_BOOT_WP, wp);

out:
    return err;
}

int mmc_set_user_wp(struct mmc_card *card, u8 wp)
{
    int err = MMC_ERR_FAILED;

    if (mmc_card_sd(card))
        goto out;

    MSDC_BUGON(card->csd.mmca_vsn >= CSD_SPEC_VER_4);
    if (card->csd.mmca_vsn < CSD_SPEC_VER_4)
        goto out;

    err = mmc_switch(card->host, card, EXT_CSD_CMD_SET_NORMAL, 
        EXT_CSD_USR_WP, wp);

out:
    return err;
}
#endif

int mmc_dev_bread(struct mmc_card *card, unsigned long blknr, u32 blkcnt, u8 *dst)
{
    struct mmc_host *host = card->host;
    u32 blksz = host->blklen;
    int tune = 0;
    int retry = 3;
    int err;
    unsigned long src;

    src = mmc_card_highcaps(card) ? blknr : blknr * blksz;

    do {
        mmc_prof_start();
        if (!tune) {
            err = host->blk_read(host, (uchar *)dst, src, blkcnt);
        } else {
            #ifdef FEATURE_MMC_RD_TUNING
            err = msdc_tune_bread(host, (uchar *)dst, src, blkcnt);
            #endif
            if (err && (host->sclk > (host->f_max >> 4)))
                mmc_set_clock(host, card->state, host->sclk >> 1);            
        }
        mmc_prof_stop();
        if (err == MMC_ERR_NONE) {
            mmc_prof_update(mmc_prof_read, blkcnt, mmc_prof_handle(host->id));
            break;
        }
        
        if (err == MMC_ERR_BADCRC || err == MMC_ERR_ACMD_RSPCRC || err == MMC_ERR_CMD_RSPCRC){
            tune = 1;retry++;}
		else if(err == MMC_ERR_READTUNEFAIL || err == MMC_ERR_CMDTUNEFAIL){
			MSDC_TRC_PRINT(MSDC_TUNING,("[SD%d] Fail to tuning,%s",host->id,
				(err == MMC_ERR_CMDTUNEFAIL)?"cmd tune failed!\n":"read tune failed!\n"));
			break;
			}
    } while (retry--);

    return err;
}

static int mmc_dev_bwrite(struct mmc_card *card, unsigned long blknr, u32 blkcnt, u8 *src)
{
    struct mmc_host *host = card->host;
    u32 blksz = host->blklen;
    u32 status;
    int tune = 0;
    int retry = 3;
    int err;
    unsigned long dst;

    dst = mmc_card_highcaps(card) ? blknr : blknr * blksz;

    do {
        mmc_prof_start();
        if (!tune) {
            err = host->blk_write(host, dst, (uchar *)src, blkcnt);
        } else {
            #ifdef FEATURE_MMC_WR_TUNING
            err = msdc_tune_bwrite(host, dst, (uchar *)src, blkcnt);
            #endif
            if (err && (host->sclk > (host->f_max >> 4)))
                mmc_set_clock(host, card->state, host->sclk >> 1);            
        }
        if (err == MMC_ERR_NONE) {
            do {
                err = mmc_send_status(host, card, &status);
                if (err) {
                    MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] Fail to send status %d\n", host->id, err));
                    break;
                }
            } while (!(status & R1_READY_FOR_DATA) ||
                (R1_CURRENT_STATE(status) == 7));
            mmc_prof_stop();
            mmc_prof_update(mmc_prof_write, blkcnt, mmc_prof_handle(host->id));
            MSDC_DBG_PRINT(MSDC_STACK,("[SD%d] Write %d bytes (DONE)\n",host->id, blkcnt * blksz));
            break;
        }

        if (err == MMC_ERR_BADCRC || err == MMC_ERR_ACMD_RSPCRC || err == MMC_ERR_CMD_RSPCRC){
            tune = 1;retry++;}
		else if(err == MMC_ERR_WRITETUNEFAIL || err == MMC_ERR_CMDTUNEFAIL){
			MSDC_TRC_PRINT(MSDC_TUNING,("[SD%d] Fail to tuning,%s",host->id,
				(err == MMC_ERR_CMDTUNEFAIL)?"cmd tune failed!\n":"write tune failed!\n"));
			break;
			}
    } while (retry--);

    return err;
}

int mmc_block_read(int dev_num, unsigned long blknr, u32 blkcnt, unsigned long *dst)
{
    struct mmc_host *host = mmc_get_host(dev_num);
    struct mmc_card *card = mmc_get_card(dev_num);
    u32 blksz    = host->blklen;
    u32 maxblks  = host->max_phys_segs;
    //u32 xfercnt  = blkcnt / maxblks;
    //u32 leftblks = blkcnt % maxblks;
    u32 leftblks;
    u8 *buf = (u8*)dst;
    int ret;
    u32 ori_blk_cnt = blkcnt;
    //printf("blknr = %d, blkcnt = %d\n", blknr, blkcnt);
    
    if (!blkcnt)
        return MMC_ERR_NONE;

    if (blknr * (blksz / MMC_BLOCK_SIZE) > card->nblks) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Out of block range: blknr(%ld) > sd_blknr(%d)\n",
            host->id, blknr, card->nblks));
        return MMC_ERR_INVALID;
    }

    do {
        leftblks=((blkcnt> maxblks) ? maxblks : blkcnt);
        ret = mmc_dev_bread(card, (unsigned long)blknr, leftblks, buf);
        if (ret) 
            return ret;
        blknr += leftblks;
        buf   += maxblks * blksz;
        blkcnt -= leftblks;
    } while ( blkcnt );

    
    //TODO : check this
    //return ret;
    return ori_blk_cnt;
    
}

int mmc_block_write(int dev_num, unsigned long blknr, u32 blkcnt, unsigned long *src)
{
    struct mmc_host *host = mmc_get_host(dev_num);
    struct mmc_card *card = mmc_get_card(dev_num);
    u32 blksz    = host->blklen;
    u32 maxblks  = host->max_phys_segs;
    //u32 xfercnt  = blkcnt / maxblks;
    //u32 leftblks = blkcnt % maxblks;
    u32 leftblks;
    u8 *buf = (u8*)src;
    int ret;
    int ori_blk_cnt = blkcnt;
    if (!blkcnt)
        return MMC_ERR_NONE;

    if (blknr * (blksz / MMC_BLOCK_SIZE) > card->nblks) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Out of block range: blknr(%ld) > sd_blknr(%d)\n",
            host->id, blknr, card->nblks));
        return MMC_ERR_INVALID;
    }

    do {
        leftblks=((blkcnt> maxblks) ? maxblks : blkcnt);
        ret = mmc_dev_bwrite(card, (unsigned long)blknr, leftblks, buf);
        if (ret) 
            return ret;
        blknr += leftblks;
        buf   += maxblks * blksz;
        blkcnt -= leftblks;
    } while ( blkcnt );

    //return ret;
    //TODO : check this
    return ori_blk_cnt;

}

int mmc_block_erase(int dev_num, unsigned long blknr_start, unsigned long cnt)
{

#if 1
     unsigned char buf0[512];
     unsigned long i = 0;
     memset(buf0, 0, 512);
     for(i = 0 ; i < cnt ; i++) {
        mmc_block_write(dev_num,  blknr_start++ , 1 , (long unsigned int *)&buf0[0]);
     }
#else    
    int ret = 0;
    struct mmc_host *host = mmc_get_host(dev_num);
    struct mmc_card *card = mmc_get_card(dev_num);
    
    ret = mmc_erase_block_start(card, blknr_start);
    if(!ret) return -1;
    ret = mmc_erase_block_end(card, blknr_start + cnt - 1);
    if(!ret) return -1;
    ret = mmc_erase(card, 0);
    if(!ret) return -1;
#endif

    return cnt;
}

void mmc_stuff_buff(u8* buf)
{
	memset(buf,0,512);
	buf[0] = 0x10;
	buf[1] = 0x06;
	buf[2] = 0x01;
	buf[3] = 0xF0;
	buf[11]= 0xAA;
	buf[12]= 0xA9;
	buf[13]= 0x87;
	buf[14]= 0x74;
	buf[15]= 0x3C;
	buf[16]= 0x71;
	buf[17]= 0xFB;
	buf[18]= 0xD4;
}

int msdc_send_cmd(struct mmc_host *host, struct mmc_command *cmd);
int msdc_wait_rsp(struct mmc_host *host, struct mmc_command *cmd);
int mmc_get_sandisk_fwid(int id, u8* buf)
{
	struct mmc_host *host;
	struct mmc_card *card;
	struct mmc_command stop;
	int err = MMC_ERR_NONE;
	u32 status;
	u32 state = 0;
	host = &sd_host[id];
    card = &sd_card[id];
	while (state != 4) {
        err = mmc_send_status(host, card, &status);
        if (err) {
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Fail to send status %d\n", host->id, err));
            return err;
        }
		state = R1_CURRENT_STATE(status);
            MSDC_TRC_PRINT(MSDC_DREG,("check card state<%d>\n", state));
			if (state == 5 || state == 6) {
                MSDC_TRC_PRINT(MSDC_STACK,("state<%d> need cmd12 to stop\n", state));
				stop.opcode  = MMC_CMD_STOP_TRANSMISSION;
       			stop.rsptyp  = RESP_R1B;
        		stop.arg     = 0;
        		stop.retries = CMD_RETRIES;
        		stop.timeout = CMD_TIMEOUT;
        		msdc_send_cmd(host, &stop);
        		msdc_wait_rsp(host, &stop); // don't tuning
			} else if (state == 7) {  // busy in programing 		
                MSDC_TRC_PRINT(MSDC_STACK,("state<%d> card is busy\n", state));
				mdelay(100);
			} else if (state != 4) {
                MSDC_TRC_PRINT(MSDC_STACK,("state<%d> ??? \n", state));
				return MMC_ERR_INVALID;		
			}
		}	
	mmc_stuff_buff(buf);
#if MSDC_USE_DMA_MODE
	err = msdc_dma_send_sandisk_fwid(host, buf,MMC_CMD50,1);
	if (err) {
                MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] Fail to send(CMD50) sandisk fwid %d\n", host->id, err));
				return err;
			}

	err = msdc_dma_send_sandisk_fwid(host, buf,MMC_CMD21,1);
	if (err) {
                MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] Fail to get(CMD21) sandisk fwid %d\n", host->id, err));
				return err;
			}
	
#else
	err = msdc_pio_send_sandisk_fwid(host, buf);
	if (err) {
                MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] Fail to send(CMD50) sandisk fwid %d\n", host->id, err));
				return err;
			}

	err = msdc_pio_get_sandisk_fwid(host, buf);
	if (err) {
                MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] Fail to get(CMD21) sandisk fwid %d\n", host->id, err));
				return err;
			}
#endif
	return err;
}

#ifdef FEATURE_MMC_BOOT_MODE
void mmc_boot_reset(struct mmc_host *host, int reset)
{
    msdc_emmc_boot_reset(host, reset);
}

int mmc_boot_up(struct mmc_host *host, int mode, int ackdis, u32 *to, u32 size)
{
    int err;

    ERR_EXIT(msdc_emmc_boot_start(host, host->sclk, 0, mode, ackdis), err, MMC_ERR_NONE);
    ERR_EXIT(msdc_emmc_boot_read(host, size, to), err, MMC_ERR_NONE);

exit:
    msdc_emmc_boot_stop(host, mode);

    return err;
}
#endif
void mmc_dump_buff(const char* szTag,const char * pbbuffer, unsigned int u4buffercb, unsigned int u4column, unsigned int u4seekpos)
{
	int  i4result=1;
	unsigned int u4cnt=0;

	u4column = (u4column > 8) ? u4column : 8;
	if(u4buffercb)
	{
		do
		{
			i4result=0;
			if((u4cnt%u4column)==0)
			{
				if(szTag==NULL)
				{
					MSDC_RAW_PRINT(MSDC_INIT,("0x%x: ",u4seekpos+u4cnt));
				}
				else
				{
					MSDC_RAW_PRINT(MSDC_INIT,("%s[%x]: ",szTag,u4seekpos+u4cnt));
				}
			}
			MSDC_RAW_PRINT(MSDC_INIT,("0x%x ,", pbbuffer[u4cnt]));
			u4cnt ++;
			if((u4cnt%u4column)==0)
			{
				MSDC_RAW_PRINT(MSDC_INIT,("\n"));
				i4result=1;
			}
		}
		while(u4cnt < u4buffercb);

		if(!i4result)
		{
			MSDC_RAW_PRINT(MSDC_INIT,("\n"));
		}
	}
}

int mmc_init_mem_card(struct mmc_host *host, struct mmc_card *card, u32 ocr)
{
    int err;
#ifndef KEEP_SLIENT_BUILD
    int id = host->id;
#endif

#ifdef FEATURE_MMC_UHS1
    int s18a = 0;
#endif
    /*
     * Sanity check the voltages that the card claims to
     * support.
     */
    if (ocr & 0x7F) {
        MSDC_TRC_PRINT(MSDC_STACK,("card claims to support voltages"
			"below the defined range. These will be ignored.\n"));
        ocr &= ~0x7F;
    }

    ocr = host->ocr = mmc_select_voltage(host, ocr);

    /*
     * Can we support the voltage(s) of the card(s)?
     */
    if (!host->ocr) {
        err = MMC_ERR_FAILED;
        goto out;
    }

    mmc_go_idle(host);

    /* send interface condition */
    if (mmc_card_sd(card))
        err = mmc_send_if_cond(host, ocr);

    /* host support HCS[30] */
    ocr |= (1 << 30);

#ifdef FEATURE_MMC_UHS1
    if (!err) {
        /* host support S18A[24] and XPC[28]=1 to support speed class */
        if (host->caps & MMC_CAP_SD_UHS1)
            ocr |= ((1 << 28) | (1 << 24));
    }
#endif

    /* send operation condition */
    if (mmc_card_sd(card)) {
        err = mmc_send_app_op_cond(host, ocr, &card->ocr);
    } else {
        /* The extra bit indicates that we support high capacity */
        err = mmc_send_op_cond(host, ocr, &card->ocr);
    }

    if (err != MMC_ERR_NONE) {
        MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] Fail in SEND_OP_COND cmd\n", id));
        goto out;
    }

    /* set hcs bit if a high-capacity card */
    card->state |= ((card->ocr >> 30) & 0x1) ? MMC_STATE_HIGHCAPS : 0;
#ifdef FEATURE_MMC_UHS1    
    s18a = (card->ocr >> 24) & 0x1;
    MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] ocr = 0x%X, s18a = %d \n", id, ocr, s18a)));

    /* S18A support by card. switch to 1.8V signal */
    if (s18a) {
        err = mmc_switch_volt(host, card);        
        if (err != MMC_ERR_NONE) {
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Fail in SWITCH_VOLT cmd\n", id));
            goto out;
        }
    }
#endif    
    /* send cid */
    err = mmc_all_send_cid(host, card->raw_cid);

    if (err != MMC_ERR_NONE) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Fail in SEND_CID cmd\n", id));
        goto out;
    }
    mmc_decode_cid(card);

    if (mmc_card_mmc(card))
        card->rca = 0x1; /* assign a rca */

    /* set/send rca */
    err = mmc_send_relative_addr(host, card, &card->rca);
    if (err != MMC_ERR_NONE) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Fail in SEND_RCA cmd\n", id));
        goto out;
    }

    /* send csd */
    err = mmc_read_csds(host, card);
    if (err != MMC_ERR_NONE) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Fail in SEND_CSD cmd\n", id));
        goto out;        
    }

    /* decode csd */
    err = mmc_decode_csd(card);
    if (err != MMC_ERR_NONE) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Fail in decode csd\n", id));
        goto out;        
    }

    /* select this card */
    err = mmc_select_card(host, card);
    if (err != MMC_ERR_NONE) {
        MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Fail in select card cmd\n", id));
        goto out;
    }

    if (mmc_card_sd(card)) {
        #ifndef FEATURE_MMC_STRIPPED
        /* send scr */
        err = mmc_read_scrs(host, card);
        if (err != MMC_ERR_NONE) {
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Fail in SEND_SCR cmd\n", id));
            goto out;
        }
        #endif

        if ((card->csd.cmdclass & CCC_SWITCH) && 
            (mmc_read_switch(host, card) == MMC_ERR_NONE)) {
            do {
                #ifdef FEATURE_MMC_UHS1
                if (s18a && (host->caps & MMC_CAP_SD_UHS1)) {
                    /* TODO: Switch driver strength first then current limit
                     *       and access mode */
                    unsigned int freq, uhs_mode, drv_type, max_curr;
                    freq = min(host->f_max, card->sw_caps.hs_max_dtr);

                    if (freq > 100000000) {
                        uhs_mode = MMC_SWITCH_MODE_SDR104;
                    } else if (freq <= 100000000 && freq > 50000000) {
                        if (card->sw_caps.ddr && (host->caps & MMC_CAP_DDR)) {
                            uhs_mode = MMC_SWITCH_MODE_DDR50;
                        } else {
                            uhs_mode = MMC_SWITCH_MODE_SDR50;
                        }
                    } else if (freq <= 50000000 && freq > 25000000) {
                        uhs_mode = MMC_SWITCH_MODE_SDR25;
                    } else {
                        uhs_mode = MMC_SWITCH_MODE_SDR12;
                    }
                    drv_type = MMC_SWITCH_MODE_DRV_TYPE_B;
                    max_curr = MMC_SWITCH_MODE_CL_200MA;

                    if (mmc_switch_drv_type(host, card, drv_type) == MMC_ERR_NONE &&
                        mmc_switch_max_cur(host, card, max_curr) == MMC_ERR_NONE && 
                        mmc_switch_uhs1(host, card, uhs_mode) == MMC_ERR_NONE) {
                        break;
                    } else {
                        mmc_switch_drv_type(host, card, MMC_SWITCH_MODE_DRV_TYPE_B);
                        mmc_switch_max_cur(host, card, MMC_SWITCH_MODE_CL_200MA);                        
                    }
                }
                #endif
                if (host->caps & MMC_CAP_SD_HIGHSPEED) {
                    mmc_switch_hs(host, card);
                    break;
                }
            } while(0);
        }

        /* set bus width */
        mmc_set_bus_width(host, card, HOST_BUS_WIDTH_4); 

        /* compute bus speed. */
        card->maxhz = (unsigned int)-1;

        if (mmc_card_highspeed(card) || mmc_card_uhs1(card)) {
            if (card->maxhz > card->sw_caps.hs_max_dtr)
                card->maxhz = card->sw_caps.hs_max_dtr;
        } else if (card->maxhz > card->csd.max_dtr) {
            card->maxhz = card->csd.max_dtr;
        }
    } else {

        /* send ext csd */
        err = mmc_read_ext_csd(host, card);
        if (err != MMC_ERR_NONE) {
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Fail in SEND_EXT_CSD cmd\n", id));
            goto out;        
        }

        /* activate high speed (if supported) */
        if ((card->ext_csd.hs_max_dtr != 0) && (host->caps & MMC_CAP_MMC_HIGHSPEED)) {
            err = mmc_switch(host, card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1);

            if (err == MMC_ERR_NONE) {
                MSDC_TRC_PRINT(MSDC_INFO,("[SD%d] Switched to High-Speed mode!\n", host->id));
				
                mmc_card_set_highspeed(card);
            }
        }

        /* set bus width */
        mmc_set_bus_width(host, card, HOST_BUS_WIDTH_8);

        /* compute bus speed. */
        card->maxhz = (unsigned int)-1;

        if (mmc_card_highspeed(card)) {
            if (card->maxhz > card->ext_csd.hs_max_dtr)
                card->maxhz = card->ext_csd.hs_max_dtr;
        } else if (card->maxhz > card->csd.max_dtr) {
            card->maxhz = card->csd.max_dtr;
        }
    }

    /* set block len. note that cmd16 is illegal while mmc card is in ddr mode */
    if (!(mmc_card_mmc(card) && mmc_card_ddr(card))) {
        err = mmc_set_blk_length(host, MMC_BLOCK_SIZE);
        if (err != MMC_ERR_NONE) {
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Fail in set blklen cmd, card state=0x%x\n", id, card->state));
            goto out;
        }
    }

    /* set clear card detect */
    if (mmc_card_sd(card))
        mmc_set_card_detect(host, card, 0);

    if (!mmc_card_sd(card) && mmc_card_blockaddr(card)) {
        /* The EXT_CSD sector count is in number or 512 byte sectors. */
        card->blklen = MMC_BLOCK_SIZE;
        card->nblks  = card->ext_csd.sectors;
    } else {
        /* The CSD capacity field is in units of read_blkbits.
         * set_capacity takes units of 512 bytes.
         */
        card->blklen = MMC_BLOCK_SIZE;
        card->nblks  = card->csd.capacity << (card->csd.read_blkbits - 9);
    }

    MSDC_TRC_PRINT(MSDC_INIT,("[SD%d] Size: %d MB, Max.Speed: %d kHz, blklen(%d), nblks(%d), ro(%d)\n",
        id, ((card->nblks / 1024) * card->blklen) / 1024 , card->maxhz / 1000,
        card->blklen, card->nblks, mmc_card_readonly(card)));
	mmc_dump_buff(" 	CID", (const char *)card->raw_cid, sizeof(card->raw_cid), 16, 0);

    card->ready = 1;

    MSDC_TRC_PRINT(MSDC_INIT,("[%s %d][SD%d] Initialized, %s%d\n", __func__, __LINE__, id, mmc_card_sd(card)?"SD":"eMMC", card->version));

out:
    return err;
}

#ifdef FEATURE_MMC_SDIO
int mmc_init_sdio_card(struct mmc_host *host, struct mmc_card *card, u32 ocr)
{
    int err, i, id = host->id;

    /*
     * Sanity check the voltages that the card claims to
     * support.
     */
    if (ocr & 0x7F) {
    	MSDC_TRC_PRINT(MSDC_INFO,("card claims to support voltages "
    	       "below the defined range. These will be ignored.\n"));
    	ocr &= ~0x7F;
    }

    ocr = host->ocr = mmc_select_voltage(host, ocr);

    /* Can we support the voltage(s) of the card(s)? */
    if (!host->ocr) {
    	err = MMC_ERR_FAILED;
    	goto out;
    }

    err = mmc_send_io_op_cond(host, host->ocr_avail, &ocr);
    if (err)
        goto out;

    card->host = host;
    card->sdio_funcs = (ocr & 0x70000000) >> 28;

    /* send rca */
    err = mmc_send_relative_addr(host, card, &card->rca);
    if (err != MMC_ERR_NONE) {
        MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] Fail in SEND_RCA cmd\n", id));
        goto out;
    }

    err = mmc_select_card(host, card);
    if (err != MMC_ERR_NONE) {
        MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] Fail in select card\n", id));
        goto out;        
    }

    /* read the common registers */
    err = mmc_sdio_read_cccr(card);
    if (err != MMC_ERR_NONE) {
        MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] Fail in reading the common register\n", id));
        goto out;        
    }

    /* read the common CIS tuples */
    err = mmc_sdio_read_common_cis(card);
    if (err != MMC_ERR_NONE) {
        MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] Fail in reading the common CIS\n", id));
        goto out;        
    }

    /* switch to high speed mode */
    err = mmc_sdio_enable_hs(card);
    if (err != MMC_ERR_NONE) {
        MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] Fail in enable SDIO HS\n", id));
        goto out;        
    }

	/*
	 * Change to the card's maximum speed.
	 */
	if (mmc_card_highspeed(card)) {
		/*
		 * The SDIO specification doesn't mention how
		 * the CIS transfer speed register relates to
		 * high-speed, but it seems that 50 MHz is
		 * mandatory.
		 */
		mmc_set_clock(host, card->state, 50000000);
	} else {
		mmc_set_clock(host, card->state, card->cis.max_dtr);
	}

    /* switch to wider bus (if supported) */
    err = mmc_sdio_enable_wide(card);

    /* initialize (but don't add) all present function */
    for (i = 0;i < (int)card->sdio_funcs;i++) {
        err = mmc_sdio_init_func(card, i + 1);
        if (err)
            goto out;
    }

    mmc_card_set_sdio(card);
    MSDC_TRC_PRINT(MSDC_INFO,("[SD%d] SDIO Initialized\n", host->id));    

out:
    return err;
}
#endif

// 2012-02-25: Apply ett tool result
static int msdc_ett_offline_to_pl(struct mmc_host *host, struct mmc_card *card)
{
    int ret = 1;
    int size = sizeof(g_mmcTable) / sizeof(mmcdev_info);
    int i, temp; 
    u32 base = host->base;
    u8  m_id = card->cid.manfid; 
	char * pro_name = card->cid.prod_name; 
    
    MSDC_TRC_PRINT(MSDC_INIT,("MSDC%u:%s: size<%d> m_id<0x%x>\n", host->id, __func__, size, m_id));

    for (i = 0; i < size; i++) {
        MSDC_TRC_PRINT(MSDC_INIT,("msdc <%d> <%s> <%s>\n", i, g_mmcTable[i].pro_name, pro_name)); 
                    	
        if ((g_mmcTable[i].m_id == m_id) && (!strncmp(g_mmcTable[i].pro_name, pro_name, 6))) {
            MSDC_TRC_PRINT(MSDC_INIT,("msdc ett index<%d>: <%d> <%d> <0x%x> <0x%x> <0x%x>\n", i, 
                g_mmcTable[i].r_smpl, g_mmcTable[i].d_smpl, 
                g_mmcTable[i].cmd_rxdly, g_mmcTable[i].rd_rxdly, g_mmcTable[i].wr_rxdly));  

            // set to msdc0 
            MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, g_mmcTable[i].r_smpl); 
            MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DSPL, g_mmcTable[i].d_smpl);
          
            MSDC_SET_FIELD(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRRDLY, g_mmcTable[i].cmd_rxdly);
            MSDC_SET_FIELD(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATRRDLY, g_mmcTable[i].rd_rxdly); 
            MSDC_SET_FIELD(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, g_mmcTable[i].wr_rxdly); 

            temp = g_mmcTable[i].rd_rxdly; temp &= 0x1F;             
            MSDC_WRITE32(MSDC_DAT_RDDLY0, (temp<<0 | temp<<8 | temp<<16 | temp<<24)); 
            MSDC_WRITE32(MSDC_DAT_RDDLY1, (temp<<0 | temp<<8 | temp<<16 | temp<<24));
                                 
            ret = 0;
            break;
        }
    }
    
    if (ret) MSDC_TRC_PRINT(MSDC_INIT,("MSDC%u:%s: failed due to Nothing for ett parameters\n", host->id,__func__));
    return ret;        	
}

int mmc_init_card(struct mmc_host *host, struct mmc_card *card)
{
    int err;
#ifndef KEEP_SLIENT_BUILD 
    int id = host->id;
#endif
    u32 ocr;

    MSDC_TRC_PRINT(MSDC_INIT,("[%s]: start\n", __func__));
    memset(card, 0, sizeof(struct mmc_card));
    mmc_prof_init(id, host, card);
    mmc_prof_start();

#ifdef FEATURE_MMC_CARD_DETECT
    if (!msdc_card_avail(host)) {
        err = MMC_ERR_INVALID;
        goto out;
    }
#endif

#if 0
    if (msdc_card_protected(host))
        mmc_card_set_readonly(card);
#endif

    mmc_card_set_present(card);
    mmc_card_set_host(card, host);
    mmc_card_set_unknown(card);

    mmc_go_idle(host);

    /* send interface condition */
    mmc_send_if_cond(host, host->ocr_avail);
#ifdef FEATURE_MMC_SDIO
    if (mmc_send_io_op_cond(host, 0, &ocr) == MMC_ERR_NONE) {
        mmc_card_set_sdio(card);
        err = mmc_init_sdio_card(host, card, ocr);
        if (err != MMC_ERR_NONE) {
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Fail in init sdio card\n", id));
            goto out;        
        }
        /* no memory present */
        if ((ocr & 0x08000000) == 0) {
            goto out;
        }
    }
#endif
    /* query operation condition */ 
    err = mmc_send_app_op_cond(host, 0, &ocr);
    if (err != MMC_ERR_NONE) {
        err = mmc_send_op_cond(host, 0, &ocr);
        if (err != MMC_ERR_NONE) {
            MSDC_ERR_PRINT(MSDC_ERROR,("[SD%d] Fail in MMC_CMD_SEND_OP_COND/SD_ACMD_SEND_OP_COND cmd\n", id));
            goto out;
        }
        mmc_card_set_mmc(card);
    } else {
        mmc_card_set_sd(card);
    }

    err = mmc_init_mem_card(host, card, ocr);

    if (err)
        goto out;

    /* change clock */
    mmc_set_clock(host, card->state, card->maxhz);
    /* 2012-02-25 lookup the ETT table */
    msdc_ett_offline_to_pl(host, card);

    #ifdef FEATURE_MMC_UHS1
    /* tune timing */
    mmc_tune_timing(host, card);
    #endif

out:
    mmc_prof_stop();
    mmc_prof_update(mmc_prof_card_init, (u32)id, (void*)err);
    if (err) {
        //msdc_power(host, MMC_POWER_OFF);
        MSDC_ERR_PRINT(MSDC_ERROR,("[%s]: failed, err=%d\n", __func__, err));
        return err;
    }
    host->card = card;    
    MSDC_TRC_PRINT(MSDC_INIT,("[%s]: finish successfully\n",__func__));
    return 0;
}

int mmc_init_host(struct mmc_host *host, int id)
{
    memset(host, 0, sizeof(struct mmc_host));

    return msdc_init(host, id);
}

#if 1

int __mmc_init(int id)
{   
    int err = MMC_ERR_NONE;
    struct mmc_host *host;
    struct mmc_card *card;

    MSDC_ASSERT(id < NR_MMC);

    host = &sd_host[id];
    card = &sd_card[id];
    err = mmc_init_host(host, id);
    if (err == MMC_ERR_NONE)
        MSDC_TRC_PRINT(MSDC_INFO,("[%s]: msdc%d mmc_init_host() finished and start mmc_init_card()\n", __func__, id));
        err = mmc_init_card(host, card);

#ifdef MTK_EMMC_SUPPORT_OTP 
    MSDC_TRC_PRINT(MSDC_INIT,("[%s]: msdc%d, use hc erase size\n", __func__, id));
    mmc_set_erase_grp_def(card, 1);
#endif

#ifdef MMC_TEST
    //mmc_test(0, NULL);
#endif

    return err;
}

#else
unsigned long mmc_wrap_bread(int dev_num, unsigned long blknr, u32 blkcnt, unsigned long *dst)
{
    return mmc_block_read(dev_num, blknr, blkcnt, dst) == MMC_ERR_NONE ? blkcnt : (unsigned long) -1;
}
unsigned long mmc_wrap_bwrite(int dev_num, unsigned long blknr, u32 blkcnt, unsigned long *src)
{
    return mmc_block_write(dev_num, blknr, blkcnt, src) == MMC_ERR_NONE ? blkcnt : (unsigned long) -1;
}
int mmc_legacy_init(int verbose)
{
    int id = verbose - 1;
    int err = MMC_ERR_NONE;
    struct mmc_host *host;
    struct mmc_card *card;
    block_dev_desc_t *bdev;

    MSDC_BUGON(id >= NR_MMC);

    host = &sd_host[id];
    card = &sd_card[id];
    bdev = &sd_dev[id];
    err = mmc_init_host(host, id);

    if (err == MMC_ERR_NONE)
        err = mmc_init_card(host, card);

    if (err == MMC_ERR_NONE && !boot_dev_found) {
        /* fill in device description */
        bdev->if_type     = IF_TYPE_MMC;
        bdev->part_type   = PART_TYPE_DOS;
        bdev->dev         = id;
        bdev->lun         = 0;
        bdev->removable   = 1;
        bdev->blksz       = MMC_BLOCK_SIZE;
        bdev->lba         = card->nblks * card->blklen / MMC_BLOCK_SIZE;
        bdev->block_read  = mmc_wrap_bread;
        bdev->block_write = mmc_wrap_bwrite;

        host->boot_type   = RAW_BOOT;

        /* FIXME. only one RAW_BOOT dev */
        if (host->boot_type == RAW_BOOT) {
            bdev->part_type = PART_TYPE_UNKNOWN; 
            bdev->removable = 0;
            boot_dev.id = id;
            boot_dev.init = 1;
            boot_dev.blkdev = bdev;
            mt6577_part_register_device(&boot_dev);
            boot_dev_found = 1;
            MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] boot device found\n", id));
        } else if (host->boot_type == FAT_BOOT) {
            #if (CONFIG_COMMANDS & CFG_CMD_FAT)
            if (0 == fat_register_device(bdev, 1)) {
                boot_dev_found = 1;
                MSDC_TRC_PRINT(MSDC_STACK,("[SD%d] FAT partition found\n", id));
            }
            #endif
        }        
    }
#ifdef MMC_TEST
    mmc_test(0, NULL);
#endif

    return err;
}
#endif

/* bitfield bit 31 ~ 28 */
#define MSDC_IP_EXIST                                  (1 << 31)
#define MSDC_ISUSEDAS_EMMC                             (1 << 30)
#define MSDC_ISUSEDAS_SDCARD                           (1 << 29)
#define MSDC_ISUSEDAS_SDIO                             (1 << 28)

/* bitfield bit 27 ~ 24  current msdc IP index */
#define MSDC_SHIP_MSDCID(result, id)                 (result) |= ((id)&0x0F) << 24

/* bitfield bit 23~ 16 sub code for SDCARD, eMMC and SDIO*/
#define MSDC_SUB_SDCARD_PLUGOUT                   (1 << 21)

/* bitfield bit 15~ 0 ship error code for mmc_core.c */
#define MSDC_SHIP_ERRORCODE(result, err)         (result) |= ((err) & 0x0000FFFF)

/* some usage macro definition */
#define IS_MSDC_USING_AS_EMMC(result)           (((result) & MSDC_IP_EXIST) && ((result) & MSDC_ISUSEDAS_EMMC))  
#define IS_MSDC_USING_AS_SDCARD(result)         (((result) & MSDC_IP_EXIST) && ((result) & MSDC_ISUSEDAS_SDCARD))
#define IS_MSDC_USING_AS_SDIO(result)           (((result) & MSDC_IP_EXIST) && ((result) & MSDC_ISUSEDAS_SDIO))
#define MSDC_GET_ERRORCODE(result)              ( (result) & 0x0000FFFF)
#define MSDC_GET_MSDCID(result)                 (((result) & 0x0F000000) >> 24)

int msdc_preinit(u32 id, struct mmc_host *host);
u32 mmc_is_exist(u32 id)
{
	int err = MMC_ERR_NONE;
	struct mmc_host *host;
	struct mmc_card *card;
	u32 ocr=0;
	u32 result=0;

	MSDC_ASSERT(id < NR_MMC);

	host = &sd_host[id];
	card = &sd_card[id];
	do
	{
		MSDC_SHIP_MSDCID(result,id);

		memset(host, 0, sizeof(struct mmc_host));

		/* stage 0: init msdc host ip push device enter pre-idle state */
		err =  msdc_preinit(id, host);
		if (err != MMC_ERR_NONE)
		{
			MSDC_SHIP_ERRORCODE(result, err);
			break;
		}

		result |= MSDC_IP_EXIST;

		memset(card, 0, sizeof(struct mmc_card));
		host->card = card;

		if (!msdc_card_avail(host))
		{
			result |= MSDC_ISUSEDAS_SDCARD;
			result |= MSDC_SUB_SDCARD_PLUGOUT;
			break;
		}
		mmc_card_set_present(card);
		mmc_card_set_host(card, host);
		mmc_card_set_unknown(card);

		/* stage 1: send cmd 0 (no resp),  push device enter idle state */
		mmc_go_idle(host);

		/* stage 2: send cmd 8 (with resp 1),  but not identify device type yet */

		/* send interface condition */
		mmc_send_if_cond(host, host->ocr_avail);

		/* stage 3: send cmd 5 (with resp 4),  if got valid response, make sure it is SDIO device for IO ocr */
		if (mmc_send_io_op_cond(host, 0, &ocr) == MMC_ERR_NONE)
		{
			result |= MSDC_ISUSEDAS_SDIO;
			mmc_card_set_sdio(card);
			break;
		}
		else
		{
			/* stage 4: send cmd 41 (with resp 3),
			* if got valid response, make sure it is SDCARD device for SDCARD Application CMD requirement.
			* if send cmd 41 failure, maybe it eMMC device.
			* query operation condition */
			err = mmc_send_app_op_cond(host, 0, &ocr);
			if (err == MMC_ERR_NONE)
			{
				result |= MSDC_ISUSEDAS_SDCARD;
				mmc_card_set_sd(card);
			}
			else
			{
				/* stage 4: send cmd 1 (with resp 3),
				* if got valid response, make sure it is emmc device for emmc switch to device ready state.
				* if send cmd 1 failure, maybe it unknow device, bad emmc device, and so on.
				* query operation condition */
				err = mmc_send_op_cond(host, 0, &ocr);
				if (err == MMC_ERR_NONE)
				{
					result |= MSDC_ISUSEDAS_EMMC;
					mmc_card_set_mmc(card);
				}
				else
				{
					MSDC_SHIP_ERRORCODE(result, err);
				}
			}
		}
	}
	while(0);
	
	MSDC_TRC_PRINT(MSDC_INIT,("mmc_is_exist(%u) : result:0x%x.err.%u)\n", id, result, err));

	return result;
}


#if  0 //defined(FPGA_PLATFORM)
#define EFUSE_EMMC_MSDC3_IsDisabled()   1
#define EFUSE_EMMC_MSDC0_IsDisabled()   0
#else

#define EFUSE_BASE                      0x10200000
#define EFUSE_M_HW2_RES0                0x000061C0
#define EFUSE_EMMC_MSDC3_DIS            (1ul << 11)
#define EFUSE_EMMC_MSDC0_DIS            (1ul << 12)
#define MT7623_IO_READ32(addr, index)   (*(volatile unsigned int*)((addr) + (index)))

#define EFUSE_EMMC_MSDC3_IsDisabled()   (!!(MT7623_IO_READ32(EFUSE_BASE,EFUSE_M_HW2_RES0) & EFUSE_EMMC_MSDC3_DIS))
#define EFUSE_EMMC_MSDC0_IsDisabled()   (!!(MT7623_IO_READ32(EFUSE_BASE,EFUSE_M_HW2_RES0) & EFUSE_EMMC_MSDC0_DIS))
#endif
u32 mmc_whoami(void)
{
	u32 result = 0;
	u32 whichmsdc =-1;
#ifndef KEEP_SLIENT_BUILD    
	int error = 0;
#else
    int error __attribute((unused)) = 0;
#endif

	if(!EFUSE_EMMC_MSDC3_IsDisabled())
	{
		result = mmc_is_exist(3);
		if(IS_MSDC_USING_AS_EMMC(result))
		{
			whichmsdc = 3;
		}
		else if(IS_MSDC_USING_AS_SDCARD(result))
		{
			MSDC_DBG_PRINT(MSDC_INIT,("!!!WARNNING: MSDC%u is used as SDCARD\n", MSDC_GET_MSDCID(result)));
#ifdef FPGA_PLATFORM
            whichmsdc = 3;
#endif  
        }
		else if(IS_MSDC_USING_AS_SDIO(result))
		{
			MSDC_DBG_PRINT(MSDC_INIT,("!!!ERROR: MSDC%u is used as SDIO\n", MSDC_GET_MSDCID(result)));
		}
		else
		{
#ifdef FPGA_PLATFORM
			result = mmc_is_exist(0);
			if(IS_MSDC_USING_AS_EMMC(result))
			{
				whichmsdc = 0;
			}
			else
			{
				if(IS_MSDC_USING_AS_SDCARD(result))
				{
					MSDC_DBG_PRINT(MSDC_INIT,("!!!WARNNING: MSDC%u is used as SDCARD\n", MSDC_GET_MSDCID(result)));
#ifdef FPGA_PLATFORM
                    whichmsdc = 0;
#endif  
                }
				else if(IS_MSDC_USING_AS_SDIO(result))
				{
					MSDC_DBG_PRINT(MSDC_INIT,("!!!WARNNING: MSDC%u is used as SDIO\n", MSDC_GET_MSDCID(result)));
				}
				else
				{
					/* Never run here!!!!*/
					error = MSDC_GET_ERRORCODE(result);
					MSDC_DBG_PRINT(MSDC_INIT,("!!!ERROR: MSDC%u can't recognize present device since 0x%x\n", MSDC_GET_MSDCID(result),error));
				}
			}
#else
			/* Never run here!!!!*/
			error = MSDC_GET_ERRORCODE(result);
			MSDC_DBG_PRINT(MSDC_INIT,("!!!ERROR: MSDC%u can't recognize present device since 0x%x\n", MSDC_GET_MSDCID(result),error));
#endif
		}
	}
	else if (!EFUSE_EMMC_MSDC0_IsDisabled())
	{
		result = mmc_is_exist(0);
		if(IS_MSDC_USING_AS_EMMC(result))
		{
			whichmsdc = 0;
		}
		else if(IS_MSDC_USING_AS_SDCARD(result))
		{
			MSDC_DBG_PRINT(MSDC_INIT,("!!!WARNNING: MSDC%u is used as SDCARD\n", MSDC_GET_MSDCID(result)));
#ifdef FPGA_PLATFORM
			whichmsdc = 0;
#endif  
		}
		else if(IS_MSDC_USING_AS_SDIO(result))
		{
			MSDC_DBG_PRINT(MSDC_INIT,("!!!WARNNING: MSDC%u is used as SDIO\n", MSDC_GET_MSDCID(result)));
		}
		else
		{
#ifdef FPGA_PLATFORM
			result = mmc_is_exist(3);
			if(IS_MSDC_USING_AS_EMMC(result))
			{
				whichmsdc = 3;
			}
			else
			{
				if(IS_MSDC_USING_AS_SDCARD(result))
				{
					MSDC_DBG_PRINT(MSDC_INIT,("!!!WARNNING: MSDC%u is used as SDCARD\n", MSDC_GET_MSDCID(result)));
#ifdef FPGA_PLATFORM
					whichmsdc = 3;
#endif  
				}
				else if(IS_MSDC_USING_AS_SDIO(result))
				{
					MSDC_DBG_PRINT(MSDC_INIT,("!!!WARNNING: MSDC%u is used as SDIO\n", MSDC_GET_MSDCID(result)));
				}
				else
				{
					/* Never run here!!!!*/
					error = MSDC_GET_ERRORCODE(result);
					MSDC_DBG_PRINT(MSDC_INIT,("!!!ERROR: MSDC%u can't recognize present device since 0x%x\n", MSDC_GET_MSDCID(result),error));
				}
			}
#else
			/* Never run here!!!!*/
			error = MSDC_GET_ERRORCODE(result);
			MSDC_DBG_PRINT(MSDC_INIT,("!!!ERROR: MSDC%u can't recognize present device since 0x%x\n", MSDC_GET_MSDCID(result),error));
#endif
		}
	}
	else
	{
		MSDC_DBG_PRINT(MSDC_INIT,("*************ALERT: MSDCs can't detect valid efuse configuration!*************\n"));
		result = mmc_is_exist(3);
		if(IS_MSDC_USING_AS_EMMC(result))
		{
			whichmsdc = 3;
		}
		else if(IS_MSDC_USING_AS_SDCARD(result))
		{
			MSDC_DBG_PRINT(MSDC_INIT,("!!!WARNNING: MSDC%u is used as SDCARD\n", MSDC_GET_MSDCID(result)));
#ifdef FPGA_PLATFORM
            whichmsdc = 3;
#endif  
        }
		else if(IS_MSDC_USING_AS_SDIO(result))
		{
			MSDC_DBG_PRINT(MSDC_INIT,("!!!WARNNING: MSDC%u is used as SDIO\n", MSDC_GET_MSDCID(result)));
		}
		else
		{
			result = mmc_is_exist(0);
			if(IS_MSDC_USING_AS_EMMC(result))
			{
				whichmsdc = 0;
			}
			else
			{
				if(IS_MSDC_USING_AS_SDCARD(result))
				{
					MSDC_DBG_PRINT(MSDC_INIT,("!!!WARNNING: MSDC%u is used as SDCARD\n", MSDC_GET_MSDCID(result)));
#ifdef FPGA_PLATFORM
                    whichmsdc = 0;
#endif  
                }
				else if(IS_MSDC_USING_AS_SDIO(result))
				{
					MSDC_DBG_PRINT(MSDC_INIT,("!!!WARNNING: MSDC%u is used as SDIO\n", MSDC_GET_MSDCID(result)));
				}
				else
				{
					/* Never run here!!!!*/
					error = MSDC_GET_ERRORCODE(result);
					MSDC_DBG_PRINT(MSDC_INIT,("!!!ERROR: MSDC%u can't recognize present device since 0x%x\n", MSDC_GET_MSDCID(result),error));
				}
			}
		}
	}

	if (whichmsdc != -1)
	{
		MSDC_TRC_PRINT(MSDC_INIT,("============Who am I, and I am eMMC (MSDC%u)================\n", whichmsdc));
	}

	return whichmsdc;
}

int mmc_info_helper(unsigned int *lba, char* vendor, char *product, char *revision){

    struct mmc_card *mmc = &sd_card[1];
    
    *lba = mmc->nblks;

    MSDC_TRC_PRINT(MSDC_INIT,("[%s]: nblks = %d, blklen = %d\n", 
        __func__, mmc->nblks, mmc->blklen ));

	sprintf(vendor, "Man %06x Snr %04x%04x",
		mmc->raw_cid[0] >> 24, (mmc->raw_cid[2] & 0xffff),
		(mmc->raw_cid[3] >> 16) & 0xffff);
	sprintf(product, "%c%c%c%c%c%c", mmc->raw_cid[0] & 0xff,
		(mmc->raw_cid[1] >> 24), (mmc->raw_cid[1] >> 16) & 0xff,
		(mmc->raw_cid[1] >> 8) & 0xff, mmc->raw_cid[1] & 0xff,
		(mmc->raw_cid[2] >> 24) & 0xff);
	sprintf(revision, "%d.%d", (mmc->raw_cid[2] >> 20) & 0xf,
		(mmc->raw_cid[2] >> 16) & 0xf);


    return 0;
}

int mmc_info_helper2(int id, 
    char *cid, int cid_len, 
    unsigned int *speed,
    unsigned int *read_bl_len,
    unsigned int *scr, int scr_len,
    unsigned int *high_capacity,
    unsigned int *blk_num,
    unsigned int *bus_width){
    
    struct mmc_card *mmc = 0;
    
    if(id < 0 || id > 1) return -1;
    
    mmc = &sd_card[id];
    memcpy(cid, mmc->raw_cid, 
        (cid_len > sizeof(mmc->raw_cid)) ? (sizeof(mmc->raw_cid)) : cid_len);
    *speed = mmc->maxhz;
    *read_bl_len = mmc->blklen;
    *scr = mmc->raw_scr[0];
    *(scr + 1) = mmc->raw_scr[1];
  //  memcpy(scr, mmc->raw_scr, 
  //      (scr_len > sizeof(mmc->raw_scr)) ? (sizeof(mmc->raw_scr)) : scr_len);
    *high_capacity = ((mmc->ocr >> 30) & 0x1) ? MMC_STATE_HIGHCAPS : 0;
    *blk_num = mmc->nblks; //card->blklen, card->nblks
    *bus_width = mmc->scr.bus_widths;
    //printf("\ntotal number of block =%d\n", mmc->nblks);
    return 0;
}

#ifdef FEATURE_MMC_BOOT_MODE
int emmc_part_read(u8 partno, u32 blknr, u32 blkcnt, unsigned long *dst)
{
	struct mmc_card *mmc = &sd_card[0];
	return mmc_part_read(mmc, partno, blknr, blkcnt, dst);

}

int emmc_part_write(u8 partno, u32 blknr, u32 blkcnt, unsigned long *src)
{
	struct mmc_card *mmc = &sd_card[0];
	return mmc_part_write(mmc, partno, blknr, blkcnt, src);
}

void emmc_dump_ext_csd(void)
{
	mmc_dump_ext_csd_p(&sd_card[0]);
}

int emmc_set_part_config(u8 cfg)
{
	return mmc_set_part_config(&sd_card[0], cfg);
}
#endif
