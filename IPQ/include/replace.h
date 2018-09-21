#ifndef REPLACE_H
#define REPLACE_H

#include <common.h>

extern int replace(unsigned long addr, uchar *value, int len);
extern int chkMAC(void);
extern int chkVer(void);
#endif
