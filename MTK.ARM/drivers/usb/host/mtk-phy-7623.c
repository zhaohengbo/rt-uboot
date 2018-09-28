#include <common.h>

#include "mtk-phy.h"

#ifdef CONFIG_PROJECT_7623
#include "mtk-phy-7623.h"

bool phy_init_flag = false;
bool slewrate_cal_flag = false;

PHY_INT32 mt7623_phy_init(struct u3phy_info *info)
{
	PHY_INT32 temp;

	if (phy_init_flag == false) {
#if CFG_DEV_U3H0
		printf("=== USB port0 phy init ===\n");
		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->usbphyacr6)
			, RG_USB20_BC11_SW_EN_OFST, RG_USB20_BC11_SW_EN, 0x0);

		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->usbphyacr0)
			, RG_USB20_INTR_EN_OFST, RG_USB20_INTR_EN, 0x1);

		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->usbphyacr5)
			, RG_USB20_HS_100U_U3_EN_OFST, RG_USB20_HS_100U_U3_EN, 0x1);

		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phyacr4)
			, RG_USB20_DP_100K_EN_OFST, RG_USB20_DP_100K_EN, 0x0);

		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phyacr4)
			, RG_USB20_DM_100K_EN_OFST, RG_USB20_DM_100K_EN, 0x0);

		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->usbphyacr6)
			, RG_USB20_OTG_VBUSCMP_EN_OFST, RG_USB20_OTG_VBUSCMP_EN, 0x0);

		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phydtm0)
			, FORCE_SUSPENDM_OFST, FORCE_SUSPENDM, 0x0);
#endif

#if CFG_DEV_U3H1
		printf("=== USB port1 phy init ===\n");
		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->usbphyacr6)
			, RG_USB20_BC11_SW_EN_OFST, RG_USB20_BC11_SW_EN, 0x0);

		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->usbphyacr0)
			, RG_USB20_INTR_EN_OFST, RG_USB20_INTR_EN, 0x1);

		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->usbphyacr5)
			, RG_USB20_HS_100U_U3_EN_OFST, RG_USB20_HS_100U_U3_EN, 0x1);

		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phyacr4)
			, RG_USB20_DP_100K_EN_OFST, RG_USB20_DP_100K_EN, 0x0);

		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phyacr4)
			, RG_USB20_DM_100K_EN_OFST, RG_USB20_DM_100K_EN, 0x0);

		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->usbphyacr6)
			, RG_USB20_OTG_VBUSCMP_EN_OFST, RG_USB20_OTG_VBUSCMP_EN, 0x0);

		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phydtm0)
			, FORCE_SUSPENDM_OFST, FORCE_SUSPENDM, 0x0);
#endif

		phy_init_flag = true;
	}

	return PHY_TRUE;
}

PHY_INT32 phy_down(struct u3phy_info *info)
{
#if CFG_DEV_U3H0
	printf("=== USB port0 phy down ===\n");
	// force_uart_en = 0
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phydtm0)
			, FORCE_UART_EN_OFST, FORCE_UART_EN, 0);
	// RG_UART_EN = 0 /* FIXME */
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phydtm1)
			, RG_UART_EN_OFST, RG_UART_EN, 0);
	// rg_usb20_gpio_ctl = 0
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phyacr4)
			, RG_USB20_GPIO_CTL_OFST, RG_USB20_GPIO_CTL, 0);
	// usb20_gpio_mode = 0
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phyacr4)
			, USB20_GPIO_MODE_OFST, USB20_GPIO_MODE, 0);

	// RG_SUSPENDM = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phydtm0)
			, RG_SUSPENDM_OFST, RG_SUSPENDM, 1);
	// force_suspendm = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phydtm0)
			, FORCE_SUSPENDM_OFST, FORCE_SUSPENDM, 1);
	
	mdelay(2);

	// RG_DPPULLDOWN = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phydtm0)
			, RG_DPPULLDOWN_OFST, RG_DPPULLDOWN, 1);
	// RG_DMPULLDOWN = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phydtm0)
			, RG_DMPULLDOWN_OFST, RG_DMPULLDOWN, 1);
	// RG_XCVRSEL[1:0] = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phydtm0)
			, RG_XCVRSEL_OFST, RG_XCVRSEL, 1);
	// RG_TERMSEL = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phydtm0)
			, RG_TERMSEL_OFST, RG_TERMSEL, 1);
	// RG_DATAIN[3:0] = 0
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phydtm0)
			, RG_DATAIN_OFST, RG_DATAIN, 0);
	// force_dp_pulldown = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phydtm0)
			, FORCE_DP_PULLDOWN_OFST, FORCE_DP_PULLDOWN, 1);
	// force_dm_pulldown = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phydtm0)
			, FORCE_DM_PULLDOWN_OFST, FORCE_DM_PULLDOWN, 1);
	// force_xcvrsel = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phydtm0)
			, FORCE_XCVRSEL_OFST, FORCE_XCVRSEL, 1);

	// force_termsel = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phydtm0)
			, FORCE_TERMSEL_OFST, FORCE_TERMSEL, 1);

	// force_datain = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phydtm0)
			, FORCE_DATAIN_OFST, FORCE_DATAIN, 1);

	// RG_USB20_BC11_SW_EN = 0
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->usbphyacr6)
			, RG_USB20_BC11_SW_EN_OFST, RG_USB20_BC11_SW_EN, 0);

	// RG_USB20_OTG_VBUSCMP_EN = 0
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->usbphyacr6)
			, RG_USB20_OTG_VBUSCMP_EN_OFST, RG_USB20_OTG_VBUSCMP_EN, 0);

	udelay(800);

	// RG_USB20_OTG_VBUSCMP_EN = 0
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phydtm0)
			, RG_SUSPENDM_OFST, RG_SUSPENDM, 0);
#endif

#if CFG_DEV_U3H1
	printf("=== USB port1 phy down ===\n");
	// force_uart_en = 0
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phydtm0)
			, FORCE_UART_EN_OFST, FORCE_UART_EN, 0);
	// RG_UART_EN = 0 /* FIXME */
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phydtm1)
			, RG_UART_EN_OFST, RG_UART_EN, 0);
	// rg_usb20_gpio_ctl = 0
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phyacr4)
			, RG_USB20_GPIO_CTL_OFST, RG_USB20_GPIO_CTL, 0);
	// usb20_gpio_mode = 0
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phyacr4)
			, USB20_GPIO_MODE_OFST, USB20_GPIO_MODE, 0);

	// RG_SUSPENDM = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phydtm0)
			, RG_SUSPENDM_OFST, RG_SUSPENDM, 1);
	// force_suspendm = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phydtm0)
			, FORCE_SUSPENDM_OFST, FORCE_SUSPENDM, 1);
	
	mdelay(2);

	// RG_DPPULLDOWN = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phydtm0)
			, RG_DPPULLDOWN_OFST, RG_DPPULLDOWN, 1);
	// RG_DMPULLDOWN = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phydtm0)
			, RG_DMPULLDOWN_OFST, RG_DMPULLDOWN, 1);
	// RG_XCVRSEL[1:0] = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phydtm0)
			, RG_XCVRSEL_OFST, RG_XCVRSEL, 1);
	// RG_TERMSEL = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phydtm0)
			, RG_TERMSEL_OFST, RG_TERMSEL, 1);
	// RG_DATAIN[3:0] = 0
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phydtm0)
			, RG_DATAIN_OFST, RG_DATAIN, 0);
	// force_dp_pulldown = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phydtm0)
			, FORCE_DP_PULLDOWN_OFST, FORCE_DP_PULLDOWN, 1);
	// force_dm_pulldown = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phydtm0)
			, FORCE_DM_PULLDOWN_OFST, FORCE_DM_PULLDOWN, 1);
	// force_xcvrsel = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phydtm0)
			, FORCE_XCVRSEL_OFST, FORCE_XCVRSEL, 1);

	// force_termsel = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phydtm0)
			, FORCE_TERMSEL_OFST, FORCE_TERMSEL, 1);

	// force_datain = 1
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phydtm0)
			, FORCE_DATAIN_OFST, FORCE_DATAIN, 1);

	// RG_USB20_BC11_SW_EN = 0
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->usbphyacr6)
			, RG_USB20_BC11_SW_EN_OFST, RG_USB20_BC11_SW_EN, 0);

	// RG_USB20_OTG_VBUSCMP_EN = 0
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->usbphyacr6)
			, RG_USB20_OTG_VBUSCMP_EN_OFST, RG_USB20_OTG_VBUSCMP_EN, 0);

	udelay(800);

	// RG_USB20_OTG_VBUSCMP_EN = 0
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->u2phydtm0)
			, RG_SUSPENDM_OFST, RG_SUSPENDM, 0);
#endif

	return PHY_TRUE;
}


//not used on SoC
PHY_INT32 phy_change_pipe_phase(struct u3phy_info *info, PHY_INT32 phy_drv, PHY_INT32 pipe_phase)
{
	return PHY_TRUE;
}

PHY_INT32 eyescan_init(struct u3phy_info *info)
{
	return PHY_TRUE;
}

PHY_INT32 phy_eyescan(struct u3phy_info *info, PHY_INT32 x_t1, PHY_INT32 y_t1, PHY_INT32 x_br, PHY_INT32 y_br, PHY_INT32 delta_x, PHY_INT32 delta_y
		, PHY_INT32 eye_cnt, PHY_INT32 num_cnt, PHY_INT32 PI_cal_en, PHY_INT32 num_ignore_cnt)
{
	return PHY_TRUE;
}

#if CFG_DEV_U3H0
PHY_INT32 u2_slew_rate_calibration(struct u3phy_info *info)
{
	PHY_INT32 i=0;
	PHY_INT32 fgRet = 0;	
	PHY_INT32 u4FmOut = 0;	
	PHY_INT32 u4Tmp = 0;

	printf("*** USB port0 slew rate calibration ***\n");
	if (slewrate_cal_flag == false) {
		slewrate_cal_flag = true;

		// => RG_USB20_HSTX_SRCAL_EN = 1
		// enable HS TX SR calibration
		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->usbphyacr5)
			, RG_USB20_HSTX_SRCAL_EN_OFST, RG_USB20_HSTX_SRCAL_EN, 0x1);
		//DRV_MSLEEP(1);
		DRV_UDELAY(1);

		// => RG_FRCK_EN = 1    
		// Enable free run clock
		U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs->fmmonr1)
			, RG_FRCK_EN_OFST, RG_FRCK_EN, 1);

		// MT6290 HS signal quality patch
		// => RG_CYCLECNT = 400
		// Setting cyclecnt =400
		U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs->fmcr0)
			, RG_CYCLECNT_OFST, RG_CYCLECNT, 0x400);

		// => RG_FREQDET_EN = 1
		// Enable frequency meter
		U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs->fmcr0)
			, RG_FREQDET_EN_OFST, RG_FREQDET_EN, 0x1);

		// wait for FM detection done, set 10ms timeout
		for(i=0; i<10; i++){
			// USB_FM_VLD = 1 /* FIXME */
			U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs->fmmonr1)
				, USB_FM_VLD_OFST, USB_FM_VLD, 0x1);

			// => u4FmOut = USB_FM_OUT
			// read FM_OUT
			u4FmOut = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs->fmmonr0));
			//printf("Port0 FM_OUT value: u4FmOut = %d(0x%08X)\n", u4FmOut, u4FmOut);

			// check if FM detection done 
			if (u4FmOut != 0)
			{
				fgRet = 0;
				printf("Port0 FM detection done! loop = %d\n", i);
			
				break;
			}

			fgRet = 1;
			DRV_MSLEEP(1);
		}
		// => RG_FREQDET_EN = 0
		// disable frequency meter
		U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs->fmcr0)
			, RG_FREQDET_EN_OFST, RG_FREQDET_EN, 0);

		// => RG_FRCK_EN = 0
		// disable free run clock
		U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs->fmmonr1)
			, RG_FRCK_EN_OFST, RG_FRCK_EN, 0);

		// => RG_USB20_HSTX_SRCAL_EN = 0
		// disable HS TX SR calibration
		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->usbphyacr5)
			, RG_USB20_HSTX_SRCAL_EN_OFST, RG_USB20_HSTX_SRCAL_EN, 0);
		DRV_MSLEEP(1);

		if(u4FmOut == 0){
			U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->usbphyacr5)
				, RG_USB20_HSTX_SRCTRL_OFST, RG_USB20_HSTX_SRCTRL, 0x4);
		
			fgRet = 1;
		}
		else{
			// set reg = (1024/FM_OUT) * 25 * 0.028 (round to the nearest digits)
			u4Tmp = (((1024 * 25 * U2_SR_COEF_7623) / u4FmOut) + 500) / 1000;
			printf("Port0 SR calibration value u1SrCalVal = %d\n", (PHY_UINT8)u4Tmp);
			U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->usbphyacr5)
				, RG_USB20_HSTX_SRCTRL_OFST, RG_USB20_HSTX_SRCTRL, u4Tmp);
		}
	}

	return fgRet;
}
#endif

#if CFG_DEV_U3H1
PHY_INT32 u2_slew_rate_calibration_p1(struct u3phy_info *info)
{
	PHY_INT32 i=0;
	PHY_INT32 fgRet = 0;	
	PHY_INT32 u4FmOut = 0;	
	PHY_INT32 u4Tmp = 0;

	printf("*** USB port1 slew rate calibration ***\n");
	if (slewrate_cal_flag == false) {
		slewrate_cal_flag = true;

		// => RG_USB20_HSTX_SRCAL_EN = 1
		// enable HS TX SR calibration
		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->usbphyacr5)
			, RG_USB20_HSTX_SRCAL_EN_OFST, RG_USB20_HSTX_SRCAL_EN, 0x1);
		//DRV_MSLEEP(1);
		DRV_UDELAY(1);

		// => RG_FRCK_EN = 1    
		// Enable free run clock
		U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs_p1->fmmonr1)
			, RG_FRCK_EN_OFST, RG_FRCK_EN, 1);

		// MT6290 HS signal quality patch
		// => RG_CYCLECNT = 400
		// Setting cyclecnt =400
		U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs_p1->fmcr0)
			, RG_CYCLECNT_OFST, RG_CYCLECNT, 0x400);

		// => RG_FREQDET_EN = 1
		// Enable frequency meter
		U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs_p1->fmcr0)
			, RG_FREQDET_EN_OFST, RG_FREQDET_EN, 0x1);

		// wait for FM detection done, set 10ms timeout
		for(i=0; i<10; i++){
			// USB_FM_VLD = 1 /* FIXME */
			U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs_p1->fmmonr1)
				, USB_FM_VLD_OFST, USB_FM_VLD, 0x1);

			// => u4FmOut = USB_FM_OUT
			// read FM_OUT
			u4FmOut = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_p1->fmmonr0));
			printf("Port1 FM_OUT value: u4FmOut = %d(0x%08X)\n", u4FmOut, u4FmOut);

			// check if FM detection done 
			if (u4FmOut != 0)
			{
				fgRet = 0;
				printf("Port 1 FM detection done! loop = %d\n", i);
			
				break;
			}

			fgRet = 1;
			DRV_MSLEEP(1);
		}
		// => RG_FREQDET_EN = 0
		// disable frequency meter
		U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs_p1->fmcr0)
			, RG_FREQDET_EN_OFST, RG_FREQDET_EN, 0);

		// => RG_FRCK_EN = 0
		// disable free run clock
		U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs_p1->fmmonr1)
			, RG_FRCK_EN_OFST, RG_FRCK_EN, 0);

		// => RG_USB20_HSTX_SRCAL_EN = 0
		// disable HS TX SR calibration
		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->usbphyacr5)
			, RG_USB20_HSTX_SRCAL_EN_OFST, RG_USB20_HSTX_SRCAL_EN, 0);
		DRV_MSLEEP(1);

		if(u4FmOut == 0){
			U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->usbphyacr5)
				, RG_USB20_HSTX_SRCTRL_OFST, RG_USB20_HSTX_SRCTRL, 0x4);
		
			fgRet = 1;
		}
		else{
			// set reg = (1024/FM_OUT) * 25 * 0.028 (round to the nearest digits)
			u4Tmp = (((1024 * 25 * U2_SR_COEF_7623) / u4FmOut) + 500) / 1000;
			printf("Port1 SR calibration value u1SrCalVal Port1= %d\n", (PHY_UINT8)u4Tmp);
			U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_p1->usbphyacr5)
				, RG_USB20_HSTX_SRCTRL_OFST, RG_USB20_HSTX_SRCTRL, u4Tmp);
		}
	}

	return fgRet;
}
#endif

/*
PHY_INT32 mt7623_phy_init(void)
{
	printf("@@@ mt7623_phy_init @@@\n");
	u3phy_init();
	u3phy_ops->init(u3phy);
	if (u3phy_ops->u2_slew_rate_calibration)
                u3phy_ops->u2_slew_rate_calibration(u3phy);
        else
                printf(KERN_ERR "WARN: PHY doesn't implement port0 u2 slew rate calibration function\n");

#if CFG_DEV_U3H1
	if (u3phy_ops->u2_slew_rate_calibration_p1)
                u3phy_ops->u2_slew_rate_calibration_p1(u3phy);
        else
                printf(KERN_ERR "WARN: PHY doesn't implement port1 u2 slew rate calibration function\n");
#endif

	return PHY_TRUE;
}
EXPORT_SYMBOL_GPL(mt7623_phy_init);
*/
PHY_INT32 mt7623_phy_down(void)
{
	phy_down(u3phy);

	return PHY_TRUE;
}
EXPORT_SYMBOL_GPL(mt7623_phy_down);
#endif
