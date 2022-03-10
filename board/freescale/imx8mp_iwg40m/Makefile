#
# Copyright (C) 2022 iWave System Technologies Pvt Ltd. 
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx8mp_iwg40m.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
ifdef CONFIG_SDRAM_SIZE_4GB
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing.o
endif
ifdef CONFIG_SDRAM_SIZE_2GB
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_2gb.o
endif
endif
