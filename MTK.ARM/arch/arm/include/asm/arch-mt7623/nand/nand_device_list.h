
#ifndef __NAND_DEVICE_LIST_H__
#define __NAND_DEVICE_LIST_H__

#define NAND_ABTC_ATAG
#define ATAG_FLASH_NUMBER_INFO       0x54430006
#define ATAG_FLASH_INFO       0x54430007

#define NAND_MAX_ID		6
#define CHIP_CNT		17
#define P_SIZE		16384
#define P_PER_BLK		256
#define C_SIZE		8192
#define RAMDOM_READ		(1<<0)
#define CACHE_READ		(1<<1)
#define RAND_TYPE_SAMSUNG 0
#define RAND_TYPE_TOSHIBA 1
#define RAND_TYPE_NONE 2

#define READ_RETRY_MAX 10
struct gFeature
{
	u32 address;
	u32 feature;
};

enum readRetryType
{
	RTYPE_MICRON,
	RTYPE_SANDISK,
	RTYPE_SANDISK_19NM,
	RTYPE_TOSHIBA,
	RTYPE_HYNIX,
	RTYPE_HYNIX_16NM
};

struct gFeatureSet
{
	u8 sfeatureCmd;
	u8 gfeatureCmd;
	u8 readRetryPreCmd;
	u8 readRetryCnt;
	u32 readRetryAddress;
	u32 readRetryDefault;
	u32 readRetryStart;
	enum readRetryType rtype;
	struct gFeature Interface;
	struct gFeature Async_timing;
};

struct gRandConfig
{
	u8 type;
	u32 seed[6];
};

enum pptbl
{
	MICRON_8K,
	HYNIX_8K,
	SANDISK_16K,
	PPTBL_NOT_SUPPORT,
};

struct MLC_feature_set
{
	enum pptbl ptbl_idx;
	struct gFeatureSet 	 FeatureSet;
	struct gRandConfig   randConfig;
};

enum flashdev_vendor
{
	VEND_SAMSUNG,
	VEND_MICRON,
	VEND_TOSHIBA,
	VEND_HYNIX,
	VEND_SANDISK,
	VEND_BIWIN,
    VEND_SPANSION,
    VEND_MXIC,
    VEND_WINBOND,
	VEND_NONE,
};

enum flashdev_IOWidth
{
	IO_8BIT = 8,
	IO_16BIT = 16,
	IO_TOGGLEDDR = 9,
	IO_TOGGLESDR = 10,
	IO_ONFI = 12,
};

typedef struct
{
   u8 id[NAND_MAX_ID];
   u8 id_length;
   u8 addr_cycle;
   enum flashdev_IOWidth iowidth;
   u16 totalsize;
   u16 blocksize;
   u16 pagesize;
   u16 sparesize;
   u32 timmingsetting;
   u32 s_acccon;
   u32 s_acccon1;
   u32 freq;
   enum flashdev_vendor vendor;
   u16 sectorsize;
   u8 devciename[30];
   u32 advancedmode;
   struct MLC_feature_set feature_set;
}flashdev_info,*pflashdev_info;

static const flashdev_info gen_FlashTable[]={		
	{{0xEF,0xF1,0x80,0x95,0x00, 0x00}, 5,5,IO_8BIT,128,128,2048,64,0x30c77fff, 0xC03222,0x101,80,VEND_WINBOND,1024, "W29N01GV",0 ,
      {PPTBL_NOT_SUPPORT, {0xEF,0xEE,0xFF,16,0x11,0,1,RTYPE_MICRON,{0x80, 0x00},{0x80, 0x01}},
	          {RAND_TYPE_NONE,{0x2D2D,1,1,1,1,1}}}},
              
	{{0xEF,0xDA,0x90,0x95,0x04, 0x00}, 5,5,IO_8BIT,256,128,2048,64,0x30c77fff, 0xC03222,0x101,80,VEND_WINBOND,1024, "W29N02GV",0 ,
      {PPTBL_NOT_SUPPORT, {0xEF,0xEE,0xFF,16,0x11,0,1,RTYPE_MICRON,{0x80, 0x00},{0x80, 0x01}},
	          {RAND_TYPE_NONE,{0x2D2D,1,1,1,1,1}}}},

	 {{0xEF,0xDC,0x90,0x95,0x54, 0x00}, 5,5,IO_8BIT,512,128,2048,64,0x30c77fff, 0xC03222,0x101,80,VEND_WINBOND,1024, "W29N04GV",0 ,
	 {PPTBL_NOT_SUPPORT, {0xEF,0xEE,0xFF,16,0x11,0,1,RTYPE_MICRON,{0x80, 0x00},{0x80, 0x01}},
		 {RAND_TYPE_NONE,{0x2D2D,1,1,1,1,1}}}},
         
	{{0xC2,0xF1,0x80,0x95,0x02,0x00}, 5,4,IO_8BIT,128,128,2048,64,0x30c77fff, 0xC03222,0x101,80,VEND_MXIC,1024, "MX30LF1G18AC",0 ,
{PPTBL_NOT_SUPPORT, {0xEF,0xEE,0xFF,16,0x11,0,1,RTYPE_MICRON,{0x80, 0x00},{0x80, 0x01}},
{RAND_TYPE_NONE,{0x2D2D,1,1,1,1,1}}}},
	
  {{0xC2,0xDA,0x90,0x95,0x06,0x00}, 5,5,IO_8BIT,256,128,2048,64,0x30c77fff, 0xC03222,0x101,80,VEND_MXIC,1024, "MX30LF2G18AC",0 ,
{PPTBL_NOT_SUPPORT, {0xEF,0xEE,0xFF,16,0x11,0,1,RTYPE_MICRON,{0x80, 0x00},{0x80, 0x01}},
{RAND_TYPE_NONE,{0x2D2D,1,1,1,1,1}}}},
		
  {{0xC2,0xDC,0x90,0x95,0x56,0x00}, 5,5,IO_8BIT,512,128,2048,64,0x30c77fff, 0xC03222,0x101,80,VEND_MXIC,1024, "MX30LF4G18AC",0 ,
{PPTBL_NOT_SUPPORT, {0xEF,0xEE,0xFF,16,0x11,0,1,RTYPE_MICRON,{0x80, 0x00},{0x80, 0x01}},
{RAND_TYPE_NONE,{0x2D2D,1,1,1,1,1}}}},

  {{0xC2,0xD3,0xD1,0x95,0x5A,0x00}, 5,5,IO_8BIT,1024,128,2048,64,0x30c77fff, 0xC03222,0x101,80,VEND_MXIC,1024, "MX60LF8G18AC",0 ,
{PPTBL_NOT_SUPPORT, {0xEF,0xEE,0xFF,16,0x11,0,1,RTYPE_MICRON,{0x80, 0x00},{0x80, 0x01}},
{RAND_TYPE_NONE,{0x2D2D,1,1,1,1,1}}}},

	{{0x1,0xF1,0x80,0x1D,0x1,0xF1}, 5,5,IO_8BIT,128,128,2048,64,0x30c77fff, 0xC03222,0x101,80,VEND_SPANSION,1024, "S34ML01G200TFI",0 ,
	{PPTBL_NOT_SUPPORT, {0xEF,0xEE,0xFF,16,0x11,0,1,RTYPE_MICRON,{0x80, 0x00},{0x80, 0x01}},	
  {RAND_TYPE_NONE,{0x2D2D,1,1,1,1,1}}}},

	{{0x1,0xDA,0x90,0x95,0x46,0x00}, 5,5,IO_8BIT,256,128,2048,128,0x30c77fff, 0xC03222,0x101,80,VEND_SPANSION,1024, "S34ML02G200TFI",0 ,
	{PPTBL_NOT_SUPPORT, {0xEF,0xEE,0xFF,16,0x11,0,1,RTYPE_MICRON,{0x80, 0x00},{0x80, 0x01}},	
  {RAND_TYPE_NONE,{0x2D2D,1,1,1,1,1}}}},
  
	{{0x1,0xDC,0x90,0x95,0x56,0x00}, 5,5,IO_8BIT,512,128,2048,128,0x30c77fff, 0xC03222,0x101,80,VEND_SPANSION,1024, "S34ML04G200TFI",0 ,
	{PPTBL_NOT_SUPPORT, {0xEF,0xEE,0xFF,16,0x11,0,1,RTYPE_MICRON,{0x80, 0x00},{0x80, 0x01}},	
  {RAND_TYPE_NONE,{0x2D2D,1,1,1,1,1}}}},

	{{0x1,0xD3,0xD1,0x95,0x5a,0x00}, 5,5,IO_8BIT,1024,128,2048,128,0x30c77fff, 0xC03222,0x101,80,VEND_SPANSION,1024, "S34ML08G201TFI",0 ,
	{PPTBL_NOT_SUPPORT, {0xEF,0xEE,0xFF,16,0x11,0,1,RTYPE_MICRON,{0x80, 0x00},{0x80, 0x01}},
	{RAND_TYPE_NONE,{0x2D2D,1,1,1,1,1}}}},	 

};

#endif
