/*
*	TFTP server     File: cmd_tftpServer.c
*/

#include <common.h>
#if defined(ASUS_PRODUCT)
#include <gpio.h>
#include <cmd_tftpServer.h>
#include <malloc.h>
#include <asm/errno.h>

#if defined(CONFIG_UBI_SUPPORT)
#include "../drivers/mtd/ubi/ubi-media.h"
#endif

#ifdef CFG_DIRECT_FLASH_TFTP
extern flash_info_t flash_info[CFG_MAX_FLASH_BANKS];/* info for FLASH chips   */
#endif

#if defined(UBOOT_STAGE1)
struct stage2_loc g_s2_loc;
#endif

const unsigned char trx_magic[4] =		"\x27\x05\x19\x56";	/* Image Magic Number */
const unsigned char nvram_magic[4] =		"\x46\x4C\x53\x48";	/* 'FLSH' */
const unsigned char nvram_magic_mac0[4] =	"\x4D\x41\x43\x30";	/* 'MAC0' Added by PaN */
const unsigned char nvram_magic_mac1[4] =	"\x4D\x41\x43\x31";	/* 'MAC1' Added by PaN */
const unsigned char nvram_magic_rdom[4] =	"\x52\x44\x4F\x4D";	/* 'RDOM' Added by PaN */
const unsigned char nvram_magic_asus[4] =	"\x41\x53\x55\x53";	/* 'ASUS' Added by PaN */

static int	TftpServerPort;		/* The UDP port at their end		*/
static int	TftpOurPort;		/* The UDP port at our end		*/
static int	TftpTimeoutCount;
static ulong	TftpBlock;		/* packet sequence number		*/
static ulong	TftpLastBlock;		/* last packet sequence number received */
static ulong	TftpBlockWrapOffset;	/* memory offset due to wrapping	*/
static int	TftpState;

uchar asuslink[] = "ASUSSPACELINK";
uchar maclink[] = "snxxxxxxxxxxx";
#define PTR_SIZE	0x800000	// 0x390000
#define BOOTBUF_SIZE	0x30000
uchar MAC0[20], RDOM[7], ASUS[24], nvramcmd[60];
unsigned char *ptr, *tail;
uint16_t RescueAckFlag = 0;
uint32_t copysize = 0;
uint32_t offset = 0;
int rc = 0;
int MAC_FLAG = 0;
int env_loc = 0;
int ubi_damaged = 0;

static void _tftpd_open(void);
static void TftpHandler(uchar * pkt, unsigned dport, IPaddr_t sip, unsigned sport, unsigned len);
static void RAMtoFlash(void);
#if !defined(UBOOT_STAGE1)
static void SolveTRX(void);
#endif
extern image_header_t header;
extern int do_bootm(cmd_tbl_t *, int, int, char * const []);
extern int do_reset(cmd_tbl_t *, int, int, char * const []);

extern IPaddr_t TempServerIP;
extern bootm_headers_t images;		/* pointers to os/initrd/fdt images */

/*
 * @return:
 * 	>0:	firmware good, length of firmware
 *	=0:	not defined
 *	<0:	invalid firmware
 */
int check_trx(int argc, char * const argv[])
{
	memset(&images, 0, sizeof(images));
	images.verify = 1;
	if (ptr)
		images.rescue_image = 1;
	if (!bootm_start(NULL, 0, argc, argv))
		return sizeof(image_header_t) + images.os.image_len;

	return -1;
}

int program_bootloader(ulong addr, ulong len)
{
	void *hdr = (void *)addr;
	const ulong max_xfer_size =
#if defined(UBOOT_STAGE1)
		CFG_BOOTSTAGE1_SIZE;
#else
		CFG_MAX_BOOTLOADER_BINARY_SIZE;
#endif

	if (len > max_xfer_size) {
		printf("Abort: Bootloader is too big or download aborted!\n");
		return -1;
	}

	switch (genimg_get_format(hdr)) {
	case IMAGE_FORMAT_LEGACY:
		/* Only FIT image is supported! */
		return -2;

#if defined(CONFIG_FIT)
	case IMAGE_FORMAT_FIT:
		if (!fit_check_format(hdr)) {
			puts("Bad FIT image format!\n");
			return -3;
		}

		if (!fit_all_image_check_hashes(hdr)) {
			puts("Bad hash in FIT image!\n");
			return -4;
		}

		break;
#endif
	default:
		puts("Unknown FIT image format!\n");
		return -5;
	}

	printf("program: from(%08lx), to(%08x), size(%lx)\n", addr, CONFIG_SYS_FLASH_BASE, len);
#if 0
#if defined(UBOOT_STAGE1)
	ranand_set_sbb_max_addr(CFG_BOOTSTAGE1_SIZE);
	ra_flash_erase_write((uchar*) addr, CFG_FLASH_BASE, len, 1);
	ranand_set_sbb_max_addr(0);
#elif defined(UBOOT_STAGE2)
	if (len <= CFG_BOOTSTAGE1_SIZE) {
		ranand_set_sbb_max_addr(CFG_BOOTSTAGE1_SIZE);
		ra_flash_erase_write((uchar*) addr, CFG_FLASH_BASE, len, 1);
		ranand_set_sbb_max_addr(0);
	} else {
		ranand_write_stage2(CONFIG_SYS_LOAD_ADDR, len);
	}
#else
	ranand_set_sbb_max_addr(CFG_BOOTLOADER_SIZE);
	ra_flash_erase_write((uchar*) addr, CONFIG_SYS_FLASH_BASE, len, 1);
	ranand_set_sbb_max_addr(0);
#endif
#else
	program_bootloader_fit_image(addr, len);
#endif

	return 0;
}

static void TftpdSend(void)
{
	volatile uchar *pkt;
	volatile uchar *xp;
	volatile ushort *s;
	int	len = 0;
	/*
	*	We will always be sending some sort of packet, so
	*	cobble together the packet headers now.
	*/
	pkt = NetTxPacket + NetEthHdrSize() + IP_UDP_HDR_SIZE;

	switch (TftpState)
	{
	case STATE_RRQ:
		xp = pkt;
		s = (ushort*)pkt;
		*s++ = htons(TFTP_DATA);
		*s++ = htons(TftpBlock);/*fullfill the data part*/
		pkt = (uchar*)s;
		len = pkt - xp;
		break;

	case STATE_WRQ:
		xp = pkt;
		s = (ushort*)pkt;
		*s++ = htons(TFTP_ACK);
		*s++ = htons(TftpBlock);
		pkt = (uchar*)s;
		len = pkt - xp;
		break;

#ifdef ET_DEBUG
		printf("send option \"timeout %s\"\n", (char *)pkt);
#endif
		pkt += strlen((char *)pkt) + 1;
		len = pkt - xp;
		break;

	case STATE_DATA:
		xp = pkt;
		s = (ushort*)pkt;
		*s++ = htons(TFTP_ACK);
		*s++ = htons(TftpBlock);
		pkt = (uchar*)s;
		len = pkt - xp;
		break;

	case STATE_FINISHACK:
		xp = pkt;
		s = (ushort*)pkt;
		*s++ = htons(TFTP_FINISHACK);
		*s++ = htons(RescueAckFlag);
		pkt = (uchar*)s;
		len = pkt - xp;
		break;

	case STATE_TOO_LARGE:
		xp = pkt;
		s = (ushort*)pkt;
		*s++ = htons(TFTP_ERROR);
		*s++ = htons(3);
		pkt = (uchar*)s;
		strcpy((char *)pkt, "File too large");
		pkt += 14 /*strlen("File too large")*/ + 1;
		len = pkt - xp;
		break;

	case STATE_BAD_MAGIC:
		xp = pkt;
		s = (ushort*)pkt;
		*s++ = htons(TFTP_ERROR);
		*s++ = htons(2);
		pkt = (uchar*)s;
		strcpy((char *)pkt, "File has bad magic");
		pkt += 18 /*strlen("File has bad magic")*/ + 1;
		len = pkt - xp;
		break;
	}

	NetSendUDPPacket(NetServerEther, NetServerIP, TftpServerPort, TftpOurPort, len);
}

static void TftpTimeout(void)
{
	puts("T ");
	NetSetTimeout(TFTPD_TIMEOUT * CONFIG_SYS_HZ, TftpTimeout);
	TftpdSend();
}

static void TftpdTimeout(void)
{
	puts("D ");
	NetSetTimeout(TFTPD_TIMEOUT * CONFIG_SYS_HZ, TftpdTimeout);
}

#if defined(UBOOT_STAGE1)
static int load_stage2(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int i, ret, space, nr_good = 0;
	char addr1[] = "ffffffffXXX";
	char *boot_argv[] = { NULL, addr1 };
	struct stage2_loc *s2 = &g_s2_loc;
	struct stage2_desc *desc;

	if ((ret = ranand_check_and_fix_stage1())) {
		char buf[16], *p;

		printf("check & fix stage1 fail!!! (ret %d)\n", ret);
		p = getenv("reprog_stage1_status");
		if (!p || simple_strtol(p, NULL, 10) != ret) {
			sprintf(buf, "%d", ret);
			setenv("reprog_stage1_status", buf);
			saveenv();
		}
	}

	ranand_locate_stage2(s2);
	/* check and fix every stage2 if necessary/possible */
	for (i = 0, desc = &s2->desc[0];
	     i < s2->count && s2->good;
	     ++i, ++desc)
	{
		int ret, wlen = s2->good->blk_len;
		unsigned char *ptr = s2->good->code;
		image_header_t *hdr;

		if (!desc->crc_error) {
			nr_good++;
			if (!desc->corrected && !desc->failed)
				continue;

			ptr = desc->code;
			wlen = desc->blk_len;
		}

		hdr = (image_header_t*) ptr;
		if (!ranand_check_space(desc->offset, wlen, desc->boundary)) {
			printf("No space to program %x bytes stage2 code to %s@%x-%x\n",
				wlen, desc->name, desc->offset, desc->boundary);
			continue;
		}
		ranand_set_sbb_max_addr(desc->boundary);
		printf("Reprogram %s@%x-%x (source %p, len %x): ",
			desc->name, desc->offset, desc->boundary, ptr, wlen);
		ret = ranand_erase_write(ptr, desc->offset, wlen);
		if (ret != wlen) {
			printf("fail. (ret %d)\n", ret);
			continue;
		}
		nr_good++;
	}
	/* if it is possible to obtain more copy of stage2 code, reprogram whole stage2 area */
	if (s2->good) {
		space = ranand_check_space(CFG_BOOTSTAGE2_OFFSET, s2->good->blk_len, CFG_BOOTLOADER_SIZE);
		if (nr_good < (space / s2->good->blk_len)) {
			debug("Reprogram stage2 area to obtain more copy of stage2 code.\n");
			ranand_write_stage2((unsigned int) s2->good->code, s2->good->len);
			ranand_locate_stage2(s2);
		}
	}
	ranand_set_sbb_max_addr(0);

	if (!s2->good) {
		printf(" \nHello!! Enter Recuse Mode: (Check error)\n\n");
		if (NetLoop(TFTPD) < 0)
			return 1;
	}

	sprintf(addr1, "%x", s2->good->code);
	do_bootm(cmdtp, 0, ARRAY_SIZE(boot_argv), boot_argv);

	return 0;
}
#endif	/* UBOOT_STAGE1 */

static int load_asus_firmware(cmd_tbl_t *cmdtp, int argc, char * const argv[])
{
	if (do_bootm(cmdtp, 0, argc, argv)) {
		printf(" \nHello!! Enter Recuse Mode: (Check error)\n\n");
		if (NetLoop(TFTPD) < 0)
			return 1;
	}

	return 0;
}

int do_tftpd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;

#if ! defined(UBOOT_STAGE1)
	const int press_times = 1;
	int i = 0;

	if (DETECT())		/* Reset to default button */
	{
		wan_red_led_on();
		printf(" \n## Enter Rescue Mode ##\n");
		setenv("autostart", "no");
		/* Wait forever for an image */
		if (NetLoop(TFTPD) < 0)
			return 1;
	}
	else if (DETECT_WPS())	/* WPS button */
	{
		/* Make sure WPS button is pressed at least press_times * 0.01s. */
		while (DETECT_WPS() && i++ < press_times) {
			udelay(10000);
		}

		if (i >= press_times) {
			while (DETECT_WPS()) {
				udelay(90000);
				i++;
				if (i & 1)
					power_led_on();
				else
					power_led_off();
			}
			leds_off();

			reset_to_default();
			do_reset (NULL, 0, 0, NULL);
		}
	}
	else
#endif /* ! UBOOT_STAGE1 */
	{
#if defined(UBOOT_STAGE1)
		ret = load_stage2(cmdtp, argc, argv);
#else
		ret = load_asus_firmware(cmdtp, argc, argv);
#endif	/* CONFIG_DUAL_TRX */
	}

	return ret;
}

U_BOOT_CMD(
	tftpd, 1, 1, do_tftpd,
	"tftpd\t -load the data by tftp protocol\n",
	NULL
);

void TftpdStart(void)
{
	ptr = (unsigned char*) CONFIG_SYS_LOAD_ADDR;

#if defined(CONFIG_NET_MULTI)
	printf("Using %s device\n", eth_get_name());
#endif
	//puts(" \nTFTP from server ");	print_IPaddr(NetServerIP);
	printf("\nOur IP address is:(%pI4)\nWait for TFTP request...\n", &NetOurIP);
	/* Check if we need to send across this subnet */
	if (NetOurGatewayIP && NetOurSubnetMask) {
		IPaddr_t OurNet 	= NetOurIP    & NetOurSubnetMask;
		IPaddr_t ServerNet 	= NetServerIP & NetOurSubnetMask;

		if (OurNet != ServerNet)
			printf("sending through gateway %pI4\n", &NetOurGatewayIP);
	}

	memset(ptr,0,sizeof(ptr));
	_tftpd_open();
}

static void _tftpd_open()
{

	printf("tftpd open\n");	// tmp test
	NetSetTimeout(TFTPD_TIMEOUT * CONFIG_SYS_HZ * 2, TftpdTimeout);
	net_set_udp_handler(TftpHandler);

	TftpOurPort = PORT_TFTP;
	TftpTimeoutCount = 0;
	TftpState = STATE_RRQ;
	TftpBlock = 0;

	/* zero out server ether in case the server ip has changed */
	memset(NetServerEther, 0, 6);
}


static void TftpHandler(uchar * pkt, unsigned dport, IPaddr_t sip, unsigned sport, unsigned len)
{
	ushort proto;
	volatile ushort *s;

	if (dport != TftpOurPort)
	{
		return;
	}
	/* don't care the packets that donot send to TFTP port */

	if (TftpState != STATE_RRQ && sport != TftpServerPort)
	{
		return;
	}

	if (len < 2)
	{
		return;
	}

	len -= 2;
	/* warning: don't use increment (++) in ntohs() macros!! */
	s = (ushort*)pkt;
	proto = *s++;
	pkt = (uchar*)s;

	switch (ntohs(proto))
	{
	case TFTP_RRQ:

		printf("\n Get read request from:(%pI4)\n", &TempServerIP);
		NetCopyIP(&NetServerIP,&TempServerIP);

		TftpServerPort = sport;
		TftpBlock = 1;
		TftpBlockWrapOffset = 0;
		TftpState = STATE_RRQ;

		if (!memcmp(pkt, asuslink, 13) || !memcmp(pkt, maclink, 13)) {
			/* it's the firmware transmitting situation */
			/* here get the IP address in little-endian from the first packet. */
			NetCopy_LEIP(&NetOurIP, pkt + 13);
			printf("Firmware Restoration assigns IP address: %pI4\n", &NetOurIP);
		}

                /* Adjust subnetmask if NetOurIP and NetServerIP are not in the same domain.
		 * If there are two IP address, in different subnet, are assigned to one network adapter.
		 *
		 * Sometimes, IP address of PC may changes, e.g. 169.254.x.y ==> 192.168.1.x.
		 * But, Firmware Restoration doesn't updates it's IP address and assigns IP address in first subnet to us.
		 * Below patch guarantees TFTP packst which is used to response ASPSSPACELINK is send to PC.
		 * But, Firmware Restoration may not be able to send TFTP data packet which contains rescue image to us
		 * due to those packets are sended to PC's default gateway instead.
		 */
		if ((NetOurIP & NetOurSubnetMask) != (NetServerIP & NetOurSubnetMask)) {
			while ( NetOurSubnetMask && ((NetOurIP & NetOurSubnetMask) != (NetServerIP & NetOurSubnetMask))) {
                                NetOurSubnetMask <<= 1;
                        }
			printf("New netmask: %pI4\n", &NetOurSubnetMask);
		}

		TftpdSend();//send a vacant Data packet as a ACK
		break;

	case TFTP_WRQ:
		TftpServerPort = sport;
		TftpBlock = 0;
		TftpState = STATE_WRQ;
		TftpdSend();
		tail = ptr;
		copysize = 0;
		break;

	case TFTP_DATA:
		//printf("case TFTPDATA\n");	// tmp test
		if (len < 2)
			return;
		len -= 2;
		TftpBlock = ntohs(*(ushort *)pkt);
		/*
		* RFC1350 specifies that the first data packet will
		* have sequence number 1. If we receive a sequence
		* number of 0 this means that there was a wrap
		* around of the (16 bit) counter.
		*/
		if (TftpBlock == 0)
		{
			printf("\n\t %lu MB reveived\n\t ", TftpBlockWrapOffset>>20);
		}
		else
		{
			if (((TftpBlock - 1) % 10) == 0)
			{/* print out progress bar */
				puts("#");
			}
			else
				if ((TftpBlock % (10 * HASHES_PER_LINE)) == 0)
				{
					puts("\n");
				}
		}
		if (TftpState == STATE_WRQ)
		{
			/* first block received */
			TftpState = STATE_DATA;
			TftpServerPort = sport;
			TftpLastBlock = 0;
			TftpBlockWrapOffset = 0;
			printf("\n First block received  \n");
			//printf("Load Addr is %x\n", ptr);	// tmp test
			//ptr = 0x80100000;

			if (TftpBlock != 1)
			{	/* Assertion */
				printf("\nTFTP error: "
					"First block is not block 1 (%ld)\n"
					"Starting again\n\n",
					TftpBlock);
				NetStartAgain();
				break;
			}
		}

		if (TftpBlock == TftpLastBlock)
		{	/* Same block again; ignore it. */
			printf("\n Same block again; ignore it \n");
			break;
		}
		TftpLastBlock = TftpBlock;
		NetSetTimeout(TFTPD_TIMEOUT * CONFIG_SYS_HZ, TftpTimeout);

		copysize += len;	/* the total length of the data */
		(void)memcpy(tail, pkt + 2, len);/* store the data part to RAM */
		tail += len;

		/*
		*	Acknowledge the block just received, which will prompt
		*	the Server for the next one.
		*/
		TftpdSend();

		if (len < TFTP_BLOCK_SIZE)
		{
		/*
		*	We received the whole thing.  Try to run it.
		*/
			puts("\ndone\n");
			TftpState = STATE_FINISHACK;
			net_set_state(NETLOOP_SUCCESS);
			RAMtoFlash();
		}
		break;

	case TFTP_ERROR:
		printf("\nTFTP error: '%s' (%d)\n",
			pkt + 2, ntohs(*(ushort *)pkt));
		puts("Starting again\n\n");
		NetStartAgain();
		break;

	default:
		break;

	}
}


#if !defined(CONFIG_SYS_NO_FLASH)
void flash_perrormsg (int err)
{
	switch (err) {
	case ERR_OK:
		break;
	case ERR_TIMOUT:
		puts ("Timeout writing to Flash\n");
		break;
	case ERR_NOT_ERASED:
		puts ("Flash not Erased\n");
		break;
	case ERR_PROTECTED:
		puts ("Can't write to protected Flash sectors\n");
		break;
	case ERR_INVAL:
		puts ("Outside available Flash\n");
		break;
	case ERR_ALIGN:
		puts ("Start and/or end address not on sector boundary\n");
		break;
	case ERR_UNKNOWN_FLASH_VENDOR:
		puts ("Unknown Vendor of Flash\n");
		break;
	case ERR_UNKNOWN_FLASH_TYPE:
		puts ("Unknown Type of Flash\n");
		break;
	case ERR_PROG_ERROR:
		puts ("General Flash Programming Error\n");
		break;
	default:
		printf ("%s[%d] FIXME: rc=%d\n", __FILE__, __LINE__, err);
		break;
	}
}
#else
void flash_perrormsg (int err)
{
	printf("Flash error code: %d\n", err);
}
#endif

#if defined(CONFIG_UBI_SUPPORT)
/* Program UBI image with or without OOB information to UBI_DEV.  */
int SolveUBI(void)
{
	printf("Solve UBI, ptr=0x%p\n", ptr);
	return __SolveUBI(ptr, CFG_UBI_DEV_OFFSET, copysize);
}
#endif

static void RAMtoFlash(void)
{
	int i=0, parseflag=0;
	void *hdr;
#if !defined(UBOOT_STAGE1)
	int j=0;
	uchar mac_temp[7],secretcode[9];
	uint8_t SCODE[5] = {0x53, 0x43, 0x4F, 0x44, 0x45};
	char *end;
	unsigned char *tmp;
#endif

	printf("RAMtoFLASH\n");	// tmp test
	/* Check for the TRX magic. If it's a TRX image, then proceed to flash it. */
	if (!memcmp(ptr, trx_magic, sizeof(trx_magic)) && (copysize > CFG_MAX_BOOTLOADER_BINARY_SIZE))
	{
#if defined(UBOOT_STAGE1)
		printf("not support Firmware in Uboot Stage1 !!!\n");
		rc = 1;	/* send fail response to firmware restoration utility */
#else
		printf("Chk trx magic\n");	// tmp test
		printf("Download of 0x%x bytes completed\n", copysize);
		printf("Check TRX and write it to FLASH \n");
		SolveTRX();
#endif /* UBOOT_STAGE1 */
	}
#if defined(CONFIG_UBI_SUPPORT)
	else if (be32_to_cpu(*(uint32_t *)ptr) == UBI_EC_HDR_MAGIC) {
		int r;

		printf("Check UBI magic\n");
		printf("Download of 0x%x bytes completed\n", copysize);
		if ((r = SolveUBI()) != 0) {
			printf("rescue UBI image failed! (r = %d)\n", r);
			net_set_state(NETLOOP_FAIL);
			TftpState = STATE_FINISHACK;
			RescueAckFlag = 0x0000;
			for (i = 0; i < 6; i++)
				TftpdSend();
			return;
		} else {
			printf("done. %d bytes written\n", copysize);
			TftpState = STATE_FINISHACK;
			RescueAckFlag = 0x0001;
			for (i = 0; i < 6; i++)
				TftpdSend();
		}

		printf("SYSTEM RESET!!!\n\n");
		do_reset (NULL, 0, 0, NULL);
	}
#endif
	else
	{
		const char *q;
		unsigned int magic_offset = 0x2FFF0;

		if (CFG_MAX_BOOTLOADER_BINARY_SIZE > 0x30000)
			magic_offset = CFG_MAX_BOOTLOADER_BINARY_SIZE - 7;

		printf("Magic number offset: %x\n", magic_offset);
		q = (char*)(ptr + magic_offset);
		printf("upgrade boot code\n");	// tmp test

#if defined(UBOOT_STAGE1) || defined(UBOOT_STAGE2)
		if (copysize > 0 && copysize <= CFG_BOOTSTAGE1_SIZE && !memcmp(ptr, trx_magic))	// uboot Stage1
		{
			ulong addr = (ulong)ptr, len = copysize;

			printf(" Download of 0x%x bytes completed\n", copysize);
			printf("Write boot Stage1 binary to FLASH \n");
			rc = program_bootloader(addr, len);
			parseflag = 5;	//avoid to write
		}
		else
#endif
		if (copysize > 0 && copysize <= CFG_MAX_BOOTLOADER_BINARY_SIZE)		// uboot
		{
			/* ptr + magic_offset may not aligned to 4-bytes boundary.
			 * use char pointer to access it instead.
			 */
			if (copysize >= (magic_offset+4) &&
				!memcmp(q, nvram_magic, sizeof(nvram_magic)) &&
				memcmp(ptr, nvram_magic_mac0, sizeof(nvram_magic_mac0)) &&
				memcmp(ptr, nvram_magic_mac1, sizeof(nvram_magic_mac1)) &&
				memcmp(ptr, nvram_magic_rdom, sizeof(nvram_magic_rdom)) &&
				memcmp(ptr, nvram_magic_asus, sizeof(nvram_magic_asus)))
			{
				printf(" Download of 0x%x bytes completed\n", copysize);
				printf("Write bootloader binary to FLASH \n");
				parseflag = 1;
			}
#if ! defined(UBOOT_STAGE1)
			else if (!memcmp(ptr, nvram_magic_mac0, sizeof(nvram_magic_mac0)))
			{
				printf("  Download of 0x%x bytes completed\n", copysize);
				memset(MAC0, 0, sizeof(MAC0));
				for (i=0; i<17; i++)
					MAC0[i] = ptr[4+i];

				i = i + 4;
				/* Debug message */
				if ((ptr[i]==SCODE[0])
					&&(ptr[i+1]== SCODE[1])
					&&(ptr[i+2]== SCODE[2])
					&&(ptr[i+3]== SCODE[3])
					&&(ptr[i+4]== SCODE[4]))
				{
					for (i=26,j=0; i<34; i++)
					{
						secretcode[j] = ptr[i];
						printf("secretcode[%d]=%x, ptr[%d]=%x\n", j, secretcode[j], i, ptr[i]);
						j++;
					}
					secretcode[j]='\0';
					printf("Write secret code = %s to FLASH\n", secretcode);
					rc = replace(0x100, secretcode, 9);

				}
				tmp=MAC0;
				for (i=0; i<6; i++)
				{
					mac_temp[i] = tmp ? simple_strtoul((char*) tmp, &end, 16) : 0;
					if (tmp)
						tmp= (uchar*) ((*end) ? end+1 : end);
				}
				mac_temp[i] = '\0';

				printf("Write MAC0 = %s  to FLASH \n", MAC0);
				MAC_FLAG=1;
				rc = replace(0x4, mac_temp, 7);
				sprintf((char*) nvramcmd, "nvram set et0macaddr=%s", (char*) MAC0);
				printf("%s\n", nvramcmd);

				parseflag = 2;
			}
			else if (!memcmp(ptr, nvram_magic_rdom, sizeof(nvram_magic_rdom)))
			{
				for (i=0; i<6; i++)
					RDOM[i] = ptr[8+i];
				RDOM[i] = '\0';
				rc = replace(0x4e, RDOM, 7);
				printf("Write RDOM = %s  to FLASH \n", RDOM);
				sprintf((char*) nvramcmd, "nvram set regulation_domain=%s", (char*) RDOM);
				printf("%s\n", nvramcmd);
				parseflag = 3;
			}
			else if (!memcmp(ptr, nvram_magic_asus, sizeof(nvram_magic_asus)))
			{
				memset(ASUS, 0, sizeof(ASUS));
				for (i=0; i<23; i++)
					ASUS[i] = ptr[4+i];
				memset(MAC0, 0, sizeof(MAC0));
				for (i=0; i<17; i++)
					MAC0[i] = ASUS[i];
				memset(RDOM, 0, sizeof(RDOM));
				rc = replace(0x4, mac_temp, 7);
				for (i=0; i<6; i++)
					RDOM[i] = ASUS[17+i];
				printf("Write MAC0 = %s  to FLASH \n", (char*) MAC0);
				sprintf((char*) nvramcmd, "nvram set et0macaddr=%s", (char*) MAC0);
				printf("%s\n", nvramcmd);
				rc = replace(0x4e, RDOM, 7);
				printf("Write RDOM = %s  to FLASH \n", RDOM);
				sprintf((char*) nvramcmd, "nvram set regulation_domain=%s", (char*) RDOM);
				printf("%s\n", nvramcmd);
				parseflag = 4;
			}
#endif	/* ! UBOOT_STAGE1 */
			else
			{
				parseflag = -1;
				printf("Download of 0x%x bytes completed\n", copysize);
				printf("Warning, Bad Magic word!!\n");
				net_set_state(STATE_BAD_MAGIC);
				TftpdSend();
				copysize = 0;
			}

		}
		else if (copysize > CFG_MAX_BOOTLOADER_BINARY_SIZE)
		{
			parseflag = 0;
			printf("    Download of 0x%x bytes completed\n", copysize);
		}
		else
		{
			parseflag = -1;
			copysize = 0;
			printf("Downloading image time out\n");
		}


		if (copysize == 0)
			return;    /* 0 bytes, don't flash */

		if (parseflag != 0)
			copysize = CFG_MAX_BOOTLOADER_BINARY_SIZE;

		if (parseflag == 1 && memcmp(ptr, trx_magic, sizeof(trx_magic))) {
			printf("Not uboot trx image!\n");
			parseflag = -1;
			rc = -1;
		}
		/* skip .trx header */
		hdr = (void*) (ptr + sizeof(image_header_t));
		copysize -= sizeof(image_header_t);
		if (parseflag == 1 && genimg_get_format(hdr) != IMAGE_FORMAT_FIT) {
			puts("Unknown uboot FIT image format!\n");
			parseflag = -1;
			rc = -1;
		}

		printf("parseflag %d\n", parseflag);
		if (parseflag == 1 && (rc = program_bootloader((ulong)hdr, copysize)) != 0)
			parseflag = -1;

		if (rc) {
			printf("rescue failed!\n");
			flash_perrormsg(rc);
			net_set_state(NETLOOP_FAIL);
			/* Network loop state */
			TftpState = STATE_FINISHACK;
			RescueAckFlag= 0x0000;
			for (i=0; i<6; i++)
				TftpdSend();
			return;
		}
		else {
			printf("done. %d bytes written\n",copysize);
			TftpState = STATE_FINISHACK;
			RescueAckFlag= 0x0001;
			for (i=0; i<6; i++)
				TftpdSend();
			if (parseflag != 0)
			{
				printf("SYSTEM RESET \n\n");
				udelay(500);
				do_reset(NULL, 0, 0, NULL);
			}
			return;
		}
	}
}

#if ! defined(UBOOT_STAGE1)
static void SolveTRX(void)
{
	int  i = 0;
	int rc = 0;
	ulong count = 0;
	int e_end = 0;
	int reset = 0;

	printf("Solve TRX, ptr=0x%x\n", (uint) ptr);
	load_addr = (unsigned long)ptr;
	if (check_trx(0, NULL) <= 0) {
		rc = reset = 1;
		printf("Check trx error!\n\n");
	}
	else {
		count = copysize;

		if (count == 0) {
			puts ("Zero length ???\n");
			return;
		}

		e_end = CFG_KERN_ADDR + count;

		printf("\n Erase kernel block !!\n From %x To %x (%lu/h:%lx)\n",
			CFG_KERN_ADDR, e_end, count, count);
		rc = ra_flash_erase_write(ptr+sizeof(image_header_t), CFG_KERN_ADDR, count-sizeof(image_header_t), 0);
#if defined(CONFIG_DUAL_TRX)
		if (rc)
			printf("Write 1st firmware fail. (r = %d)\n", rc);
		rc = ra_flash_erase_write(ptr+sizeof(image_header_t), CFG_KERN2_ADDR, count-sizeof(image_header_t), 0);
		if (rc)
			printf("Write 2nd firmware fail. (r = %d)\n", rc);
#else
		if (rc)
			printf("Write firmware fail. (r = %d)\n", rc);
#endif
	}

	if (rc) {
		printf("rescue failed! (%d)\n", rc);
		net_set_state(NETLOOP_FAIL);
		TftpState = STATE_FINISHACK;
		RescueAckFlag= 0x0000;
		for (i=0; i<6; i++)
			TftpdSend();
	}
	else {
		printf("done. %lu bytes written\n", count);
		TftpState = STATE_FINISHACK;
		RescueAckFlag= 0x0001;
		for (i=0; i<6; i++)
			TftpdSend();
		reset = 1;
	}

	if (reset) {
		printf("\nSYSTEM RESET!!!\n\n");
		udelay(500);
		do_reset(NULL, 0, 0, NULL);
	}
}
#endif /* ! UBOOT_STAGE1 */

#endif	/* ASUS_PRODUCT */
