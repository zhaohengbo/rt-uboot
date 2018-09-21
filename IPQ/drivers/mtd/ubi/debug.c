/*
 * Copyright (c) International Business Machines Corp., 2006
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Author: Artem Bityutskiy (Битюцкий Артём)
 */

/*
 * Here we keep all the UBI debugging stuff which should normally be disabled
 * and compiled-out, but it is extremely helpful when hunting bugs or doing big
 * changes.
 */
#include <ubi_uboot.h>

#ifdef CONFIG_MTD_UBI_DEBUG

#include "ubi.h"

/**
 * ubi_dbg_dump_ec_hdr - dump an erase counter header.
 * @ec_hdr: the erase counter header to dump
 */
void ubi_dbg_dump_ec_hdr(const struct ubi_ec_hdr *ec_hdr)
{
	dbg_msg("erase counter header dump:");
	dbg_msg("magic          %#08x", be32_to_cpu(ec_hdr->magic));
	dbg_msg("version        %d",    (int)ec_hdr->version);
	dbg_msg("ec             %lu",  (unsigned long)be64_to_cpu(ec_hdr->ec));
	dbg_msg("vid_hdr_offset %d",    be32_to_cpu(ec_hdr->vid_hdr_offset));
	dbg_msg("data_offset    %d",    be32_to_cpu(ec_hdr->data_offset));
	dbg_msg("image_seq      %d\n", be32_to_cpu(ec_hdr->image_seq));
	dbg_msg("hdr_crc        %#08x", be32_to_cpu(ec_hdr->hdr_crc));
	dbg_msg("erase counter header hexdump:");
	print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_OFFSET, 32, 1,
		       ec_hdr, UBI_EC_HDR_SIZE, 1);
}

/**
 * ubi_dbg_dump_vid_hdr - dump a volume identifier header.
 * @vid_hdr: the volume identifier header to dump
 */
void ubi_dbg_dump_vid_hdr(const struct ubi_vid_hdr *vid_hdr)
{
	dbg_msg("volume identifier header dump:");
	dbg_msg("magic     %08x", be32_to_cpu(vid_hdr->magic));
	dbg_msg("version   %d",   (int)vid_hdr->version);
	dbg_msg("vol_type  %d",   (int)vid_hdr->vol_type);
	dbg_msg("copy_flag %d",   (int)vid_hdr->copy_flag);
	dbg_msg("compat    %d",   (int)vid_hdr->compat);
	dbg_msg("vol_id    %d",   be32_to_cpu(vid_hdr->vol_id));
	dbg_msg("lnum      %d",   be32_to_cpu(vid_hdr->lnum));
	dbg_msg("data_size %d",   be32_to_cpu(vid_hdr->data_size));
	dbg_msg("used_ebs  %d",   be32_to_cpu(vid_hdr->used_ebs));
	dbg_msg("data_pad  %d",   be32_to_cpu(vid_hdr->data_pad));
	dbg_msg("sqnum     %lu",
		(unsigned long)be64_to_cpu(vid_hdr->sqnum));
	dbg_msg("hdr_crc   %08x", be32_to_cpu(vid_hdr->hdr_crc));
	dbg_msg("volume identifier header hexdump:");
}

/**
 * ubi_dbg_dump_vol_info- dump volume information.
 * @vol: UBI volume description object
 */
void ubi_dbg_dump_vol_info(const struct ubi_volume *vol)
{
	dbg_msg("volume information dump:");
	dbg_msg("vol_id          %d", vol->vol_id);
	dbg_msg("reserved_pebs   %d", vol->reserved_pebs);
	dbg_msg("alignment       %d", vol->alignment);
	dbg_msg("data_pad        %d", vol->data_pad);
	dbg_msg("vol_type        %d", vol->vol_type);
	dbg_msg("name_len        %d", vol->name_len);
	dbg_msg("usable_leb_size %d", vol->usable_leb_size);
	dbg_msg("used_ebs        %d", vol->used_ebs);
	dbg_msg("used_bytes      %ld", (long)vol->used_bytes);
	dbg_msg("last_eb_bytes   %d", vol->last_eb_bytes);
	dbg_msg("corrupted       %d", vol->corrupted);
	dbg_msg("upd_marker      %d", vol->upd_marker);

	if (vol->name_len <= UBI_VOL_NAME_MAX &&
	    strnlen(vol->name, vol->name_len + 1) == vol->name_len) {
		dbg_msg("name            %s", vol->name);
	} else {
		dbg_msg("the 1st 5 characters of the name: %c%c%c%c%c",
			vol->name[0], vol->name[1], vol->name[2],
			vol->name[3], vol->name[4]);
	}
}

/**
 * ubi_dbg_dump_vtbl_record - dump a &struct ubi_vtbl_record object.
 * @r: the object to dump
 * @idx: volume table index
 */
void ubi_dbg_dump_vtbl_record(const struct ubi_vtbl_record *r, int idx)
{
	int name_len = be16_to_cpu(r->name_len);

	printk(KERN_DEBUG "Volume table record %d dump:\n", idx);
	printk(KERN_DEBUG "\treserved_pebs   %d\n",
	       be32_to_cpu(r->reserved_pebs));
	printk(KERN_DEBUG "\talignment       %d\n", be32_to_cpu(r->alignment));
	printk(KERN_DEBUG "\tdata_pad        %d\n", be32_to_cpu(r->data_pad));
	printk(KERN_DEBUG "\tvol_type        %d\n", (int)r->vol_type);
	printk(KERN_DEBUG "\tupd_marker      %d\n", (int)r->upd_marker);
	printk(KERN_DEBUG "\tname_len        %d\n", name_len);

	if (r->name[0] == '\0') {
		printk(KERN_DEBUG "\tname            NULL\n");
		return;
	}

	if (name_len <= UBI_VOL_NAME_MAX &&
	    strnlen(&r->name[0], name_len + 1) == name_len) {
		printk(KERN_DEBUG "\tname            %s\n", &r->name[0]);
	} else {
		printk(KERN_DEBUG "\t1st 5 characters of name: %c%c%c%c%c\n",
			r->name[0], r->name[1], r->name[2], r->name[3],
			r->name[4]);
	}
	printk(KERN_DEBUG "\tcrc             %#08x\n", be32_to_cpu(r->crc));
}

/**
 * ubi_dbg_dump_sv - dump a &struct ubi_scan_volume object.
 * @sv: the object to dump
 */
void ubi_dbg_dump_sv(const struct ubi_scan_volume *sv)
{
	printk(KERN_DEBUG "Volume scanning information dump:\n");
	printk(KERN_DEBUG "\tvol_id         %d\n", sv->vol_id);
	printk(KERN_DEBUG "\thighest_lnum   %d\n", sv->highest_lnum);
	printk(KERN_DEBUG "\tleb_count      %d\n", sv->leb_count);
	printk(KERN_DEBUG "\tcompat         %d\n", sv->compat);
	printk(KERN_DEBUG "\tvol_type       %d\n", sv->vol_type);
	printk(KERN_DEBUG "\tused_ebs       %d\n", sv->used_ebs);
	printk(KERN_DEBUG "\tlast_data_size %d\n", sv->last_data_size);
	printk(KERN_DEBUG "\tdata_pad       %d\n", sv->data_pad);
}

/**
 * ubi_dbg_dump_seb - dump a &struct ubi_scan_leb object.
 * @seb: the object to dump
 * @type: object type: 0 - not corrupted, 1 - corrupted
 */
void ubi_dbg_dump_seb(const struct ubi_scan_leb *seb, int type)
{
	dbg_msg("eraseblock scanning information dump:");
	dbg_msg("ec       %d", seb->ec);
	dbg_msg("pnum     %d", seb->pnum);
	if (type == 0) {
		dbg_msg("lnum     %d", seb->lnum);
		dbg_msg("scrub    %d", seb->scrub);
		dbg_msg("sqnum    %lu", (unsigned long)seb->sqnum);
	}
}

/**
 * ubi_dbg_dump_mkvol_req - dump a &struct ubi_mkvol_req object.
 * @req: the object to dump
 */
void ubi_dbg_dump_mkvol_req(const struct ubi_mkvol_req *req)
{
	char nm[17];

	dbg_msg("volume creation request dump:");
	dbg_msg("vol_id    %d",   req->vol_id);
	dbg_msg("alignment %d",   req->alignment);
	dbg_msg("bytes     %ld", (long)req->bytes);
	dbg_msg("vol_type  %d",   req->vol_type);
	dbg_msg("name_len  %d",   req->name_len);

	memcpy(nm, req->name, 16);
	nm[16] = 0;
	dbg_msg("the 1st 16 characters of the name: %s", nm);
}

/**
 * ubi_dbg_dump_flash - dump a region of flash.
 * @ubi: UBI device description object
 * @pnum: the physical eraseblock number to dump
 * @offset: the starting offset within the physical eraseblock to dump
 * @len: the length of the region to dump
 */
void ubi_dbg_dump_flash(struct ubi_device *ubi, int pnum, int offset, int len)
{
	int err;
	size_t read;
	void *buf;
	loff_t addr = (loff_t)pnum * ubi->peb_size + offset;

	buf = vmalloc(len);
	if (!buf)
		return;
	err = ubi->mtd->read(ubi->mtd, addr, len, &read, buf);
	if (err && err != -EUCLEAN) {
		ubi_err("error %d while reading %d bytes from PEB %d:%d, "
			"read %zd bytes", err, len, pnum, offset, read);
		goto out;
	}

	dbg_msg("dumping %d bytes of data from PEB %d, offset %d",
		len, pnum, offset);
	print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_OFFSET, 32, 1, buf, len, 1);
out:
	vfree(buf);
	return;
}

#endif /* CONFIG_MTD_UBI_DEBUG_MSG */
