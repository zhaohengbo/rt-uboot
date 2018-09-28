#ifndef _SYNC_WRITE_H
#define _SYNC_WRITE_H

#if 0
#define reg_dummy_read(b)   \
        do {    \
            int c = *b; \
            int c ++; \
        } while (0)


#define dsb()   \
        do {    \
            __asm__ __volatile__ ("dsb" : : : "memory"); \
        } while (0)

#define mt65xx_reg_sync_writel(v, a) \
        do {    \
            *(volatile unsigned int *)(a) = (v);    \
            dsb(); \
        } while (0)

#define mt65xx_reg_sync_writew(v, a) \
        do {    \
            *(volatile unsigned short *)(a) = (v);    \
            dsb(); \
        } while (0)

#define mt65xx_reg_sync_writeb(v, a) \
        do {    \
            *(volatile unsigned char *)(a) = (v);    \
            dsb(); \
        } while (0)
#endif

extern void mt_reg_sync_writel();
extern void mt_reg_sync_writew();
extern void mt_reg_sync_writeb();

#endif
