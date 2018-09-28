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
#include <serial.h>
#include <common.h>
#include <config.h>

#include <asm/arch/typedefs.h>
#include <asm/arch/mt6735.h>
#include <asm/arch/uart.h> 

#define UART_BASE(uart)             (uart)

#define UART_RBR(uart)              (UART_BASE(uart)+0x0)       /* Read only */
#define UART_THR(uart)              (UART_BASE(uart)+0x0)       /* Write only */
#define UART_IER(uart)              (UART_BASE(uart)+0x4)
#define UART_IIR(uart)              (UART_BASE(uart)+0x8)       /* Read only */
#define UART_FCR(uart)              (UART_BASE(uart)+0x8)       /* Write only */
#define UART_LCR(uart)              (UART_BASE(uart)+0xc)
#define UART_MCR(uart)              (UART_BASE(uart)+0x10)
#define UART_LSR(uart)              (UART_BASE(uart)+0x14)
#define UART_MSR(uart)              (UART_BASE(uart)+0x18)
#define UART_SCR(uart)              (UART_BASE(uart)+0x1c)
#define UART_DLL(uart)              (UART_BASE(uart)+0x0)       
#define UART_DLH(uart)              (UART_BASE(uart)+0x4)       
#define UART_EFR(uart)              (UART_BASE(uart)+0x8)       
#define UART_XON1(uart)             (UART_BASE(uart)+0x10)      
#define UART_XON2(uart)             (UART_BASE(uart)+0x14)      
#define UART_XOFF1(uart)            (UART_BASE(uart)+0x18)      
#define UART_XOFF2(uart)            (UART_BASE(uart)+0x1c)      
#define UART_AUTOBAUD_EN(uart)      (UART_BASE(uart)+0x20)
#define UART_HIGHSPEED(uart)        (UART_BASE(uart)+0x24)
#define UART_SAMPLE_COUNT(uart)     (UART_BASE(uart)+0x28)
#define UART_SAMPLE_POINT(uart)     (UART_BASE(uart)+0x2c)
#define UART_AUTOBAUD_REG(uart)     (UART_BASE(uart)+0x30)
#define UART_RATE_FIX_AD(uart)      (UART_BASE(uart)+0x34)
#define UART_AUTOBAUD_SAMPLE(uart)  (UART_BASE(uart)+0x38)
#define UART_GUARD(uart)            (UART_BASE(uart)+0x3c)
#define UART_ESCAPE_DAT(uart)       (UART_BASE(uart)+0x40)
#define UART_ESCAPE_EN(uart)        (UART_BASE(uart)+0x44)
#define UART_SLEEP_EN(uart)         (UART_BASE(uart)+0x48)
#define UART_VFIFO_EN(uart)         (UART_BASE(uart)+0x4c)
#define UART_RXTRI_AD(uart)         (UART_BASE(uart)+0x50)


#define UART_SET_BITS(BS,REG)       ((*(volatile U32*)(REG)) |= (U32)(BS))
#define UART_CLR_BITS(BS,REG)       ((*(volatile U32*)(REG)) &= ~((U32)(BS)))
#define UART_WRITE16(VAL, REG)      DRV_WriteReg(REG,VAL)
#define UART_READ32(REG)            DRV_Reg32(REG)
#define UART_WRITE32(VAL, REG)      DRV_WriteReg32(REG,VAL)

#if 1//CFG_FPGA_PLATFORM  //mtk40224
volatile unsigned int g_uart = UART1;
#define UART_SRC_CLK  FPGA_UART_CLOCK
#else
volatile unsigned int g_uart = UART3;
#define UART_SRC_CLK  EVB_UART_CLOCK
#endif

// output uart baudrate
unsigned int g_brg = CONFIG_BAUDRATE;

void mtk_uart_power_on(MT65XX_UART uart_base)
{
	/* UART Powr PDN and Reset*/
    //#define AP_PERI_GLOBALCON_RST0 (PERI_CON_BASE+0x0)
    #define AP_PERI_GLOBALCON_PDN0 (INFRACFG_AO_BASE+0x084)
    if (uart_base == UART1)
        UART_SET_BITS(1 << 22, AP_PERI_GLOBALCON_PDN0); /* Power on UART1 */
    else if (uart_base == UART2)
        UART_SET_BITS(1 << 23, AP_PERI_GLOBALCON_PDN0); /* Power on UART2 */
	else if (uart_base == UART3)
        UART_SET_BITS(1 << 24, AP_PERI_GLOBALCON_PDN0); /* Power on UART2 */
	else if (uart_base == UART4)
        UART_SET_BITS(1 << 25, AP_PERI_GLOBALCON_PDN0); /* Power on UART2 */
    return;
}


void uart_setbrg(void)
{
    U32 uartclk = UART_SRC_CLK;
    U32 baudrate = g_brg;

#if 1//CFG_FPGA_PLATFORM  //mtk40224
    #define MAX_SAMPLE_COUNT 256
    
    U16 tmp;
    U32 divisor;
    U32 sample_data;
    U32 sample_count;
    U32 sample_point;    

    // Setup N81,(UART_WLS_8 | UART_NONE_PARITY | UART_1_STOP) = 0x03
    UART_WRITE32(0x0003, UART_LCR(g_uart));

   /*
    * NoteXXX: Below is the sample code to set UART baud rate.
    *          I assume that when system is reset, the UART clock rate is 26MHz 
    *          and baud rate is 115200.
    *          use UART1_HIGHSPEED = 0x3 can get more sample count to get better UART sample rate    
    *          based on baud_rate = uart clock frequency / (sampe_count * divisor)
    *          divisor = (DLH+DLL)
    */         

    // In order to get better UART sample rate, set UART1_HIGHSPEED = 0x3. 
    // And we can calculate sample count for reducing effect of UART sample rate variation
    UART_WRITE32(0x0003, UART_HIGHSPEED(g_uart));

    // calculate sample_data = sample_count*divisor
    // round off the result for approximating to the real baudrate  
    sample_data = (uartclk+(baudrate/2))/baudrate;
    // calculate divisor  
    divisor = (sample_data+(MAX_SAMPLE_COUNT-1))/MAX_SAMPLE_COUNT;
    // calculate sample count
    sample_count = sample_data/divisor;
    // calculate sample point (count from 0) 
    sample_point = (sample_count-1)/2;
    // set sample count (count from 0)        
    UART_WRITE32((sample_count-1), UART_SAMPLE_COUNT(g_uart));
    // set sample point 
    UART_WRITE32(sample_point, UART_SAMPLE_POINT(g_uart));
    
    tmp = UART_READ32(UART_LCR(g_uart));        /* DLAB start */ 
    UART_WRITE32((tmp | UART_LCR_DLAB), UART_LCR(g_uart));   

    UART_WRITE32((divisor&0xFF), UART_DLL(g_uart));
    UART_WRITE32(((divisor>>8)&0xFF), UART_DLH(g_uart));
    UART_WRITE32(tmp, UART_LCR(g_uart));
    
#else
    unsigned int byte;
    unsigned int highspeed;
    unsigned int quot, divisor, remainder;

    if (baudrate <= 115200 ) {
        highspeed = 0;
        quot = 16;
    } else {
        highspeed = 2;
        quot = 4;
    }

    /* Set divisor DLL and DLH  */
    divisor   =  uartclk / (quot * baudrate);
    remainder =  uartclk % (quot * baudrate);

    if (remainder >= (quot / 2) * baudrate)
        divisor += 1;

    UART_WRITE16(highspeed, UART_HIGHSPEED(g_uart));
    byte = UART_READ32(UART_LCR(g_uart));     /* DLAB start */
    UART_WRITE32((byte | UART_LCR_DLAB), UART_LCR(g_uart));
    UART_WRITE32((divisor & 0x00ff), UART_DLL(g_uart));
    UART_WRITE32(((divisor >> 8)&0x00ff), UART_DLH(g_uart));
    //UART_WRITE32(byte, UART_LCR(g_uart));     /* DLAB end */
    // Setup N81,(UART_WLS_8 | UART_NONE_PARITY | UART_1_STOP) = 0x03
    UART_WRITE32(0x0003, UART_LCR(g_uart));    
#endif
}

int serial_nonblock_getc(void)
{
    return (int)UART_READ32(UART_RBR(g_uart));
}

void mtk_serial_set_current_uart(MT65XX_UART uart_base)
{
    g_uart = uart_base;
}

int mtk_uart_init (void)
{
// Iverson 20150522 : GPIO setting is defined in preloader.
#if 0
#if !CFG_FPGA_PLATFORM
	#ifdef GPIO_UART_UTXD0_PIN
    mt_set_gpio_mode(GPIO_UART_UTXD0_PIN, GPIO_UART_UTXD0_PIN_M_UTXD);
    mt_set_gpio_dir(GPIO_UART_UTXD0_PIN, GPIO_DIR_OUT);
    #endif

    #ifdef GPIO_UART_URXD0_PIN
    mt_set_gpio_mode(GPIO_UART_URXD0_PIN, GPIO_UART_URXD0_PIN_M_URXD);
    mt_set_gpio_dir(GPIO_UART_URXD0_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_UART_URXD0_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_UART_URXD0_PIN, GPIO_PULL_UP); 
    #endif

    #ifdef GPIO_UART_UTXD1_PIN
    mt_set_gpio_mode(GPIO_UART_UTXD1_PIN, GPIO_UART_UTXD1_PIN_M_UTXD);
    mt_set_gpio_dir(GPIO_UART_UTXD1_PIN, GPIO_DIR_OUT);
    #endif

    #ifdef GPIO_UART_URXD1_PIN
    mt_set_gpio_mode(GPIO_UART_URXD1_PIN, GPIO_UART_URXD1_PIN_M_URXD);
    mt_set_gpio_dir(GPIO_UART_URXD1_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_UART_URXD1_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_UART_URXD1_PIN, GPIO_PULL_UP); 
    #endif
    
    #ifdef GPIO_UART_UTXD2_PIN
    mt_set_gpio_mode(GPIO_UART_UTXD2_PIN, GPIO_UART_UTXD2_PIN_M_UTXD);
    mt_set_gpio_dir(GPIO_UART_UTXD2_PIN, GPIO_DIR_OUT);
    #endif

    #ifdef GPIO_UART_URXD2_PIN
    mt_set_gpio_mode(GPIO_UART_URXD2_PIN, GPIO_UART_URXD2_PIN_M_URXD);
    mt_set_gpio_dir(GPIO_UART_URXD2_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_UART_URXD2_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_UART_URXD2_PIN, GPIO_PULL_UP); 
    #endif
    
    #ifdef GPIO_UART_UTXD3_PIN
    mt_set_gpio_mode(GPIO_UART_UTXD3_PIN, GPIO_UART_UTXD3_PIN_M_UTXD);
    mt_set_gpio_dir(GPIO_UART_UTXD3_PIN, GPIO_DIR_OUT);
    #endif

    #ifdef GPIO_UART_URXD3_PIN
    mt_set_gpio_mode(GPIO_UART_URXD3_PIN, GPIO_UART_URXD3_PIN_M_URXD);
    mt_set_gpio_dir(GPIO_UART_URXD3_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_UART_URXD3_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_UART_URXD3_PIN, GPIO_PULL_UP); 
    #endif
#endif

    /* uartclk != 0, means use custom bus clock; uartclk == 0, means use defaul bus clk */
    if(0 == uartclk){ // default bus clk 
        //uartclk = mtk_get_bus_freq()*1000/4;
	uartclk = UART_SRC_CLK;
    }
    
#if (CFG_OUTPUT_PL_LOG_TO_UART1	&& !(defined(HW_INIT_ONLY) || defined(SLT) || defined(DUMMY_AP) || defined(TINY)))
	// init UART1 
    mtk_serial_set_current_uart(UART2);
       
    UART_SET_BITS(UART_FCR_FIFO_INIT, UART_FCR(g_uart)); /* clear fifo */
    UART_WRITE16(UART_NONE_PARITY | UART_WLS_8 | UART_1_STOP, UART_LCR(g_uart));
    serial_setbrg(uartclk, CFG_LOG_BAUDRATE);
    

	//set GPIO for UART1
    #ifdef GPIO_UART_UTXD1_PIN
    mt_set_gpio_mode(GPIO_UART_UTXD1_PIN, GPIO_UART_URXD1_PIN_M_URXD);
    mt_set_gpio_dir(GPIO_UART_UTXD1_PIN, GPIO_DIR_OUT);
    #endif
    
    #ifdef GPIO_UART_URXD1_PIN
    mt_set_gpio_mode(GPIO_UART_URXD1_PIN, GPIO_UART_URXD1_PIN_M_URXD);
    mt_set_gpio_dir(GPIO_UART_URXD1_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_UART_URXD1_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_UART_URXD1_PIN, GPIO_PULL_UP); 
    #endif
#endif //#if CFG_OUTPUT_PL_LOG_TO_UART1

    mtk_serial_set_current_uart(CFG_UART_META);
    
    #if !(CFG_FPGA_PLATFORM)
    mtk_uart_power_on(CFG_UART_META);
    #endif

    UART_SET_BITS(UART_FCR_FIFO_INIT, UART_FCR(g_uart)); /* clear fifo */
    UART_WRITE16(UART_NONE_PARITY | UART_WLS_8 | UART_1_STOP, UART_LCR(g_uart));
    serial_setbrg(uartclk, CFG_META_BAUDRATE);


    mtk_serial_set_current_uart(CFG_UART_LOG);
    
    #if !(CFG_FPGA_PLATFORM)
    mtk_uart_power_on(CFG_UART_LOG);
    #endif
#endif

    UART_SET_BITS(UART_FCR_FIFO_INIT, UART_FCR(g_uart)); /* clear fifo */
    UART_WRITE16(UART_NONE_PARITY | UART_WLS_8 | UART_1_STOP, UART_LCR(g_uart));
    uart_setbrg();

    return 0;
}

void PutUARTByte (const char c)
{
    while (!(UART_READ32 (UART_LSR(g_uart)) & UART_LSR_THRE))
    {
    }

    if (c == '\n')
        UART_WRITE32 ((unsigned int) '\r', UART_THR(g_uart));

    UART_WRITE32 ((unsigned int) c, UART_THR(g_uart));
}

int uart_getc(void)  /* returns -1 if no data available */
{
	while (!(DRV_Reg32(UART_LSR(g_uart)) & UART_LSR_DR)); 	
 	return (int)DRV_Reg32(UART_RBR(g_uart));
}

void uart_puts(const char *s)
{
	while (*s)
		PutUARTByte(*s++);
}

int uart_tstc(void)
{
    return (DRV_Reg32(UART_LSR(g_uart)) & UART_LSR_DR);
}

static struct serial_device mt7622_uart_drv = {
	.name	= "mt7622_uart",
	.start	= mtk_uart_init,
	.stop	= NULL,
	.setbrg	= uart_setbrg,
	.putc	= PutUARTByte,
	.puts	= uart_puts,
	.getc	= uart_getc,
	.tstc	= uart_tstc,
};

void pl01x_serial_initialize(void)
{
	serial_register(&mt7622_uart_drv);
}

struct serial_device *default_serial_console(void)
{
	return &mt7622_uart_drv;
}



