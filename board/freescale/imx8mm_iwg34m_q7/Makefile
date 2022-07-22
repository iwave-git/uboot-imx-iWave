#
# Copyright 2022 iWave Systems Technologies Pvt Ltd.
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx8mm_iwg34m_q7.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
ifdef CONFIG_SDRAM_SIZE_1GB
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing.o
else
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing_2gb.o
endif
endif
