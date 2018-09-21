// vim:cin
/* 
 * Copyright 2013, ASUSTeK Inc.
 * All Rights Reserved.
 */

#ifndef _FLASH_WRAPPER_H_
#define _FLASH_WRAPPER_H_

#if defined(ASUS_PRODUCT)
//extern void ranand_set_sbb_max_addr(loff_t addr);
static inline void ranand_set_sbb_max_addr(unsigned long addr) { }
static inline unsigned long ra_flash_offset(unsigned long addr)
{
	if (addr >= CONFIG_SYS_FLASH_BASE)
		return addr - CONFIG_SYS_FLASH_BASE;
	else
		return addr;
}
extern int ra_flash_init_layout(void);

#if defined(CONFIG_UBI_SUPPORT)
extern int choose_active_eeprom_set(void);
extern int __SolveUBI(unsigned char *ptr, unsigned int offset, unsigned int copysize);
extern int ranand_check_and_fix_bootloader(void);
#else
static inline int choose_active_eeprom_set(void) { return 0; }
static inline int ranand_check_and_fix_bootloader(void) { return 0; }
#endif

extern int get_ubi_volume_idseq_by_addr(const char *name);

/* Below function use absolute address, include CFG_FLASH_BASE. */
extern int ra_flash_read(uchar * buf, ulong addr, ulong len);
extern int ra_flash_erase_write(uchar * buf, ulong addr, ulong len, int prot);
extern int ra_flash_erase(ulong addr, ulong len);

/* Below function use relative address, respect to start address of factory area. */
extern int ra_factory_read(uchar *buf, ulong off, ulong len);
extern int ra_factory_erase_write(uchar *buf, ulong off, ulong len, int prot);
#else	/* !ASUS_PRODUCT */
static inline void ranand_set_sbb_max_addr(unsigned long addr) { }
static inline int ra_flash_init_layout(void) { return 0; }
static inline unsigned long ra_flash_offset(unsigned long addr) { return addr; }
static inline int choose_active_eeprom_set(void) { return 0; }
static inline int __SolveUBI(unsigned char *ptr, unsigned int offset, unsigned int copysize) { return 0; }
static inline int get_ubi_volume_idseq_by_addr(const char *name) { return 0; }
static inline int ra_flash_read(uchar * buf, ulong addr, ulong len) { return 0; }
static inline int ra_flash_erase_write(uchar * buf, ulong addr, ulong len, int prot) { return 0; }
static inline int ra_flash_erase(ulong addr, ulong len) { return 0; }
static inline int ra_factory_read(uchar *buf, ulong off, ulong len) { return 0; }
static inline int ra_factory_erase_write(uchar *buf, ulong off, ulong len, int prot) { return 0; }
#endif	/* ASUS_PRODUCT */
#endif	/* _FLASH_WRAPPER_H_ */
