#include <common.h>
#include <command.h>

#if defined (CONFIG_CMD_NET) && defined(CONFIG_RT2880_ETH)

#include <malloc.h>
#include <net.h>
#include <rt_mmap.h>
#include <miiphy.h>
#include <asm/errno.h>

#if defined (CONFIG_RTL8367)
#include "rtl8367c/include/smi.h"
#include "rtl8367c/include/port.h"
#include "rtl8367c/include/rtk_switch.h"
#include "rtl8367c/include/rtl8367c_asicdrv_port.h"
#endif

#if defined (GE_MII_FORCE_100) || defined (GE_RVMII_FORCE_100)
#define	MAC_TO_100SW_MODE
#elif defined (GE_MII_AN) || defined (GE_RMII_AN)
#define	MAC_TO_100PHY_MODE
#elif defined (GE_RGMII_AN) || defined (GE_RGMII_INTERNAL_P0_AN) || defined (GE_RGMII_INTERNAL_P4_AN)
#define	MAC_TO_GIGAPHY_MODE
#elif defined (GE_RGMII_FORCE_1000) || defined (GE_TRGMII_FORCE_2600)
#define	MAC_TO_MT7530_MODE
#endif

#undef DEBUG
#define BIT(x)              ((1 << x))

/* bits range: for example BITS(16,23) = 0xFF0000
 *   ==>  (BIT(m)-1)   = 0x0000FFFF     ~(BIT(m)-1)   => 0xFFFF0000
 *   ==>  (BIT(n+1)-1) = 0x00FFFFFF
 */
#define BITS(m,n)   (~(BIT(m)-1) & ((BIT(n) - 1) | BIT(n)))

/* ====================================== */
//GDMA1 uni-cast frames destination port
#define GDM_UFRC_P_CPU     ((u32)(~(0x7 << 12)))
#define GDM_UFRC_P_GDMA1   (1 << 12)
#define GDM_UFRC_P_GDMA2   (2 << 12)
#define GDM_UFRC_P_DROP    (7 << 12)
//GDMA1 broad-cast MAC address frames
#define GDM_BFRC_P_CPU     ((u32)(~(0x7 << 8)))
#define GDM_BFRC_P_GDMA1   (1 << 8)
#define GDM_BFRC_P_GDMA2   (2 << 8)
#define GDM_BFRC_P_PPE     (6 << 8)
#define GDM_BFRC_P_DROP    (7 << 8)
//GDMA1 multi-cast MAC address frames
#define GDM_MFRC_P_CPU     ((u32)(~(0x7 << 4)))
#define GDM_MFRC_P_GDMA1   (1 << 4)
#define GDM_MFRC_P_GDMA2   (2 << 4)
#define GDM_MFRC_P_PPE     (6 << 4)
#define GDM_MFRC_P_DROP    (7 << 4)
//GDMA1 other MAC address frames destination port
#define GDM_OFRC_P_CPU     ((u32)(~(0x7)))
#define GDM_OFRC_P_GDMA1   1
#define GDM_OFRC_P_GDMA2   2
#define GDM_OFRC_P_PPE     6
#define GDM_OFRC_P_DROP    7

#define RST_DRX_IDX0      BIT(16)
#define RST_DTX_IDX0      BIT(0)

#define TX_WB_DDONE       BIT(6)
#define RX_DMA_BUSY       BIT(3)
#define TX_DMA_BUSY       BIT(1)
#define RX_DMA_EN         BIT(2)
#define TX_DMA_EN         BIT(0)

#define GP1_FRC_EN        BIT(15)
#define GP1_FC_TX         BIT(11)
#define GP1_FC_RX         BIT(10)
#define GP1_LNK_DWN       BIT(9)
#define GP1_AN_OK         BIT(8)

/*
 * FE_INT_STATUS
 */
#define CNT_PPE_AF       BIT(31)
#define CNT_GDM1_AF      BIT(29)
#define PSE_P1_FC        BIT(22)
#define PSE_P0_FC        BIT(21)
#define PSE_FQ_EMPTY     BIT(20)
#define GE1_STA_CHG      BIT(18)
#define TX_COHERENT      BIT(17)
#define RX_COHERENT      BIT(16)

#define TX_DONE_INT1     BIT(9)
#define TX_DONE_INT0     BIT(8)
#define RX_DONE_INT0     BIT(2)
#define TX_DLY_INT       BIT(1)
#define RX_DLY_INT       BIT(0)

/*
 * Ethernet chip registers.RT2880
 */
/* Old FE with New PDMA */
#define PDMA_RELATED		0x0800
/* 1. PDMA */
#define TX_BASE_PTR0            (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x000)
#define TX_MAX_CNT0             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x004)
#define TX_CTX_IDX0             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x008)
#define TX_DTX_IDX0             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x00C)

#define TX_BASE_PTR1            (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x010)
#define TX_MAX_CNT1             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x014)
#define TX_CTX_IDX1             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x018)
#define TX_DTX_IDX1             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x01C)

#define TX_BASE_PTR2            (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x020)
#define TX_MAX_CNT2             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x024)
#define TX_CTX_IDX2             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x028)
#define TX_DTX_IDX2             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x02C)

#define TX_BASE_PTR3            (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x030)
#define TX_MAX_CNT3             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x034)
#define TX_CTX_IDX3             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x038)
#define TX_DTX_IDX3             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x03C)

#define RX_BASE_PTR0            (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x100)
#define RX_MAX_CNT0             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x104)
#define RX_CALC_IDX0            (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x108)
#define RX_DRX_IDX0             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x10C)

#define RX_BASE_PTR1            (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x110)
#define RX_MAX_CNT1             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x114)
#define RX_CALC_IDX1            (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x118)
#define RX_DRX_IDX1             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x11C)

#define PDMA_INFO               (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x200)
#define PDMA_GLO_CFG            (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x204)
#define PDMA_RST_IDX            (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x208)
#define PDMA_RST_CFG            (RALINK_FRAME_ENGINE_BASE + PDMA_RST_IDX)
#define DLY_INT_CFG             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x20C)
#define FREEQ_THRES             (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x210)
#define INT_STATUS              (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x220) /* FIXME */
#define INT_MASK                (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x228) /* FIXME */
#define PDMA_WRR                (RALINK_FRAME_ENGINE_BASE + PDMA_RELATED+0x280)
#define PDMA_SCH_CFG            (PDMA_WRR)

/* TODO: change FE_INT_STATUS->INT_STATUS 
 * FE_INT_ENABLE->INT_MASK */
#define MDIO_ACCESS         RALINK_FRAME_ENGINE_BASE + 0x00
#define MDIO_CFG            RALINK_FRAME_ENGINE_BASE + 0x04
#define FE_DMA_GLO_CFG      RALINK_FRAME_ENGINE_BASE + 0x08
#define FE_RST_GLO          RALINK_FRAME_ENGINE_BASE + 0x0C
#define FE_INT_STATUS       RALINK_FRAME_ENGINE_BASE + 0x10
#define FE_INT_ENABLE       RALINK_FRAME_ENGINE_BASE + 0x14
#define FC_DROP_STA         RALINK_FRAME_ENGINE_BASE + 0x18
#define FOE_TS_T            RALINK_FRAME_ENGINE_BASE + 0x1C

#define PAD_RGMII2_MDIO_CFG            RALINK_SYSCTL_BASE + 0x58

#define GDMA1_RELATED       0x0500
#define GDMA1_FWD_CFG       (RALINK_FRAME_ENGINE_BASE + GDMA1_RELATED + 0x00)
#define GDMA1_SHRP_CFG      (RALINK_FRAME_ENGINE_BASE + GDMA1_RELATED + 0x04)
#define GDMA1_MAC_ADRL      (RALINK_FRAME_ENGINE_BASE + GDMA1_RELATED + 0x08)
#define GDMA1_MAC_ADRH      (RALINK_FRAME_ENGINE_BASE + GDMA1_RELATED + 0x0C)
#define GDMA2_RELATED       0x1500
#define GDMA2_FWD_CFG       (RALINK_FRAME_ENGINE_BASE + GDMA2_RELATED + 0x00)
#define GDMA2_SHRP_CFG      (RALINK_FRAME_ENGINE_BASE + GDMA2_RELATED + 0x04)
#define GDMA2_MAC_ADRL      (RALINK_FRAME_ENGINE_BASE + GDMA2_RELATED + 0x08)
#define GDMA2_MAC_ADRH      (RALINK_FRAME_ENGINE_BASE + GDMA2_RELATED + 0x0C)

#define PSE_RELATED         0x0040
#define PSE_FQFC_CFG        (RALINK_FRAME_ENGINE_BASE + PSE_RELATED + 0x00)
#define CDMA_FC_CFG         (RALINK_FRAME_ENGINE_BASE + PSE_RELATED + 0x04)
#define GDMA1_FC_CFG        (RALINK_FRAME_ENGINE_BASE + PSE_RELATED + 0x08)
#define GDMA2_FC_CFG        (RALINK_FRAME_ENGINE_BASE + PSE_RELATED + 0x0C)
#define CDMA_OQ_STA         (RALINK_FRAME_ENGINE_BASE + PSE_RELATED + 0x10)
#define GDMA1_OQ_STA        (RALINK_FRAME_ENGINE_BASE + PSE_RELATED + 0x14)
#define GDMA2_OQ_STA        (RALINK_FRAME_ENGINE_BASE + PSE_RELATED + 0x18)
#define PSE_IQ_STA          (RALINK_FRAME_ENGINE_BASE + PSE_RELATED + 0x1C)

#define CDMA_RELATED        0x0080
#define CDMA_CSG_CFG        (RALINK_FRAME_ENGINE_BASE + CDMA_RELATED + 0x00)
#define CDMA_SCH_CFG        (RALINK_FRAME_ENGINE_BASE + CDMA_RELATED + 0x04)


#define INTERNAL_LOOPBACK_ENABLE 1
#define INTERNAL_LOOPBACK_DISABLE 0

#define TOUT_LOOP   1000
#define ENABLE 1
#define DISABLE 0

VALID_BUFFER_STRUCT  rt2880_free_buf_list;
VALID_BUFFER_STRUCT  rt2880_busing_buf_list;
static BUFFER_ELEM   rt2880_free_buf[PKTBUFSRX];

/*=========================================
      PDMA RX Descriptor Format define
=========================================*/

//-------------------------------------------------
typedef struct _PDMA_RXD_INFO1_  PDMA_RXD_INFO1_T;

struct _PDMA_RXD_INFO1_
{
    unsigned int    PDP0;
};
//-------------------------------------------------
typedef struct _PDMA_RXD_INFO2_    PDMA_RXD_INFO2_T;

struct _PDMA_RXD_INFO2_
{
	unsigned int    PLEN1                   : 14;
	unsigned int    LS1                     : 1;
	unsigned int    UN_USED                 : 1;
	unsigned int    PLEN0                   : 14;
	unsigned int    LS0                     : 1;
	unsigned int    DDONE_bit               : 1;
};
//-------------------------------------------------
typedef struct _PDMA_RXD_INFO3_  PDMA_RXD_INFO3_T;

struct _PDMA_RXD_INFO3_
{
	unsigned int    PDP1;
};
//-------------------------------------------------
typedef struct _PDMA_RXD_INFO4_    PDMA_RXD_INFO4_T;

struct _PDMA_RXD_INFO4_
{
	unsigned int    FOE_Entry           	: 14;
	unsigned int    CRSN                	: 5;
	unsigned int    SP               	: 3;
	unsigned int    L4F                 	: 1;
	unsigned int    L4VLD               	: 1;
	unsigned int    TACK                	: 1;
	unsigned int    IP4F                	: 1;
	unsigned int    IP4                 	: 1;
	unsigned int    IP6                 	: 1;
	unsigned int    UN_USE1             	: 4;
};

struct PDMA_rxdesc {
	PDMA_RXD_INFO1_T rxd_info1;
	PDMA_RXD_INFO2_T rxd_info2;
	PDMA_RXD_INFO3_T rxd_info3;
	PDMA_RXD_INFO4_T rxd_info4;
};
/*=========================================
      PDMA TX Descriptor Format define
=========================================*/
//-------------------------------------------------
typedef struct _PDMA_TXD_INFO1_  PDMA_TXD_INFO1_T;

struct _PDMA_TXD_INFO1_
{
	unsigned int    SDP0;
};
//-------------------------------------------------
typedef struct _PDMA_TXD_INFO2_    PDMA_TXD_INFO2_T;

struct _PDMA_TXD_INFO2_
{
	unsigned int    SDL1                  : 14;
	unsigned int    LS1_bit               : 1;
	unsigned int    BURST_bit             : 1;
	unsigned int    SDL0                  : 14;
	unsigned int    LS0_bit               : 1;
	unsigned int    DDONE_bit             : 1;
};
//-------------------------------------------------
typedef struct _PDMA_TXD_INFO3_  PDMA_TXD_INFO3_T;

struct _PDMA_TXD_INFO3_
{
	unsigned int    SDP1;
};
//-------------------------------------------------
typedef struct _PDMA_TXD_INFO4_    PDMA_TXD_INFO4_T;

struct _PDMA_TXD_INFO4_
{
    unsigned int    VLAN_TAG            :16;
    unsigned int    INS                 : 1;
    unsigned int    RESV                : 2;
    unsigned int    UDF                 : 6;
    unsigned int    FPORT               : 3;
    unsigned int    TSO                 : 1;
    unsigned int    TUI_CO              : 3;
};

struct PDMA_txdesc {
	PDMA_TXD_INFO1_T txd_info1;
	PDMA_TXD_INFO2_T txd_info2;
	PDMA_TXD_INFO3_T txd_info3;
	PDMA_TXD_INFO4_T txd_info4;
};

#ifndef CONFIG_SYS_CACHELINE_SIZE
#define CONFIG_SYS_CACHELINE_SIZE	64
#endif	/* CONFIG_SYS_CACHELINE_SIZE */

#define RAETH_NUM_TX_DESC	8
#define RAETH_NUM_RX_DESC	8

static  struct PDMA_txdesc tx_ring0_cache[RAETH_NUM_TX_DESC] __attribute__ ((aligned(CONFIG_SYS_CACHELINE_SIZE)));	/* TX descriptor ring */
static  struct PDMA_rxdesc rx_ring_cache[RAETH_NUM_RX_DESC] __attribute__ ((aligned(CONFIG_SYS_CACHELINE_SIZE)));	/* RX descriptor ring */

static int rx_dma_owner_idx0;                             /* Point to the next RXD DMA wants to use in RXD Ring#0.  */
static int rx_wants_alloc_idx0;                           /* Point to the next RXD CPU wants to allocate to RXD Ring #0. */
static int tx_cpu_owner_idx0;                             /* Point to the next TXD in TXD_Ring0 CPU wants to use */
static volatile struct PDMA_rxdesc *rx_ring;
static volatile struct PDMA_txdesc *tx_ring0;

static int rx_desc_cnt;
static int rx_desc_threshold;

static char rxRingSize;
static char txRingSize;

static int   rt2880_eth_init(struct eth_device* dev, bd_t* bis);
static int   rt2880_eth_send(struct eth_device* dev, void *packet, int length);
static int   rt2880_eth_recv(struct eth_device* dev);
void  rt2880_eth_halt(struct eth_device* dev);

int   mii_mgr_read(u32 phy_addr, u32 phy_register, u32 *read_data);
int   mii_mgr_write(u32 phy_addr, u32 phy_register, u32 write_data);

static int   rt2880_eth_setup(struct eth_device* dev);
static int   rt2880_eth_initd;

#define phys_to_bus(a) (a)

volatile uchar	*PKT_HEADER_Buf;
static volatile uchar	PKT_HEADER_Buf_Pool[(PKTBUFSRX * PKTSIZE_ALIGN) + PKTALIGN];
extern uchar	*NetTxPacket;	/* THE transmit packet */
volatile uchar	RxPktBuf[PKTBUFSRX][1536];

#define PIODIR_R  (RALINK_PIO_BASE + 0X24)
#define PIODATA_R (RALINK_PIO_BASE + 0X20)
#define PIODIR3924_R  (RALINK_PIO_BASE + 0x4c)
#define PIODATA3924_R (RALINK_PIO_BASE + 0x48)


void START_ETH(struct eth_device *dev ) {
	s32 omr;
	omr=RALINK_REG(PDMA_GLO_CFG);
	udelay(100);
	omr |= TX_WB_DDONE | RX_DMA_EN | TX_DMA_EN ;
		
	RALINK_REG(PDMA_GLO_CFG)=omr;

	udelay(500);
}


void STOP_ETH(struct eth_device *dev)
{
	s32 omr;
	omr=RALINK_REG(PDMA_GLO_CFG);
	udelay(100);
	omr &= ~(TX_WB_DDONE | RX_DMA_EN | TX_DMA_EN) ;
	RALINK_REG(PDMA_GLO_CFG)=omr;
	udelay(500);
}


BUFFER_ELEM *rt2880_free_buf_entry_dequeue(VALID_BUFFER_STRUCT *hdr)
{
	int     zero = 0;           /* causes most compilers to place this */
	/* value in a register only once */
	BUFFER_ELEM  *node;

	/* Make sure we were not passed a null pointer. */
	if (!hdr) {
		return (NULL);
	}

	/* If there is a node in the list we want to remove it. */
	if (hdr->head) {
		/* Get the node to be removed */
		node = hdr->head;

		/* Make the hdr point the second node in the list */
		hdr->head = node->next;

		/* If this is the last node the headers tail pointer needs to be nulled
		   We do not need to clear the node's next since it is already null */
		if (!(hdr->head)) {
			hdr->tail = (BUFFER_ELEM *)zero;
		}

		node->next = (BUFFER_ELEM *)zero;




	}
	else {
		node = NULL;
		return (node);
	}

	/*  Restore the previous interrupt lockout level.  */

	/* Return a pointer to the removed node */

	//shnat_validation_flow_table_entry[node->index].state = SHNAT_FLOW_TABLE_NODE_USED;
	return (node);
}

static BUFFER_ELEM *rt2880_free_buf_entry_enqueue(VALID_BUFFER_STRUCT *hdr, BUFFER_ELEM *item)
{
	int zero =0;

	if (!hdr) {
		return (NULL);
	}

	if (item != NULL)
	{
		/* Temporarily lockout interrupts to protect global buffer variables. */
		// Sys_Interrupt_Disable_Save_Flags(&cpsr_flags);

		/* Set node's next to point at NULL */
		item->next = (BUFFER_ELEM *)zero;

		/*  If there is currently a node in the linked list, we want to add the
		    new node to the end. */
		if (hdr->head) {
			/* Make the last node's next point to the new node. */
			hdr->tail->next = item;

			/* Make the roots tail point to the new node */
			hdr->tail = item;
		}
		else {
			/* If the linked list was empty, we want both the root's head and
			   tial to point to the new node. */
			hdr->head = item;
			hdr->tail = item;
		}

		/*  Restore the previous interrupt lockout level.  */

	}
	else
	{
		printf("\n shnat_flow_table_free_entry_enqueue is called,item== NULL \n");
	}

	return(item);

} /* MEM_Buffer_Enqueue */

#if defined(CONFIG_PHYLIB)
int rt2880_phy_read(struct mii_dev *bus, int phy_addr, int dev_addr,
                   int reg_addr)
{
        u32 data;
	mii_mgr_read(phy_addr, reg_addr, &data);

        return data;
}

int rt2880_phy_write(struct mii_dev *bus, int phy_addr, int dev_addr,
                    int reg_addr, u16 data)
{
	mii_mgr_write(phy_addr, reg_addr, data);

	return 0;
}

phy_interface_t rt2880_phy_interface_get(void)
{
#if defined (GE_MII_FORCE_100) || defined (GE_MII_AN)
	return PHY_INTERFACE_MODE_MII;
#elif defined (CONFIG_GE1_SGMII_FORCE_2500)
	return PHY_INTERFACE_MODE_SGMII;
#elif defined (GE_RMII_AN)
	return PHY_INTERFACE_MODE_RMII;
#elif defined (GE_RGMII_FORCE_1000) || defined (GE_RGMII_AN)
	return PHY_INTERFACE_MODE_RGMII;
#else
	return PHY_INTERFACE_MODE_MII;
#endif
}

int rt2880_phylib_init(struct eth_device *dev, int phyid)
{
	struct mii_dev *bus;
	struct phy_device *phydev;
	phy_interface_t interface;
	int ret;

	bus = mdio_alloc();
	if (!bus) {
		printf("mdio_alloc failed\n");
		return -ENOMEM;
	}
	bus->read = rt2880_phy_read;
	bus->write = rt2880_phy_write;
	sprintf(bus->name, dev->name);

	ret = mdio_register(bus);
	if (ret) {
		printf("mdio_register failed\n");
		free(bus);
		return -ENOMEM;
	}

	interface = rt2880_phy_interface_get();
	phydev = phy_connect(bus, phyid, dev, interface);
	if (!phydev) {
		printf("phy_connect failed\n");
		return -ENODEV;
	}

	phy_config(phydev);
	phy_startup(phydev);

	return 0;
}
#endif

#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII)
static int mii_reg_read(const char *devname, u8 phy_adr, u8 reg_ofs, u16 * data)
{
	u32 read_data;

	printf("phy_adr=0x%x, reg_ofs=0x%x\n", phy_adr, reg_ofs);
	mii_mgr_read(phy_adr, reg_ofs, &read_data);
	*data = read_data;
	printf("data = 0x%x\n", *data);
	return 0;
}

static int mii_reg_write(const char *devname, u8 phy_adr, u8 reg_ofs, u16 data)
{
	mii_mgr_write(phy_adr, reg_ofs, data);
	return 0;
}
#endif

int rt2880_eth_initialize(bd_t *bis)
{
	struct	eth_device* 	dev;
	int	i;

	if (!(dev = (struct eth_device *) malloc (sizeof *dev))) {
		printf("Failed to allocate memory\n");
		return 0;
	}

	memset(dev, 0, sizeof(*dev));

	sprintf(dev->name, "mtk_eth");

	dev->iobase = RALINK_FRAME_ENGINE_BASE;
	dev->init   = rt2880_eth_init;
	dev->halt   = rt2880_eth_halt;
	dev->send   = rt2880_eth_send;
	dev->recv   = rt2880_eth_recv;

	eth_register(dev);

#if defined(CONFIG_PHYLIB)
	rt2880_phylib_init(dev, MAC_TO_GIGAPHY_MODE_ADDR);
#elif defined(CONFIG_MII) || defined(CONFIG_CMD_MII)
	miiphy_register(dev->name, mii_reg_read, mii_reg_write);
#endif

	rt2880_eth_initd =0;
	PKT_HEADER_Buf = PKT_HEADER_Buf_Pool;
	NetTxPacket = NULL;

	rx_ring = (struct PDMA_rxdesc *)((ulong)&rx_ring_cache[0]);
	tx_ring0 = (struct PDMA_txdesc *)((ulong)&tx_ring0_cache[0]);

	rt2880_free_buf_list.head = NULL;
	rt2880_free_buf_list.tail = NULL;

	rt2880_busing_buf_list.head = NULL;
	rt2880_busing_buf_list.tail = NULL;

	//2880_free_buf

	/*
	 *	Setup packet buffers, aligned correctly.
	 */
	rt2880_free_buf[0].pbuf = (unsigned char *)&RxPktBuf[0];
	rt2880_free_buf[0].pbuf += PKTALIGN - 1;
	rt2880_free_buf[0].pbuf -= (ulong)rt2880_free_buf[0].pbuf % PKTALIGN;
	rt2880_free_buf[0].next = NULL;

	rt2880_free_buf_entry_enqueue(&rt2880_free_buf_list,&rt2880_free_buf[0]);

#ifdef DEBUG
	printf("\n rt2880_free_buf[0].pbuf = 0x%08X \n",rt2880_free_buf[0].pbuf);
#endif
	for (i = 1; i < PKTBUFSRX; i++) {
		rt2880_free_buf[i].pbuf = rt2880_free_buf[0].pbuf + (i)*PKTSIZE_ALIGN;
		rt2880_free_buf[i].next = NULL;
#ifdef DEBUG
		printf("\n rt2880_free_buf[%d].pbuf = 0x%08X\n",i,rt2880_free_buf[i].pbuf);
#endif
		rt2880_free_buf_entry_enqueue(&rt2880_free_buf_list,&rt2880_free_buf[i]);
	}

	for (i = 0; i < PKTBUFSRX; i++)
	{
		rt2880_free_buf[i].tx_idx = RAETH_NUM_TX_DESC;
#ifdef DEBUG
		printf("\n rt2880_free_buf[%d] = 0x%08X,rt2880_free_buf[%d].next=0x%08X \n",i,&rt2880_free_buf[i],i,rt2880_free_buf[i].next);
#endif
	}

	return 1;
}

#if defined(MT7623_ASIC_BOARD)
void mt7623_ethifsys_init(void)
{
#define TRGPLL_CON0                    (0x10209280)
#define TRGPLL_CON1                    (0x10209284)
#define TRGPLL_CON2                    (0x10209288)
#define TRGPLL_PWR_CON0                (0x1020928C)
#define ETHPLL_CON0                    (0x10209290)
#define ETHPLL_CON1                    (0x10209294)
#define ETHPLL_CON2                    (0x10209298)
#define ETHPLL_PWR_CON0                (0x1020929C)
#define ETH_PWR_CON                    (0x100062A0)
#define HIF_PWR_CON                    (0x100062A4)

	printf("Enter mt7623_ethifsys_init()\n");

	//=========================================================================
	// Enable ETHPLL & TRGPLL
	//=========================================================================
	// xPLL PWR ON
	u32 temp, pwr_ack_status;
	temp = RALINK_REG(ETHPLL_PWR_CON0);
	RALINK_REG(ETHPLL_PWR_CON0) = temp | 0x1;

	temp = RALINK_REG(TRGPLL_PWR_CON0);
	RALINK_REG(TRGPLL_PWR_CON0) = temp | 0x1;

	udelay(5); // wait for xPLL_PWR_ON ready (min delay is 1us)

	// xPLL ISO Disable
	temp = RALINK_REG(ETHPLL_PWR_CON0);
	RALINK_REG(ETHPLL_PWR_CON0) = temp & ~0x2;

	temp = RALINK_REG(TRGPLL_PWR_CON0);
	RALINK_REG(TRGPLL_PWR_CON0) = temp & ~0x2;

	// xPLL Frequency Set

	temp = RALINK_REG(ETHPLL_CON0);
	RALINK_REG(ETHPLL_CON0) = temp | 0x1;

#if defined (CONFIG_GE1_TRGMII_FORCE_2900)
	temp = RALINK_REG(TRGPLL_CON0);
	RALINK_REG(TRGPLL_CON0) = temp | 0x1;
#elif defined (CONFIG_GE1_TRGMII_FORCE_2600)	
	RALINK_REG(TRGPLL_CON1) = 0xB2000000;
	temp = RALINK_REG(TRGPLL_CON0);
	RALINK_REG(TRGPLL_CON0) = temp | 0x1;
#elif defined (CONFIG_GE1_TRGMII_FORCE_2000)
	RALINK_REG(TRGPLL_CON1) = 0xCCEC4EC5;
	RALINK_REG(TRGPLL_CON0) = 0x121;
#else
	temp = RALINK_REG(TRGPLL_CON0);
	RALINK_REG(TRGPLL_CON0) = temp | 0x1;
#endif

	udelay(40); // wait for PLL stable (min delay is 20us)

	//=========================================================================
	// Power on ETHDMASYS and HIFSYS
	//=========================================================================
	// Power on ETHDMASYS

	RALINK_REG(0x10006000) = 0x0b160001;
	
	pwr_ack_status = (RALINK_REG(ETH_PWR_CON) & 0x0000f000) >> 12;
	if(pwr_ack_status == 0x0) {
	        printf("ETH already turn on and power on flow will be skipped\n");
	} else {
	        temp = RALINK_REG(ETH_PWR_CON);
	        RALINK_REG(ETH_PWR_CON) = temp | 0x4;          // PWR_ON
	        temp = RALINK_REG(ETH_PWR_CON);
	        RALINK_REG(ETH_PWR_CON) = temp | 0x8;          // PWR_ON_S

	        udelay(5); // wait power settle time (min delay is 1us)

	        temp = RALINK_REG(ETH_PWR_CON);
	        RALINK_REG(ETH_PWR_CON) = temp & ~0x10;      // PWR_CLK_DIS
	        temp = RALINK_REG(ETH_PWR_CON);
	        RALINK_REG(ETH_PWR_CON) = temp & ~0x2;        // PWR_ISO
	        temp = RALINK_REG(ETH_PWR_CON);
	        RALINK_REG(ETH_PWR_CON) = temp & ~0x100;   // SRAM_PDN 0
	        temp = RALINK_REG(ETH_PWR_CON);
	        RALINK_REG(ETH_PWR_CON) = temp & ~0x200;   // SRAM_PDN 1
	        temp = RALINK_REG(ETH_PWR_CON);
	        RALINK_REG(ETH_PWR_CON) = temp & ~0x400;   // SRAM_PDN 2
	        temp = RALINK_REG(ETH_PWR_CON);
	        RALINK_REG(ETH_PWR_CON) = temp & ~0x800;   // SRAM_PDN 3

	        udelay(5); // wait SRAM settle time (min delay is 1Us)

	        temp = RALINK_REG(ETH_PWR_CON);
	        RALINK_REG(ETH_PWR_CON) = temp | 0x1;          // PWR_RST_B
	}

	// Power on HIFSYS
	pwr_ack_status = (RALINK_REG(HIF_PWR_CON) & 0x0000f000) >> 12;
	if(pwr_ack_status == 0x0) {
	        printf("HIF already turn on and power on flow will be skipped\n");

	} else {
		temp = RALINK_REG(HIF_PWR_CON);
		RALINK_REG(HIF_PWR_CON) = temp | 0x4;	       // PWR_ON
		temp = RALINK_REG(HIF_PWR_CON);
		RALINK_REG(HIF_PWR_CON) = temp | 0x8;	       // PWR_ON_S

		udelay(5); // wait power settle time (min delay is 1us)

		temp = RALINK_REG(HIF_PWR_CON);
		RALINK_REG(HIF_PWR_CON) = temp & ~0x10;      // PWR_CLK_DIS
		temp = RALINK_REG(HIF_PWR_CON);
		RALINK_REG(HIF_PWR_CON) = temp & ~0x2;	      // PWR_ISO
		temp = RALINK_REG(HIF_PWR_CON);
		RALINK_REG(HIF_PWR_CON) = temp & ~0x100;   // SRAM_PDN 0
		temp = RALINK_REG(HIF_PWR_CON);
		RALINK_REG(HIF_PWR_CON) = temp & ~0x200;   // SRAM_PDN 1
		temp = RALINK_REG(HIF_PWR_CON);
		RALINK_REG(HIF_PWR_CON) = temp & ~0x400;   // SRAM_PDN 2
		temp = RALINK_REG(HIF_PWR_CON);
		RALINK_REG(HIF_PWR_CON) = temp & ~0x800;   // SRAM_PDN 3

		udelay(5); // wait SRAM settle time (min delay is 1Us)

		temp = RALINK_REG(HIF_PWR_CON);
		RALINK_REG(HIF_PWR_CON) = temp | 0x1;	       // PWR_RST_B
	}

	/* Release mt7530 reset */
	temp = le32_to_cpu(*(volatile u_long *)(0x1b000034));
	temp &= ~(BIT(2));
	*(volatile u_long *)(0x1b000034) = temp;
}

void mt7623_pinmux_set(void)
{
	u32 regValue;
	
	printf("[mt7623_pinmux_set]start\n");
	/* Pin277: ESW_RST (1) */
	regValue = le32_to_cpu(*(volatile u_long *)(0x10005ad0));
	regValue &= ~(BITS(6,8));
	regValue |= BIT(6);
	*(volatile u_long *)(0x10005ad0) = regValue;

	/* Pin262: G2_TXEN (1) */
	regValue = le32_to_cpu(*(volatile u_long *)(0x10005aa0));
	regValue &= ~(BITS(6,8));
	regValue |= BIT(6);
	*(volatile u_long *)(0x10005aa0) = regValue;
	/* Pin263: G2_TXD3 (1) */
	regValue = le32_to_cpu(*(volatile u_long *)(0x10005aa0));
	regValue &= ~(BITS(9,11));
	regValue |= BIT(9);
	*(volatile u_long *)(0x10005aa0) = regValue;
	/* Pin264: G2_TXD2 (1) */
	regValue = le32_to_cpu(*(volatile u_long *)(0x10005aa0));
	regValue &= ~(BITS(12,14));
	regValue |= BIT(12);
	*(volatile u_long *)(0x10005aa0) = regValue;
	/* Pin265: G2_TXD1 (1) */
	regValue = le32_to_cpu(*(volatile u_long *)(0x10005ab0));
	regValue &= ~(BITS(0,2));
	regValue |= BIT(0);
	*(volatile u_long *)(0x10005ab0) = regValue;
	/* Pin266: G2_TXD0 (1) */
	regValue = le32_to_cpu(*(volatile u_long *)(0x10005ab0));
	regValue &= ~(BITS(3,5));
	regValue |= BIT(3);
	*(volatile u_long *)(0x10005ab0) = regValue;
	/* Pin267: G2_TXC (1) */
	regValue = le32_to_cpu(*(volatile u_long *)(0x10005ab0));
	regValue &= ~(BITS(6,8));
	regValue |= BIT(6);
	*(volatile u_long *)(0x10005ab0) = regValue;
	/* Pin268: G2_RXC (1) */
	regValue = le32_to_cpu(*(volatile u_long *)(0x10005ab0));
	regValue &= ~(BITS(9,11));
	regValue |= BIT(9);
	*(volatile u_long *)(0x10005ab0) = regValue;
	/* Pin269: G2_RXD0 (1) */
	regValue = le32_to_cpu(*(volatile u_long *)(0x10005ab0));
	regValue &= ~(BITS(12,14));
	regValue |= BIT(12);
	*(volatile u_long *)(0x10005ab0) = regValue;
	/* Pin270: G2_RXD1 (1) */
	regValue = le32_to_cpu(*(volatile u_long *)(0x10005ac0));
	regValue &= ~(BITS(0,2));
	regValue |= BIT(0);
	*(volatile u_long *)(0x10005ac0) = regValue;
	/* Pin271: G2_RXD2 (1) */
	regValue = le32_to_cpu(*(volatile u_long *)(0x10005ac0));
	regValue &= ~(BITS(3,5));
	regValue |= BIT(3);
	*(volatile u_long *)(0x10005ac0) = regValue;
	/* Pin272: G2_RXD3 (1) */
	regValue = le32_to_cpu(*(volatile u_long *)(0x10005ac0));
	regValue &= ~(BITS(6,8));
	regValue |= BIT(6);
	*(volatile u_long *)(0x10005ac0) = regValue;
	/* Pin274: G2_RXDV (1) */
	regValue = le32_to_cpu(*(volatile u_long *)(0x10005ac0));
	regValue &= ~(BITS(12,14));
	regValue |= BIT(12);
	*(volatile u_long *)(0x10005ac0) = regValue;

	/* Pin275: MDC (1) */
	regValue = le32_to_cpu(*(volatile u_long *)(0x10005ad0));
	regValue &= ~(BITS(0,2));
	regValue |= BIT(0);
	*(volatile u_long *)(0x10005ad0) = regValue;
	/* Pin276: MDIO (1) */
	regValue = le32_to_cpu(*(volatile u_long *)(0x10005ad0));
	regValue &= ~(BITS(3,5));
	regValue |= BIT(3);
	*(volatile u_long *)(0x10005ad0) = regValue;
	printf("[mt7623_pinmux_set]end\n");
}

void wait_loop(void) {
	int i,j;
	int read_data;
	j =0;
	while (j< 10) {
		for(i = 0; i<32; i = i+1){
			read_data = *(volatile u_long *)(0x1B110610);
			 *(volatile u_long *)(0x1B110610) = read_data;
		}
		j++;
	}
}

void trgmii_calibration_7623(void) {

	unsigned int  tap_a[5] = {0, 0, 0, 0, 0}; // minumum delay for all correct
	unsigned int  tap_b[5] = {0, 0, 0, 0, 0}; // maximum delay for all correct
	unsigned int  final_tap[5];
	unsigned int  rxc_step_size;
	unsigned int  rxd_step_size;
	unsigned int  read_data;
	unsigned int  tmp;
	unsigned int  rd_wd;
	int  i;
	unsigned int err_cnt[5];
	unsigned int init_toggle_data;
	unsigned int err_flag[5];
	unsigned int err_total_flag;
	unsigned int training_word;
	unsigned int rd_tap;
	unsigned int is_mt7623_e1 = 0;

	u32  TRGMII_7623_base;
	u32  TRGMII_7623_RD_0;
	u32  TRGMII_RCK_CTRL;
	TRGMII_7623_base = ETHDMASYS_ETH_SW_BASE+0x0300;
	TRGMII_7623_RD_0 = TRGMII_7623_base + 0x10;
	TRGMII_RCK_CTRL = TRGMII_7623_base;
	rxd_step_size =0x1;
	rxc_step_size =0x4;
	init_toggle_data = 0x00000055;
	training_word    = 0x000000AC;

	//printk("Calibration begin ........");
	*(volatile u_long *)(TRGMII_7623_base +0x04) &= 0x3fffffff;   // RX clock gating in MT7623
	*(volatile u_long *)(TRGMII_7623_base +0x00) |= 0x80000000;   // Assert RX  reset in MT7623
	*(volatile u_long *)(TRGMII_7623_base +0x78) |= 0x00002000;   // Set TX OE edge in  MT7623
	*(volatile u_long *)(TRGMII_7623_base +0x04) |= 0xC0000000;   // Disable RX clock gating in MT7623
	*(volatile u_long *)(TRGMII_7623_base )      &= 0x7fffffff;   // Release RX reset in MT7623
	//printk("Check Point 1 .....\n");
	for (i = 0 ; i<5 ; i++) {
		*(volatile u_long *)(TRGMII_7623_RD_0 + i*8) |= 0x80000000;   // Set bslip_en = 1
	}

	//printk("Enable Training Mode in MT7530\n");
	mii_mgr_read(0x1F,0x7A40,&read_data);
	read_data |= 0xc0000000;
	mii_mgr_write(0x1F,0x7A40,read_data);  //Enable Training Mode in MT7530
	err_total_flag = 0;
	//printk("Adjust RXC delay in MT7623\n");
	read_data =0x0;
	while (err_total_flag == 0 && read_data != 0x68) {
		//printk("2nd Enable EDGE CHK in MT7623\n");
		/* Enable EDGE CHK in MT7623*/
		for (i = 0 ; i<5 ; i++) {
			tmp = *(volatile u_long *)(TRGMII_7623_RD_0 + i*8);
			tmp |= 0x40000000;
			*(volatile u_long *)(TRGMII_7623_RD_0 + i*8) = tmp & 0x4fffffff;
		}
		wait_loop();
		err_total_flag = 1;
		for  (i = 0 ; i<5 ; i++) {
			err_cnt[i] = ((*(volatile u_long *)(TRGMII_7623_RD_0 + i*8)) >> 8)  & 0x0000000f;
			rd_wd = ((*(volatile u_long *)(TRGMII_7623_RD_0 + i*8)) >> 16)  & 0x000000ff;
			//printk("ERR_CNT = %d, RD_WD =%x\n",err_cnt[i],rd_wd);
			if ( err_cnt[i] !=0 ) {
				err_flag[i] = 1;
			}
			else if (rd_wd != 0x55) {
				err_flag[i] = 1;
			}	
			else {
				err_flag[i] = 0;
			}
			err_total_flag = err_flag[i] &  err_total_flag;
		}

		//printk("2nd Disable EDGE CHK in MT7623\n");
		/* Disable EDGE CHK in MT7623*/
		for (i = 0 ; i<5 ; i++) {
			tmp = *(volatile u_long *)(TRGMII_7623_RD_0 + i*8);
			tmp |= 0x40000000;
			*(volatile u_long *)(TRGMII_7623_RD_0 + i*8) = tmp & 0x4fffffff;
		}
		wait_loop();
		/* Adjust RXC delay */
		if(is_mt7623_e1)
			*(volatile u_long *)(TRGMII_7623_base +0x00) |= 0x80000000;   // Assert RX  reset in MT7623
		*(volatile u_long *)(TRGMII_7623_base +0x04) &= 0x3fffffff;   // RX clock gating in MT7623
		read_data = *(volatile u_long *)(TRGMII_7623_base);
		if (err_total_flag == 0) {
		  tmp = (read_data & 0x0000007f) + rxc_step_size;
		  //printk(" RXC delay = %d\n", tmp);
		  read_data >>= 8;
		  read_data &= 0xffffff80;
		  read_data |= tmp;
		  read_data <<=8;
		  read_data &= 0xffffff80;
		  read_data |=tmp;
		  *(volatile u_long *)(TRGMII_7623_base)  =   read_data;
 		} else {
		  tmp = (read_data & 0x0000007f) + 16;
		  //printk(" RXC delay = %d\n", tmp);
		  read_data >>= 8;
		  read_data &= 0xffffff80;
		  read_data |= tmp;
		  read_data <<=8;
		  read_data &= 0xffffff80;
		  read_data |=tmp;
		  *(volatile u_long *)(TRGMII_7623_base)  =   read_data;
		}
		  read_data &=0x000000ff;
		  if(is_mt7623_e1)
		  	*(volatile u_long *)(TRGMII_7623_base )      &= 0x7fffffff;   // Release RX reset in MT7623
		  *(volatile u_long *)(TRGMII_7623_base +0x04) |= 0xC0000000;   // Disable RX clock gating in MT7623
		  for (i = 0 ; i<5 ; i++) {
		  	*(volatile u_long *)(TRGMII_7623_RD_0 + i*8) =  (*(volatile u_long *)(TRGMII_7623_RD_0 + i*8)) | 0x80000000;  // Set bslip_en = ~bit_slip_en
		  }
	}
	//printk("Finish RXC Adjustment while loop\n");
	//printk("Read RD_WD MT7623\n");
	/* Read RD_WD MT7623*/
	for  (i = 0 ; i<5 ; i++) {
		  rd_tap = 0;
			while (err_flag[i] != 0 && rd_tap != 128) {
			/* Enable EDGE CHK in MT7623*/
			tmp = *(volatile u_long *)(TRGMII_7623_RD_0 + i*8);
			tmp |= 0x40000000;
			*(volatile u_long *)(TRGMII_7623_RD_0 + i*8) = tmp & 0x4fffffff;
			wait_loop();
			read_data = *(volatile u_long *)(TRGMII_7623_RD_0 + i*8);
			err_cnt[i] = (read_data >> 8)  & 0x0000000f;     // Read MT7623 Errcnt
			rd_wd = (read_data >> 16)  & 0x000000ff;
			if (err_cnt[i] != 0 || rd_wd !=0x55){
		           err_flag [i] =  1;
			}   
			else {
			   err_flag[i] =0;
		        }	
			/* Disable EDGE CHK in MT7623*/
			*(volatile u_long *)(TRGMII_7623_RD_0 + i*8) &= 0x4fffffff;
			tmp |= 0x40000000;
			*(volatile u_long *)(TRGMII_7623_RD_0 + i*8) = tmp & 0x4fffffff;
			wait_loop();
			//err_cnt[i] = ((read_data) >> 8)  & 0x0000000f;     // Read MT7623 Errcnt
			if (err_flag[i] !=0) {
			    rd_tap    = (read_data & 0x0000007f) + rxd_step_size;                     // Add RXD delay in MT7623
			    read_data = (read_data & 0xffffff80) | rd_tap;
			    *(volatile u_long *)(TRGMII_7623_RD_0 + i*8) = read_data;
			    tap_a[i] = rd_tap;
			} else {
                            rd_tap    = (read_data & 0x0000007f) + 48;
			    read_data = (read_data & 0xffffff80) | rd_tap;
			    *(volatile u_long *)(TRGMII_7623_RD_0 + i*8) = read_data;
			}	
			//err_cnt[i] = (*(volatile u_long *)(TRGMII_7623_RD_0 + i*8) >> 8)  & 0x0000000f;     // Read MT7623 Errcnt

		}
		//printf("7623 %dth bit  Tap_a = %d\n", i, tap_a[i]);
	}
	//printk("Last While Loop\n");
	for  (i = 0 ; i<5 ; i++) {
		//printk(" Bit%d\n", i);
		while ((err_flag[i] == 0) && (rd_tap !=128)) {
			read_data = *(volatile u_long *)(TRGMII_7623_RD_0 + i*8);
			rd_tap    = (read_data & 0x0000007f) + rxd_step_size;                     // Add RXD delay in MT7623
			read_data = (read_data & 0xffffff80) | rd_tap;
			*(volatile u_long *)(TRGMII_7623_RD_0 + i*8) = read_data;
			/* Enable EDGE CHK in MT7623*/
			tmp = *(volatile u_long *)(TRGMII_7623_RD_0 + i*8);
			tmp |= 0x40000000;
			*(volatile u_long *)(TRGMII_7623_RD_0 + i*8) = tmp & 0x4fffffff;
			wait_loop();
#if 0			
			err_cnt[i] = ((*(volatile u_long *)(TRGMII_7623_RD_0 + i*8)) >> 8)  & 0x0000000f;     // Read MT7623 Errcnt
#else
			read_data = *(volatile u_long *)(TRGMII_7623_RD_0 + i*8);
			err_cnt[i] = (read_data >> 8)  & 0x0000000f;     // Read MT7623 Errcnt
			rd_wd = (read_data >> 16)  & 0x000000ff;
			if (err_cnt[i] != 0 || rd_wd !=0x55){
				err_flag [i] =  1;
			}
			else {
				err_flag[i] =0;
			}
#endif
			
			/* Disable EDGE CHK in MT7623*/
			tmp = *(volatile u_long *)(TRGMII_7623_RD_0 + i*8);
			tmp |= 0x40000000;
			*(volatile u_long *)(TRGMII_7623_RD_0 + i*8) = tmp & 0x4fffffff;
			wait_loop();
			//err_cnt[i] = ((*(volatile u_long *)(TRGMII_7623_RD_0 + i*8)) >> 8)  & 0x0000000f;     // Read MT7623 Errcnt

		}
		tap_b[i] =   rd_tap;// -rxd_step_size;                                        // Record the max delay TAP_B
		//printf("7623 tap_b[%d] is %d \n", i,tap_b[i]);
		final_tap[i] = (tap_a[i]+tap_b[i])/2;                                              //  Calculate RXD delay = (TAP_A + TAP_B)/2
		//printk("%dth bit Final Tap = %d\n", i, final_tap[i]);
		read_data = (read_data & 0xffffff80) | final_tap[i];
		*(volatile u_long *)(TRGMII_7623_RD_0 + i*8) = read_data;
	}
//	/*word alignment*/
//	mii_mgr_read(0x1F,0x7A50,&read_data);
//	read_data &= ~(0xff);
//	read_data |= 0xac;
//	mii_mgr_write(0x1F,0x7A50,read_data);
//	while (i <10) {
//		wait_loop();
//		read_data = *(volatile u_long *)(TRGMII_7623_RD_0+i*8);
//		printk(" MT7623 training word = %x\n", read_data);
//	}


	mii_mgr_read(0x1F,0x7A40,&read_data);
	//printk(" MT7530 0x7A40 = %x\n", read_data);
	read_data &=0x3fffffff;
	mii_mgr_write(0x1F,0x7A40,read_data);
}


void trgmii_calibration_7530(void){ 

	unsigned int  tap_a[5] = {0, 0, 0, 0, 0};
	unsigned int  tap_b[5] = {0, 0, 0, 0, 0};
	unsigned int  final_tap[5];
	unsigned int  rxc_step_size;
	unsigned int  rxd_step_size;
	unsigned int  read_data;
	unsigned int  tmp;
	int  i,j;
	unsigned int err_cnt[5];
	unsigned int rd_wd;
	unsigned int init_toggle_data;
	unsigned int err_flag[5];
	unsigned int err_total_flag;
	unsigned int training_word;
	unsigned int rd_tap;
	unsigned int is_mt7623_e1 = 0;
#define DEVINFO_BASE                    0x17000000

	u32  TRGMII_7623_base;
	u32  TRGMII_7530_RD_0;
	u32  TRGMII_RCK_CTRL;
	u32 TRGMII_7530_base;
	u32 TRGMII_7530_TX_base;
	TRGMII_7623_base = 0x1B110300;
	TRGMII_7530_base = 0x7A00;
	TRGMII_7530_RD_0 = TRGMII_7530_base + 0x10;
	TRGMII_RCK_CTRL = TRGMII_7623_base;
	rxd_step_size = 0x1;
	rxc_step_size = 0x8;
	init_toggle_data = 0x00000055;
	training_word = 0x000000AC;

	TRGMII_7530_TX_base = TRGMII_7530_base + 0x50;

	tmp = *(volatile u_long *)(DEVINFO_BASE+0x8);
	if(tmp == 0x0000CA00)
	{
		is_mt7623_e1 = 1;
		printf("===MT7623 E1 only===\n");
	}

	//printk("Calibration begin ........\n");
	*(volatile u_long *)(TRGMII_7623_base + 0x40) |= 0x80000000;
	mii_mgr_read(0x1F, 0x7a10, &read_data);
	//printk("TRGMII_7530_RD_0 is %x\n", read_data);

	mii_mgr_read(0x1F,TRGMII_7530_base+0x04,&read_data);
	read_data &= 0x3fffffff;
	mii_mgr_write(0x1F,TRGMII_7530_base+0x04,read_data);     // RX clock gating in MT7530

	mii_mgr_read(0x1F,TRGMII_7530_base+0x78,&read_data);
	read_data |= 0x00002000;
	mii_mgr_write(0x1F,TRGMII_7530_base+0x78,read_data);     // Set TX OE edge in  MT7530

	mii_mgr_read(0x1F,TRGMII_7530_base,&read_data);
	read_data |= 0x80000000;
	mii_mgr_write(0x1F,TRGMII_7530_base,read_data);          // Assert RX  reset in MT7530


	mii_mgr_read(0x1F,TRGMII_7530_base,&read_data);
	read_data &= 0x7fffffff;
	mii_mgr_write(0x1F,TRGMII_7530_base,read_data);          // Release RX reset in MT7530

	mii_mgr_read(0x1F,TRGMII_7530_base+0x04,&read_data);
	read_data |= 0xC0000000;
	mii_mgr_write(0x1F,TRGMII_7530_base+0x04,read_data);     // Disable RX clock gating in MT7530

	//printk("Enable Training Mode in MT7623\n");
	/*Enable Training Mode in MT7623*/
	*(volatile u_long *)(TRGMII_7623_base + 0x40) &= 0xbfffffff;
	if(is_mt7623_e1)
	*(volatile u_long *)(TRGMII_7623_base + 0x40) |= 0x80000000;
	else
#if defined (CONFIG_GE1_TRGMII_FORCE_2000)
		*(volatile u_long *)(TRGMII_7623_base + 0x40) |= 0xc0000000;
#else
        	*(volatile u_long *)(TRGMII_7623_base + 0x40) |= 0x80000000;
#endif
	*(volatile u_long *)(TRGMII_7623_base + 0x78) &= 0xfffff0ff;
	if(is_mt7623_e1)
	*(volatile u_long *)(TRGMII_7623_base + 0x78) |= 0x00000400;
	else{	
		*(volatile u_long *)(TRGMII_7623_base + 0x50) &= 0xfffff0ff;        
		*(volatile u_long *)(TRGMII_7623_base + 0x58) &= 0xfffff0ff;
		*(volatile u_long *)(TRGMII_7623_base + 0x60) &= 0xfffff0ff;
		*(volatile u_long *)(TRGMII_7623_base + 0x68) &= 0xfffff0ff;
		*(volatile u_long *)(TRGMII_7623_base + 0x70) &= 0xfffff0ff;
		*(volatile u_long *)(TRGMII_7623_base + 0x78) |= 0x00000800; 
	} 
        //==========================================

	err_total_flag =0;
	//printk("Adjust RXC delay in MT7530\n");
	read_data =0x0;
	while (err_total_flag == 0 && (read_data != 0x68)) {
		//printk("2nd Enable EDGE CHK in MT7530\n");
		/* Enable EDGE CHK in MT7530*/
		for (i = 0 ; i<5 ; i++) {
			mii_mgr_read(0x1F,TRGMII_7530_RD_0+i*8,&read_data);
			read_data |= 0x40000000;
			read_data &= 0x4fffffff;
			mii_mgr_write(0x1F,TRGMII_7530_RD_0+i*8,read_data);
		        wait_loop();
		        //printk("2nd Disable EDGE CHK in MT7530\n");
			mii_mgr_read(0x1F,TRGMII_7530_RD_0+i*8,&err_cnt[i]);
		        //printk("***** MT7530 %dth bit ERR_CNT =%x\n",i, err_cnt[i]);
		        //printk("MT7530 %dth bit ERR_CNT =%x\n",i, err_cnt[i]);
			err_cnt[i] >>= 8;
			err_cnt[i] &= 0x0000ff0f;
			rd_wd  = err_cnt[i] >> 8;
		        rd_wd &= 0x000000ff;	
			err_cnt[i] &= 0x0000000f;
			//mii_mgr_read(0x1F,0x7a10,&read_data);
			if ( err_cnt[i] !=0 ) {
				err_flag[i] = 1;
			}
			else if (rd_wd != 0x55) {
                                err_flag[i] = 1;
			} else {	
				err_flag[i] = 0;
			}
			if (i==0) {
			   err_total_flag = err_flag[i];
			} else {
			   err_total_flag = err_flag[i] & err_total_flag;
			}    	
		/* Disable EDGE CHK in MT7530*/
			mii_mgr_read(0x1F,TRGMII_7530_RD_0+i*8,&read_data);
			read_data |= 0x40000000;
			read_data &= 0x4fffffff;
			mii_mgr_write(0x1F,TRGMII_7530_RD_0+i*8,read_data);
		          wait_loop();
		}
		/*Adjust RXC delay*/
		if (err_total_flag ==0) {
	           mii_mgr_read(0x1F,TRGMII_7530_base,&read_data);
	           read_data |= 0x80000000;
	           mii_mgr_write(0x1F,TRGMII_7530_base,read_data);          // Assert RX  reset in MT7530

		   mii_mgr_read(0x1F,TRGMII_7530_base+0x04,&read_data);
		   read_data &= 0x3fffffff;
		   mii_mgr_write(0x1F,TRGMII_7530_base+0x04,read_data);       // RX clock gating in MT7530

		   mii_mgr_read(0x1F,TRGMII_7530_base,&read_data);
		   tmp = read_data;
		   tmp &= 0x0000007f;
		   tmp += rxc_step_size;
		   //printk("Current rxc delay = %d\n", tmp);
		   read_data &= 0xffffff80;
		   read_data |= tmp;
		   mii_mgr_write (0x1F,TRGMII_7530_base,read_data);
		   mii_mgr_read(0x1F,TRGMII_7530_base,&read_data);
		   //printk("Current RXC delay = %x\n", read_data); 

	           mii_mgr_read(0x1F,TRGMII_7530_base,&read_data);
	           read_data &= 0x7fffffff;
	           mii_mgr_write(0x1F,TRGMII_7530_base,read_data);          // Release RX reset in MT7530

		   mii_mgr_read(0x1F,TRGMII_7530_base+0x04,&read_data);
		   read_data |= 0xc0000000;
		   mii_mgr_write(0x1F,TRGMII_7530_base+0x04,read_data);       // Disable RX clock gating in MT7530
                }
		read_data = tmp;
	}
	//printk("RXC delay is %d\n", tmp);
	//printk("Finish RXC Adjustment while loop\n");

	//printk("Read RD_WD MT7530\n");
	/* Read RD_WD MT7530*/
	for  (i = 0 ; i<5 ; i++) {
		rd_tap = 0;
		while (err_flag[i] != 0 && rd_tap != 128) {
			/* Enable EDGE CHK in MT7530*/
			mii_mgr_read(0x1F,TRGMII_7530_RD_0+i*8,&read_data);
			read_data |= 0x40000000;
			read_data &= 0x4fffffff;
			mii_mgr_write(0x1F,TRGMII_7530_RD_0+i*8,read_data);
		        wait_loop();
			err_cnt[i] = (read_data >> 8) & 0x0000000f; 
		        rd_wd = (read_data >> 16) & 0x000000ff;
		        //printk("##### %dth bit  ERR_CNT = %x RD_WD =%x ######\n", i, err_cnt[i],rd_wd);
			if (err_cnt[i] != 0 || rd_wd !=0x55){
		           err_flag [i] =  1;
			}   
			else {
			   err_flag[i] =0;
		        }	
			if (err_flag[i] !=0 ) { 
			   rd_tap = (read_data & 0x0000007f) + rxd_step_size;                        // Add RXD delay in MT7530
			   read_data = (read_data & 0xffffff80) | rd_tap;
			   mii_mgr_write(0x1F,TRGMII_7530_RD_0+i*8,read_data);
			   tap_a[i] = rd_tap;
			} else {
			   tap_a[i] = (read_data & 0x0000007f);			                    // Record the min delay TAP_A
	                   rd_tap   =  tap_a[i] + 0x4;  		   
			   read_data = (read_data & 0xffffff80) | rd_tap  ;
			   mii_mgr_write(0x1F,TRGMII_7530_RD_0+i*8,read_data);
			}	

			/* Disable EDGE CHK in MT7530*/
			mii_mgr_read(0x1F,TRGMII_7530_RD_0+i*8,&read_data);
			read_data |= 0x40000000;
			read_data &= 0x4fffffff;
			mii_mgr_write(0x1F,TRGMII_7530_RD_0+i*8,read_data);
		        wait_loop();

		}
		//printf("7530 %dth bit  Tap_a = %d\n", i, tap_a[i]);
	}
	//printk("Last While Loop\n");
	for  (i = 0 ; i<5 ; i++) {
	rd_tap =0;
		while (err_flag[i] == 0 && (rd_tap!=128)) {
			/* Enable EDGE CHK in MT7530*/
			mii_mgr_read(0x1F,TRGMII_7530_RD_0+i*8,&read_data);
			read_data |= 0x40000000;
			read_data &= 0x4fffffff;
			mii_mgr_write(0x1F,TRGMII_7530_RD_0+i*8,read_data);
			wait_loop();
#if 0
			err_cnt[i] = (read_data >> 8) & 0x0000000f;
#else
			err_cnt[i] = (read_data >> 8) & 0x0000000f;
			rd_wd = (read_data >> 16) & 0x000000ff;
			if (err_cnt[i] != 0 || rd_wd !=0x55){
				err_flag [i] =  1;
			}
			else {
				err_flag[i] =0;
			}
#endif
			//rd_tap = (read_data & 0x0000007f) + 0x4;                                    // Add RXD delay in MT7530
			if (err_cnt[i] == 0 && (rd_tap!=128)) {
			    rd_tap = (read_data & 0x0000007f) + rxd_step_size;                        // Add RXD delay in MT7530
			    read_data = (read_data & 0xffffff80) | rd_tap;
			    mii_mgr_write(0x1F,TRGMII_7530_RD_0+i*8,read_data);
			}    
			/* Disable EDGE CHK in MT7530*/
			mii_mgr_read(0x1F,TRGMII_7530_RD_0+i*8,&read_data);
			read_data |= 0x40000000;
			read_data &= 0x4fffffff;
			mii_mgr_write(0x1F,TRGMII_7530_RD_0+i*8,read_data);
			wait_loop();
		}
		tap_b[i] = rd_tap;// - rxd_step_size;                                     // Record the max delay TAP_B
		//printf("%dth bit  Tap_b = %d,\n", i, tap_b[i]);
		final_tap[i] = (tap_a[i]+tap_b[i])/2;                                     //  Calculate RXD delay = (TAP_A + TAP_B)/2
		//printk("%dth bit Final Tap = %d\n", i, final_tap[i]);

		read_data = ( read_data & 0xffffff80) | final_tap[i];
		mii_mgr_write(0x1F,TRGMII_7530_RD_0+i*8,read_data);
	}
	if(is_mt7623_e1)
	        *(volatile u_long *)(TRGMII_7623_base + 0x40) &=0x3fffffff;
	else
#if defined (CONFIG_GE1_TRGMII_FORCE_2000)
	        *(volatile u_long *)(TRGMII_7623_base + 0x40) &=0x7fffffff;
#else
                *(volatile u_long *)(TRGMII_7623_base + 0x40) &=0x3fffffff;
#endif
	
}
#endif 	/* defined(MT7623_ASIC_BOARD) */

#if defined(MT7622_ASIC_BOARD)
static void mt7622_ethifsys_pinmux(void)
{
#define GE2_RGMII_GPIO	0x10211300

	u32 temp;

	//temp = sys_reg_read(ge2_rgmii);
	temp = (le32_to_cpu(*(volatile u_long *)GE2_RGMII_GPIO));
	temp &= 0xffff0fff;
	//sys_reg_write(ge2_rgmii, temp);
	*(volatile u_long *)(GE2_RGMII_GPIO) = temp;
}

static void mt7622_ethifsys_init(void)
{
#define INFRACFG_A0_BASE		0x10000000
#define INFRA_TOPAXI_PROTECTEN	(INFRACFG_A0_BASE + 0x220)

#define SLEEP_BASE				0x10006000
#define POWRON_CONFIG_EN		(SLEEP_BASE + 0x000)
#define SLEEP_ETH_PWR_CON		(SLEEP_BASE + 0x2E0)
#define SLEEP_PWR_STA			(SLEEP_BASE + 0x60C)
#define SLEEP_PWR_STAS			(SLEEP_BASE + 0x610)

#define PWR_RST_0_D	0
#define PWR_ISO_1_D	1
#define PWR_ONN_1_D	2
#define PWR_ONS_1_D	3
#define PWR_CKD_1_D	4
#define PWR_SRAM_PDN_LSB	8

	/* power on mtcmos */
	u32 eth_pwr_on_seq0 = (0xF << PWR_SRAM_PDN_LSB) | (0x0 << PWR_ONS_1_D) |
						  (0x1 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x1 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D);
	u32 eth_pwr_on_seq1 = (0xF << PWR_SRAM_PDN_LSB) | (0x1 << PWR_ONS_1_D) |
						  (0x1 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x1 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D);
	u32 eth_pwr_on_seq2 = (0xF << PWR_SRAM_PDN_LSB) | (0x1 << PWR_ONS_1_D) |
						  (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x1 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D);
	u32 eth_pwr_on_seq3 = (0xF << PWR_SRAM_PDN_LSB) | (0x1 << PWR_ONS_1_D) |
						  (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x0 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D);
	/* power on ini sram */
	u32 eth_pwr_on_seq4 = (0xD << PWR_SRAM_PDN_LSB) | (0x1 << PWR_ONS_1_D) |
						  (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x0 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D);
	u32 eth_pwr_on_seq5 = (0x9 << PWR_SRAM_PDN_LSB) | (0x1 << PWR_ONS_1_D) |
						  (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x0 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D);
	u32 eth_pwr_on_seq6 = (0x1 << PWR_SRAM_PDN_LSB) | (0x1 << PWR_ONS_1_D) |
						  (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x0 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D);
	/* power on mtcmos */
	u32 eth_pwr_on_seq7 = (0x1 << PWR_SRAM_PDN_LSB) | (0x1 << PWR_ONS_1_D) |
						  (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x0 << PWR_ISO_1_D) | (0x1 << PWR_RST_0_D);
	/* power on sram */
	u32 eth_pwr_on_seq8 = (0x0 << PWR_SRAM_PDN_LSB) | (0x1 << PWR_ONS_1_D) |
						  (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x0 << PWR_ISO_1_D) | (0x1 << PWR_RST_0_D);
	u32 pwr_ack_status, pwr_acks_status, temp;
	/*=========================================================================*/
	/* Enable ETHPLL & TRGPLL*/
	/*=========================================================================*/
	/* PLL init in preloader */

	/*=========================================================================*/
	/* Power on ETHDMASYS and HIFSYS*/
	/*=========================================================================*/
	/* Power on ETHDMASYS*/
	//sys_reg_write(POWRON_CONFIG_EN, 0x0B160001);
	*(volatile u_long *)(POWRON_CONFIG_EN) = 0x0B160001;
	//pwr_ack_status = (sys_reg_read(SLEEP_PWR_STA) & 0x01000000) >> 24;
	pwr_ack_status = (le32_to_cpu(*(volatile u_long *)(SLEEP_PWR_STA)) & 0x01000000) >> 24;
	//pwr_acks_status = (sys_reg_read(SLEEP_PWR_STAS) & 0x01000000) >> 24;
	pwr_acks_status = (le32_to_cpu(*(volatile u_long *)(SLEEP_PWR_STAS)) & 0x01000000) >> 24;

	if ((pwr_ack_status != 0x0) || (pwr_acks_status != 0x0)) {
		printf("ETH already turn on and power on flow will be skipped...\n");
	} else {
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq0);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq0;
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq1);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq1;
		while ((pwr_ack_status != 0x0) || (pwr_acks_status != 0x0)) {
			//pwr_ack_status = (sys_reg_read(SLEEP_PWR_STA) & 0x01000000) >> 24;
			pwr_ack_status = (le32_to_cpu(*(volatile u_long *)(SLEEP_PWR_STA)) & 0x01000000) >> 24;
			//pwr_acks_status = (sys_reg_read(SLEEP_PWR_STAS) & 0x01000000) >> 24;
			pwr_acks_status = (le32_to_cpu(*(volatile u_long *)(SLEEP_PWR_STAS)) & 0x01000000) >> 24;
			printf("Wait for power up ETH...\n");
		}
		printf("Power up ETH Done !!\n");
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq2);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq2;
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq3);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq3;
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq4);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq4;
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq5);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq5;
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq6);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq6;
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq7);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq7;
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq8);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq8;
		udelay(100);

		//temp = sys_reg_read(INFRA_TOPAXI_PROTECTEN);
		temp = le32_to_cpu(*(volatile u_long *)(INFRA_TOPAXI_PROTECTEN));
		//sys_reg_write(INFRA_TOPAXI_PROTECTEN, temp & 0xfffdfff7);	/* [3], [17] */
		*(volatile u_long *)(INFRA_TOPAXI_PROTECTEN) = temp & 0xfffdfff7;
		printf("ETH Protect Released...!!\n");
	}
}

#if defined (CONFIG_GE1_SGMII_FORCE_2500)
static void mt7622_switch_reset(void)
{
	unsigned int reg_value;
	
		/* set GPIO54 as GPIO mode */
#define GPIO_MODE2 0x10211330
		reg_value = le32_to_cpu(*(volatile u_long *)(GPIO_MODE2));
		reg_value &= 0xfff0ffff;
		reg_value |= (1 << 16); /* 1: GPIO54 (IO) */
		*(volatile u_long *)(GPIO_MODE2) = reg_value;

		/* set GPIO54 as output mode */
#define GPIO_DIR1 0x10211010
		reg_value = le32_to_cpu(*(volatile u_long *)(GPIO_DIR1));
		reg_value |= (1 << 22);	/* output */
		*(volatile u_long *)(GPIO_DIR1) = reg_value;

		/* set GPIO54 output low */
#define GPIO_DOUT2 0x10211110
		reg_value = le32_to_cpu(*(volatile u_long *)(GPIO_DOUT2));
		reg_value &= ~(1 << 22); /* output 0 */
		*(volatile u_long *)(GPIO_DOUT2) = reg_value;
		mdelay(100);

		/* set GPIO54 output high */
		reg_value = le32_to_cpu(*(volatile u_long *)(GPIO_DOUT2));
		reg_value |= (1 << 22); /* output 1 */
		*(volatile u_long *)(GPIO_DOUT2) = reg_value;
		mdelay(100);
}

static void mt7622_rtl8367_sw_init(void)
{
#if defined (CONFIG_RTL8367)
	int ret = 100;
	unsigned int reg_value;

	ret = rtk_switch_init();
	printf("rtk_switch_init ret = %d!!!!!!!!!!!!\n", ret);
	mdelay(500);

	reg_value = 0;
	smi_write(0x13A0, 0x5678);
	mdelay(100);
	smi_read(0x13A0, &reg_value);
	printf("rtk_switch reg = 0x%x !!!!!!!!!!!!\n", reg_value);
	printf("Set RTL8367S SGMII 2.5Gbps\n");
	rtk_port_mac_ability_t mac_cfg ;
	rtk_mode_ext_t mode ;

	mode = MODE_EXT_HSGMII;
	mac_cfg.forcemode = MAC_FORCE;
	mac_cfg.speed = PORT_SPEED_2500M;
	mac_cfg.duplex = PORT_FULL_DUPLEX;
	mac_cfg.link = PORT_LINKUP;
	mac_cfg.nway = DISABLED;
	mac_cfg.txpause = DISABLED;
	mac_cfg.rxpause = DISABLED;
	ret = rtk_port_macForceLinkExt_set(EXT_PORT0, mode,&mac_cfg);
	printf("rtk_port_macForceLinkExt_set return value is %d\n", ret);
	ret = rtk_port_sgmiiNway_set(EXT_PORT0, DISABLED);
	printf("rtk_port_sgmiiNway_set return value is %d\n", ret);
#endif	/* CONFIG_RTL8367 */
}

static void mt7622_ethifsys_sgmii_init(void)
{
	unsigned int reg_value;

	/* Enable SGMII and choose GMAC2 */
#define ETHSYS_BASE 0x1b000000
	//reg_value = sys_reg_read(virt_addr + 0x14);
	reg_value = le32_to_cpu(*(volatile u_long *)(ETHSYS_BASE + 0x14));
	printf(" 0x1b000014 = 0x%08x\n", reg_value);
	reg_value |= (1 << 9);
	reg_value &= ~(1 << 8);	/* GMAC1 */
	//sys_reg_write(virt_addr + 0x14, reg_value);
	*(volatile u_long *)(ETHSYS_BASE + 0x14) = reg_value;
	//reg_value = sys_reg_read(virt_addr + 0x14);
	reg_value = le32_to_cpu(*(volatile u_long *)(ETHSYS_BASE + 0x14));

	/* set GMAC1 forced link at 1Gbps FDX */
#define GMAC1_BASE 0x1b110100
	//sys_reg_write(GMAC1_BASE, 0x2105e30b);
	*(volatile u_long *)(GMAC1_BASE) = 0x2105e30b;

	/* Set SGMII GEN2 speed(2.5G) */
#define SGMII_GEN_BASE 0x1b12a000
	//reg_value = sys_reg_read(SGMII_GEN_BASE + 0x28);
	reg_value = le32_to_cpu(*(volatile u_long *)(SGMII_GEN_BASE + 0x28));
	reg_value |= 1 << 2;
	//sys_reg_write(SGMII_GEN_BASE + 0x28, reg_value);
	*(volatile u_long *)(SGMII_GEN_BASE + 0x28) = reg_value;

	/* disable SGMII AN */
#define SGMII_AN_BASE 0x1b128000
	//reg_value = sys_reg_read(SGMII_AN_BASE);
	reg_value = le32_to_cpu(*(volatile u_long *)(SGMII_AN_BASE));
	reg_value &= ~(1 << 12);
	//sys_reg_write(SGMII_AN_BASE, reg_value);
	*(volatile u_long *)(SGMII_AN_BASE) = reg_value;

	/* SGMII force mode setting */
	//reg_value = sys_reg_read(SGMII_AN_BASE + 0x20);
	reg_value = le32_to_cpu(*(volatile u_long *)(SGMII_AN_BASE + 0x20));
	//sys_reg_write(SGMII_AN_BASE + 0x20, 0x31120019);
	*(volatile u_long *)(SGMII_AN_BASE + 0x20) = 0x31120019;
	//reg_value = sys_reg_read(SGMII_AN_BASE + 0x20);
	reg_value = le32_to_cpu(*(volatile u_long *)(SGMII_AN_BASE + 0x20));

	/* Release PHYA power down state */
	//reg_value = sys_reg_read(SGMII_AN_BASE + 0xE8);
	reg_value = le32_to_cpu(*(volatile u_long *)(SGMII_AN_BASE + 0xE8));
	reg_value &= ~(1 << 4);
	//sys_reg_write(SGMII_AN_BASE + 0xE8, reg_value);
	*(volatile u_long *)(SGMII_AN_BASE + 0xE8) = reg_value;
}
#endif	/* CONFIG_GE1_SGMII_FORCE_2500 */

#if defined (CONFIG_GE1_ESW)
static void mt7622_ethifsys_esw_init(void)
{
	/* EPHY setting */
	*(volatile u_long *)(0x1b11000c) = 1;
	*(volatile u_long *)(0x1b118084) = 0xffdf1f00;
	*(volatile u_long *)(0x1b118090) = 0x7f7f;
	*(volatile u_long *)(0x1b1180c8) = 0x05503f38;
	
	/* To CPU setting */
	*(volatile u_long *)(0x1b11808c) = 0x02404040;

	/* vlan untag */
	*(volatile u_long *)(0x1b118098) = 0x7f7f;

	/* AGPIO setting */
	*(volatile u_long *)(0x102118F0) = 0x00555555;
}
#endif
#endif /* defined(MT7622_ASIC_BOARD) */



#if (defined(MT7626_ASIC_BOARD)) || (defined(MT7626_FPGA_BOARD))
static void mt7626_ethifsys_pinmux(void)
{
#define GE2_RGMII_GPIO	0x10211300

	u32 temp;

	//temp = sys_reg_read(ge2_rgmii);
	temp = (le32_to_cpu(*(volatile u_long *)GE2_RGMII_GPIO));
	temp &= 0xffff0fff;
	//sys_reg_write(ge2_rgmii, temp);
	*(volatile u_long *)(GE2_RGMII_GPIO) = temp;
}

static void mt7626_ethifsys_init(void)
{
#define INFRACFG_A0_BASE		0x10000000
#define INFRA_TOPAXI_PROTECTEN	(INFRACFG_A0_BASE + 0x220)

#define SLEEP_BASE				0x10006000
#define POWRON_CONFIG_EN		(SLEEP_BASE + 0x000)
#define SLEEP_ETH_PWR_CON		(SLEEP_BASE + 0x2E0)
#define SLEEP_PWR_STA			(SLEEP_BASE + 0x60C)
#define SLEEP_PWR_STAS			(SLEEP_BASE + 0x610)

#define PWR_RST_0_D	0
#define PWR_ISO_1_D	1
#define PWR_ONN_1_D	2
#define PWR_ONS_1_D	3
#define PWR_CKD_1_D	4
#define PWR_SRAM_PDN_LSB	8

	/* power on mtcmos */
	u32 eth_pwr_on_seq0 = (0xF << PWR_SRAM_PDN_LSB) | (0x0 << PWR_ONS_1_D) |
						  (0x1 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x1 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D);
	u32 eth_pwr_on_seq1 = (0xF << PWR_SRAM_PDN_LSB) | (0x1 << PWR_ONS_1_D) |
						  (0x1 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x1 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D);
	u32 eth_pwr_on_seq2 = (0xF << PWR_SRAM_PDN_LSB) | (0x1 << PWR_ONS_1_D) |
						  (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x1 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D);
	u32 eth_pwr_on_seq3 = (0xF << PWR_SRAM_PDN_LSB) | (0x1 << PWR_ONS_1_D) |
						  (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x0 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D);
	/* power on ini sram */
	u32 eth_pwr_on_seq4 = (0xD << PWR_SRAM_PDN_LSB) | (0x1 << PWR_ONS_1_D) |
						  (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x0 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D);
	u32 eth_pwr_on_seq5 = (0x9 << PWR_SRAM_PDN_LSB) | (0x1 << PWR_ONS_1_D) |
						  (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x0 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D);
	u32 eth_pwr_on_seq6 = (0x1 << PWR_SRAM_PDN_LSB) | (0x1 << PWR_ONS_1_D) |
						  (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x0 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D);
	/* power on mtcmos */
	u32 eth_pwr_on_seq7 = (0x1 << PWR_SRAM_PDN_LSB) | (0x1 << PWR_ONS_1_D) |
						  (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x0 << PWR_ISO_1_D) | (0x1 << PWR_RST_0_D);
	/* power on sram */
	u32 eth_pwr_on_seq8 = (0x0 << PWR_SRAM_PDN_LSB) | (0x1 << PWR_ONS_1_D) |
						  (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) |
						  (0x0 << PWR_ISO_1_D) | (0x1 << PWR_RST_0_D);
	u32 pwr_ack_status, pwr_acks_status, temp;
	/*=========================================================================*/
	/* Enable ETHPLL & TRGPLL*/
	/*=========================================================================*/
	/* PLL init in preloader */

	/*=========================================================================*/
	/* Power on ETHDMASYS and HIFSYS*/
	/*=========================================================================*/
	/* Power on ETHDMASYS*/
	//sys_reg_write(POWRON_CONFIG_EN, 0x0B160001);
	*(volatile u_long *)(POWRON_CONFIG_EN) = 0x0B160001;
	//pwr_ack_status = (sys_reg_read(SLEEP_PWR_STA) & 0x01000000) >> 24;
	pwr_ack_status = (le32_to_cpu(*(volatile u_long *)(SLEEP_PWR_STA)) & 0x01000000) >> 24;
	//pwr_acks_status = (sys_reg_read(SLEEP_PWR_STAS) & 0x01000000) >> 24;
	pwr_acks_status = (le32_to_cpu(*(volatile u_long *)(SLEEP_PWR_STAS)) & 0x01000000) >> 24;

	if ((pwr_ack_status != 0x0) || (pwr_acks_status != 0x0)) {
		printf("ETH already turn on and power on flow will be skipped...\n");
	} else {
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq0);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq0;
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq1);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq1;
		while ((pwr_ack_status != 0x0) || (pwr_acks_status != 0x0)) {
			//pwr_ack_status = (sys_reg_read(SLEEP_PWR_STA) & 0x01000000) >> 24;
			pwr_ack_status = (le32_to_cpu(*(volatile u_long *)(SLEEP_PWR_STA)) & 0x01000000) >> 24;
			//pwr_acks_status = (sys_reg_read(SLEEP_PWR_STAS) & 0x01000000) >> 24;
			pwr_acks_status = (le32_to_cpu(*(volatile u_long *)(SLEEP_PWR_STAS)) & 0x01000000) >> 24;
			printf("Wait for power up ETH...\n");
		}
		printf("Power up ETH Done !!\n");
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq2);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq2;
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq3);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq3;
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq4);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq4;
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq5);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq5;
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq6);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq6;
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq7);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq7;
		//temp = sys_reg_read(SLEEP_ETH_PWR_CON);
		temp = le32_to_cpu(*(volatile u_long *)(SLEEP_ETH_PWR_CON));
		//sys_reg_write(SLEEP_ETH_PWR_CON, (temp & 0xfffff0e0) | eth_pwr_on_seq8);
		*(volatile u_long *)(SLEEP_ETH_PWR_CON) = (temp & 0xfffff0e0) | eth_pwr_on_seq8;
		udelay(100);

		//temp = sys_reg_read(INFRA_TOPAXI_PROTECTEN);
		temp = le32_to_cpu(*(volatile u_long *)(INFRA_TOPAXI_PROTECTEN));
		//sys_reg_write(INFRA_TOPAXI_PROTECTEN, temp & 0xfffdfff7);	/* [3], [17] */
		*(volatile u_long *)(INFRA_TOPAXI_PROTECTEN) = temp & 0xfffdfff7;
		printf("ETH Protect Released...!!\n");
	}
}

#if defined (CONFIG_GE1_SGMII_FORCE_2500)
static void mt7626_switch_reset(void)
{
	unsigned int reg_value;
		/* set GPIO54 as GPIO mode */
#define GPIO_MODE2 0x10211330
		reg_value = le32_to_cpu(*(volatile u_long *)(GPIO_MODE2));
		reg_value &= 0xfff0ffff;
		reg_value |= (1 << 16); /* 1: GPIO54 (IO) */
		*(volatile u_long *)(GPIO_MODE2) = reg_value;

		/* set GPIO54 as output mode */
#define GPIO_DIR1 0x10211010
		reg_value = le32_to_cpu(*(volatile u_long *)(GPIO_DIR1));
		reg_value |= (1 << 22);	/* output */
		*(volatile u_long *)(GPIO_DIR1) = reg_value;

		/* set GPIO54 output low */
#define GPIO_DOUT2 0x10211110
		reg_value = le32_to_cpu(*(volatile u_long *)(GPIO_DOUT2));
		reg_value &= ~(1 << 22); /* output 0 */
		*(volatile u_long *)(GPIO_DOUT2) = reg_value;
		mdelay(100);

		/* set GPIO54 output high */
		reg_value = le32_to_cpu(*(volatile u_long *)(GPIO_DOUT2));
		reg_value |= (1 << 22); /* output 1 */
		*(volatile u_long *)(GPIO_DOUT2) = reg_value;
		mdelay(100);
}

static void mt7626_rtl8367_sw_init(void)
{
#if defined (CONFIG_RTL8367)
	int ret = 100;
	unsigned int reg_value;

	ret = rtk_switch_init();
	printf("rtk_switch_init ret = %d!!!!!!!!!!!!\n", ret);
	mdelay(500);

	reg_value = 0;
	smi_write(0x13A0, 0x5678);
	mdelay(100);
	smi_read(0x13A0, &reg_value);
	printf("rtk_switch reg = 0x%x !!!!!!!!!!!!\n", reg_value);
	printf("Set RTL8367S SGMII 2.5Gbps\n");
	rtk_port_mac_ability_t mac_cfg ;
	rtk_mode_ext_t mode ;

	mode = MODE_EXT_HSGMII;
	mac_cfg.forcemode = MAC_FORCE;
	mac_cfg.speed = PORT_SPEED_2500M;
	mac_cfg.duplex = PORT_FULL_DUPLEX;
	mac_cfg.link = PORT_LINKUP;
	mac_cfg.nway = DISABLED;
	mac_cfg.txpause = DISABLED;
	mac_cfg.rxpause = DISABLED;
	ret = rtk_port_macForceLinkExt_set(EXT_PORT0, mode,&mac_cfg);
	printf("rtk_port_macForceLinkExt_set return value is %d\n", ret);
	ret = rtk_port_sgmiiNway_set(EXT_PORT0, DISABLED);
	printf("rtk_port_sgmiiNway_set return value is %d\n", ret);
#endif	/* CONFIG_RTL8367 */
}

static void mt7626_ethifsys_sgmii_init(void)
{
	unsigned int reg_value;

	/* Enable SGMII and choose GMAC2 */
#define ETHSYS_BASE 0x1b000000
	//reg_value = sys_reg_read(virt_addr + 0x14);
	reg_value = le32_to_cpu(*(volatile u_long *)(ETHSYS_BASE + 0x14));
	printf(" 0x1b000014 = 0x%08x\n", reg_value);
	reg_value |= (1 << 9);
	reg_value &= ~(1 << 8);	/* GMAC1 */
	//sys_reg_write(virt_addr + 0x14, reg_value);
	*(volatile u_long *)(ETHSYS_BASE + 0x14) = reg_value;
	//reg_value = sys_reg_read(virt_addr + 0x14);
	reg_value = le32_to_cpu(*(volatile u_long *)(ETHSYS_BASE + 0x14));

	/* set GMAC1 forced link at 1Gbps FDX */
#define GMAC1_BASE 0x1b110100
	//sys_reg_write(GMAC1_BASE, 0x2105e30b);
	*(volatile u_long *)(GMAC1_BASE) = 0x2105e30b;

	/* Set SGMII GEN2 speed(2.5G) */
#define SGMII_GEN_BASE 0x1b12a000
	//reg_value = sys_reg_read(SGMII_GEN_BASE + 0x28);
	reg_value = le32_to_cpu(*(volatile u_long *)(SGMII_GEN_BASE + 0x28));
	reg_value |= 1 << 2;
	//sys_reg_write(SGMII_GEN_BASE + 0x28, reg_value);
	*(volatile u_long *)(SGMII_GEN_BASE + 0x28) = reg_value;

	/* disable SGMII AN */
#define SGMII_AN_BASE 0x1b128000
	//reg_value = sys_reg_read(SGMII_AN_BASE);
	reg_value = le32_to_cpu(*(volatile u_long *)(SGMII_AN_BASE));
	reg_value &= ~(1 << 12);
	//sys_reg_write(SGMII_AN_BASE, reg_value);
	*(volatile u_long *)(SGMII_AN_BASE) = reg_value;

	/* SGMII force mode setting */
	//reg_value = sys_reg_read(SGMII_AN_BASE + 0x20);
	reg_value = le32_to_cpu(*(volatile u_long *)(SGMII_AN_BASE + 0x20));
	//sys_reg_write(SGMII_AN_BASE + 0x20, 0x31120019);
	*(volatile u_long *)(SGMII_AN_BASE + 0x20) = 0x31120019;
	//reg_value = sys_reg_read(SGMII_AN_BASE + 0x20);
	reg_value = le32_to_cpu(*(volatile u_long *)(SGMII_AN_BASE + 0x20));

	/* Release PHYA power down state */
	//reg_value = sys_reg_read(SGMII_AN_BASE + 0xE8);
	reg_value = le32_to_cpu(*(volatile u_long *)(SGMII_AN_BASE + 0xE8));
	reg_value &= ~(1 << 4);
	//sys_reg_write(SGMII_AN_BASE + 0xE8, reg_value);
	*(volatile u_long *)(SGMII_AN_BASE + 0xE8) = reg_value;
}
#endif	/* CONFIG_GE1_SGMII_FORCE_2500 */

#if defined (CONFIG_GE1_ESW)
static void mt7626_ethifsys_esw_init(void)
{
	/* EPHY setting */
	*(volatile u_long *)(0x1b11000c) = 1;
	*(volatile u_long *)(0x1b118084) = 0xffdf1f00;
	*(volatile u_long *)(0x1b118090) = 0x7f7f;
	*(volatile u_long *)(0x1b1180c8) = 0x05503f38;

	/* To CPU setting */
	*(volatile u_long *)(0x1b11808c) = 0x02404040;

	/* vlan untag */
	*(volatile u_long *)(0x1b118098) = 0x7f7f;

	/* AGPIO setting */
	*(volatile u_long *)(0x102118F0) = 0x00555555;
}
#endif
#endif /* (defined(MT7626_ASIC_BOARD)) || (defined(MT7626_FPGA_BOARD)) */



#if defined (MAC_TO_MT7530_MODE)
static void IsSwitchVlanTableBusy(void)
{
	int j = 0;
	unsigned int value = 0;

	for (j = 0; j < 20; j++) {
	    mii_mgr_read(31, 0x90, &value);
	    if ((value & 0x80000000) == 0 ){ //table busy
		break;
	    }
	    udelay(70000);
	}
	if (j == 20)
	    printf("set vlan timeout value=0x%x.\n", value);
}
#endif

static void LANWANPartition(void)
{
#if defined (MAC_TO_MT7530_MODE)
	/* Set  MT7530 */
	printf("set LAN/WAN LLLLW\n");
	/* LLLLW, wan at P4 */
	/* LAN/WAN ports as security mode */
	mii_mgr_write(31, 0x2004, 0xff0003);	//port0
	mii_mgr_write(31, 0x2104, 0xff0003);	//port1
	mii_mgr_write(31, 0x2204, 0xff0003);	//port2
	mii_mgr_write(31, 0x2304, 0xff0003);	//port3
	mii_mgr_write(31, 0x2404, 0xff0003);	//port4

	/* set PVID */
	mii_mgr_write(31, 0x2014, 0x10001);		//port0
	mii_mgr_write(31, 0x2114, 0x10001);		//port1
	mii_mgr_write(31, 0x2214, 0x10001);		//port2
	mii_mgr_write(31, 0x2314, 0x10001);		//port3
	mii_mgr_write(31, 0x2414, 0x10002);		//port4

	/* VLAN member */
	IsSwitchVlanTableBusy();
	mii_mgr_write(31, 0x94, 0x404f0001);	//VAWD1
	mii_mgr_write(31, 0x90, 0x80001001);	//VTCR, VID=1
	IsSwitchVlanTableBusy();
	mii_mgr_write(31, 0x94, 0x40500001);	//VAWD1
	mii_mgr_write(31, 0x90, 0x80001002);	//VTCR, VID=2
	IsSwitchVlanTableBusy();
#endif
}

static void eth_pinmux_power_init(void)
{
#if defined(MT7623_ASIC_BOARD)
	mt7623_ethifsys_init();
	mt7623_pinmux_set();
#elif defined(MT7622_ASIC_BOARD)
	mt7622_ethifsys_pinmux();
	mt7622_ethifsys_init();
#elif (defined(MT7626_ASIC_BOARD)) || (defined(MT7626_FPGA_BOARD))
	mt7626_ethifsys_pinmux();
	mt7626_ethifsys_init();
#endif
}

static void eth_switch_init(void)
{
#if defined (CONFIG_GE1_SGMII_FORCE_2500)
	mt7622_switch_reset();
	mt7622_rtl8367_sw_init();		/* RTL8367 Init */
	mt7622_ethifsys_sgmii_init();		/* MT7622 SGMII Init */
	rtk_port_phyEnableAll_set(ENABLED);
#elif (defined (CONFIG_GE1_ESW)) && ((defined(MT7622_ASIC_BOARD)) || (defined(MT7622_FPGA_BOARD)))
	mt7622_ethifsys_esw_init();
#elif (defined (CONFIG_GE1_ESW)) && ((defined(MT7626_ASIC_BOARD)) || (defined(MT7626_FPGA_BOARD)))
	mt7626_ethifsys_esw_init();
#elif defined (CONFIG_GE1_TRGMII_FORCE_2000) || defined (CONFIG_GE1_TRGMII_FORCE_2600)
	*(volatile u_long *)(0x1b00002c) |=  (1<<11);
#elif defined (GE_RGMII_FORCE_1000) && defined (CONFIG_USE_GE1)
	*(volatile u_long *)(0x1b00002c) &= ~(1<<11);
#endif	/* CONFIG_GE1_TRGMII_FORCE_2000 */
}

void setup_internal_gsw(void)
{
#if defined(MT7623_ASIC_BOARD)
	u32	i;
	u32	regValue;
	u32     xtal_mode;

	printf("Enter setup_internal_gsw()\n");

#if 0
	/* GE1: RGMII mode setting */
	*(volatile u_long *)(0xfb110300) = 0x80020000;
	*(volatile u_long *)(0xfb110304) = 0x00980000;
	*(volatile u_long *)(0xfb110300) = 0x40020000;
	*(volatile u_long *)(0xfb110304) = 0xc0980000;
	*(volatile u_long *)(0xfb110310) = 0x00000041;
	*(volatile u_long *)(0xfb110318) = 0x00000044;
	*(volatile u_long *)(0xfb110320) = 0x00000043;
	*(volatile u_long *)(0xfb110328) = 0x00000042;
	*(volatile u_long *)(0xfb110330) = 0x00000042;
	*(volatile u_long *)(0xfb110340) = 0x00020000;
	*(volatile u_long *)(0xfb110390) &= 0xfffffff8; //RGMII mode
#else
	/* GE1: TRGMII mode setting */	
	*(volatile u_long *)(0x1b110390) |= 0x00000002; //TRGMII mode
#endif

	// reset switch
        //regValue = *(volatile u_long *)(0x1b00000c);
        /*MT7530 Reset. Flows for MT7623A and MT7623N are both excuted.*/
        /* Should Modify this section if EFUSE is ready*/
        /*For MT7623N reset MT7530*/
        //if(!(regValue & (1<<16)))
        {
                *(volatile u_long *)(0x10005520) &= ~(1<<1);
                udelay(1000);
                *(volatile u_long *)(0x10005520) |= (1<<1);
                mdelay(100);
        }
        //printf("Assert MT7623 RXC reset\n");
        *(volatile u_long *)(0x1b110300) |= 0x80000000;   // Assert MT7623 RXC reset
        /*For MT7623 reset MT7530*/
        *(volatile u_long *)(RALINK_SYSCTL_BASE + 0x34) |= (0x1 << 2);
        udelay(1000);
        *(volatile u_long *)(RALINK_SYSCTL_BASE + 0x34) &= ~(0x1 << 2);
        //mdelay(100);
        mdelay(1000);	/* NOTE(Nelson): workaround for UBNT RFB */

	/* Wait for Switch Reset Completed*/
        for(i=0;i<100;i++)
        {
                mdelay(10);
                mii_mgr_read(31, 0x7800 ,&regValue);
                if(regValue != 0){
                        printf("MT7530 Reset Completed!!\n");
                        break;
                }
                if(i == 99)
                        printf("MT7530 Reset Timeout!!\n");
        }

	for(i=0;i<=4;i++) 
	{	
	       //turn off PHY
	       mii_mgr_read(i, 0x0 ,&regValue);
	       regValue |= (0x1<<11);
               mii_mgr_write(i, 0x0, regValue);
        }
        mii_mgr_write(31, 0x7000, 0x3); //reset switch
        udelay(100);

        //sysRegWrite(RALINK_ETH_SW_BASE+0x100, 0x2105e33b);//(GE1, Force 1000M/FD, FC ON)
        RALINK_REG(RALINK_ETH_SW_BASE+0x100) = 0x2105e33b;  //(GE1, Force 1000M/FD, FC ON)
        mii_mgr_write(31, 0x3600, 0x5e33b);
        mii_mgr_read(31, 0x3600 ,&regValue);
        //sysRegWrite(RALINK_ETH_SW_BASE+0x200, 0x00008000);//(GE2, Link down)
        RALINK_REG(RALINK_ETH_SW_BASE+0x200) = 0x00008000;  //(GE2, Link down)

        //regValue = 0x117ccf; //Enable Port 6, P5 as GMAC5, P5 disable*/
        mii_mgr_read(31, 0x7804 ,&regValue);
#if defined (CONFIG_USE_GE1)
        regValue &= ~(1<<8); //Enable Port 6
        regValue |= (1<<6); //Disable Port 5
        regValue &= ~(1<<13); // Port5 connects to internal phy

#elif defined (CONFIG_USE_GE2)
        //RGMII2=Normal mode
        *(volatile u_long *)(RALINK_SYSCTL_BASE + 0x60) &= ~(0x1 << 15);

        //GMAC2= RGMII mode
        *(volatile u_long *)(SYSCFG1) &= ~(0x3 << 14);
        mii_mgr_write(31, 0x3500, 0x56300); //MT7530 P5 AN, we can ignore this setting??????
        //sysRegWrite(RALINK_ETH_SW_BASE+0x200, 0x21056300);//(GE2, auto-polling)
        RALINK_REG(RALINK_ETH_SW_BASE+0x200) = 0x21056300; //(GE2, auto-polling)
        enable_auto_negotiate();//set polling address

        /* set MT7530 Port 5 to PHY 0/4 mode */
#if defined (GE_RGMII_INTERNAL_P0_AN)
        regValue &= ~((1<<13)|(1<<6));
        regValue |= ((1<<7)|(1<<16)|(1<<20));
#elif defined (GE_RGMII_INTERNAL_P4_AN)
        regValue &= ~((1<<13)|(1<<6)|(1<<20));
        regValue |= ((1<<7)|(1<<16));
#endif
#endif	/* CONFIG_USE_GE1 */

        /*Set MT7530 phy direct access mode**/
        regValue &= ~(1<<5);

        regValue |= (1<<16);//change HW-TRAP
        printf("change HW-TRAP to 0x%x\n",regValue);
        mii_mgr_write(31, 0x7804 ,regValue);

	mii_mgr_read(31, 0x7800, &regValue);
        regValue = (regValue >> 9) & 0x3;
        if(regValue == 0x3)//25Mhz Xtal
                xtal_mode = 1;
        else if(regValue == 0x2) //40Mhz
                xtal_mode = 2;
        else
                xtal_mode = 3;

        if(xtal_mode == 1) { //25Mhz Xtal
                /* do nothing */
        } else if(xtal_mode == 2) { //40Mhz
                mii_mgr_write(0, 13, 0x1f);  // disable MT7530 core clock
                mii_mgr_write(0, 14, 0x410);
                mii_mgr_write(0, 13, 0x401f);
                mii_mgr_write(0, 14, 0x0);

                mii_mgr_write(0, 13, 0x1f);  // disable MT7530 PLL
	    mii_mgr_write(0, 14, 0x40d);
	    mii_mgr_write(0, 13, 0x401f);
	    mii_mgr_write(0, 14, 0x2020);

	    mii_mgr_write(0, 13, 0x1f);  // for MT7530 core clock = 500Mhz
	    mii_mgr_write(0, 14, 0x40e);
	    mii_mgr_write(0, 13, 0x401f);
	    mii_mgr_write(0, 14, 0x119);

	    mii_mgr_write(0, 13, 0x1f);  // enable MT7530 PLL
	    mii_mgr_write(0, 14, 0x40d);
	    mii_mgr_write(0, 13, 0x401f);
	    mii_mgr_write(0, 14, 0x2820);

                udelay(20); //suggest by CD

                mii_mgr_write(0, 13, 0x1f);  // enable MT7530 core clock
                mii_mgr_write(0, 14, 0x410);
                mii_mgr_write(0, 13, 0x401f);
        }else {//20MHz
                /*TODO*/
        }

	 mii_mgr_write(0, 14, 0x1);  /*RGMII*/
	/* set MT7530 central align */
        mii_mgr_read(31, 0x7830, &regValue);
        regValue &= ~1;
        regValue |= 1<<1;
        mii_mgr_write(31, 0x7830, regValue);

        mii_mgr_read(31, 0x7a40, &regValue);
        regValue &= ~(1<<30);
        mii_mgr_write(31, 0x7a40, regValue);

        regValue = 0x855;
        mii_mgr_write(31, 0x7a78, regValue);

	
	mii_mgr_write(31, 0x7b00, 0x104);  //delay setting for 10/1000M
	mii_mgr_write(31, 0x7b04, 0x10);  //delay setting for 10/1000M

	/*Tx Driving*/
	mii_mgr_write(31, 0x7a54, 0x88);  //lower GE1 driving
	mii_mgr_write(31, 0x7a5c, 0x88);  //lower GE1 driving
	mii_mgr_write(31, 0x7a64, 0x88);  //lower GE1 driving
	mii_mgr_write(31, 0x7a6c, 0x88);  //lower GE1 driving
	mii_mgr_write(31, 0x7a74, 0x88);  //lower GE1 driving
	mii_mgr_write(31, 0x7a7c, 0x88);  //lower GE1 driving
	mii_mgr_write(31, 0x7810, 0x11);  //lower GE2 driving
	/*Set MT7623A/MT7623N TX Driving*/
	*(volatile u_long *)(0x1b110354) = 0x88;
	*(volatile u_long *)(0x1b11035c) = 0x88;
	*(volatile u_long *)(0x1b110364) = 0x88;
	*(volatile u_long *)(0x1b11036c) = 0x88;
	*(volatile u_long *)(0x1b110374) = 0x88;
	*(volatile u_long *)(0x1b11037c) = 0x88;
#if defined (GE_RGMII_AN)
	*(volatile u_long *)(0x10005f00) = 0xe00; //Set GE2 driving and slew rate
#else
	*(volatile u_long *)(0x10005f00) = 0xa00; //Set GE2 driving and slew rate
#endif
	*(volatile u_long *)(0x100054c0) = 0x5;   //set GE2 TDSEL
	*(volatile u_long *)(0x10005ed0) = 0;	  //set GE2 TUNE

	/* TRGMII Clock */
//      printf("Set TRGMII mode clock stage 1\n");
        mii_mgr_write(0, 13, 0x1f);
        mii_mgr_write(0, 14, 0x410);
        mii_mgr_write(0, 13, 0x401f);
        mii_mgr_write(0, 14, 0x1);
        mii_mgr_write(0, 13, 0x1f);
        mii_mgr_write(0, 14, 0x404);
        mii_mgr_write(0, 13, 0x401f);
        if (xtal_mode == 1){ //25MHz
#if defined (CONFIG_GE1_TRGMII_FORCE_2900)
                mii_mgr_write(0, 14, 0x1d00); // 362.5MHz
#elif defined (CONFIG_GE1_TRGMII_FORCE_2600)
                mii_mgr_write(0, 14, 0x1a00); // 325MHz
#elif defined (CONFIG_GE1_TRGMII_FORCE_2000)
                mii_mgr_write(0, 14, 0x1400); //250MHz
#else
                mii_mgr_write(0, 14, 0x0a00); //125MHz
#endif
        }else if(xtal_mode == 2){//40MHz
#if defined (CONFIG_GE1_TRGMII_FORCE_2900)
                mii_mgr_write(0, 14, 0x1220); // 362.5MHz
#elif defined (CONFIG_GE1_TRGMII_FORCE_2600)
                mii_mgr_write(0, 14, 0x1040); // 325MHz
#elif defined (CONFIG_GE1_TRGMII_FORCE_2000)
                mii_mgr_write(0, 14, 0x0c80); //250MHz
#else
                mii_mgr_write(0, 14, 0x0640); //125MHz
#endif
        }
//      printf("Set TRGMII mode clock stage 2\n");
        mii_mgr_write(0, 13, 0x1f);
        mii_mgr_write(0, 14, 0x405);
        mii_mgr_write(0, 13, 0x401f);
        mii_mgr_write(0, 14, 0x0);

//      printf("Set TRGMII mode clock stage 3\n");
        mii_mgr_write(0, 13, 0x1f);
        mii_mgr_write(0, 14, 0x409);
        mii_mgr_write(0, 13, 0x401f);
	if(xtal_mode == 1)	/* 25MHz */
		mii_mgr_write(0, 14, 0x0057);
	else
		mii_mgr_write(0, 14, 0x0087);

//      printf("Set TRGMII mode clock stage 4\n");
        mii_mgr_write(0, 13, 0x1f);
        mii_mgr_write(0, 14, 0x40a);
        mii_mgr_write(0, 13, 0x401f);
        if(xtal_mode == 1)	/* 25MHz */
		mii_mgr_write(0, 14, 0x0057);
	else
		mii_mgr_write(0, 14, 0x0087);

//      printf("Set TRGMII mode clock stage 5\n");
        mii_mgr_write(0, 13, 0x1f);
        mii_mgr_write(0, 14, 0x403);
        mii_mgr_write(0, 13, 0x401f);
        mii_mgr_write(0, 14, 0x1800);

//      printf("Set TRGMII mode clock stage 6\n");
        mii_mgr_write(0, 13, 0x1f);
        mii_mgr_write(0, 14, 0x403);
        mii_mgr_write(0, 13, 0x401f);
        mii_mgr_write(0, 14, 0x1c00);

//      printf("Set TRGMII mode clock stage 7\n");
        mii_mgr_write(0, 13, 0x1f);
        mii_mgr_write(0, 14, 0x401);
        mii_mgr_write(0, 13, 0x401f);
        mii_mgr_write(0, 14, 0xc020);

//      printf("Set TRGMII mode clock stage 8\n");
        mii_mgr_write(0, 13, 0x1f);
        mii_mgr_write(0, 14, 0x406);
        mii_mgr_write(0, 13, 0x401f);
        mii_mgr_write(0, 14, 0xa030);

//      printf("Set TRGMII mode clock stage 9\n");
        mii_mgr_write(0, 13, 0x1f);
        mii_mgr_write(0, 14, 0x406);
        mii_mgr_write(0, 13, 0x401f);
        mii_mgr_write(0, 14, 0xa038);

        udelay(120); // for MT7623 bring up test

//      printf("Set TRGMII mode clock stage 10\n");
        mii_mgr_write(0, 13, 0x1f);
        mii_mgr_write(0, 14, 0x410);
        mii_mgr_write(0, 13, 0x401f);
        mii_mgr_write(0, 14, 0x3);

//      printf("Set TRGMII mode clock stage 11\n");

        mii_mgr_read(31, 0x7830 ,&regValue);
        regValue &=0xFFFFFFFC;
        regValue |=0x00000001;
        mii_mgr_write(31, 0x7830, regValue);

//      printf("Set TRGMII mode clock stage 12\n");
        mii_mgr_read(31, 0x7a40 ,&regValue);
        regValue &= ~(0x1<<30);
        regValue &= ~(0x1<<28);
        mii_mgr_write(31, 0x7a40, regValue);

        //mii_mgr_write(31, 0x7a78, 0x855);
        mii_mgr_write(31, 0x7a78, 0x55);
//      printf(" Adjust MT7530 TXC delay\n");
        udelay(100); // for mt7623 bring up test

	printf(" Release MT7623 RXC Reset\n");
    *(volatile u_long *)(0x1b110300) &= 0x7fffffff;   // Release MT7623 RXC reset
#if defined(MT7623_ASIC_BOARD)
#if defined (CONFIG_USE_GE1)
        trgmii_calibration_7623();
        trgmii_calibration_7530();
        //*(volatile u_long *)(0xfb110300) |= (0x1f << 24);     //Just only for 312.5/325MHz
        *(volatile u_long *)(0x1b110300) |= 0x80000000;         // Assert RX  reset in MT7623
        *(volatile u_long *)(0x1b110300 )&= 0x7fffffff;   // Release RX reset in MT7623
#if defined (GE_RGMII_FORCE_1000)
    /*GE1@125MHz(RGMII mode) TX delay adjustment*/
            *(volatile u_long *)(0x1b110350) = 0x55;
            *(volatile u_long *)(0x1b110358) = 0x55;
            *(volatile u_long *)(0x1b110360) = 0x55;
            *(volatile u_long *)(0x1b110368) = 0x55;
            *(volatile u_long *)(0x1b110370) = 0x55;
            *(volatile u_long *)(0x1b110378) = 0x855;
#endif  /* CONFIG_GE1_RGMII_FORCE_1000 */
#endif  /* CONFIG_GE1_RGMII_FORCE_1000 */
#endif
        /*TRGMII DEBUG*/
    mii_mgr_read(31, 0x7a00 ,&regValue);
    regValue |= (0x1<<31);
    mii_mgr_write(31, 0x7a00, regValue);
    mdelay(1);
    regValue &= ~(0x1<<31);
    mii_mgr_write(31, 0x7a00, regValue);
    mdelay(100);
	
	for(i=0;i<=4;i++)
	{
	    mii_mgr_write(i, 13, 0x7);
	    mii_mgr_write(i, 14, 0x3C);
	    mii_mgr_write(i, 13, 0x4007);
	    mii_mgr_write(i, 14, 0x0);
	}

	//Disable EEE 10Base-Te:
	for(i=0;i<=4;i++)
	{
	    mii_mgr_write(i, 13, 0x1f);
	    mii_mgr_write(i, 14, 0x027b);
	    mii_mgr_write(i, 13, 0x401f);
	    mii_mgr_write(i, 14, 0x1177);
	}

	for(i=0;i<=4;i++)
        {
        	//turn on PHY
                mii_mgr_read(i, 0x0 ,&regValue);
                regValue &= ~(0x1<<11);
                mii_mgr_write(i, 0x0, regValue);
        }

        for(i=0;i<=4;i++) {
                mii_mgr_read(i, 4, &regValue);
                regValue |= (3<<7); //turn on 100Base-T Advertisement
                mii_mgr_write(i, 4, regValue);

                mii_mgr_read(i, 9, &regValue);
                regValue |= (3<<8); //turn on 1000Base-T Advertisement
                mii_mgr_write(i, 9, regValue);

                //restart AN
                mii_mgr_read(i, 0, &regValue);
                regValue |= (1 << 9);
                mii_mgr_write(i, 0, regValue);
        }

	mii_mgr_read(31, 0x7808 ,&regValue);
	regValue |= (3<<16); //Enable INTR
	mii_mgr_write(31, 0x7808 ,regValue);
#endif
}

static void eth_trgmii_calibration(void)
{
#if 0
//#if defined(MT7623_ASIC_BOARD)
#if defined (CONFIG_USE_GE1)
	trgmii_calibration_7623();
	trgmii_calibration_7530();
	//*(volatile u_long *)(0xfb110300) |= (0x1f << 24);	//Just only for 312.5/325MHz
	*(volatile u_long *)(0x1b110340) = 0x00020000;
	*(volatile u_long *)(0x1b110304) &= 0x3fffffff; 	// RX clock gating in MT7623
	*(volatile u_long *)(0x1b110300) |= 0x80000000; 	// Assert RX  reset in MT7623
	*(volatile u_long *)(0x1b110300 )      &= 0x7fffffff;	// Release RX reset in MT7623
	*(volatile u_long *)(0x1b110300 +0x04) |= 0xC0000000;	// Disable RX clock gating in MT7623
#if defined (GE_RGMII_FORCE_1000)
	/*GE1@125MHz(RGMII mode) TX delay adjustment*/
	*(volatile u_long *)(0x1b110350) = 0x55;
	*(volatile u_long *)(0x1b110358) = 0x55;
	*(volatile u_long *)(0x1b110360) = 0x55;
	*(volatile u_long *)(0x1b110368) = 0x55;
	*(volatile u_long *)(0x1b110370) = 0x55;
	*(volatile u_long *)(0x1b110378) = 0x855;
#endif	/* CONFIG_GE1_RGMII_FORCE_1000 */
#endif	/* CONFIG_USE_GE1 */
#endif	/* defined(MT7623_ASIC_BOARD) */
}

static int rt2880_eth_init(struct eth_device* dev, bd_t* bis)
{
	if(rt2880_eth_initd == 0)
	{
		eth_pinmux_power_init();

		rt2880_eth_setup(dev);

		eth_switch_init();

		setup_internal_gsw();

		eth_trgmii_calibration();

		LANWANPartition();
	}
	else
	{
		START_ETH(dev);
	}

	rt2880_eth_initd = 1;
	return (1);
}

#if defined (MAC_TO_GIGAPHY_MODE) || defined (MAC_TO_100PHY_MODE)
void enable_auto_negotiate(void)
{
	u32 regValue;

	/* FIXME: we don't know how to deal with PHY end addr */
	//regValue = sysRegRead(ESW_PHY_POLLING);
	regValue = le32_to_cpu(*(volatile u_long *)(RALINK_ETH_SW_BASE+0x0000));
	regValue |= (1<<31);        /* phy auto-polling enable */
	regValue &= ~(0x1f);        /* reset phy start addr */
	regValue &= ~(0x1f<<8);     /* reset phy end addr */

#if defined (CONFIG_USE_GE1)
	regValue |= (MAC_TO_GIGAPHY_MODE_ADDR << 0);//setup PHY address for auto polling (Start Addr).
	regValue |= (MAC_TO_GIGAPHY_MODE_ADDR << 8);// setup PHY address for auto polling (End Addr).
#elif defined (CONFIG_USE_GE2)
	regValue |= ((MAC_TO_GIGAPHY_MODE_ADDR-1)&0x1f << 0);//setup PHY address for auto polling (Start Addr).
	regValue |= (MAC_TO_GIGAPHY_MODE_ADDR << 8);// setup PHY address for auto polling (End Addr).
#endif

	/*kurtis: AN is strange*/
	*(volatile u_long *)(RALINK_ETH_SW_BASE + 0x0000) = regValue;
}
#endif

int isDMABusy(struct eth_device* dev)
{
	u32 reg;

	reg = RALINK_REG(PDMA_GLO_CFG);

	if((reg & RX_DMA_BUSY)){
		return 1;
	}

	if((reg & TX_DMA_BUSY)){
		printf("\n  TX_DMA_BUSY !!! ");
		return 1;
	}
	return 0;
}

static int rt2880_eth_setup(struct eth_device* dev)
{
	u32	i;
	u32	regValue;
	u16	wTmp;
	unsigned long   addr, len;

	printf("\n Waitting for RX_DMA_BUSY status Start... ");
	while(1)
		if(!isDMABusy(dev))
			break;
	printf("done\n\n");

	// Case1: GigaPhy
#if defined (MAC_TO_GIGAPHY_MODE)
	enable_auto_negotiate();

#if !defined(CONFIG_PHYLIB)
	/* Disable pause ability */
	printf("Disable pause ability\n");
	mii_mgr_read(MAC_TO_GIGAPHY_MODE_ADDR, 4, &regValue);
	regValue &= ~(1<<10); //disable pause ability
	mii_mgr_write(MAC_TO_GIGAPHY_MODE_ADDR, 4, regValue);

	mii_mgr_read(MAC_TO_GIGAPHY_MODE_ADDR, 0, &regValue);
	regValue |= 1<<9; //restart AN
	mii_mgr_write(MAC_TO_GIGAPHY_MODE_ADDR, 0, regValue);
#endif

#if defined (CONFIG_USE_GE1)
	RALINK_REG(RALINK_ETH_SW_BASE+0x100) = 0x20056300;//(P0, Auto mode)
	RALINK_REG(RALINK_ETH_SW_BASE+0x200) = 0x00008000;//(P1, Down)
	RALINK_REG(RT2880_SYSCFG1_REG) &= ~(0x3 << 12); //GE1_MODE=RGMII Mode  
#elif defined (CONFIG_USE_GE2)
	RALINK_REG(RALINK_ETH_SW_BASE+0x200) = 0x20056300;//(P1, Auto mode)
	RALINK_REG(RALINK_ETH_SW_BASE+0x100) = 0x00008000;//(P0, Down)
	RALINK_REG(RT2880_SYSCFG1_REG) &= ~(0x3 << 14); //GE2_MODE=RGMII Mode
#endif

	// Case2. 10/100 Switch or 100PHY
#elif defined (MAC_TO_100SW_MODE) ||  defined (MAC_TO_100PHY_MODE)
	/* 0=RGMII, 1=MII, 2=RvMii, 3=RMii */
	regValue = RALINK_REG(RT2880_SYSCFG1_REG);
	regValue &= ~(0xF << 12);

#if defined (CONFIG_USE_GE1)
#if defined (GE_MII_FORCE_100) || defined (GE_RVMII_FORCE_100)
	RALINK_REG(RALINK_ETH_SW_BASE+0x100) = 0x2005e337;//(P0, Force mode, Link Up, 100Mbps, Full-Duplex, FC ON)
	RALINK_REG(RT2880_SYSCFG1_REG) &= ~(0x3 << 12); //GE1_MODE=Mii Mode
	RALINK_REG(RT2880_SYSCFG1_REG) |= (0x1 << 12);
#elif defined (GE_MII_AN)
	RALINK_REG(RALINK_ETH_SW_BASE+0x100) = 0x20056300;//(P0, Auto mode)
	RALINK_REG(RT2880_SYSCFG1_REG) &= ~(0x3 << 12); //GE1_MODE=Mii Mode
	RALINK_REG(RT2880_SYSCFG1_REG) |= (0x1 << 12);
	enable_auto_negotiate();
#endif
	regValue = RALINK_REG(RALINK_ETH_SW_BASE + 0x100);
	printf("\n RALINK_REG(RALINK_ETH_SW_BASE + 0x100)=0x%x\n", regValue);
#elif defined (CONFIG_USE_GE2)
#if defined (GE_MII_FORCE_100) || defined (GE_RVMII_FORCE_100)
	RALINK_REG(RALINK_ETH_SW_BASE+0x200) = 0x2005e337;//(P0, Force mode, Link Up, 100Mbps, Full-Duplex, FC ON)
	RALINK_REG(RT2880_SYSCFG1_REG) &= ~(0x3 << 14); //GE2_MODE=Mii Mode
	RALINK_REG(RT2880_SYSCFG1_REG) |= (0x1 << 14);
#elif defined (GE_RMII_AN)
	RALINK_REG(RALINK_ETH_SW_BASE+0x200) = 0x20056300;//(P0, Auto mode)
	RALINK_REG(RT2880_SYSCFG1_REG) &= ~(0x3 << 14); //GE2_MODE=RMii Mode
	RALINK_REG(RT2880_SYSCFG1_REG) |= (0x3 << 14);
	/* P2_RMII_CKRX = RXC, P2_RMII_CKIN = output */
	regValue = RALINK_REG(RALINK_ETH_SW_BASE + 0x08);
	RALINK_REG(RALINK_ETH_SW_BASE + 0x08) = regValue | (0x3 << 22);
	enable_auto_negotiate();
#elif defined (GE_MII_AN)
	RALINK_REG(RALINK_ETH_SW_BASE+0x200) = 0x20056300;//(P0, Auto mode)
	RALINK_REG(RT2880_SYSCFG1_REG) &= ~(0x3 << 14); //GE2_MODE=Mii Mode
	RALINK_REG(RT2880_SYSCFG1_REG) |= (0x1 << 14);
	enable_auto_negotiate();
#endif
	regValue = RALINK_REG(RALINK_ETH_SW_BASE + 0x200);
	printf("\n RALINK_REG(RALINK_ETH_SW_BASE + 0x200)=0x%x\n", regValue);
	regValue = RALINK_REG(RALINK_ETH_SW_BASE + 0x08);
	printf("\n RALINK_REG(RALINK_ETH_SW_BASE + 0x08)=0x%x\n", regValue);
	regValue = RALINK_REG(RT2880_SYSCFG1_REG);
	printf("\n RALINK_REG(RT2880_SYSCFG1_REG)=0x%x\n", regValue);
#endif	/* CONFIG_USE_GE1 */

#endif // MAC_TO_GIGAPHY_MODE //

#if defined (CONFIG_USE_GE1)
	/* Set MAC address. */
	wTmp = (u16)dev->enetaddr[0];
	regValue = (wTmp << 8) | dev->enetaddr[1];

	RALINK_REG(GDMA1_MAC_ADRH)=regValue;
	// printf("\n dev->iobase=%08X,GDMA1_MAC_ADRH=%08X\n ",dev->iobase, regValue);

	wTmp = (u16)dev->enetaddr[2];
	regValue = (wTmp << 8) | dev->enetaddr[3];
	regValue = regValue << 16;
	wTmp = (u16)dev->enetaddr[4];
	regValue |= (wTmp<<8) | dev->enetaddr[5];

	RALINK_REG(GDMA1_MAC_ADRL)=regValue;
	// printf("\n dev->iobase=%08X,GDMA1_MAC_ADRL=%08X\n ",dev->iobase, regValue);

	regValue = RALINK_REG(GDMA1_FWD_CFG);
	//printf("\n old,GDMA1_FWD_CFG = %08X \n",regValue);

	//Uni-cast frames forward to CPU
	regValue = regValue & GDM_UFRC_P_CPU;
	//Broad-cast MAC address frames forward to CPU
	regValue = regValue & GDM_BFRC_P_CPU;
	//Multi-cast MAC address frames forward to CPU
	regValue = regValue & GDM_MFRC_P_CPU;
	//Other MAC address frames forward to CPU
	regValue = regValue & GDM_OFRC_P_CPU;

	RALINK_REG(GDMA1_FWD_CFG)=regValue;
	udelay(500);
	regValue = RALINK_REG(GDMA1_FWD_CFG);
#elif defined (CONFIG_USE_GE2)
	wTmp = (u16)dev->enetaddr[0];
	regValue = (wTmp << 8) | dev->enetaddr[1];
	RALINK_REG(GDMA2_MAC_ADRH)=regValue;

	wTmp = (u16)dev->enetaddr[2];
	regValue = (wTmp << 8) | dev->enetaddr[3];
	regValue = regValue << 16;
	wTmp = (u16)dev->enetaddr[4];
	regValue |= (wTmp<<8) | dev->enetaddr[5];
	RALINK_REG(GDMA2_MAC_ADRL)=regValue;

	regValue = RALINK_REG(GDMA2_FWD_CFG);

	regValue = regValue & GDM_UFRC_P_CPU;
	//Broad-cast MAC address frames forward to CPU
	regValue = regValue & GDM_BFRC_P_CPU;
	//Multi-cast MAC address frames forward to CPU
	regValue = regValue & GDM_MFRC_P_CPU;
	//Other MAC address frames forward to CPU
	regValue = regValue & GDM_OFRC_P_CPU;

	RALINK_REG(GDMA2_FWD_CFG)=regValue;
	udelay(500);
	regValue = RALINK_REG(GDMA2_FWD_CFG);
#endif	/* CONFIG_USE_GE1 */

	for (i = 0; i < RAETH_NUM_RX_DESC; i++) {
		rx_ring[i].rxd_info2.DDONE_bit = 0;

		{
			BUFFER_ELEM *buf;
			buf = rt2880_free_buf_entry_dequeue(&rt2880_free_buf_list);
			NetRxPackets[i] = buf->pbuf;
			rx_ring[i].rxd_info2.LS0= 0;
			rx_ring[i].rxd_info2.PLEN0= PKTSIZE_ALIGN;
			rx_ring[i].rxd_info1.PDP0 = cpu_to_le32(phys_to_bus((u32) NetRxPackets[i]));
		}
	}

	for (i=0; i < RAETH_NUM_TX_DESC; i++) {
		tx_ring0[i].txd_info2.LS0_bit = 1;
		tx_ring0[i].txd_info2.DDONE_bit = 1;
		/* PN:
		 *  0:CPU
		 *  1:GE1
		 *  2:GE2 (for RT2883)
		 *  6:PPE
		 *  7:Discard
		 */
#if defined (CONFIG_USE_GE1)
		tx_ring0[i].txd_info4.FPORT=1;
#elif defined (CONFIG_USE_GE2)
		tx_ring0[i].txd_info4.FPORT=2;
#endif

	}
	rxRingSize = RAETH_NUM_RX_DESC;
	txRingSize = RAETH_NUM_TX_DESC;

	rx_dma_owner_idx0 = 0;
	rx_wants_alloc_idx0 = (RAETH_NUM_RX_DESC - 1);
	tx_cpu_owner_idx0 = 0;

	rx_desc_cnt = 0;
	rx_desc_threshold = CONFIG_SYS_CACHELINE_SIZE / sizeof(struct PDMA_rxdesc);

	regValue=RALINK_REG(PDMA_GLO_CFG);
	udelay(100);

	{
		regValue &= 0x0000FFFF;

		RALINK_REG(PDMA_GLO_CFG)=regValue;
		udelay(500);
		regValue=RALINK_REG(PDMA_GLO_CFG);
	}

	/* Tell the adapter where the TX/RX rings are located. */
	addr = (unsigned long)&rx_ring_cache[0] & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	len = (RAETH_NUM_RX_DESC * sizeof(rx_ring_cache[0]) + 2 * CONFIG_SYS_CACHELINE_SIZE) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	flush_dcache_range(addr, addr + len);
	RALINK_REG(RX_BASE_PTR0)=phys_to_bus((u32) &rx_ring[0]);
	//printf("\n rx_ring=%08X ,RX_BASE_PTR0 = %08X \n",&rx_ring[0],RALINK_REG(RX_BASE_PTR0));

	addr = (unsigned long)&tx_ring0_cache[0] & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	len = (RAETH_NUM_TX_DESC * sizeof(tx_ring0_cache[0]) + 2 * CONFIG_SYS_CACHELINE_SIZE) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	flush_dcache_range(addr, addr + len);
	RALINK_REG(TX_BASE_PTR0)=phys_to_bus((u32) &tx_ring0[0]);
	//printf("\n tx_ring0=%08X, TX_BASE_PTR0 = %08X \n",&tx_ring0[0],RALINK_REG(TX_BASE_PTR0));

	RALINK_REG(RX_MAX_CNT0)=cpu_to_le32((u32) RAETH_NUM_RX_DESC);
	RALINK_REG(TX_MAX_CNT0)=cpu_to_le32((u32) RAETH_NUM_TX_DESC);

	RALINK_REG(TX_CTX_IDX0)=cpu_to_le32((u32) tx_cpu_owner_idx0);
	RALINK_REG(PDMA_RST_IDX)=cpu_to_le32((u32)RST_DTX_IDX0);

	RALINK_REG(RX_CALC_IDX0)=cpu_to_le32((u32) (RAETH_NUM_RX_DESC - 1));
	RALINK_REG(PDMA_RST_IDX)=cpu_to_le32((u32)RST_DRX_IDX0);

	udelay(500);
	START_ETH(dev);
	
	return 1;
}

static int rt2880_eth_send(struct eth_device* dev, void *packet, int length)
{
	int		status = -1;
	int		i;
	int		retry_count = 0, temp;
	unsigned long   addr, len;

	//printf("Enter rt2880_eth_send()\n");

Retry:
	if (retry_count > 10) {
		return (status);
	}

	if (length <= 0) {
		printf("%s: bad packet size: %d\n", dev->name, length);
		return (status);
	}

	for(i = 0; tx_ring0[tx_cpu_owner_idx0].txd_info2.DDONE_bit == 0 ; i++)

	{
		if (i >= TOUT_LOOP) {
			goto Done;
		}
	}

	temp = RALINK_REG(TX_DTX_IDX0);

	if(temp == (tx_cpu_owner_idx0+1) % RAETH_NUM_TX_DESC) {
		puts(" @ ");
		goto Done;
	}

	tx_ring0[tx_cpu_owner_idx0].txd_info1.SDP0 = cpu_to_le32(phys_to_bus((u32) packet));
	tx_ring0[tx_cpu_owner_idx0].txd_info2.SDL0 = length;

#if 0
	printf("==========TX==========(CTX=%d)\n",tx_cpu_owner_idx0);
	printf("tx_ring0[tx_cpu_owner_idx0].txd_info1 =%x\n",tx_ring0[tx_cpu_owner_idx0].txd_info1);
	printf("tx_ring0[tx_cpu_owner_idx0].txd_info2 =%x\n",tx_ring0[tx_cpu_owner_idx0].txd_info2);
	printf("tx_ring0[tx_cpu_owner_idx0].txd_info3 =%x\n",tx_ring0[tx_cpu_owner_idx0].txd_info3);
	printf("tx_ring0[tx_cpu_owner_idx0].txd_info4 =%x\n",tx_ring0[tx_cpu_owner_idx0].txd_info4);
#endif

	tx_ring0[tx_cpu_owner_idx0].txd_info2.DDONE_bit = 0;
	status = length;

	addr = (unsigned long)packet & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	len = ((unsigned long)length + 2 * CONFIG_SYS_CACHELINE_SIZE ) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	flush_dcache_range(addr, addr + len);

	addr = (unsigned long)&tx_ring0_cache[0] & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	len = (RAETH_NUM_TX_DESC * sizeof(tx_ring0_cache[0]) + 2 * CONFIG_SYS_CACHELINE_SIZE) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	flush_dcache_range(addr, addr + len);

	tx_cpu_owner_idx0 = (tx_cpu_owner_idx0+1) % RAETH_NUM_TX_DESC;
	RALINK_REG(TX_CTX_IDX0)=cpu_to_le32((u32) tx_cpu_owner_idx0);

	return status;
Done:
	udelay(500);
	retry_count++;
	goto Retry;
}


static int rt2880_eth_recv(struct eth_device* dev)
{
	int length = 0;
	int inter_loopback_cnt =0;
	u32 *rxd_info;
	unsigned long   addr, len;
	int i, rx_dma_owner_idx_tmp = 0;

	for (; ; ) {

		addr = (unsigned long)&rx_ring_cache[0] & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
		len = (RAETH_NUM_RX_DESC * sizeof(rx_ring_cache[0]) + 2 * CONFIG_SYS_CACHELINE_SIZE) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
		invalidate_dcache_range(addr, addr + len);

		//rxd_info = (u32 *)KSEG1ADDR(&rx_ring[rx_dma_owner_idx0].rxd_info2);
		rxd_info = (u32 *)(&rx_ring[rx_dma_owner_idx0].rxd_info2);

		if ( (*rxd_info & BIT(31)) == 0 )
			break;

		udelay(1);
			length = rx_ring[rx_dma_owner_idx0].rxd_info2.PLEN0;

		if(length == 0)
		{
			printf("\n Warring!! Packet Length has error !!,In normal mode !\n");
		}

		if(rx_ring[rx_dma_owner_idx0].rxd_info4.SP == 6)
		{// Packet received from CPU port
			printf("\n Normal Mode,Packet received from CPU port,plen=%d \n",length);
			//print_packet((void *)KSEG1ADDR(NetRxPackets[rx_dma_owner_idx0]),length);
			inter_loopback_cnt++;
			length = inter_loopback_cnt;//for return
		}
		else {
			addr = (unsigned long)NetRxPackets[rx_dma_owner_idx0] & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
			len = (length + 2 * CONFIG_SYS_CACHELINE_SIZE ) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
			invalidate_dcache_range(addr, addr + len);

			//NetReceive((void *)KSEG1ADDR(NetRxPackets[rx_dma_owner_idx0]), length );
			NetReceive((void *)(NetRxPackets[rx_dma_owner_idx0]), length );
		}

		rx_desc_cnt++;
		if (rx_desc_cnt == rx_desc_threshold) {
			for (i = rx_desc_threshold; i > 0; i--) {
				rx_dma_owner_idx_tmp = rx_dma_owner_idx0 - rx_desc_threshold + i;
				rx_ring[rx_dma_owner_idx_tmp].rxd_info2.DDONE_bit = 0;
				rx_ring[rx_dma_owner_idx_tmp].rxd_info2.LS0= 0;
				rx_ring[rx_dma_owner_idx_tmp].rxd_info2.PLEN0= PKTSIZE_ALIGN;

#if 0
			printf("=====RX=======(CALC=%d LEN=%d)\n",rx_dma_owner_idx0, length);
			printf("rx_ring[rx_dma_owner_idx0].rxd_info1 =%x\n",rx_ring[rx_dma_owner_idx0].rxd_info1);
			printf("rx_ring[rx_dma_owner_idx0].rxd_info2 =%x\n",rx_ring[rx_dma_owner_idx0].rxd_info2);
			printf("rx_ring[rx_dma_owner_idx0].rxd_info3 =%x\n",rx_ring[rx_dma_owner_idx0].rxd_info3);
			printf("rx_ring[rx_dma_owner_idx0].rxd_info4 =%x\n",rx_ring[rx_dma_owner_idx0].rxd_info4);
#endif
			}

			/* because hw will access the rx desc, sw flush the desc after updating it */
			addr = (unsigned long)&rx_ring_cache[rx_dma_owner_idx_tmp] & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
			len = CONFIG_SYS_CACHELINE_SIZE;
			flush_dcache_range(addr, addr + len);

			/*  Move point to next RXD which wants to alloc*/
			RALINK_REG(RX_CALC_IDX0)=cpu_to_le32((u32) rx_dma_owner_idx0);

			rx_desc_cnt = 0;
		}

		/* Update to Next packet point that was received.
		 */
		rx_dma_owner_idx0 = (rx_dma_owner_idx0 + 1) % RAETH_NUM_RX_DESC;

		//printf("\n ************************************************* \n");
		//printf("\n RX_CALC_IDX0=%d \n", RALINK_REG(RX_CALC_IDX0));
		//printf("\n RX_DRX_IDX0 = %d \n",RALINK_REG(RX_DRX_IDX0));
		//printf("\n ************************************************* \n");
	}
	return length;
}

void rt2880_eth_halt(struct eth_device* dev)
{
	 STOP_ETH(dev);
	//gmac_phy_switch_gear(DISABLE);
	//printf(" STOP_ETH \n");
	//dump_reg();
}

#if 0
static void print_packet( u8 * buf, int length )
{

	int i;
	int remainder;
	int lines;


	printf("Packet of length %d \n", length );


	lines = length / 16;
	remainder = length % 16;

	for ( i = 0; i < lines ; i ++ ) {
		int cur;

		for ( cur = 0; cur < 8; cur ++ ) {
			u8 a, b;

			a = *(buf ++ );
			b = *(buf ++ );
			printf("%02X %02X ", a, b );
		}
		printf("\n");
	}
	for ( i = 0; i < remainder/2 ; i++ ) {
		u8 a, b;

		a = *(buf ++ );
		b = *(buf ++ );
		printf("%02X %02X ", a, b );
	}
	printf("\n");

}
#endif

#define RT_RDM_DUMP_RANGE	16  // unit=16bytes
u32 register_control = 0;

int rdm_ioctl(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u32 offset, rtvalue;
	u32 address;
	u32 count=0;

	if(!memcmp(argv[0],"reg.s",sizeof("reg.s")))
	{
		register_control = simple_strtoul(argv[1], NULL, 16);
		printf("switch register base addr to 0x%08x\n", register_control);
	}
	else if(!memcmp(argv[0],"reg.r",sizeof("reg.r")))
	{
		offset = simple_strtoul(argv[1], NULL, 16);
		rtvalue = (*(volatile u32 *)(register_control + offset));
		printf("write offset 0x%x, value 0x%x\n", offset, rtvalue);
	}
	else if(!memcmp(argv[0],"reg.w",sizeof("reg.w")))
	{
		offset = simple_strtoul(argv[1], NULL, 16);
		rtvalue = simple_strtoul(argv[2], NULL, 16);
		*(volatile u32 *)(register_control + offset) = rtvalue;
		printf("write offset 0x%x, value 0x%x\n", offset, *(volatile u32 *)(register_control + offset));
	}
	else if(!memcmp(argv[0],"reg.d",sizeof("reg.d")))
	{
		offset = simple_strtoul(argv[1], NULL, 16);
		for (count=0; count < RT_RDM_DUMP_RANGE ; count++) {
			address = register_control + offset + (count*16);
			printf("%08X: ", address);
			printf("%08X %08X %08X %08X\n", 
				le32_to_cpu(*(volatile u32 *)(address)),
				le32_to_cpu(*(volatile u32 *)(address+4)),
				le32_to_cpu(*(volatile u32 *)(address+8)),
				le32_to_cpu(*(volatile u32 *)(address+12)));
		}
	}
	return 0;
}

U_BOOT_CMD(
 	reg,	4,	1,	rdm_ioctl,
 	"reg   - Ralink PHY register R/W command !!\n",
 	"reg.s [phy_addr(hex)] - set register base \n"
 	"reg.r [phy_addr(hex)] - read register \n"
 	"reg.w [phy_addr(hex)] [data(HEX)] - write register \n"
 	"reg.d [phy register(hex)] - dump registers \n"
 	""
);

int EswCntRead(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int pkt_cnt = 0;
	int i = 0;

	printf("\n		  <<CPU>>			 \n");
	printf("		    |				 \n");
	printf("+-----------------------------------------------+\n");
	printf("|		  <<PSE>>		        |\n");
	printf("+-----------------------------------------------+\n");
	printf("		   |				 \n");
	printf("+-----------------------------------------------+\n");
	printf("|		  <<GDMA>>		        |\n");
	printf("| GDMA1_RX_GBCNT  : %010u (Rx Good Bytes)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2400));
	printf("| GDMA1_RX_GPCNT  : %010u (Rx Good Pkts)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2408));
	printf("| GDMA1_RX_OERCNT : %010u (overflow error)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2410));	
	printf("| GDMA1_RX_FERCNT : %010u (FCS error)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2414));	
	printf("| GDMA1_RX_SERCNT : %010u (too short)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2418));	
	printf("| GDMA1_RX_LERCNT : %010u (too long)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x241C));	
	printf("| GDMA1_RX_CERCNT : %010u (checksum error)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2420));	
	printf("| GDMA1_RX_FCCNT  : %010u (flow control)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2424));	
	printf("| GDMA1_TX_SKIPCNT: %010u (about count)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2428));	
	printf("| GDMA1_TX_COLCNT : %010u (collision count)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x242C));	
	printf("| GDMA1_TX_GBCNT  : %010u (Tx Good Bytes)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2430));	
	printf("| GDMA1_TX_GPCNT  : %010u (Tx Good Pkts)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2438));	
	printf("|						|\n");
	printf("| GDMA2_RX_GBCNT  : %010u (Rx Good Bytes)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2440));	
	printf("| GDMA2_RX_GPCNT  : %010u (Rx Good Pkts)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2448));	
	printf("| GDMA2_RX_OERCNT : %010u (overflow error)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2450));	
	printf("| GDMA2_RX_FERCNT : %010u (FCS error)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2454));	
	printf("| GDMA2_RX_SERCNT : %010u (too short)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2458));	
	printf("| GDMA2_RX_LERCNT : %010u (too long)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x245C));	
	printf("| GDMA2_RX_CERCNT : %010u (checksum error)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2460));	
	printf("| GDMA2_RX_FCCNT  : %010u (flow control)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2464));	
	printf("| GDMA2_TX_SKIPCNT: %010u (skip)		|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2468));	
	printf("| GDMA2_TX_COLCNT : %010u (collision)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x246C));	
	printf("| GDMA2_TX_GBCNT  : %010u (Tx Good Bytes)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2470));	
	printf("| GDMA2_TX_GPCNT  : %010u (Tx Good Pkts)	|\n", RALINK_REG(RALINK_FRAME_ENGINE_BASE+0x2478));	
	printf("+-----------------------------------------------+\n");

#define DUMP_EACH_PORT(base)					\
	for(i=0; i < 7;i++) {					\
		mii_mgr_read(31, (base) + (i*0x100), &pkt_cnt); \
		printf("%8u ", pkt_cnt);			\
	}							\
	printf("\n");

	{
		printf("===================== %8s %8s %8s %8s %8s %8s %8s\n","Port0", "Port1", "Port2", "Port3", "Port4", "Port5", "Port6");
		printf("Tx Drop Packet      :"); DUMP_EACH_PORT(0x4000);
		printf("Tx CRC Error        :"); DUMP_EACH_PORT(0x4004);
		printf("Tx Unicast Packet   :"); DUMP_EACH_PORT(0x4008);
		printf("Tx Multicast Packet :"); DUMP_EACH_PORT(0x400C);
		printf("Tx Broadcast Packet :"); DUMP_EACH_PORT(0x4010);
		printf("Tx Collision Event  :"); DUMP_EACH_PORT(0x4014);
		printf("Tx Pause Packet     :"); DUMP_EACH_PORT(0x402C);
		printf("Rx Drop Packet      :"); DUMP_EACH_PORT(0x4060);
		printf("Rx Filtering Packet :"); DUMP_EACH_PORT(0x4064);
		printf("Rx Unicast Packet   :"); DUMP_EACH_PORT(0x4068);
		printf("Rx Multicast Packet :"); DUMP_EACH_PORT(0x406C);
		printf("Rx Broadcast Packet :"); DUMP_EACH_PORT(0x4070);
		printf("Rx Alignment Error  :"); DUMP_EACH_PORT(0x4074);
		printf("Rx CRC Error	    :"); DUMP_EACH_PORT(0x4078);
		printf("Rx Undersize Error  :"); DUMP_EACH_PORT(0x407C);
		printf("Rx Fragment Error   :"); DUMP_EACH_PORT(0x4080);
		printf("Rx Oversize Error   :"); DUMP_EACH_PORT(0x4084);
		printf("Rx Jabber Error     :"); DUMP_EACH_PORT(0x4088);
		printf("Rx Pause Packet     :"); DUMP_EACH_PORT(0x408C);
		mii_mgr_write(31, 0x4fe0, 0xf0);
		mii_mgr_write(31, 0x4fe0, 0x800000f0);
	} 
	printf("\n");

	return 0;
}

U_BOOT_CMD(
 	esw_read,	4,	1,	EswCntRead,
 	"esw_read   - Dump external switch/GMAC status !!\n",
 	"esw_read   - Dump external switch/GMAC status \n"
 	""
);

#endif	/* CONFIG_CMD_NET && CONFIG_RT2880_ETH */
