#
# Copyright 2022 iWave Systems Technologies Pvt Ltd.
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx8mm_iwg34m.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
ifdef CONFIG_SDRAM_SIZE_1GB
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing.o
else
ifdef CONFIG_SDRAM_SIZE_2GB
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_2gb.o
else
ifdef CONFIG_SDRAM_SIZE_4GB
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_4gb.o
endif
endif
endif
endif
