#include <asm/arch/mt_typedefs.h>
#include <asm/arch/debug.h>

//  Error Code No.
#define RSUCCESS        0
#define ERACCESS        1
#define ERINVAL         2
#define ERWRAPPER		3

#define GPIO_WR32(addr, data)   ((*(volatile unsigned long*)(addr)) = (unsigned long)(data))
#define GPIO_RD32(addr)         (*(volatile unsigned long * const)(addr))
#define GPIO_SET_BITS(BIT,REG)   ((*(volatile unsigned long*)(REG)) = (unsigned long)(BIT))
#define GPIO_CLR_BITS(BIT,REG)   ((*(volatile unsigned long*)(REG)) &= ~((unsigned long)(BIT)))
#define MAX_GPIO_REG_BITS      16
#define MAX_GPIO_MODE_PER_REG  5
#define GPIO_MODE_BITS         3 

typedef enum GPIO_PIN
{    
    GPIO_UNSUPPORTED = -1,    
	GPIO0 = 0,
    GPIO1  , GPIO2  , GPIO3  , GPIO4  , GPIO5  , GPIO6  , GPIO7  , 
    GPIO8  , GPIO9  , GPIO10 , GPIO11 , GPIO12 , GPIO13 , GPIO14 , GPIO15 , 
    GPIO16 , GPIO17 , GPIO18 , GPIO19 , GPIO20 , GPIO21 , GPIO22 , GPIO23 , 
    GPIO24 , GPIO25 , GPIO26 , GPIO27 , GPIO28 , GPIO29 , GPIO30 , GPIO31 , 
    GPIO32 , GPIO33 , GPIO34 , GPIO35 , GPIO36 , GPIO37 , GPIO38 , GPIO39 , 
    GPIO40 , GPIO41 , GPIO42 , GPIO43 , GPIO44 , GPIO45 , GPIO46 , GPIO47 , 
    GPIO48 , GPIO49 , GPIO50 , GPIO51 , GPIO52 , GPIO53 , GPIO54 , GPIO55 , 
    GPIO56 , GPIO57 , GPIO58 , GPIO59 , GPIO60 , GPIO61 , GPIO62 , GPIO63 , 
    GPIO64 , GPIO65 , GPIO66 , GPIO67 , GPIO68 , GPIO69 , GPIO70 , GPIO71 , 
    GPIO72 , GPIO73 , GPIO74 , GPIO75 , GPIO76 , GPIO77 , GPIO78 , GPIO79 , 
    GPIO80 , GPIO81 , GPIO82 , GPIO83 , GPIO84 , GPIO85 , GPIO86 , GPIO87 , 
    GPIO88 , GPIO89 , GPIO90 , GPIO91 , GPIO92 , GPIO93 , GPIO94 , GPIO95 , 
    GPIO96 , GPIO97 , GPIO98 , GPIO99 , GPIO100, GPIO101, GPIO102, GPIO103, 
    GPIO104, GPIO105, GPIO106, GPIO107, GPIO108, GPIO109, GPIO110, GPIO111, 
    GPIO112, GPIO113, GPIO114, GPIO115, GPIO116, GPIO117, GPIO118, GPIO119, 
    GPIO120, GPIO121, GPIO122, GPIO123, GPIO124, GPIO125, GPIO126, GPIO127, 
    GPIO128, GPIO129, GPIO130, GPIO131, GPIO132, GPIO133, GPIO134, GPIO135, 
    GPIO136, GPIO137, GPIO138, GPIO139, GPIO140, GPIO141, GPIO142, GPIO143,
    GPIO144, GPIO145, GPIO146, GPIO147, GPIO148, GPIO149, GPIO150, GPIO151,
    GPIO152, GPIO153, GPIO154, GPIO155, GPIO156, GPIO157, GPIO158, GPIO159,
    GPIO160, GPIO161, GPIO162, GPIO163, GPIO164, GPIO165, GPIO166, GPIO167,
    GPIO168, GPIO169, GPIO170, GPIO171, GPIO172, GPIO173, GPIO174, GPIO175,
    GPIO176, GPIO177, GPIO178, GPIO179, GPIO180, GPIO181, GPIO182, GPIO183,
    GPIO184, GPIO185, GPIO186, GPIO187, GPIO188, GPIO189, GPIO190, GPIO191,
    GPIO192, GPIO193, GPIO194, GPIO195, GPIO196, GPIO197, GPIO198, GPIO199,
    GPIO200, GPIO201, GPIO202, GPIO203, GPIO204, GPIO205, GPIO206, GPIO207,
    GPIO208, GPIO209, GPIO210, GPIO211, GPIO212, GPIO213, GPIO214, GPIO215,
    GPIO216, GPIO217, GPIO218, GPIO219, GPIO220, GPIO221, GPIO222, GPIO223,
    GPIO224, GPIO225, GPIO226, GPIO227, GPIO228, GPIO229, GPIO230, GPIO231,
    GPIO232, GPIO233, GPIO234, GPIO235, GPIO236, GPIO237, GPIO238, GPIO239,
    GPIO240, GPIO241, GPIO242, GPIO243, GPIO244, GPIO245, GPIO246, GPIO247,
    GPIO248, GPIO249, GPIO250, GPIO251, GPIO252, GPIO253, GPIO254, GPIO255,
    GPIO256, GPIO257, GPIO258, GPIO259, GPIO260, GPIO261, GPIO262, GPIO263,
    GPIO264, GPIO265, GPIO266, GPIO267, GPIO268, GPIO269, GPIO270, GPIO271,
    GPIO272, GPIO273, GPIO274, GPIO275, GPIO276, GPIO277, GPIO278, GPIO279,
    MT_GPIO_BASE_MAX
}GPIO_PIN;

#define MAX_GPIO_PIN    (MT_GPIO_BASE_MAX) 

/******************************************************************************
* Enumeration for Clock output
******************************************************************************/
/* GPIO MODE CONTROL VALUE*/
typedef enum {
    GPIO_MODE_UNSUPPORTED = -1,
    GPIO_MODE_GPIO  = 0,
    GPIO_MODE_00    = 0,
    GPIO_MODE_01    = 1,
    GPIO_MODE_02    = 2,
    GPIO_MODE_03    = 3,
    GPIO_MODE_04    = 4,
    GPIO_MODE_05    = 5,
    GPIO_MODE_06    = 6,
    GPIO_MODE_07    = 7,

    GPIO_MODE_MAX,
    GPIO_MODE_DEFAULT = GPIO_MODE_01,
} GPIO_MODE;
/*----------------------------------------------------------------------------*/
/* GPIO DIRECTION */
typedef enum {
    GPIO_DIR_UNSUPPORTED = -1,
    GPIO_DIR_IN     = 0,
    GPIO_DIR_OUT    = 1,

    GPIO_DIR_MAX,
    GPIO_DIR_DEFAULT = GPIO_DIR_IN,
} GPIO_DIR;
/*----------------------------------------------------------------------------*/
/* GPIO PULL ENABLE*/
typedef enum {
    GPIO_PULL_EN_UNSUPPORTED = -1,
    GPIO_PULL_DISABLE = 0,
    GPIO_PULL_ENABLE  = 1,

    GPIO_PULL_EN_MAX,
    GPIO_PULL_EN_DEFAULT = GPIO_PULL_ENABLE,
} GPIO_PULL_EN;
/*----------------------------------------------------------------------------*/
/* GPIO PULL-UP/PULL-DOWN*/
typedef enum {
    GPIO_PULL_UNSUPPORTED = -1,
    GPIO_PULL_DOWN  = 0,
    GPIO_PULL_UP    = 1,

    GPIO_PULL_MAX,
    GPIO_PULL_DEFAULT = GPIO_PULL_DOWN
} GPIO_PULL;
/*----------------------------------------------------------------------------*/
/* GPIO OUTPUT */
typedef enum {
    GPIO_OUT_UNSUPPORTED = -1,
    GPIO_OUT_ZERO = 0,
    GPIO_OUT_ONE  = 1,

    GPIO_OUT_MAX,
    GPIO_OUT_DEFAULT = GPIO_OUT_ZERO,
    GPIO_DATA_OUT_DEFAULT = GPIO_OUT_ZERO,  /*compatible with DCT*/
} GPIO_OUT;
/* GPIO INPUT */
typedef enum {
    GPIO_IN_UNSUPPORTED = -1,
    GPIO_IN_ZERO = 0,
    GPIO_IN_ONE  = 1,

    GPIO_IN_MAX,
} GPIO_IN;

/*----------------------------------------------------------------------------*/
typedef struct {    
    u16 val;        
    u16 _align1;
    u16 set;
    u16 _align2;
    u16 rst;
    u16 _align3[3];
} VAL_REGS;
/*----------------------------------------------------------------------------*/
typedef struct {
    VAL_REGS    dir1[11];                   /*0x0000 ~ 0x00AF: 11*16 bytes*/
    VAL_REGS    msdc1_ctrl6;                /*0x00B0 ~ 0x00BF: 1*16 bytes*/
    VAL_REGS    dir2[7];                    /*0x00C0 ~ 0x012F: 7*16 bytes*/
    VAL_REGS    msdc3_ctrl4;                /*0x0130 ~ 0x013F: 1*16 bytes*/
    VAL_REGS    msdc3_ctrl5;                /*0x0140 ~ 0x014F: 1*16 bytes*/
    VAL_REGS    pullen[18];                 /*0x0150 ~ 0x026F: 18*16 bytes*/
    unsigned char       rsv00[16];          /*0x0270 ~ 0x027F: 16 bytes*/
    VAL_REGS    pullsel[18];                /*0x0280 ~ 0x039F: 18*16 bytes*/
    VAL_REGS    msdc3_ctrl7;                /*0x03A0 ~ 0x03AF: 1*16 bytes*/
    unsigned char       rsv01[96];          /*0x03B0 ~ 0x040F: 96 bytes*/
    VAL_REGS    bias_ctrl3;                 /*0x0410 ~ 0x041F: 1*16 bytes*/
    VAL_REGS    bias_ctrl4;                 /*0x0420 ~ 0x042F: 1*16 bytes*/
    VAL_REGS    msdc3_ctrl8;                /*0x0430 ~ 0x043F: 1*16 bytes*/
    VAL_REGS    od33_ctrl_11_14[4];         /*0x0440 ~ 0x047F: 4*16 bytes*/
    unsigned char       rsv02[64];          /*0x0480 ~ 0x04BF: 64 bytes*/
    VAL_REGS    od33_ctrl_8_10[3];          /*0x04C0 ~ 0x04EF: 3*16 bytes*/
    unsigned char       rsv03[16];          /*0x04F0 ~ 0x04FF: 16 bytes*/
    VAL_REGS    dout[18];                   /*0x0500 ~ 0x061F: 18*16 bytes*/
    VAL_REGS    msdc3_ctrl6;                /*0x0620 ~ 0x062F: 1*16 bytes*/
    VAL_REGS    din[18];                    /*0x0630 ~ 0x074F: 18*16 bytes*/
    unsigned char       rsv04[16];          /*0x0750 ~ 0x075F: 16 bytes*/
    VAL_REGS    mode[56];                   /*0x0760 ~ 0x0ADF: 56*16 bytes*/
    unsigned char       rsv05[48];          /*0x0AE0 ~ 0x0B0F: 48 bytes*/
    VAL_REGS    bank;                       /*0x0B10 ~ 0x0B1F: 16 bytes*/
    VAL_REGS       ies[3];                  /*0x0B20 ~ 0x0B4F: 3*16 bytes*/
    VAL_REGS       smt[3];                  /*0x0B50 ~ 0x0B7F: 3*16 bytes*/
    VAL_REGS       tdsel[6];                /*0x0B80 ~ 0x0BDF: 16 bytes*/
    VAL_REGS    od33_ctrl_4_7[4];           /*0x0BE0 ~ 0x0C1F: 3*16 bytes*/
    VAL_REGS       rdsel[6];                /*0x0C20 ~ 0x0C7F: 6*16 bytes*/
    VAL_REGS       drvn0_en;                /*0x0C80 ~ 0x0C8F: 16 bytes*/
    VAL_REGS       msdc3_ctrl0;             /*0x0C90 ~ 0x0C9F: 16 bytes*/
    VAL_REGS       drvp0_en;                /*0x0CA0 ~ 0x0CAF: 16 bytes*/
    VAL_REGS       msdc3_ctrl1;             /*0x0CB0 ~ 0x0CBF: 16 bytes*/
    VAL_REGS       msdc0_ctrl[7];           /*0x0CC0 ~ 0x0D2F: 7*16 bytes*/
    VAL_REGS       msdc1_ctrl[6];           /*0x0D30 ~ 0x0D8F: 6*16 bytes*/
    VAL_REGS       msdc2_ctrl[6];           /*0x0D90 ~ 0x0DEF: 6*16 bytes*/
    VAL_REGS       tm;                      /*0x0DF0 ~ 0x0DFF: 16 bytes*/
    VAL_REGS       usb;                     /*0x0E00 ~ 0x0E0F: 16 bytes*/
    
    VAL_REGS       od33_ctrl_0_3[4];        /*0x0E10 ~ 0x0E4F: 4*16 bytes*/
    VAL_REGS       kpad_ctrl[2];            /*0x0E50 ~ 0x0E6F: 2*16 bytes*/
    VAL_REGS       eint_ctrl[2];            /*0x0E70 ~ 0x0E8F: 2*16 bytes*/
    unsigned char       rsv06[32];          /*0x0E90 ~ 0x0EAF: 32 bytes*/
    VAL_REGS       bias_ctrl[3];            /*0x0EB0 ~ 0x0EDF: 3*16 bytes*/
    unsigned char       rsv07[32];          /*0x0EE0 ~ 0x0EFF: 32 bytes*/
    VAL_REGS       drv_sel_10_12[3];        /*0x0F00 ~ 0x0F3F: 3*16 bytes*/
    VAL_REGS       msdc3_ctrl3;             /*0x0F40 ~ 0x0F4F: 16 bytes*/
    VAL_REGS       drv_sel_0_6[7];          /*0x0F50 ~ 0x0FBF: 3*16 bytes*/
    VAL_REGS       msdc3_ctrl2;             /*0x0FC0 ~ 0x0FCF: 16 bytes*/
    VAL_REGS       drv_sel_8_9[3];          /*0x0FD0 ~ 0x0FFF: 3*16 bytes*/
} GPIO_REGS;
#define GPIO_DIR_GRP2_PIN_BASE GPIO176

/*----------------------------------------------------------------------------*/

typedef struct {
    unsigned int no     : 16;
    unsigned int mode   : 3;    
    unsigned int pullsel: 1;
    unsigned int din    : 1;
    unsigned int dout   : 1;
    unsigned int pullen : 1;
    unsigned int dir    : 1;
    unsigned int dinv   : 1;
    unsigned int _align : 7; 
} GPIO_CFG; 

/*-------for msdc pupd-----------*/
struct msdc_pupd {
	u16	reg;
	unsigned char	bit;
};

/******************************************************************************
* GPIO Driver interface 
******************************************************************************/
/*direction*/
int mt_set_gpio_dir(unsigned long pin, unsigned long dir);
int mt_get_gpio_dir(unsigned long pin);

/*pull enable*/
int mt_set_gpio_pull_enable(unsigned long pin, unsigned long enable);
int mt_get_gpio_pull_enable(unsigned long pin);
/*pull select*/
int mt_set_gpio_pull_select(unsigned long pin, unsigned long select);    
int mt_get_gpio_pull_select(unsigned long pin);

/*input/output*/
int mt_set_gpio_out(unsigned long pin, unsigned long output);
int mt_get_gpio_out(unsigned long pin);
int mt_get_gpio_in(unsigned long pin);

/*mode control*/
int mt_set_gpio_mode(unsigned long pin, unsigned long mode);
int mt_get_gpio_mode(unsigned long pin);
