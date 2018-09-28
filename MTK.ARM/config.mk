#
# (C) Copyright 2000-2013
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# SPDX-License-Identifier:	GPL-2.0+
#
#########################################################################

# clean the slate ...
PLATFORM_RELFLAGS =
PLATFORM_CPPFLAGS =
PLATFORM_LDFLAGS =

#########################################################################

# Some architecture config.mk files need to know what CPUDIR is set to,
# so calculate CPUDIR before including ARCH/SOC/CPU config.mk files.
# Check if arch/$ARCH/cpu/$CPU exists, otherwise assume arch/$ARCH/cpu contains
# CPU-specific code.
CPUDIR=arch/$(ARCH)/cpu/$(CPU)
ifneq ($(SRCTREE)/$(CPUDIR),$(wildcard $(SRCTREE)/$(CPUDIR)))
CPUDIR=arch/$(ARCH)/cpu
endif

sinclude $(TOPDIR)/arch/$(ARCH)/config.mk	# include architecture dependend rules
sinclude $(TOPDIR)/$(CPUDIR)/config.mk		# include  CPU	specific rules

ifdef	SOC
sinclude $(TOPDIR)/$(CPUDIR)/$(SOC)/config.mk	# include  SoC	specific rules
endif
ifneq ($(BOARD),)
ifdef	VENDOR
BOARDDIR = $(VENDOR)/$(BOARD)
else
BOARDDIR = $(BOARD)
endif
endif
ifdef	BOARD
sinclude $(TOPDIR)/board/$(BOARDDIR)/config.mk	# include board specific rules
endif

#########################################################################

RELFLAGS= $(PLATFORM_RELFLAGS)

OBJCOPYFLAGS += --gap-fill=0xff

CPPFLAGS = $(RELFLAGS)
CPPFLAGS += -pipe $(PLATFORM_CPPFLAGS)

LDFLAGS += $(PLATFORM_LDFLAGS)
LDFLAGS_FINAL += -Bstatic
#MT7623
ifeq ($(PDMA_NEW),y)
CPPFLAGS += -DPDMA_NEW
endif

ifeq ($(RX_SCATTER_GATTER_DMA),y)
CPPFLAGS += -DRX_SCATTER_GATTER_DMA
endif

ifeq ($(CONFIG_USE_GE2),y)
CPPFLAGS += -DCONFIG_USE_GE2
endif

ifeq ($(CONFIG_USE_GE1),y)
CPPFLAGS += -DCONFIG_USE_GE1
endif

ifeq ($(GE_RGMII_INTERNAL_P0_AN),y)
CPPFLAGS += -DGE_RGMII_INTERNAL_P0_AN
endif

ifeq ($(GE_RGMII_INTERNAL_P4_AN),y)
CPPFLAGS += -DGE_RGMII_INTERNAL_P4_AN
endif

ifeq ($(GE_MII_FORCE_100),y)
CPPFLAGS += -DGE_MII_FORCE_100
endif

ifeq ($(GE_RVMII_FORCE_100),y)
CPPFLAGS += -DGE_RVMII_FORCE_100
endif

ifeq ($(GE_MII_AN),y)
CPPFLAGS += -DGE_MII_AN
endif

ifeq ($(GE_RGMII_FORCE_1000),y)
CPPFLAGS += -DGE_RGMII_FORCE_1000
endif

ifeq ($(GE_RGMII_AN),y)
CPPFLAGS += -DGE_RGMII_AN
endif

ifeq ($(CONFIG_GE1_TRGMII_FORCE_2600),y)
CPPFLAGS += -DCONFIG_GE1_TRGMII_FORCE_2600
endif

ifeq ($(MAC_TO_GIGAPHY_MODE),y)
CPPFLAGS += -DMAC_TO_GIGAPHY_MODE
endif

ifdef MAC_TO_GIGAPHY_MODE_ADDR
CPPFLAGS += -DMAC_TO_GIGAPHY_MODE_ADDR=$(MAC_TO_GIGAPHY_MODE_ADDR)
endif 

ifeq ($(FW_UPGRADE_BY_USB),y)
CPPFLAGS += -DFW_UPGRADE_BY_USB
endif
ifeq ($(FW_UPGRADE_BY_SDXC),y)
CPPFLAGS += -DFW_UPGRADE_BY_SDXC
endif

ifeq ($(FW_UPGRADE_BY_WEBUI),y)
CPPFLAGS += -DFW_UPGRADE_BY_WEBUI
endif

ifeq ($(DUAL_IMAGE_SUPPORT),y)
CPPFLAGS += -DDUAL_IMAGE_SUPPORT
endif

