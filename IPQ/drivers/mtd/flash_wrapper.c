// vim:cin
/*
 * Copyright 2013, ASUSTeK Inc.
 * All Rights Reserved.
 */
#include <common.h>
#include <command.h>
#include <asm/errno.h>
#include <malloc.h>

#define QC98XX_EEPROM1_OFFSET		0x1000
#define QC98XX_EEPROM2_OFFSET		0x5000
#define QC98XX_EEPROM_SIZE_LARGEST	12064

#include <flash_wrapper.h>
#if defined(CONFIG_UBI_SUPPORT)
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <ubi_uboot.h>
#include <jffs2/load_kernel.h>

/* common/cmd_mtdparts.c */
extern int do_mtdparts(cmd_tbl_t *cmdtp, int flag, int argc, char * argv[]);

/* drivers/mtd/nand/nand.c */
extern struct mtd_info nand_info[CONFIG_SYS_MAX_NAND_DEVICE];
extern int nand_curr_device;

int choose_active_eeprom_set(void);

/* To compatible with MTK's WiFi driver, put factory header to
 * last NAND Flash page of every LEB in Factory/Factory2 volume.
 */
#define EEPROM_SET_HEADER_OFFSET	(LEB_SIZE - 2*1024)
#define MAX_EEPROM_SET_LENGTH		EEPROM_SET_HEADER_OFFSET
/* two eeprom set per factory volume. two factory volume. */
#define NR_EEPROM_SETS		((CFG_UBI_FACTORY_SIZE + CFG_UBI_FACTORY2_SIZE) / LEB_SIZE)

#ifdef crc32
#undef crc32
#endif

#define FACTORY_IMAGE_MAGIC	0x46545259	/* 'F', 'T', 'R', 'Y' */
typedef struct eeprom_set_hdr_s {
	uint32_t ih_magic;	/* Image Header Magic Number = 'F', 'T', 'R', 'Y' */
	uint32_t ih_hcrc;	/* Image Header CRC Checksum    */
	uint32_t ih_hdr_ver;	/* Image Header Version Number  */
	uint32_t ih_write_ver;	/* Number of writes             */
	uint32_t ih_dcrc;	/* Image Data CRC Checksum      */
} eeprom_set_hdr_t;
#else
#define MAX_EEPROM_SET_LENGTH		65536
extern int do_spi_flash(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
#endif

static int update_qca99xx_eeprom_csum(void *ptr)
{
	int i;
	uint16_t *p = ptr, orig_csum, sum = 0;

	if (!ptr)
		return -1;

	orig_csum = *(p + 1);
	*(p + 1) = 0;
	for (i = 0; i < (QC98XX_EEPROM_SIZE_LARGEST / 2); ++i, ++p)
		sum ^= __le16_to_cpu(*p);

	p = ptr;
	*(p + 1) = __cpu_to_le16(sum ^ 0xFFFF);

	printf("csum %04x -> %04x\n", orig_csum, *(p + 1));
	return 0;
}

/*
 * Whether o1 ~ (o1 + l1 - 1) overlaps o2 ~ (o2 + l2 - 1) or not.
 * @return:
 * 	0:	o1 ~ (o1 + l1 + 1) not overlaps o2 ~ (o2 + l2 - 1)
 *  otherwise:	o1 ~ (o1 + l1 + 1) overlaps o2 ~ (o2 + l2 - 1)
 */
static int in_range(ulong o1, ulong l1, ulong o2, ulong l2)
{
	ulong e1 = o1 + l1, e2 = o2 + l2;

	if (e1 <= o2 || o1 >= e2)
		return 0;

	return 1;
}

#if defined(CONFIG_UBI_SUPPORT)
/* The factory_offset is used to shift address to actived copy in factory data.
 * In most cases, it should point to first copy of Factory volume.
 * If 2-nd copy of Factory volume is choosed, add LEB_SIZE to it.
 * If 1-st copy of Factory2 volume is choosed, add 2 * LEB_SIZE to it.
 * If 2-nd copy of Factory2 volume is choosed, add 3 * LEB_SIZE to it.
 */
static unsigned long factory_offset = 0;
static unsigned char *eeprom_set = NULL;
static int all_sets_damaged = 0;

const static struct ubi_vol_s {
	const char name[12];
	unsigned int size;	/* volume length in bytes */
	int type;		/* 0: static; otherwise: dynamic */
} ubi_vol[] = {
	{ "nvram",	CFG_UBI_NVRAM_SIZE, 	1 },
	{ "Factory",	CFG_UBI_FACTORY_SIZE,	1 },
	{ "Factory2",	CFG_UBI_FACTORY2_SIZE,	1 },
	{ "linux",	CFG_UBI_FIRMWARE_SIZE,	1 },
	{ "linux2",	CFG_UBI_FIRMWARE2_SIZE,	1 },
	/* last volume size would be auto-resize according to free space */
	{ "jffs2",	CFG_UBI_APP_SIZE,	1 },
	{ "", 0, 0 },
};

/* Get pointer to ubi_volume by volume name.
 * @ubi:	pointer to ubi_device
 * @name:	volume name
 * @return:
 * 	NULL:	invalid parameter or not found
 *  otherwise:	success
 */
static struct ubi_volume* get_vol_by_name(struct ubi_device *ubi, const char *name)
{
	int i;
	struct ubi_volume *v = NULL;

	if (!ubi || !name)
		return v;

	for (i = 0; i < (ubi->vtbl_slots + 1) && !v; ++i) {
		if (!ubi->volumes[i] || strcmp(name, ubi->volumes[i]->name))
			continue;

		v = ubi->volumes[i];
	}

	return v;
}

/* Find specified volume is X-th volume.
 * @return:
 * 	-1:	invalid parameter
 * 	-2:	UBI device is not attached
 * 	-3:	volume not found
 *     >0:	the volume is at X-th. X start from 1
 *	0:	not defined.
 */
int get_ubi_volume_idseq_by_addr(const char *name)
{
	int i, seq = 0;
	struct ubi_device *ubi;
	struct ubi_volume *v = NULL;

	if (!name || *name == '\0')
		return -1;
	if ((ubi = get_ubi_device()) == NULL)
		return -2;
	if ((v = get_vol_by_name(ubi, name)) == NULL)
		return -3;

	for (i = 0; i < (ubi->vtbl_slots + 1); ++i) {
		if (!ubi->volumes[i] || !strcmp(name, ubi->volumes[i]->name))
			continue;

		if (ubi->volumes[i]->vol_id < v->vol_id)
			seq++;
	}

	seq++;
	return seq;
}

/* Get volume name and it's start offset address.
 * @addr:	relative address to flash
 * @offset:	if success, it would be store start offset of the volume regards to Flash.
 * @return
 *  NULL:	specified address is not belong to any volume.
 *  otherwise:	volume name
 */
char *get_ubi_volume_param(unsigned long addr, unsigned long *offset, unsigned int *size)
{
	char *found = NULL;
	unsigned long flash_offset = CFG_UBI_DEV_OFFSET;
	const struct ubi_vol_s *p;

	if (!offset || !size)
		return NULL;

	for (p = &ubi_vol[0]; !found && p->size; ++p) {
		if (addr >= flash_offset && addr < (flash_offset + p->size)) {
			found = (char*) p->name;
			*offset = flash_offset;
			*size = p->size;
			break;
		}

		flash_offset += p->size;
	}

	return found;
}

/* Initialize UBI volumes
 * @return:
 * 	0:	success
 *  otherwise:	fail
 */
int init_ubi_volumes(struct ubi_device *ubi)
{
	int id, rcode, show=0;
	struct ubi_volume *v;
	unsigned int size;
	const struct ubi_vol_s *p, *q;
	char tmp[] = "ffffffffXXXX", tmp1[] = "ffffffffXXX", type[] = "dynamicXXX";
	char *ubi_create_vol[] = { "ubi", "createvol", NULL, tmp, type };
	char *ubi_info_wlayout[] = { "ubi", "info", "wlayout" };

	if (!ubi) {
		char *mtdparts[] = { "mtdparts" };
		char *ubi_part[] = { "ubi", "part", "UBI_DEV" };

		/* Show MTD partition layout */
		setenv("mtdids", MTDIDS);
		setenv("mtdparts", MTDPARTS);
		do_mtdparts(NULL, 0, ARRAY_SIZE(mtdparts), mtdparts);

		printf("Initialize UBI device area!!\n");
		rcode = do_ubi(NULL, 0, ARRAY_SIZE(ubi_part), ubi_part);

		if (rcode && rcode != 1)
			return rcode;
	}

	for (p = &ubi_vol[0], id = 0; p->size; ++p, ++id) {
		q = p + 1;
		v = get_vol_by_name(ubi, p->name);
		if (v) {
			/* volume already exist. sanity check */
			if (v->vol_id != id) {
				printf("UBI volume [%s] id %d mismatch! (expect %d)\n",
					p->name, v->vol_id, id);
			}

			size = v->reserved_pebs * v->usable_leb_size;
			if (size < p->size && q->size)
				printf("UBI volume [%s] size %x smaller than %x!\n", p->name, size, p->size);
			continue;
		}

		show++;
		ubi_create_vol[2] = (char*) p->name;
		sprintf(tmp, "%x", p->size);
		/* If next volume size is zero, assign all available
		 * space to current volume instead of specified size.
		 */
		if (q->size == 0)
			strcpy(tmp, "0");
		if (p->type)
			strcpy(type, "dynamic");
		else
			strcpy(type, "static");
		rcode = do_ubi(NULL, 0, ARRAY_SIZE(ubi_create_vol), ubi_create_vol);
		if (rcode < 0)
			printf("Create volume %s fail. rcode 0x%x\n", p->name, rcode);

		if (!p->type) {
			char *ubi_write_vol[] = { "ubi", "writevol", tmp, (char*)p->name, tmp1 };

			sprintf(tmp, "%x", CONFIG_SYS_LOAD_ADDR);
			sprintf(tmp1, "%x", p->size);
			memset((void*)CONFIG_SYS_LOAD_ADDR, 0x00, p->size);
			do_ubi(NULL, 0, ARRAY_SIZE(ubi_write_vol), ubi_write_vol);
		}
	}

	if (show) {
		printf("Latest UBI volumes layout.\n");
		do_ubi(NULL, 0, ARRAY_SIZE(ubi_info_wlayout), ubi_info_wlayout);
	}

	return 0;
}
#endif

#if defined(CONFIG_UBI_SUPPORT)
int ranand_block_isbad(loff_t offs)
{
	struct mtd_info *mtd = &nand_info[nand_curr_device];

	return mtd->block_isbad(mtd, offs);

}

/** Erase blocks.
 * @offs:	block alilgned address
 * @len:	block aligned length
 * @return:
 * 	0:	success
 *  -EIO:	I/O error
 */
int ranand_erase(unsigned int offs, int len)
{
	struct mtd_info *mtd = &nand_info[nand_curr_device];
	int r;
	struct erase_info instr;

	if (!mtd)
		return -ENODEV;

	memset(&instr, 0, sizeof(instr));
	instr.mtd = mtd;
	instr.addr = offs;
	instr.len = len;
	r = mtd->erase(mtd, &instr);

	return r;
}

/** Write data to NAND Flash
 * @buffer:	pointer to data
 * @to:		start offset address of NAND Flash to be written
 * @datalen:	number of bytes to be written
 * @return:
 *  >=0		length of writted bytes
 *  < 0		error occurs
 *  -EIO:	write page fail.
 *  -EAGAIN:	write-disturb, suggested caller to erase and write again.
 *  -ENODEV:	No such device.
 */
int ranand_write(unsigned char *buf, unsigned int to, int datalen)
{
	struct mtd_info *mtd = &nand_info[nand_curr_device];
	int r;
	size_t retlen;

	if (!mtd)
		return -ENODEV;

	r = mtd->write(mtd, to, datalen, &retlen, buf);
	if (r < 0)
		return r;

	return retlen;
}

/* read data from NAND Flash
 * @return:
 *     >0:	success, number of read bytes.
 *     =0:	none of any bytes are read
 *     -1:	invalid parameter
 *  otherwise:	not defined.
 *  If return value is less than datalen, it may be caused by uncorrectable error.
 */
int ranand_read(unsigned char *buf, unsigned int from, int datalen)
{
	struct mtd_info *mtd = &nand_info[nand_curr_device];
	int r;
	size_t retlen;

	if (!mtd)
		return -ENODEV;

	r = mtd->read(mtd, from, datalen, &retlen, buf);
	if (r < 0)
		return r;

	return retlen;
}

/* erase and write to nand flash
 * @return:
 *    >=0:	success, number of bytes be written to flash
 *     -1:	invalid parameter
 *  otherwise:	error
 */
int ranand_erase_write(unsigned char *buf, unsigned int offs, int count)
{
	int r;

	r = ranand_erase(offs, count);
	if (r)
		return r;
	return ranand_write(buf, offs, count);
}

#ifdef UBOOT_ON_NAND
/**
 * Check bootloader and fix it if necessary.
 * @return:
 *      0:      success, nothing happen
 *  negative:   error
 *  positive:   reprogram bootloader success
 */
int ranand_check_and_fix_bootloader(void)
{
	struct mtd_info *mtd = &nand_info[nand_curr_device];
	int r, retry;
	unsigned int o;
	uint8_t buf[0x20000] __attribute__ ((aligned(4))), *p = &buf[0];

	if (!mtd)
		return -ENODEV;

	if (mtd->erasesize > sizeof(buf) && !(p = malloc(mtd->erasesize))) {
		printf("%s: allocate %d bytes failed!\n", __func__, mtd->erasesize);
		return -ENOMEM;
	}

#if defined CONFIG_IPQ806X
	run_command("ipq_nand sbl", 0);
#endif
	puts("Check bootloader area ... \n");
	for (o = 0; o < CONFIG_MAX_BL_BINARY_SIZE; o += mtd->erasesize) {
		if (ranand_block_isbad(o))
			continue;
		r = ranand_read(p, o, mtd->erasesize);
		if (r == mtd->erasesize)
			continue;
		else if (r < mtd->erasesize && (r != -EUCLEAN && r != -EBADMSG)) {
			printf("%s: read offset %08x failed, ret %d\n",
				__func__, o, r);
			continue;
		}

		printf("Fix block @ offset %08x ... ", o);
		for (retry = 0; retry < 3; ++retry) {
			if ((r = ranand_erase_write(p, o, mtd->erasesize)) != mtd->erasesize)
				continue;

			break;
		}
		printf("%s\n", (r == mtd->erasesize)? "OK" : "FAIL");
	}

#if defined CONFIG_IPQ806X
	run_command("ipq_nand linux", 0);
#endif
	if (p != &buf[0])
		free(p);

	return 0;
}
#endif /* UBOOT_ON_NAND */
#endif	/* CONFIG_UBI_SUPPORT */


/**
 * Initialize Flash layout.
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
int ra_flash_init_layout(void)
{
#if defined(CONFIG_UBI_SUPPORT)
	int r;
	struct ubi_device *ubi;
	char *ubi_part[] = { "ubi", "part", "UBI_DEV" };
	char *ubi_info_layout[] = { "ubi", "info", "wlayout" };

	eeprom_set = malloc(LEB_SIZE);
	if (!eeprom_set)
		printf("Allocate %d bytes memory for eeprom_set fail!\n", LEB_SIZE);

	setenv("mtdids", MTDIDS);
	setenv("mtdparts", MTDPARTS);
	r = do_ubi(NULL, 0, ARRAY_SIZE(ubi_part), ubi_part);
	if (r) {
		printf("Mount UBI device fail. (r = %d)\n", r);
		return -ENOENT;
	}
	do_ubi(NULL, 0, ARRAY_SIZE(ubi_info_layout), ubi_info_layout);

	ubi = get_ubi_device();
	r = init_ubi_volumes(ubi);
	if (r)
		return -ENOENT;

	choose_active_eeprom_set();
#else
	int r;
	char *sf_probe[] = { "sf", "probe" };

	r = do_spi_flash(NULL, 0, ARRAY_SIZE(sf_probe), sf_probe);

	if (r)
		printf("%s: Probe SPI Flash fail. (r = %d)\n", __func__, r);
#endif
	return 0;
}

/**
 * Read data from NAND/SPI/Parallel_NOR Flash.
 * @buf:	buffer address
 * @addr:	Absolute address to read (include CONFIG_SYS_FLASH_BASE).
 * @len:	length to read
 * @return:
 * 	0:	success
 *     -1:	invalid parameter
 *  -ENOENT:	UBI not ready
 *     -3:	address not belongs to UBI device
 *     -5:	error or read length is not equal to len
 *  otherwise:	error
 */
int ra_flash_read(uchar * buf, ulong addr, ulong len)
{
	int ret = 0;
	unsigned int flash_offset = addr - CONFIG_SYS_FLASH_BASE;
#if defined(CONFIG_UBI_SUPPORT)
#else
	char addr_buf[15], off_buf[15], len_buf[15];
	char *sf_read[] = { "sf", "read", addr_buf, off_buf, len_buf };
#endif

	if (!buf || !len || addr < CONFIG_SYS_FLASH_BASE) {
		debug("%s(): invalid parameter. buf %p addr 0x%08lx len 0x%08lx\n",
			__func__, buf, addr, len);
		return -1;
	}

#if defined(CONFIG_UBI_SUPPORT)
	if (flash_offset >= CFG_UBI_DEV_OFFSET) {
		/* Redirect to UBI volume */
		char *vol;
		char addr_buf[] = "ffffffffXXX", len_buf[] = "ffffffffXXX", vol_offset_buf[] = "ffffffffXXX";
		char *ubi_readvol[] = { "ubi", "readvol", addr_buf, NULL, len_buf, vol_offset_buf };
		unsigned int size;
		unsigned long vol_offset;
		const struct ubi_device *ubi = get_ubi_device();

		if (!ubi)
			return -ENOENT;

		vol = get_ubi_volume_param(flash_offset, &vol_offset, &size);
		if (!vol) {
			printf("%s: addr %08lx not belongs to any volume\n", __func__, addr);
			return -3;
		}
		ubi_readvol[3] = vol;
		sprintf(addr_buf, "%x", (unsigned int)buf);
		sprintf(len_buf, "%lx", len);
		sprintf(vol_offset_buf, "%lx", flash_offset - vol_offset);
		return do_ubi(NULL, 0, ARRAY_SIZE(ubi_readvol), ubi_readvol);
	} else {
		ret = ranand_read(buf, flash_offset, len);
	}
#else
	sprintf(addr_buf, "%x", (uint) buf);
	sprintf(off_buf, "%x", flash_offset);
	sprintf(len_buf, "%lx", len);
	ret = do_spi_flash(NULL, 0, ARRAY_SIZE(sf_read), sf_read);
	if (!ret)
		ret = len;
#endif

	if (ret > 0 && ret == len)
		return 0;
	else
		return -5;
}


/**
 * Write data to NAND/SPI/Parallel_NOR Flash.
 * @buf:	buffer address
 * @addr:	Absolute address to read. (include CONFIG_SYS_FLASH_BASE)
 * @len:	length to read
 * @prot:	Unprotect/protect sector. (Parallel NOR Flash only)
 * @return:
 * 	0:	success
 *  -ENOENT:	UBI not ready
 *  otherwise:	error
 ******************************************************************
 * WARNING
 ******************************************************************
 * For Parallel NOR Flash, you must specify sector-aligned address.
 */
int ra_flash_erase_write(uchar * buf, ulong addr, ulong len, int prot)
{
	int ret = 0;
	unsigned int flash_offset = addr - CONFIG_SYS_FLASH_BASE;
#if defined(CONFIG_UBI_SUPPORT)
#else
	char addr_buf[15], off_buf[15], len_buf[15];
	char *sf_prog[] = { "sf", "prog", addr_buf, off_buf, len_buf };
#endif

	if (!buf || !len || addr < CONFIG_SYS_FLASH_BASE) {
		debug("%s(): invalid parameter. buf %p addr 0x%08lx len 0x%08lx\n",
			__func__, buf, addr, len);
		return -1;
	}

#if defined(CONFIG_UBI_SUPPORT)
	if (flash_offset >= CFG_UBI_DEV_OFFSET) {
		/* Redirect to UBI volume */
		int r;
		char *vol;
		char addr_buf[] = "ffffffffXXX", len_buf[] = "ffffffffXXX";
		char *ubi_readvol[] = { "ubi", "readvol", addr_buf, NULL, len_buf };
		char *ubi_writevol[] = { "ubi", "writevol", addr_buf, NULL, len_buf };
		unsigned char *tmp;
		unsigned int size;
		unsigned long vol_offset, o;
		const struct ubi_device *ubi = get_ubi_device();

		if (!ubi)
			return -ENOENT;

		vol = get_ubi_volume_param(flash_offset, &vol_offset, &size);
		if (!vol) {
			printf("%s: addr %08lx not belongs to any volume\n", __func__, addr);
			return -2;
		}

		/* For Factory volume, always read whole volume, update data, and write whole volume. */
		o = flash_offset - vol_offset;
		if (!strcmp(vol, "Factory") || !strcmp(vol, "Factory2")) {
			/* Read whole volume,  update data, write back to volume. */
			tmp = malloc(size);
			if (!tmp) {
				printf("%s: allocate %u bytes buffer fail.\n", __func__, size);
				return -ENOMEM;
			}

			if (len < size) {
				ubi_readvol[3] = vol;
				sprintf(addr_buf, "%x", (unsigned int)tmp);
				sprintf(len_buf, "%x", size);
				r = do_ubi(NULL, 0, ARRAY_SIZE(ubi_readvol), ubi_readvol);
				if (r) {
					printf("%s: read volume [%s] fail. (r = %d)\n", __func__, vol, r);
					free(tmp);
					return r;
				}
			}

			memcpy(tmp + o, buf, len);
			ubi_writevol[3] = vol;
			sprintf(addr_buf, "%x", (unsigned int)tmp);
			sprintf(len_buf, "%x", size);
			r = do_ubi(NULL, 0, ARRAY_SIZE(ubi_writevol), ubi_writevol);
			if (r) {
				printf("%s: write volume [%s] fail. (r = %d)\n", __func__, vol, r);
				free(tmp);
				return r;
			}

			free(tmp);
		} else {
			if (o) {
				printf("Start offset address have to be zero!\n");
				return -EINVAL;
			}

			ubi_writevol[3] = vol;
			sprintf(addr_buf, "%x", (unsigned int)buf);
			sprintf(len_buf, "%lx", len);
			r = do_ubi(NULL, 0, ARRAY_SIZE(ubi_writevol), ubi_writevol);
			if (r) {
				printf("%s: write volume [%s] fail. (r = %d)\n", __func__, vol, r);
				return r;
			}
			return 0;
		}
	} else {
		ranand_set_sbb_max_addr(CFG_UBI_DEV_OFFSET);
		ret = ranand_erase_write(buf, flash_offset, len);
		if (ret == len)
			ret = 0;
		ranand_set_sbb_max_addr(0);
	}
#else
	sprintf(addr_buf, "%x", (uint) buf);
	sprintf(off_buf, "%x", flash_offset);
	sprintf(len_buf, "%lx", len);
	ret = do_spi_flash(NULL, 0, ARRAY_SIZE(sf_prog), sf_prog);
	if (ret) {
		printf("%s: write offset %x len %lx fail. (r = %d)\n",
			__func__, flash_offset, len, ret);
		return ret;
	}
#endif

	return ret;
}


/**
 * Erase NAND/SPI/Parallel_NOR Flash.
 * @addr:	Absolute address to read.
 * 		If addr is less than CONFIG_SYS_FLASH_BASE, ignore it.
 * @len:	length to read
 * @return:
 * 	0:	success
 *  otherwise:	error
 ******************************************************************
 * WARNING
 ******************************************************************
 * For Parallel NOR Flash, you must specify sector-aligned address.
 */
int ra_flash_erase(ulong addr, ulong len)
{
	int ret = 0;
	unsigned int flash_offset = addr - CONFIG_SYS_FLASH_BASE;
#if defined(CONFIG_UBI_SUPPORT)
#else
	char off_buf[15], len_buf[15];
	char *sf_erase[] = { "sf", "erase", off_buf, len_buf };
#endif

	if (!len || addr < CONFIG_SYS_FLASH_BASE) {
		debug("%s(): invalid parameter. addr 0x%08lx len 0x%08lx\n",
			__func__, addr, len);
		return -1;
	}

#if defined(CONFIG_UBI_SUPPORT)
	if (flash_offset < CFG_UBI_DEV_OFFSET)
		ret = ranand_erase(flash_offset, len);
#else
	sprintf(off_buf, "%x", flash_offset);
	sprintf(len_buf, "+%lx", len);		/* round up to next sector boundary */
	ret = do_spi_flash(NULL, 0, ARRAY_SIZE(sf_erase), sf_erase);
	if (ret) {
		printf("%s: erase offset %x len %lx fail. (r = %d)\n",
			__func__, flash_offset, len, ret);
		return ret;
	}
#endif

	return ret;
}

#if defined(CONFIG_UBI_SUPPORT)
/* Check EEPROM set checksum in RAM.
 * @buf:	pointer to one copy of EEPROM set.
 * 		length of the EEPROM set must greater than or equal to LEB_SIZE
 * @return:
 *     >=0:	OK, write times of the EEPROM set
 * 	-1:	Invalid parameter.
 * 	-2:	Invalid magic number.
 * 	-3:	Invalid header checksum.
 * 	-4:	Invalid data checksum
 *  otherwise:	Not defined.
 */
static int check_eeprom_set_checksum(unsigned char *buf)
{
	unsigned long hcrc, checksum;
	eeprom_set_hdr_t header, *hdr = &header;

	if (!buf)
		return -1;

	memcpy(hdr, buf + EEPROM_SET_HEADER_OFFSET, sizeof(eeprom_set_hdr_t));
	if (hdr->ih_magic != htonl(FACTORY_IMAGE_MAGIC))
		return -2;

	hcrc = ntohl(hdr->ih_hcrc);
	hdr->ih_hcrc = 0;
	/* check header checksum */
	checksum = crc32(0, (const unsigned char*) hdr, sizeof(eeprom_set_hdr_t));
	if (hcrc != checksum) {
		debug("Header checksum mismatch. (%lx/%lx)\n", hcrc, checksum);
		return -3;
	}

	/* check image checksum */
	checksum = crc32(0, buf, EEPROM_SET_HEADER_OFFSET);
	if (ntohl(hdr->ih_dcrc) != checksum) {
		debug("Data checksum mismatch. (%x/%lx)\n", ntohl(hdr->ih_dcrc), checksum);
		return -4;
	}

	return ntohl(hdr->ih_write_ver);
}

/* Check all EEPROM set in Factory, Factory2 volume and select correct and latest one.
 * @return:
 * 	0:	Success. Latest and correct EEPROM set is copied to RAM.
 *     -1:	All EEPROM set is damaged. (always choose first EEPROM set)
 */
int choose_active_eeprom_set(void)
{
	int i, r, w, active_set = -1, ret = 0;
	unsigned int o, max_w = 0;
	unsigned char buf[LEB_SIZE];

	for (i = 0, o = 0; i < NR_EEPROM_SETS; ++i, o += LEB_SIZE) {
		r = ra_flash_read(buf, CFG_FACTORY_ADDR + o, LEB_SIZE);
		if (r) {
			printf("Read data fail at 0x%x\n", i * LEB_SIZE);
			continue;
		}
		w = check_eeprom_set_checksum(buf);

		if (w >= 0) {
			printf("EEPROM set %d: OK (version %d)\n", i, w);
			if (w > max_w || (active_set < 0 && !max_w)) {
				active_set = i;
				max_w = w;
			}
		} else {
			printf("EEPROM set %d: DAMAGED ", i);
			if (w == -2)
				printf("(Invalid magic number)\n");
			else if (w == -3)
				printf("(Invalid header checksum)\n");
			else if (w == -4)
				printf("(Invalid data checksum)\n");
			else
				printf("(w = %d)\n", w);
		}
	}

	/* All EEPROM set is damaged. choose first one. */
	all_sets_damaged = 0;
	if (active_set < 0) {
		ret = -1;
		active_set = 0;
		all_sets_damaged = 1;
	}

	factory_offset = active_set * LEB_SIZE;
	r = ra_flash_read(eeprom_set, CFG_FACTORY_ADDR + factory_offset, LEB_SIZE);
	printf("Select EEPROM set %d at offset 0x%lx.\n", active_set, factory_offset);

	return ret;
}

/* Reload EEPROM set if necessary. If active EEPROM set in Flash is damaged too, choose again.
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
static int __reload_eeprom_set(void)
{
	int r,w;

	w = check_eeprom_set_checksum(eeprom_set);
	if (w < 0) {
		printf("EEPROM set in RAM damaged! (w = %d, all_sets_damaged %d)\n",
			w, all_sets_damaged);
		if (!all_sets_damaged) {
			printf("Read from Flash offset 0x%lx!\n", factory_offset);
			r = ra_flash_read(eeprom_set, CFG_FACTORY_ADDR + factory_offset, LEB_SIZE);
			if (r) {
				printf("Read EEPROM set from Flash 0x%lx fail! (r = %d)\n", factory_offset, r);
				return -2;
			}
			w = check_eeprom_set_checksum(eeprom_set);
			if (w < 0) {
				printf("EEPROM set in RAM still damaged. Select new one!. (w = %d)\n", w);
					choose_active_eeprom_set();
			}
		}
	}
	return 0;
}

/* Update header checksum, data checksum, write times, etc.
 * @return:
 * 	0:	Success
 * 	-1:	Invalid parameter.
 */
static int update_eeprom_checksum(unsigned char *buf)
{
	unsigned long checksum;
	eeprom_set_hdr_t *hdr;

	if (!buf)
		return -1;

	hdr = (eeprom_set_hdr_t *) (buf + EEPROM_SET_HEADER_OFFSET);
	checksum = crc32(0, (const unsigned char *)buf, MAX_EEPROM_SET_LENGTH);

	/* reset write version to 0 if header magic number is incorrect or wrap */
	if (hdr->ih_magic != htonl(FACTORY_IMAGE_MAGIC) ||
	    ntohl(hdr->ih_write_ver) >= 0x7FFFFFFFU)
		hdr->ih_write_ver = htonl(0);

	/* Build new header */
	hdr->ih_magic = htonl(FACTORY_IMAGE_MAGIC);
	hdr->ih_hcrc = 0;
	hdr->ih_hdr_ver = htonl(1);
	hdr->ih_write_ver = htonl(ntohl(hdr->ih_write_ver) + 1);
	hdr->ih_dcrc = htonl(checksum);

	checksum = crc32(0, (const unsigned char *)hdr, sizeof(eeprom_set_hdr_t));
	hdr->ih_hcrc = htonl(checksum);

	debug("header/data checksum: 0x%08x/0x%08x\n", ntohl(hdr->ih_hcrc), ntohl(hdr->ih_dcrc));
	return 0;
}

/* Write EEPROM set in RAM to all factory volume.
 * @return:
 * 	0:	Success.
 *  otherwise:	fail.
 */
static int update_eeprom_sets(void)
{
	int r, ret = 0;
	unsigned int o;
	unsigned char buf[CFG_UBI_FACTORY_SIZE];

	for (o = 0; o < CFG_UBI_FACTORY_SIZE; o += LEB_SIZE)
		memcpy(buf + o, eeprom_set, LEB_SIZE);
	for (o = 0; o < (NR_EEPROM_SETS * LEB_SIZE); o += CFG_UBI_FACTORY_SIZE) {
		r = ra_flash_erase_write(buf, CFG_FACTORY_ADDR + o, CFG_UBI_FACTORY_SIZE, 0);
		if (r) {
			printf("Write EEPROM set to 0x%x fail. (r = %d)\n", CFG_FACTORY_ADDR + o, r);
			ret--;
			continue;
		}
	}
	all_sets_damaged = 0;

	return ret;
}
#endif	/* CONFIG_UBI_SUPPORT */


/**
 * Read date from FACTORY area. Only first MAX_EEPROM_SET_LENGTH can be read.
 * @buf:	buffer address
 * @off:	offset address respect to FACTORY partition
 * @len:	length to read
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
int ra_factory_read(uchar *buf, ulong off, ulong len)
{
	if (!buf || !len || off >= MAX_EEPROM_SET_LENGTH || len > MAX_EEPROM_SET_LENGTH || (off + len) > MAX_EEPROM_SET_LENGTH) {
		debug("%s(): invalid parameter. buf %p off %08lx len %08lx (%08x/%08x).\n",
			__func__, buf, off, len, CFG_FACTORY_ADDR, MAX_EEPROM_SET_LENGTH);
		return -1;
	}

#if defined(CONFIG_UBI_SUPPORT)
	if (!get_ubi_device())
		return -ENOENT;
	if (__reload_eeprom_set())
		return -1;

	memcpy(buf, eeprom_set + off, len);
	return 0;
#else
	off += CFG_FACTORY_ADDR;
	return ra_flash_read(buf, off, len);
#endif
}


/**
 * Write date to FACTORY area. Only first MAX_EEPROM_SET_LENGTH can be read.
 * For Parallel Flash, whole factory area would be unprotect, read,
 * modified data, write, and protect.
 * @buf:	buffer address
 * @off:	offset address respect to FACTORY partition
 * @len:	length to write
 * @prot:	Unprotect/protect sector. (Parallel NOR Flash only)
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
int ra_factory_erase_write(uchar *buf, ulong off, ulong len, int prot)
{
#if !defined(CONFIG_UBI_SUPPORT)
	int ret = 0;
	uchar factory_buf[CFG_FACTORY_SIZE];
#endif

	if (!buf || !len || off >= MAX_EEPROM_SET_LENGTH || len > MAX_EEPROM_SET_LENGTH || (off + len) > MAX_EEPROM_SET_LENGTH) {
		debug("%s(): invalid parameter. buf %p off %08lx len %08lx prot %d (%08x/%08x).\n",
			__func__, buf, off, len, prot, CFG_FACTORY_ADDR, MAX_EEPROM_SET_LENGTH);
		return -1;
	}

#if defined(CONFIG_UBI_SUPPORT)
	if (__reload_eeprom_set())
		return -1;

	memcpy(eeprom_set + off, buf, len);
	if (in_range(off, len, QC98XX_EEPROM1_OFFSET, QC98XX_EEPROM_SIZE_LARGEST)) {
		update_qca99xx_eeprom_csum((void*)eeprom_set + QC98XX_EEPROM1_OFFSET);
	}
	if (in_range(off, len, QC98XX_EEPROM2_OFFSET, QC98XX_EEPROM_SIZE_LARGEST)) {
		update_qca99xx_eeprom_csum((void*)eeprom_set + QC98XX_EEPROM2_OFFSET);
	}
	update_eeprom_checksum(eeprom_set);
	return update_eeprom_sets();
#else
	if (in_range(off, len, QC98XX_EEPROM1_OFFSET, QC98XX_EEPROM_SIZE_LARGEST) ||
	    in_range(off, len, QC98XX_EEPROM2_OFFSET, QC98XX_EEPROM_SIZE_LARGEST)) {
		ret = ra_factory_read(factory_buf, 0, sizeof(factory_buf));
		if (ret) {
			debug("%s: read offset 0 len %d from Factory fail. (ret %d)\n",
				__func__, sizeof(factory_buf), ret);
			return ret;
		}
		memcpy(factory_buf + off, buf, len);
		if (in_range(off, len, QC98XX_EEPROM1_OFFSET, QC98XX_EEPROM_SIZE_LARGEST)) {
			update_qca99xx_eeprom_csum((void*)factory_buf + QC98XX_EEPROM1_OFFSET);
		}
		if (in_range(off, len, QC98XX_EEPROM2_OFFSET, QC98XX_EEPROM_SIZE_LARGEST)) {
			update_qca99xx_eeprom_csum((void*)factory_buf + QC98XX_EEPROM2_OFFSET);
		}
		ret = ra_flash_erase_write(factory_buf, CFG_FACTORY_ADDR, sizeof(factory_buf), prot);
	} else {
		off += CFG_FACTORY_ADDR;
		ret = ra_flash_erase_write(buf, off, len, prot);
	}

	return ret;
#endif
}

#ifdef DEBUG_FACTORY_RW
int do_factory_read(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i, r, force = 0;
	size_t len;
	unsigned long addr;
	unsigned char buf[256], *p = &buf[0];

	if (argc < 3)
		return EINVAL;

	if (argc == 4)
		force = !!simple_strtoul(argv[3], NULL, 16);

	addr = simple_strtoul(argv[1], NULL, 16);
	len = simple_strtoul(argv[2], NULL, 16);
	if (len >= sizeof(buf)) {
		p = malloc(len + 1);
		if (!p)
			return ENOMEM;
	}

	if (force)
		choose_active_eeprom_set();

	r = ra_factory_read(p, addr, len);
	if (!r) {
		for (i = 0; i < len; ++i) {
			if (!(i % 16))
				printf("\n");
			printf("%02X ", *(p + i));
		}
		printf("\n");
	} else {
		printf("%s: buf %p len %x fail. (r = %d)\n", __func__, p, len, r);
	}

	if (p != &buf[0])
		free(p);

	return 0;
}

U_BOOT_CMD(
	factory_read,	4,	1,	do_factory_read,
	"factory_read	- read factory area\n",
	"factory_offset length [[0]|1]	- read factory area\n"
);

int do_factory_write(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int r;
	size_t len;
	unsigned long ram_addr, flash_offset;

	if (argc < 3)
		return EINVAL;

	ram_addr = simple_strtoul(argv[1], NULL, 16);
	flash_offset = simple_strtoul(argv[2], NULL, 16);
	len = simple_strtoul(argv[3], NULL, 16);
	r = ra_factory_erase_write((uchar*) ram_addr, flash_offset, len, 0);
	if (!r) {
		printf("%s: OK\n", __func__);
	} else {
		printf("%s: ram_addr %lx flash_offset %lx len %x fail. (r = %d)\n",
			__func__, ram_addr, flash_offset, len, r);
	}

	return 0;
}

U_BOOT_CMD(
	factory_write,	4,	1,	do_factory_write,
	"factory_write	- write factory area\n",
	"ram_addr factory_offset length - write factory area\n"
);

#endif	/* DEBUG_FACTORY_RW */

#ifdef DEBUG_FLASH_WRAPPER
int do_flash_erase(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int r;
	size_t len;
	unsigned long addr;

	if (argc < 3)
		return EINVAL;

	addr = simple_strtoul(argv[1], NULL, 16);
	if (addr < CONFIG_SYS_FLASH_BASE) {
		printf("flash_addr must greater than or equal to 0x%x\n", CONFIG_SYS_FLASH_BASE);
		return EINVAL;
	}
	len = simple_strtoul(argv[2], NULL, 16);
	r = ra_flash_erase(addr, len);
	if (!r) {
		printf("%s: OK\n", __func__);
	} else {
		printf("%s: addr %lx len %x fail. (r = %d)\n", __func__, addr, len, r);
	}

	return 0;
}

U_BOOT_CMD(
	flash_erase,	4,	1,	do_flash_erase,
	"flash_erase	- erase flash area\n",
	"flash_addr length - erase flash area\n"
);

int do_flash_read(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i, r;
	size_t len;
	unsigned long addr;
	unsigned char buf[256], *p = &buf[0];

	if (argc < 3)
		return EINVAL;

	addr = simple_strtoul(argv[1], NULL, 16);
	if (addr < CONFIG_SYS_FLASH_BASE) {
		printf("flash_addr must greater than or equal to 0x%x\n", CONFIG_SYS_FLASH_BASE);
		return EINVAL;
	}
	len = simple_strtoul(argv[2], NULL, 16);
	if (len >= sizeof(buf)) {
		p = malloc(len + 1);
		if (!p)
			return ENOMEM;
	}

	r = ra_flash_read(p, addr, len);
	if (!r) {
		for (i = 0; i < len; ++i) {
			if (!(i % 16))
				printf("\n");
			printf("%02X ", *(p + i));
		}
		printf("\n");
	} else {
		printf("%s: buf %p len %x fail. (r = %d)\n", __func__, p, len, r);
	}

	if (p != &buf[0])
		free(p);

	return 0;
}

U_BOOT_CMD(
	flash_read,	4,	1,	do_flash_read,
	"flash_read	- read flash area\n",
	"flash_offset length - read flash area\n"
);

int do_flash_erase_write(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int r;
	size_t len;
	unsigned long ram_addr, flash_addr;

	if (argc < 4)
		return EINVAL;

	ram_addr = simple_strtoul(argv[1], NULL, 16);
	flash_addr = simple_strtoul(argv[2], NULL, 16);
	len = simple_strtoul(argv[3], NULL, 16);
	if (flash_addr < CONFIG_SYS_FLASH_BASE) {
		printf("flash_addr must greater than or equal to 0x%x\n", CONFIG_SYS_FLASH_BASE);
		return EINVAL;
	}
	r = ra_flash_erase_write((uchar*)ram_addr, flash_addr, len, 0);
	if (!r) {
		printf("%s: OK\n", __func__);
	} else {
		printf("%s: ram_addr %lx flash_addr %lx len %x fail. (r = %d)\n", __func__, ram_addr, flash_addr, len, r);
	}

	return 0;
}

U_BOOT_CMD(
	flash_erase_write,	4,	1,	do_flash_erase_write,
	"flash_erase_write	- erase and write flash area\n",
	"ram_addr flash_addr length - erase and write flash area\n"
);

#endif	/* DEBUG_FLASH_WRAPPER */

#if defined(CONFIG_UBI_SUPPORT)
/* Return offset of last non-0xFF byte.
 * @return:
 *     -1:	All bytes between *p ~ *(p + len - 1) is 0xFF.
 *    >=0:	Last non 0xFF between *p ~ *(p + len - 1)
 */
static int last_non_ff(unsigned char *p, size_t len)
{
	int i;

	if (!p || !len)
		return 0;

	for (i = len - 1; i >= 0; --i) {
		if (*(p + i) != 0xFF)
			break;
	}

	return i;
}

#ifndef ROUNDUP
#define	ROUNDUP(x, y)		((((x)+((y)-1))/(y))*(y))
#endif

extern u32 crc32_le(u32 crc, unsigned char const *p, size_t len);
/* Change erase counter and recalculate checksum
 * @hdr:	poinster to EC header
 * @ec:		new erase counter
 * @return:
 * 	0:	success
 *     -1:	invalid parameter
 *     -2:	invalid UBI EC eader
 */
static int change_ech(struct ubi_ec_hdr *hdr, unsigned long ec)
{
	uint32_t crc;

	if (!hdr)
		return -1;

	/* Check the EC header */
	if (be32_to_cpu(hdr->magic) != UBI_EC_HDR_MAGIC) {
		printf("bad UBI magic %#08x, should be %#08x\n",
		       be32_to_cpu(hdr->magic), UBI_EC_HDR_MAGIC);
		return -2;
	}

	hdr->ec = cpu_to_be64(ec);
	crc = crc32_le(UBI_CRC32_INIT, (unsigned char const *)hdr, UBI_EC_HDR_SIZE_CRC);
	hdr->hdr_crc = cpu_to_be32(crc);

	return 0;
}

/* Erase blocks belongs to UBI_DEV
 * @return:
 * 	0:	success
 *     -1:	invalid parameter
 *     -2:	UBI is not detached.
 *  otherwise:	error
 */
static int erase_ubi_block(unsigned int offset, unsigned int end_offset, int mean_ec, const struct ubi_ec_hdr *base_ec_hdr)
{
	const struct mtd_info *mtd = &nand_info[nand_curr_device];
	unsigned int o = offset;
	int erase_len = end_offset - offset;
	int write_ec, w, retry;
	struct ubi_ec_hdr ec_flash, *ec_f = &ec_flash;
	unsigned long ec;
	u32 crc;
	const struct ubi_device *ubi = get_ubi_device();

	if (offset < CFG_UBI_DEV_OFFSET || offset >= mtd->size ||
	    end_offset <= CFG_UBI_DEV_OFFSET || end_offset > mtd->size||
	    offset >= end_offset || (erase_len % mtd->erasesize) || mean_ec < 0 ||
	    !base_ec_hdr || be32_to_cpu(base_ec_hdr->magic) != UBI_EC_HDR_MAGIC)
		return -1;
	if (ubi) {
		printf("%s: Detach UBI device prior to erase!\n", __func__);
		return -2;
	}

	for (; erase_len > 0; erase_len -= mtd->erasesize, o += mtd->erasesize) {
		if (ranand_block_isbad(o)) {
			printf("skip bad-block at 0x%x\n", o);
			continue;
		}

		/* preserved erase counter */
		write_ec = 0;
		ec = mean_ec + 1;
		if ((w = ranand_read((unsigned char*)ec_f, o, sizeof(*ec_f))) == sizeof(*ec_f)) {
			if (be32_to_cpu(ec_f->magic) == UBI_EC_HDR_MAGIC &&
			    (crc = crc32_le(UBI_CRC32_INIT, (unsigned char const *)ec_f, UBI_EC_HDR_SIZE_CRC)) == be32_to_cpu(ec_f->hdr_crc))
			{
				ec = be64_to_cpu(ec_f->ec) + 1;
			}
			memcpy(ec_f, base_ec_hdr, sizeof(*ec_f));
			change_ech(ec_f, ec);
			write_ec = 1;
		}

		retry = 0;
retry_erase_ubi:
		if (ranand_erase(o, mtd->erasesize)) {
			printf("erase block at 0x%x fail, leave it alone and ignore it\n", o);
			continue;
		}

		if (write_ec && (w = ranand_write((unsigned char *)ec_f, o, sizeof(*ec_f))) != sizeof(*ec_f)) {
			printf("write EC header back to empty block fail. (w = %d, retry %d)\n", w, retry);
			if (++retry <= 3)
				goto retry_erase_ubi;
		}
	}

	return 0;
}

/* Read first EEPROM set and judge whether it is worth to backup or not.
 * @factory:	pointer to a buffer that is used to return a full EEPROM set.
 * 		length of the buffer must greather than or equal to LEB_SIZE.
 * @return:
 * 	1:	read success and worth to backup the factory
 * 	0:	read success but don't worth to backup the factory
 *     <0:	error, don't backup the factory
 */
static int backup_factory(unsigned char *factory)
{
	int r;
	unsigned char *mac = factory + 4;
	const unsigned char default_oui_mac[3] = { 0x00, 0x0C, 0x43 };
	const unsigned char zero_mac[6] = { 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00 };
	const unsigned char default_init_mac[6] = { 0x00, 0x11, 0x11, 0x11, 0x11, 0x11 };

	if (!factory)
		return -1;

	if ((r = ra_flash_read(factory, CFG_FACTORY_ADDR, LEB_SIZE)) != 0) {
		printf("Read factory fail! (r = %d)\n", r);
		return -2;
	}

	if ((r = check_eeprom_set_checksum(factory)) < 0) {
		printf("Invalid EEPROM set! (r = %d)\n", r);
		return -3;
	}

	if (*mac == 0xFF) {
		printf("MAC[0] = 0xFF, drop factory\n");
		return 0;
	}

	if (!memcmp(mac, default_oui_mac, 3)) {
		printf("OUI = 00:0C:43, drop factory\n");
		return 0;
	}

	if (!memcmp(mac, zero_mac, 6)) {
		printf("MAC = 0, drop factory\n");
		return 0;
	}

	if (!memcmp(mac, default_init_mac, 6)) {
		printf("MAC = 00:11:11:11:11:11, drop factory\n");
		return 0;
	}

	return 1;
}

/* Program UBI image with or without OOB information to UBI_DEV.
 * @return:
 * 	0:	success
 * 	1:	image length is not multiple of block_size or block_size + oob_size both
 * 	2:	can't found EC header at first page of a block
 *     -1:	invalid parameter
 *     -2:	allocate buffer for strip oob information from input image fail
 *     -3:	write page fail.
 *     -4:	out of nand flash space to program UBI image
 *     -5:	attach UBI_DEV fail
 *     -6:	restore factory raw data to first EEPROM set fail.
 *     -7:	sync factory fail
 *  otherwise	error
 */
int __SolveUBI(unsigned char *ptr, unsigned int start_offset, unsigned int copysize)
{
	const struct mtd_info *mtd = &nand_info[nand_curr_device];
	const int pages_per_block = mtd->erasesize / mtd->writesize;
	const int block_size_incl_oob = mtd->erasesize + mtd->oobsize * pages_per_block;
	const struct ubi_device *ubi = get_ubi_device();
	size_t len, blk_len, empty_blk_len;
	unsigned int magic_addr;
	char *ubi_detach[] = { "ubi", "detach"};
	int r, block, pos, omit_oob = 0, datalen, retry;
	int in_page_len = mtd->writesize, in_blk_len = mtd->erasesize;
	int ubi_dev_len = mtd->size - CFG_UBI_DEV_OFFSET;
	unsigned int offset = start_offset;
	unsigned char *p, *q, *data, *blk_buf = NULL;
	int mean_ec = 0;
	struct ubi_ec_hdr ec_flash, *ec_f = &ec_flash;
	unsigned long ec;
	u32 crc;
	int restore_factory = 0;
	unsigned char factory_raw_data[LEB_SIZE];

	if (!ptr || !copysize || copysize < mtd->writesize || start_offset % mtd->erasesize)
		return -1;

	printf("Check UBI image length\n");
	if (!(copysize % (mtd->erasesize)) && !(copysize % block_size_incl_oob)) {
		debug("0x%x is least common multiple of erasesize and block_size_incl_oob\n", copysize);
		if (be32_to_cpu(*(uint32_t*)(ptr + mtd->erasesize)) == UBI_EC_HDR_MAGIC)
			omit_oob=0;
		else if (be32_to_cpu(*(uint32_t*)(ptr + block_size_incl_oob)) == UBI_EC_HDR_MAGIC)
			omit_oob=1;
		else {
			printf("Can't find UBI image of block1!\n");
			return 2;
		}
	} else if (!(copysize % (mtd->erasesize))) {
		omit_oob = 0;
	} else if (!(copysize % block_size_incl_oob)) {
		omit_oob = 1;
	} else {
		printf("%x bytes is not multiple of 0x%x or 0x%x, omit\n",
			copysize, mtd->erasesize, mtd->erasesize + mtd->oobsize * pages_per_block);
		return 1;
	}

	if (omit_oob) {
		in_page_len = mtd->writesize + mtd->oobsize;
		in_blk_len = block_size_incl_oob;
		printf("OOB information found, omit it\n");
	}

	/* check EC header of first page of every block */
	printf("Check all EC header of UBI image\n");
	for (len = copysize, magic_addr = (unsigned int)ptr, block = 0;
		len > 0;
		len -= in_blk_len, magic_addr += in_blk_len, block++)
	{
		if (be32_to_cpu(*(uint32_t*)magic_addr) == UBI_EC_HDR_MAGIC)
			continue;

		printf("can't found EC header at block 0x%x of image (offset 0x%x), stop!\n",
			(unsigned int) block, (unsigned int) block * mtd->erasesize);
		return 2;
	}

	if (omit_oob) {
		blk_buf = malloc(mtd->erasesize);
		if (!blk_buf) {
			printf("can't %d bytes allocate buffer to strip OOB information from input image!\n",
				mtd->erasesize);
			return -2;
		}
	}

	if (ubi) {
		mean_ec = ubi->mean_ec;

		/* Is it possible/worth to backup factory? */
		if (backup_factory(factory_raw_data) == 1) {
			printf("Backup factory\n");
			restore_factory = 1;
		}

		/* detach UBI_DEV */
		do_ubi(NULL, 0, ARRAY_SIZE(ubi_detach), ubi_detach);
	}
	ubi = NULL;

	/* now, we can't call any flash wrapper or functions that would trigger UBI action. */
	/* erase leading block */
	if (offset > CFG_UBI_DEV_OFFSET) {
		printf("erase leading blocks (offset 0x%x~%x length 0x%x, %d blocks)\n",
			CFG_UBI_DEV_OFFSET, offset, offset - CFG_UBI_DEV_OFFSET,
			(offset - CFG_UBI_DEV_OFFSET) / mtd->erasesize);
		erase_ubi_block(CFG_UBI_DEV_OFFSET, offset, mean_ec, (struct ubi_ec_hdr *)ptr);
	}

	/* program input image page by page */
	printf("program UBI image to 0x%x, length 0x%x%s\n", offset, copysize, (omit_oob)? ", omit OOB":"");
	for (len = copysize, data = ptr, block = 0; len > 0 && ubi_dev_len > 0; offset += mtd->erasesize)
	{
		p = data;
		if (ranand_block_isbad(offset)) {
			printf("skip bad-block at 0x%x\n", offset);
			continue;
		}

		/* preserved erase counter */
		ec = mean_ec + 1;
		if (!ranand_read((unsigned char*)ec_f, offset, sizeof(*ec_f))) {
			if (be32_to_cpu(ec_f->magic) == UBI_EC_HDR_MAGIC &&
			    (crc = crc32_le(UBI_CRC32_INIT, (unsigned char const *)ec_f, UBI_EC_HDR_SIZE_CRC)) == be32_to_cpu(ec_f->hdr_crc))
			{
				ec = be64_to_cpu(ec_f->ec) + 1;
			}
		}
		change_ech((struct ubi_ec_hdr *)p, ec);

		if (omit_oob) {
			/* copy the block to bounce buffer page by page to strip OOB information */
			for (blk_len = mtd->erasesize, p = data, q = blk_buf;
				blk_len > 0;
				blk_len -= mtd->writesize, p += in_page_len, q += mtd->writesize)
			{
				memcpy(q, p, mtd->writesize);
			}
			p = blk_buf;
		}

		/* find position of last non-0xFF in this block. don't write tailed empty page to flash */
		pos = last_non_ff(p, mtd->erasesize);
		q = p + ROUNDUP(pos + 1, mtd->writesize);

		retry = 0;
		datalen = q - p;
retry_solve_ubi:
		if (ranand_erase(offset, mtd->erasesize)) {
			printf("erase block at 0x%x fail, leave it alone and ignore it\n", offset);
			continue;
		}
		if ((r = ranand_write(p, offset, datalen)) != datalen) {
			printf("write to 0x%x, length 0x%x fail. (r = %d, retry %d)\n", offset, datalen, r, retry);
			if (++retry <= 3)
				goto retry_solve_ubi;

			return -3;
		}
		empty_blk_len = mtd->erasesize - datalen;

		if (empty_blk_len > 0) {
			printf("skip %d tailed empty page of block %d (0x%x bytes)\n",
				empty_blk_len / mtd->writesize, block, empty_blk_len);
		}

		len -= in_blk_len;
		data += in_blk_len;
		block++;
		ubi_dev_len -= mtd->erasesize;
	}

	if (len > 0 && ubi_dev_len <= 0) {
		printf("Out of space to program image! (len 0x%x)", len);
		return -4;
	}

	/* erase remain blocks */
	printf("erase remain blocks (offset 0x%x length 0x%x, %d blocks)\n",
		offset, ubi_dev_len, ubi_dev_len / mtd->erasesize);
	erase_ubi_block(offset, mtd->size, mean_ec, (struct ubi_ec_hdr *)ptr);

	/* don't need to restore factory, return success */
	if (!restore_factory) {
		printf("Success.\n");
		return 0;
	}

	printf("Restore factory\n");
	if ((r = ra_flash_init_layout()) != 0) {
		printf("Attach UBI_DEV fail! (r = %d)\n", r);
		return -5;
	}

	if ((r = ra_flash_erase_write(factory_raw_data, CFG_FACTORY_ADDR, LEB_SIZE, 0)) != 0) {
		printf("Restore factory to first EEPROM set fail! (r = %d)\n", r);
		return -6;
	}

	/* Reload factory to RAM */
	printf("Reload factory\n");
	choose_active_eeprom_set();

	printf("Success.\n");
	return 0;
}
#endif	/* CONFIG_UBI_SUPPORT */
