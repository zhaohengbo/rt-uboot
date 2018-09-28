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

#ifndef _MMC_HAL_H_
#define _MMC_HAL_H_

#include "mmc_types.h"
#include "mmc_core.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int  msdc_init(struct mmc_host *host, int id);
extern int  msdc_cmd(struct mmc_host *host, struct mmc_command *cmd);
extern int  msdc_card_protected(struct mmc_host *host);
extern int  msdc_card_avail(struct mmc_host *host);
extern void msdc_power(struct mmc_host *host, u8 mode);
extern void msdc_config_bus(struct mmc_host *host, u32 width);
extern void msdc_config_clock(struct mmc_host *host, int state, u32 hz);
extern void msdc_set_timeout(struct mmc_host *host, u32 ns, u32 clks);
extern int  msdc_switch_volt(struct mmc_host *host, int volt);
extern int  msdc_tune_uhs1(struct mmc_host *host, struct mmc_card *card);
extern int  msdc_pio_read(struct mmc_host *host, u32 *ptr, u32 size);
extern void msdc_set_blklen(struct mmc_host *host, u32 blklen);
extern void msdc_set_blknum(struct mmc_host *host, u32 blknum);

extern int  msdc_emmc_boot_reset(struct mmc_host *host, int reset);
extern int  msdc_emmc_boot_start(struct mmc_host *host, u32 hz, int ddr, int mode, int ackdis);
extern int  msdc_emmc_boot_read(struct mmc_host *host, u32 size, u32 *to);
extern int  msdc_emmc_boot_stop(struct mmc_host *host, int mode);

/* unused in download agent */
extern void msdc_sdioirq(struct mmc_host *host, int enable);
extern void msdc_sdioirq_register(struct mmc_host *host, hw_irq_handler_t handler);
extern void msdc_set_blksz(struct mmc_host *host, u32 sz); /* FIXME */
extern int  msdc_fifo_read(struct mmc_host *host, u32 *ptr, u32 size);
extern int  msdc_fifo_write(struct mmc_host *host, u32 *ptr, u32 size);    

#ifdef __cplusplus
}
#endif

#endif /* _MMC_HAL_H_ */

