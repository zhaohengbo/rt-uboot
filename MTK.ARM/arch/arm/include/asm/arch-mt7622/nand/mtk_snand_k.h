#ifndef __MTK_SNAND_K_H__
#define __MTK_SNAND_K_H__

#ifdef CONFIG_MTK_SPI_NAND_SUPPORT

//#include <asm/arch/mt_typedefs.h>
//#include <mach/mt_reg_base.h>
//#include <platform/mtk_nand.h>
//#include "snand_device_list.h"

// Supported SPI protocols
typedef enum {
    SF_UNDEF = 0
   ,SPI      = 1
   ,SPIQ     = 2
   ,QPI      = 3
} SNAND_Mode;

// delay latency
#define SNAND_DEV_RESET_LATENCY_US           (3 * 10)    // 0.3 us * 100
#define SNAND_MAX_RDY_AFTER_RST_LATENCY_US   (10000)     // 10 ms
#define SNAND_MAX_READ_BUSY_US               (100 * 40)  // period * 40, safe value (plus ECC engine processing time). MTK_INTERNAL_SPI_NAND_1 spec: Max 100 us
#define SNAND_MAX_NFI_RESET_US               (10)         // [By Designer Curtis Tsai] 50T @ 26Mhz (1923 ns) is enough

// Standard Commands
#define SNAND_CMD_BLOCK_ERASE               (0xD8)
#define SNAND_CMD_GET_FEATURES              (0x0F)
#define SNAND_CMD_FEATURES_BLOCK_LOCK       (0xA0)
#define SNAND_CMD_FEATURES_OTP              (0xB0)
#define SNAND_CMD_FEATURES_STATUS           (0xC0)
#define SNAND_CMD_PAGE_READ                 (0x13)
#define SNAND_CMD_PROGRAM_EXECUTE           (0x10)
#define SNAND_CMD_PROGRAM_LOAD              (0x02)
#define SNAND_CMD_PROGRAM_LOAD_X4           (0x32)
#define SNAND_CMD_PROGRAM_RANDOM_LOAD       (0x84)
#define SNAND_CMD_PROGRAM_RANDOM_LOAD_X4    (0xC4)
#define SNAND_CMD_READ_ID                   (0x9F)
#define SNAND_CMD_RANDOM_READ               (0x03)
#define SNAND_CMD_RANDOM_READ_SPIQ          (0x6B)
#define SNAND_CMD_SET_FEATURES              (0x1F)

#define SNAND_CMD_SW_RESET                  (0xFF)
#define SNAND_CMD_WRITE_ENABLE              (0x06)

// Status register
#define SNAND_STATUS_OIP                    (0x01)
#define SNAND_STATUS_WEL                    (0x02)
#define SNAND_STATUS_ERASE_FAIL             (0x04)
#define SNAND_STATUS_PROGRAM_FAIL           (0x08)
#define SNAND_STATUS_TOO_MANY_ERROR_BITS    (0x20)
#define SNAND_STATUS_ERROR_BITS_CORRECTED   (0x10)
#define SNAND_STATUS_ERROR_BITS_CORRECTED_FULL   (0x30)
#define SNAND_STATUS_ECC_STATUS_MASK        (0x30)
#define SNAND_STATUS_ERROR_BITS_CORRECTED   (0x10)

// OTP register
#define SNAND_OTP_ECC_ENABLE                (0x10)
#define SNAND_OTP_QE                        (0x01)

// Block lock register
#define SNAND_BLOCK_LOCK_BITS               (0x3E)

#define SNF_BASE                            (NFI_BASE)    // Serial Flash Interface                   //[stanley] done

// SNF registers
#define RW_SNAND_MAC_CTL              ((P_U32)(SNF_BASE + 0x0500))
#define RW_SNAND_MAC_OUTL             ((P_U32)(SNF_BASE + 0x0504))
#define RW_SNAND_MAC_INL              ((P_U32)(SNF_BASE + 0x0508))

#define RW_SNAND_RD_CTL1              ((P_U32)(SNF_BASE + 0x050C))
#define RW_SNAND_RD_CTL2              ((P_U32)(SNF_BASE + 0x0510))
#define RW_SNAND_RD_CTL3              ((P_U32)(SNF_BASE + 0x0514))

#define RW_SNAND_GF_CTL1              ((P_U32)(SNF_BASE + 0x0518))
#define RW_SNAND_GF_CTL3              ((P_U32)(SNF_BASE + 0x0520))

#define RW_SNAND_PG_CTL1              ((P_U32)(SNF_BASE + 0x0524))
#define RW_SNAND_PG_CTL2              ((P_U32)(SNF_BASE + 0x0528))
#define RW_SNAND_PG_CTL3              ((P_U32)(SNF_BASE + 0x052C))

#define RW_SNAND_ER_CTL               ((P_U32)(SNF_BASE + 0x0530))
#define RW_SNAND_ER_CTL2              ((P_U32)(SNF_BASE + 0x0534))

#define RW_SNAND_MISC_CTL             ((P_U32)(SNF_BASE + 0x0538))
#define RW_SNAND_MISC_CTL2            ((P_U32)(SNF_BASE + 0x053C))

#define RW_SNAND_DLY_CTL1             ((P_U32)(SNF_BASE + 0x0540))
#define RW_SNAND_DLY_CTL2             ((P_U32)(SNF_BASE + 0x0544))
#define RW_SNAND_DLY_CTL3             ((P_U32)(SNF_BASE + 0x0548))
#define RW_SNAND_DLY_CTL4             ((P_U32)(SNF_BASE + 0x054C))

#define RW_SNAND_STA_CTL1             ((P_U32)(SNF_BASE + 0x0550))
#define RW_SNAND_STA_CTL2             ((P_U32)(SNF_BASE + 0x0554))
#define RW_SNAND_STA_CTL3             ((P_U32)(SNF_BASE + 0x0558))
#define RW_SNAND_CNFG                 ((P_U32)(SNF_BASE + 0x055C))


#define RW_SNAND_GPRAM_DATA           ((P_U32)(SNF_BASE + 0x0800))

// RW_SNAND_DLY_CTL2
#define SNAND_SFIO0_IN_DLY_MASK       (0x0000003F)
#define SNAND_SFIO1_IN_DLY_MASK       (0x00003F00)
#define SNAND_SFIO2_IN_DLY_MASK       (0x003F0000)
#define SNAND_SFIO3_IN_DLY_MASK       (0x3F000000)

// RW_SNAND_DLY_CTL3
#define SNAND_SFCK_OUT_DLY_MASK       (0x00003F00)
#define SNAND_SFCK_OUT_DLY_OFFSET     (8)
#define SNAND_SFCK_SAM_DLY_MASK       (0x0000003F)
#define SNAND_SFCK_SAM_DLY_OFFSET     (0)
#define SNAND_SFIFO_WR_EN_DLY_SEL_MASK (0x3F000000)

// RW_SNAND_RD_CTL1
#define SNAND_PAGE_READ_CMD_OFFSET    (24)
#define SNAND_PAGE_READ_ADDRESS_MASK  (0x00FFFFFF)

// RW_SNAND_RD_CTL2
#define SNAND_DATA_READ_DUMMY_OFFSET  (8)
#define SNAND_DATA_READ_CMD_MASK      (0x000000FF)

// RW_SNAND_RD_CTL3
#define SNAND_DATA_READ_ADDRESS_MASK  (0x0000FFFF)

// RW_SNAND_MISC_CTL
#define SNAND_DATA_READ_MODE_X1       (0x0)
#define SNAND_DATA_READ_MODE_X4       (0x2)
#define SNAND_CLK_INVERSE             (0x20)
#define SNAND_SAMPLE_CLK_INVERSE      (1 << 22)
#define SNAND_4FIFO_EN                (1 << 24)
#define SNAND_DATA_READ_MODE_OFFSET   (16)
#define SNAND_DATA_READ_MODE_MASK     (0x00070000)
#define SNAND_FIFO_RD_LTC_MASK        (0x06000000)
#define SNAND_FIFO_RD_LTC_OFFSET      (25)
#define SNAND_FIFO_RD_LTC_0           (0)
#define SNAND_FIFO_RD_LTC_2           (2)
#define SNAND_PG_LOAD_X4_EN           (1 << 20)
#define SNAND_DATARD_CUSTOM_EN        (0x00000040)
#define SNAND_PG_LOAD_CUSTOM_EN       (0x00000080)
#define SNAND_SW_RST                  (0x10000000)
#define SNAND_LATCH_LAT_MASK          (0x00000300)
#define SNAND_LATCH_LAT_OFFSET        (8)

// RW_SNAND_MISC_CTL2
#define SNAND_PROGRAM_LOAD_BYTE_LEN_OFFSET    (16)
#define SNAND_READ_DATA_BYTE_LEN_OFFSET       (0)

// RW_SNAND_PG_CTL1
#define SNAND_PG_EXE_CMD_OFFSET               (16)
#define SNAND_PG_LOAD_CMD_OFFSET              (8)
#define SNAND_PG_WRITE_EN_CMD_OFFSET          (0)

// RW_SNAND_PG_CTL2
#define SNAND_PG_LOAD_CMD_DUMMY_OUT_OFFSET    (12)
#define SNAND_PG_LOAD_ADDR_MASK               (0x0000FFFF)

// RW_SNAND_GF_CTL1
#define SNAND_GF_STATUS_MASK          (0x000000FF)

// RW_SNAND_GF_CTL3
#define SNAND_GF_LOOP_LIMIT_MASK      (0x000F0000)
#define SNAND_GF_POLLING_CYCLE_MASK   (0x0000FFFF)
#define SNAND_GF_LOOP_LIMIT_OFFSET    (16)

// RW_SNAND_STA_CTL1
#define SNAND_AUTO_BLKER              (0x01000000)
#define SNAND_AUTO_READ               (0x02000000)
#define SNAND_AUTO_PROGRAM            (0x04000000)
#define SNAND_CUSTOM_READ             (0x08000000)
#define SNAND_CUSTOM_PROGRAM          (0x10000000)

// RW_SNAND_STA_CTL2
#define SNAND_DATARD_BYTE_CNT_OFFSET  (16)
#define SNAND_DATARD_BYTE_CNT_MASK    (0x1FFF0000)

// RW_SNAND_MAC_CTL
#define SNAND_WIP                     (0x00000001)  // b0
#define SNAND_WIP_READY               (0x00000002)  // b1
#define SNAND_TRIG                    (0x00000004)  // b2
#define SNAND_MAC_EN                  (0x00000008)  // b3
#define SNAND_MAC_SIO_SEL             (0x00000010)  // b4

// RW_SNAND_DIRECT_CTL
#define SNAND_QPI_EN                  (0x00000001)  // b0
#define SNAND_CMD1_EXTADDR_EN         (0x00000002)  // b1
#define SNAND_CMD2_EN                 (0x00000004)  // b2
#define SNAND_CMD2_EXTADDR_EN         (0x00000008)  // b3
#define SNAND_DR_MODE_MASK            (0x00000070)  // b4~b6
#define SNAND_NO_RELOAD               (0x00000080)  // b7
#define SNAND_DR_CMD2_DUMMY_CYC_MASK  (0x00000F00)  // b8~b11
#define SNAND_DR_CMD1_DUMMY_CYC_MASK  (0x0000F000)  // b12~b15
#define SNAND_DR_CMD2_DUMMY_CYC_OFFSET         (8)
#define SNAND_DR_CMD1_DUMMY_CYC_OFFSET        (12)
#define SNAND_DR_CMD2_MASK            (0x00FF0000)  // b16~b23
#define SNAND_DR_CMD1_MASK            (0xFF000000)  // b24~b31
#define SNAND_DR_CMD2_OFFSET                  (16)
#define SNAND_DR_CMD1_OFFSET                  (24)

// RW_SNAND_ER_CTL
#define SNAND_ER_CMD_OFFSET                    (8)
#define SNAND_ER_CMD_MASK             (0x0000FF00)
#define SNAND_AUTO_ERASE_TRIGGER      (0x00000001)

// RW_SNAND_GF_CTL3
#define SNAND_LOOP_LIMIT_OFFSET               (16)
#define SNAND_LOOP_LIMIT_MASK         (0x000F0000)
#define SNAND_LOOP_LIMIT_NO_LIMIT            (0xF)
#define SNAND_POLLING_CYCLE_MASK      (0x0000FFFF)

// SFI Non-Volitile Register
#define SNAND_WDT_DEV_BUSY              0x00000001    // (b0)
#define SNAND_WDT_DEV_SUSPEND           0x00000002    // (b1)
#define SNAND_WDT_DEV_WAIT_TIME_MASK    0x0000F000    // (b15-b12)
#define SNAND_WDT_GET_DEV_WAIT_TIME(a)  (((a)&SNAND_WDT_DEV_WAIT_TIME_MASK)>>12)

//----------------------------------------
// NFI related SW configurations
//----------------------------------------
#define SNAND_NFI_FDM_SIZE            (8)
#define SNAND_NFI_FDM_ECC_SIZE        (8)

//----------------------------------------
// Advanced Mode
//----------------------------------------
#define SNAND_ADV_READ_SPLIT                (0x00000001)
#define SNAND_ADV_VENDOR_RESERVED_BLOCKS    (0x00000002)

//----------------------------------------
// Spare Format
//----------------------------------------
#define SNAND_SPARE_FORMAT_1            (0x00000001)

//----------------------------------------
// Exported APIs
//----------------------------------------



#endif	// CONFIG_MTK_SPI_NAND_SUPPORT

#endif	// __MTK_SNAND_K_H__

