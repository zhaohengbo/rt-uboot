#include $(srctree)/drivers/misc/mediatek/Makefile.custom
 ifeq ($(CONFIG_MTK_SPI_NAND_SUPPORT),y)
obj-$(CONFIG_MTK_MTD_NAND) += mtk_snand_k.o
obj-$(CONFIG_MTK_MTD_NAND) += bmt.o
obj-$(CONFIG_MTK_MTD_NAND) += partition_mt.o
else
obj-$(CONFIG_MTK_MTD_NAND) += mtk_nand.o
obj-$(CONFIG_MTK_MTD_NAND) += bmt.o
obj-$(CONFIG_MTK_MTD_NAND) += partition_mt.o
endif
