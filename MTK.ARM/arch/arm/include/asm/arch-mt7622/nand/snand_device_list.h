
#ifndef __SNAND_DEVICE_LIST_H__
#define __SNAND_DEVICE_LIST_H__

#define SNAND_MAX_ID		2
#define SNAND_CHIP_CNT		2

typedef struct
{
    u8 id[SNAND_MAX_ID];
    u8 id_length;
    u16 totalsize;
    u16 blocksize;
    u16 pagesize;
    u16 sparesize;
    u32 SNF_DLY_CTL1;
    u32 SNF_DLY_CTL2;
    u32 SNF_DLY_CTL3;
    u32 SNF_DLY_CTL4;
    u32 SNF_MISC_CTL;
    u32 SNF_DRIVING;
    u8 devicename[30];
    u32 advancedmode;
}snand_flashdev_info,*psnandflashdev_info;

static const snand_flashdev_info gen_snand_FlashTable[]={
	{{0xC8,0xF4}, 2,512,128,2048,112,0x00000000,0x00000000,0x1A00001A,0x00000000,0x0552000A ,0x01,"GD5F4GQ4UAYIG",0x00000003}, 
	//{{0xEF,0xAA}, 2,128,128,2048,64,0x00000000,0x00000000,0x1A00001A,0x00000000,0x0552000A ,0x01,"Winbond 1Gb",0x00000000}, // Winbond 1Gb
	{{0xEF,0xAA}, 2,64,128,2048,64,0x00000000,0x00000000,0x1A00001A,0x00000000,0x0552000A ,0x01,"Winbond 512Mb",0x00000000}, // Winbond 512Mb
};

#endif
