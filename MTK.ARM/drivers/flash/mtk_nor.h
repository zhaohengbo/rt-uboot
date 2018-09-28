/*----------------------------------------------------------------------------*
 * No Warranty                                                                *
 * Except as may be otherwise agreed to in writing, no warranties of any      *
 * kind, whether express or implied, are given by MTK with respect to any MTK *
 * Deliverables or any use thereof, and MTK Deliverables are provided on an   *
 * "AS IS" basis.  MTK hereby expressly disclaims all such warranties,        *
 * including any implied warranties of merchantability, non-infringement and  *
 * fitness for a particular purpose and any warranties arising out of course  *
 * of performance, course of dealing or usage of trade.  Parties further      *
 * acknowledge that Company may, either presently and/or in the future,       *
 * instruct MTK to assist it in the development and the implementation, in    *
 * accordance with Company's designs, of certain softwares relating to        *
 * Company's product(s) (the "Services").  Except as may be otherwise agreed  *
 * to in writing, no warranties of any kind, whether express or implied, are  *
 * given by MTK with respect to the Services provided, and the Services are   *
 * provided on an "AS IS" basis.  Company further acknowledges that the       *
 * Services may contain errors, that testing is important and Company is      *
 * solely responsible for fully testing the Services and/or derivatives       *
 * thereof before they are used, sublicensed or distributed.  Should there be *
 * any third party action brought against MTK, arising out of or relating to  *
 * the Services, Company agree to fully indemnify and hold MTK harmless.      *
 * If the parties mutually agree to enter into or continue a business         *
 * relationship or other arrangement, the terms and conditions set forth      *
 * hereunder shall remain effective and, unless explicitly stated otherwise,  *
 * shall prevail in the event of a conflict in the terms in any agreements    *
 * entered into between the parties.                                          *
 *---------------------------------------------------------------------------*/
/******************************************************************************
* [File]			mtk_nor.h
* [Version]			v1.0
* [Revision Date]	2011-05-04
* [Author]			Shunli Wang, shunli.wang@mediatek.inc, 82896, 2012-04-27
* [Description]
*	SPI-NOR Driver Header File
* [Copyright]
*	Copyright (C) 2011 MediaTek Incorporation. All Rights Reserved.
******************************************************************************/

//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------

//#include "x_typedef.h"

#define __UBOOT_NOR__ 1
#define BD_NOR_ENABLE

//-----------------------------------------------------------------------------
// Constant definitions
//-----------------------------------------------------------------------------
#define SFLASH_POLLINGREG_COUNT     500000
#define SFLASH_WRITECMD_TIMEOUT     100000
#define SFLASH_WRITEBUSY_TIMEOUT    500000
#define SFLASH_ERASESECTOR_TIMEOUT  200000
#define SFLASH_CHIPERASE_TIMEOUT    500000

#define SFLASH_WRITE_PROTECTION_VAL 0x00
#define SFLASH_MAX_DMA_SIZE      (1024*8)

#define REGION_RANK_FIRST_FLASH           (0x1<<0)
#define REGION_RANK_FIRST_WHOLE_FLASH     (0x1<<1)
#define REGION_RANK_SECOND_FLASH          (0x1<<2)
#define REGION_RANK_SECOND_WHOLE_FLASH    (0x1<<3)
#define REGION_RANK_TWO_WHOLE_FLASH       (REGION_RANK_FIRST_FLASH | \
	                                       REGION_RANK_FIRST_WHOLE_FLASH | \
	                                       REGION_RANK_SECOND_FLASH | \
	                                       REGION_RANK_SECOND_WHOLE_FLASH)




//-----------------------------------------------------------------------------
// Type definitions
//-----------------------------------------------------------------------------
typedef unsigned int		UINT32;
typedef unsigned short		UINT16;
typedef unsigned char		UINT8;
typedef signed int		INT32;
typedef signed short		INT16;
typedef signed char		INT8;
typedef void			VOID;
typedef char			CHAR;

#define VECTOR_FLASH		60


#define SFLASH_BASE         	(0x0000000)

#ifdef BD_NOR_ENABLE
//#define SFLASH_MEM_BASE         ((UINT32)0xFA000000)//mmu on
//#define SFLASH_REG_BASE         ((UINT32)(0xFD000000 + 0x11000))
 #define SFLASH_REG_BASE		0x11014000 
#else
#define SFLASH_MEM_BASE         (0xF0000000 + 0x8000000)
#define SFLASH_REG_BASE         (0xF0000000 + 0x08700)
#endif

#define SFLASH_CMD_REG          ((UINT32)0x00)
#define SFLASH_CNT_REG          ((UINT32)0x04)
#define SFLASH_RDSR_REG         ((UINT32)0x08)
#define SFLASH_RDATA_REG        ((UINT32)0x0C)
#ifdef BD_NOR_ENABLE
#define SFLASH_RADR0_REG        ((UINT32)0x10)
#define SFLASH_RADR1_REG        ((UINT32)0x14)
#define SFLASH_RADR2_REG        ((UINT32)0x18)
#define SFLASH_WDATA_REG        ((UINT32)0x1C)
#define SFLASH_PRGDATA0_REG     ((UINT32)0x20)
#define SFLASH_PRGDATA1_REG     ((UINT32)0x24)
#define SFLASH_PRGDATA2_REG     ((UINT32)0x28)
#define SFLASH_PRGDATA3_REG     ((UINT32)0x2C)
#define SFLASH_PRGDATA4_REG     ((UINT32)0x30)
#define SFLASH_PRGDATA5_REG     ((UINT32)0x34)
#define SFLASH_SHREG0_REG       ((UINT32)0x38)
#define SFLASH_SHREG1_REG       ((UINT32)0x3C)
#define SFLASH_SHREG2_REG       ((UINT32)0x40)
#define SFLASH_SHREG3_REG       ((UINT32)0x44)
#define SFLASH_SHREG4_REG       ((UINT32)0x48)
#define SFLASH_SHREG5_REG       ((UINT32)0x4C)
#define SFLASH_SHREG6_REG       ((UINT32)0x50)
#define SFLASH_SHREG7_REG       ((UINT32)0x54)
#define SFLASH_SHREG8_REG       ((UINT32)0x58)
#define SFLASH_SHREG9_REG       ((UINT32)0x5C)
#define SFLASH_FLHCFG_REG      	((UINT32)0x84)
#define SFLASH_PP_DATA_REG      ((UINT32)0x98)
#define SFLASH_PREBUF_STUS_REG  ((UINT32)0x9C)
#define SFLASH_SF_INTRSTUS_REG  ((UINT32)0xA8)
#define SFLASH_SF_INTREN_REG    ((UINT32)0xAC)
#define SFLASH_SF_TIME_REG      ((UINT32)0x94)
#define SFLASH_CHKSUM_CTL_REG   ((UINT32)0xB8)
#define SFLASH_CHECKSUM_REG     ((UINT32)0xBC)
#define SFLASH_CMD2_REG     	((UINT32)0xC0)
#define SFLASH_WRPROT_REG       ((UINT32)0xC4)
#define SFLASH_RADR3_REG        ((UINT32)0xC8)
#define SFLASH_DUAL_REG         ((UINT32)0xCC)
#define SFLASH_DELSEL0_REG    	((UINT32)0xA0)
#define SFLASH_DELSEL1_REG    	((UINT32)0xA4)
#define SFLASH_DELSEL2_REG    	((UINT32)0xD0)
#define SFLASH_DELSEL3_REG    	((UINT32)0xD4)
#define SFLASH_DELSEL4_REG    	((UINT32)0xD8)
#else
#define SFLASH_RADR0_REG        ((UINT32)0x20)
#define SFLASH_RADR1_REG        ((UINT32)0x24)
#define SFLASH_RADR2_REG        ((UINT32)0x28)
#define SFLASH_WDATA_REG        ((UINT32)0x2C)
#define SFLASH_PRGDATA0_REG     ((UINT32)0x30)
#define SFLASH_PRGDATA1_REG     ((UINT32)0x34)
#define SFLASH_PRGDATA2_REG     ((UINT32)0x38)
#define SFLASH_PRGDATA3_REG     ((UINT32)0x3C)
#define SFLASH_PRGDATA4_REG     ((UINT32)0x40)
#define SFLASH_PRGDATA5_REG     ((UINT32)0x44)
#define SFLASH_SHREG0_REG       ((UINT32)0x48)
#define SFLASH_SHREG1_REG       ((UINT32)0x4C)
#define SFLASH_SHREG2_REG       ((UINT32)0x50)
#define SFLASH_SHREG3_REG       ((UINT32)0x54)
#define SFLASH_SHREG4_REG       ((UINT32)0x58)
#define SFLASH_SHREG5_REG       ((UINT32)0x5C)
#define SFLASH_PP_DATA_REG      ((UINT32)0x80)
#define SFLASH_PREBUF_STUS_REG  ((UINT32)0x84)
#define SFLASH_SF_INTRSTUS_REG  ((UINT32)0x88)
#define SFLASH_SF_INTREN_REG    ((UINT32)0x8C)
#define SFLASH_SF_TIME_REG      ((UINT32)0xA4)
#define SFLASH_CHKSUM_CTL_REG   ((UINT32)0xA8)
#define SFLASH_CHECKSUM_REG     ((UINT32)0xAC)
#define SFLASH_CKGEN_REG        ((UINT32)0xB0)
#define SFLASH_SAMPLE_EDGE_REG  ((UINT32)0xB4)
#define SFLASH_DELAY_SELECTION  ((UINT32)0xB8)
#define SFLASH_RADR3_REG        ((UINT32)0xBC)
#define SFLASH_DUAL_REG         ((UINT32)0xC0)
#define SFLASH_PREFETCH_REG     ((UINT32)0xC4)
#define SFLASH_PRGDATA6_REG     ((UINT32)0xC8)
#define SFLASH_PRGDATA7_REG     ((UINT32)0xCC)
#define SFLASH_PRGDATA_RISC0    ((UINT32)0xD0)
#define SFLASH_PRGDATA_RISC1    ((UINT32)0xD4)
#define SFLASH_SHREG6_REG       ((UINT32)0xD8)
#define SFLASH_SHREG7_REG       ((UINT32)0xDC)
#define SFLASH_SHREG_RISC0      ((UINT32)0xE0)
#define SFLASH_SHREG_RISC1      ((UINT32)0xE4)

#endif
#define SFLASH_CFG1_REG         ((UINT32)0x60)
#define SFLASH_CFG2_REG         ((UINT32)0x64)
#define SFLASH_CFG3_REG         ((UINT32)0x68)
#define SFLASH_STATUS0_REG      ((UINT32)0x70)
#define SFLASH_STATUS1_REG      ((UINT32)0x74)
#define SFLASH_STATUS2_REG      ((UINT32)0x78)
#define SFLASH_STATUS3_REG      ((UINT32)0x7C)
//-----------------------------------------------------------------------------
#define SFLASH_WRBUF_SIZE       128


//-----------------------------------------------------------------------------
#ifdef BD_NOR_ENABLE
#define MAX_FLASHCOUNT  1
#else
#define MAX_FLASHCOUNT  2
#endif
#define SFLASHHWNAME_LEN    48

typedef struct
{
    UINT8   u1MenuID;
    UINT8   u1DevID1;
    UINT8   u1DevID2;
    UINT8   u1PPType;
    UINT32  u4ChipSize;
    UINT32  u4SecSize;
    UINT32  u4CEraseTimeoutMSec;

    UINT8   u1WRENCmd;
    UINT8   u1WRDICmd;
    UINT8   u1RDSRCmd;
    UINT8   u1WRSRCmd;
    UINT8   u1READCmd;
    UINT8   u1FASTREADCmd;
    UINT8   u1READIDCmd;
    UINT8   u1SECERASECmd;
    UINT8   u1CHIPERASECmd;
    UINT8   u1PAGEPROGRAMCmd;
    UINT8   u1AAIPROGRAMCmd;
    UINT8   u1DualREADCmd;
    UINT8   u1Protection;
    CHAR    pcFlashStr[SFLASHHWNAME_LEN];
} SFLASHHW_VENDOR_T;

//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------
#define SFLASH_WREG8(offset, value)       ((*(volatile unsigned int *)(SFLASH_REG_BASE + offset)) = (value & 0xFF))

#define SFLASH_RREG8(offset)              ((*(volatile unsigned int *)(SFLASH_REG_BASE + offset)) & 0xFF)

#define SFLASH_WREG32(offset, value)      (*(volatile unsigned int *)(SFLASH_REG_BASE + offset)) = (value)
#define SFLASH_RREG32(offset)             (*(volatile unsigned int *)(SFLASH_REG_BASE + offset))

#define SFLASH_WRITE32(offset, value)     (*(volatile unsigned int *)(offset)) = (value)
#define SFLASH_READ32(offset)             (*(volatile unsigned int *)(offset))

#ifdef BD_NOR_ENABLE
// Do Reset
#define SFLASH_DMA_RESET                    \
{                                           \
  SFLASH_WRITE32(0xF1014718, 0x0);          \
  SFLASH_WRITE32(0xF1014718, 0x2);          \
  SFLASH_WRITE32(0xF1014718, 0x0);          \
}

// DMA Address Setting
#define SFLASH_DMA_ADDR_SETTING(src, deststart, len)      \
{                                                         \
  SFLASH_WRITE32(0xF101471C, src);                        \
  SFLASH_WRITE32(0xF1014720, deststart);                  \
  SFLASH_WRITE32(0xF1014724, deststart+len);              \
}

// DMA Trigger
#define SFLASH_DMA_START                    \
{											\
  SFLASH_WRITE32(0xF1014718, 0x05);			\
}

// DMA Completion Check
#define SFLASH_DMA_IS_COMPLETED            (0x0 == (SFLASH_READ32(0xF1014718) & 0x1))

#else
// Do Reset
#define SFLASH_DMA_RESET                    \
{                                           \
  SFLASH_WRITE32(0xF00085A0, 0x0);          \
  SFLASH_WRITE32(0xF00085A0, 0x2);          \
  SFLASH_WRITE32(0xF00085A0, 0x4);          \
}

// DMA Address Setting
#define SFLASH_DMA_ADDR_SETTING(src, deststart, len)      \
{                                                         \
  SFLASH_WRITE32(0xF00085A4, src);                        \
  SFLASH_WRITE32(0xF00085A8, deststart);                  \
  SFLASH_WRITE32(0xF00085AC, deststart+len);              \
}

// DMA Trigger
#define SFLASH_DMA_START                    \
{											\
  SFLASH_WRITE32(0xF00085A0, 0x05);			\
}

// DMA Completion Check
#define SFLASH_DMA_IS_COMPLETED            (0x0 == (SFLASH_READ32(0xF00085A0) & 0x1))
#endif

// ISR related Macro
#define IRQEN_REG                0xF0008034
#define SFLASH_IRQEN_SHIFT       (22)

#define SFLASH_AAIWRE            (0x1<<7)
#define SFLASH_DMARDE            (0x1<<6)
#define SFLASH_WSRE              (0x1<<5)
#define SFLASH_WRE               (0x1<<4)
#define SFLASH_PRGE              (0x1<<2)
#define SFLASH_RSRE              (0x1<<1)
#define SFLASH_RDE               (0x1<<0)

#define SFLASH_EN_INT            SFLASH_DMARDE

// HOST attribute
#define SFLASH_USE_DMA           (0x1<<0)
#define SFLASH_USE_ISR           (0x1<<1)
#define SFLASH_USE_DUAL_READ     (0x1<<2)

#define LoWord(d)     ((UINT16)(d & 0x0000ffffL))
#define HiWord(d)     ((UINT16)((d >> 16) & 0x0000ffffL))
#define LoByte(w)     ((UINT8)(w & 0x00ff))
#define HiByte(w)     ((UINT8)((w >> 8) & 0x00ff))
#define MidWord(d)    ((UINT16)((d >>8) & 0x00ffff))

#define BYTE0(arg)    (*(UINT8 *)&arg)
#define BYTE1(arg)    (*((UINT8 *)&arg + 1))
#define BYTE2(arg)    (*((UINT8 *)&arg + 2))
#define BYTE3(arg)    (*((UINT8 *)&arg + 3))


/* Function prototype */
int mtk_nor_init(void);
u32 mtk_nor_read(u32 from, u32 len, size_t *retlen, u_char *buf);
u32 mtk_nor_write(u32 to, u32 len, size_t *retlen, const u_char *buf);
u32 mtk_nor_erase(u32 addr, u32 len);

