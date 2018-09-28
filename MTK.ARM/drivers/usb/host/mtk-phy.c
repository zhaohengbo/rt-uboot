
#include <malloc.h>

#define U3_PHY_LIB
#include "mtk-phy.h"
#ifdef CONFIG_PROJECT_7623
#include "mtk-phy-7623.h"
#endif
#ifdef CONFIG_PROJECT_PHY


static struct u3phy_operator project_operators = {
	.init = mt7623_phy_init,
	.change_pipe_phase = phy_change_pipe_phase,
	.eyescan_init = eyescan_init,
	.eyescan = phy_eyescan,
	.u2_slew_rate_calibration = u2_slew_rate_calibration,
};
#endif


PHY_INT32 u3phy_init(){
#ifndef CONFIG_PROJECT_PHY
	PHY_INT32 u3phy_version;
#endif
	
	//if (u3phy != NULL)
	//	return PHY_TRUE;

	u3phy = malloc(sizeof(struct u3phy_info));

	if (u3phy == NULL){
		printf("u3phy == NULL \n");
		return PHY_FALSE;
	}


//#if defined (CONFIG_RALINK_MT7621)
//	u3phy_p1 = malloc(sizeof(struct u3phy_info));
//	if (u3phy_p1 == NULL){
//		printf("u3phy_p1 == NULL \n");
//		return PHY_FALSE;
//	}
//#endif

	u3phy->phyd_version_addr = U3_PHYD_B2_BASE + 0xe4;
//#if defined (CONFIG_RALINK_MT7621)
//	u3phy_p1->phyd_version_addr = U3_PHYD_B2_BASE_P1 + 0xe4;
//#endif

//#ifdef CONFIG_ARCH_MT7623
#if CFG_DEV_U3H0
	u3phy->u2phy_regs = (struct u2phy_reg *)U2_PHY_BASE;
	u3phy->u3phyd_regs = (struct u3phyd_reg *)U3_PHYD_BASE;
	u3phy->u3phyd_bank2_regs = (struct u3phyd_bank2_reg *)U3_PHYD_B2_BASE;
	u3phy->u3phya_regs = (struct u3phya_reg *)U3_PHYA_BASE;
	u3phy->u3phya_da_regs = (struct u3phya_da_reg *)U3_PHYA_DA_BASE;
	u3phy->sifslv_chip_regs = (struct sifslv_chip_reg *)SIFSLV_CHIP_BASE;		
	u3phy->sifslv_fm_regs = (struct sifslv_fm_feg *)SIFSLV_FM_FEG_BASE;	
#endif

#if CFG_DEV_U3H1
	u3phy->u2phy_regs_p1 = (struct u2phy_reg *)U2_PHY_BASE_P1;
	u3phy->u3phyd_regs_p1 = (struct u3phyd_reg *)U3_PHYD_BASE_P1;
	u3phy->u3phyd_bank2_regs_p1 = (struct u3phyd_bank2_reg *)U3_PHYD_B2_BASE_P1;
	u3phy->u3phya_regs_p1 = (struct u3phya_reg *)U3_PHYA_BASE_P1;
	u3phy->u3phya_da_regs_p1 = (struct u3phya_da_reg *)U3_PHYA_DA_BASE_P1;
	u3phy->sifslv_chip_regs_p1 = (struct sifslv_chip_reg *)SIFSLV_CHIP_BASE_P1;		
	u3phy->sifslv_fm_regs_p1 = (struct sifslv_fm_feg *)SIFSLV_FM_FEG_BASE_P1;	
#endif
	u3phy_ops = (struct u3phy_operator *)&project_operators;
//#endif

	return PHY_TRUE;
}

PHY_INT32 U3PhyWriteField8(PHY_INT32 addr, PHY_INT32 offset, PHY_INT32 mask, PHY_INT32 value){
	PHY_INT8 cur_value;
	PHY_INT8 new_value;

	cur_value = U3PhyReadReg8(addr);
	new_value = (cur_value & (~mask)) | (value << offset);
	//udelay(i2cdelayus);
	//printf("write addr=%lx, cur_val=%lx, new_val=%lx\n", addr, cur_value, new_value);
	U3PhyWriteReg8(addr, new_value);
	return PHY_TRUE;
}

PHY_INT32 U3PhyWriteField32(PHY_INT32 addr, PHY_INT32 offset, PHY_INT32 mask, PHY_INT32 value){
	PHY_INT32 cur_value;
	PHY_INT32 new_value;

	cur_value = U3PhyReadReg32(addr);
	new_value = (cur_value & (~mask)) | ((value << offset) & mask);
	//printf("write addr=%lx, cur_val=%lx, new_val=%lx\n", addr, cur_value, new_value);
	U3PhyWriteReg32(addr, new_value);
	//DRV_MDELAY(100);

	return PHY_TRUE;
}

PHY_INT32 U3PhyReadField8(PHY_INT32 addr,PHY_INT32 offset,PHY_INT32 mask){
	//printf("read addr=%lx, cur_val=%lx\n", addr, (U3PhyReadReg8(addr) & mask) >> offset);
	
	return ((U3PhyReadReg8(addr) & mask) >> offset);
}

PHY_INT32 U3PhyReadField32(PHY_INT32 addr, PHY_INT32 offset, PHY_INT32 mask){

	//printf("read addr=%lx, cur_val=%lx\n", addr, (U3PhyReadReg8(addr) & mask) >> offset);
	return ((U3PhyReadReg32(addr) & mask) >> offset);
}

