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
 
#ifndef _MSDC_CFG_H_
#define _MSDC_CFG_H_

//#include "platform.h"

/*--------------------------------------------------------------------------*/
/* Common Definition                                                        */
/*--------------------------------------------------------------------------*/
#if CFG_FPGA_PLATFORM
#define FPGA_PLATFORM
//#define FPGA_SDCARD_TRIALRUN
#else
//#define MTK_MSDC_BRINGUP_DEBUG
#endif


#define MSDC_MAX_NUM            (4)

#define MSDC_USE_REG_OPS_DUMP   (0)
#define MSDC_USE_DMA_MODE       (0)


#if (1 == MSDC_USE_DMA_MODE)
//#define MSDC_DMA_BOUNDARY_LIMITAION
#define MSDC_DMA_DEFAULT_MODE            MSDC_MODE_DMA_ENHANCED
#endif

//#define FEATURE_MMC_UHS1
//#define FEATURE_MMC_BOOT_MODE
//#define FEATURE_MMC_WR_TUNING
//#define FEATURE_MMC_CARD_DETECT
#define FEATURE_MMC_STRIPPED
#define FEATURE_MMC_RD_TUNING
#define FEATURE_MMC_CM_TUNING
//#define FEATURE_MSDC_ENH_DMA_MODE

/* Maybe we discard these macro definition */
//#define MMC_PROFILING
//#define FEATURE_MMC_SDIO
//#define FEATURE_MMC_BOOT_MODE
//#define MTK_EMMC_SUPPORT_OTP
//#define MMC_TEST
//#define MMC_BOOT_TEST
//#define MMC_ICE_DOWNLOAD
//#define MSDC_USE_PATCH_BIT2_TURNING_WITH_ASYNC
//#define ACMD23_WITH_NEW_PATH
//#define FEATURE_MMC_CARD_DETECT

/*--------------------------------------------------------------------------*/
/* Debug Definition                                                         */
/*--------------------------------------------------------------------------*/
#define KEEP_SLIENT_BUILD
#define ___MSDC_DEBUG___
#endif /* end of _MSDC_CFG_H_ */

