/*
 * Copyright (C) 2022-23 iWave System Technologies Pvt Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <env.h>
#include <init.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/global_data.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <asm/arch/imx8mm_pins.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <i2c.h>
#include <asm/io.h>
#include <asm/setup.h>
#include "../common/tcpc.h"
#include <usb.h>
#include <imx_sip.h>
#include <linux/arm-smccc.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <linux/delay.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

static iomux_v3_cfg_t const uart_pads[] = {
	IMX8MM_PAD_UART4_RXD_UART4_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART4_TXD_UART4_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MM_PAD_GPIO1_IO02_WDOG1_WDOG_B  | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

int board_early_init_f(void)
{
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

	init_uart_clk(3);

	return 0;
}

#if IS_ENABLED(CONFIG_FEC_MXC)
static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(&gpr->gpr[1], 0x2000, 0);

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);

#ifndef CONFIG_DM_ETH
	/* enable rgmii rxc skew and phy mode select to RGMII copper */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x00);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x82ee);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);
#endif

	return 0;
}
#endif

int board_usb_init(int index, enum usb_init_type init)
{
	int ret = 0;

	debug("board_usb_init %d, type %d\n", index, init);

	imx8m_usb_power(index, true);

	return ret;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;

	debug("board_usb_cleanup %d, type %d\n", index, init);

	imx8m_usb_power(index, false);
	return ret;
}

#define LT8912_RESET_GPIO IMX_GPIO_NR(4,23)

static iomux_v3_cfg_t lt8912_reset_pads[] = {
	IMX8MM_PAD_SAI2_RXD0_GPIO4_IO23 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

void lt8912_reset(void)
{
	imx_iomux_v3_setup_multiple_pads(lt8912_reset_pads, ARRAY_SIZE(lt8912_reset_pads));
	gpio_request(LT8912_RESET_GPIO, "lt8912_reset");

	mdelay(30);
	gpio_direction_output(LT8912_RESET_GPIO, 0);
	mdelay(120);
	gpio_direction_output(LT8912_RESET_GPIO, 1);
	gpio_free(LT8912_RESET_GPIO);
}

#define DISPMIX				9
#define MIPI				10

int board_init(void)
{
	struct arm_smccc_res res;

	if (IS_ENABLED(CONFIG_FEC_MXC))
		setup_fec();

	lt8912_reset();

	arm_smccc_smc(IMX_SIP_GPC, IMX_SIP_GPC_PM_DOMAIN,
		      DISPMIX, true, 0, 0, 0, 0, &res);
	arm_smccc_smc(IMX_SIP_GPC, IMX_SIP_GPC_PM_DOMAIN,
		      MIPI, true, 0, 0, 0, 0, &res);

	return 0;
}

int board_mmc_get_env_dev(int devno)
{
	if(devno == 0)
		return devno + 2;

	else if (devno == 2)
		return devno - 2;

	else
		return devno;
}

int mmc_map_to_kernel_blk(int devno)
{
	return devno;
}

#define GPIO_PAD_CFG_CTRL PAD_CTL_DSE1

#define BCONFIG_0 IMX_GPIO_NR(1, 9)
#define BCONFIG_1 IMX_GPIO_NR(1, 11)
#define BCONFIG_2 IMX_GPIO_NR(1, 10)
#define BCONFIG_3 IMX_GPIO_NR(1, 0)
#define BCONFIG_4 IMX_GPIO_NR(3, 1)
#define BCONFIG_5 IMX_GPIO_NR(2, 19)
#define BCONFIG_6 IMX_GPIO_NR(2, 20)

int const board_config_pads[] = {
	BCONFIG_0,
	BCONFIG_1,
	BCONFIG_2,
	BCONFIG_3,
	BCONFIG_4,
	BCONFIG_5,
	BCONFIG_6,
};

static iomux_v3_cfg_t const board_cfg[] = {
	IMX8MM_PAD_SD2_WP_GPIO2_IO20 | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
	IMX8MM_PAD_SD2_RESET_B_GPIO2_IO19 | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL), 
	IMX8MM_PAD_NAND_CE0_B_GPIO3_IO1 | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
	IMX8MM_PAD_GPIO1_IO00_GPIO1_IO0	| MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
	IMX8MM_PAD_GPIO1_IO10_GPIO1_IO10 | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
	IMX8MM_PAD_GPIO1_IO11_GPIO1_IO11 | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
	IMX8MM_PAD_GPIO1_IO09_GPIO1_IO9	| MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
};

void get_board_info(int *pcb_rev, int *bom_rev)
{
	int i, revision =0;

	imx_iomux_v3_setup_multiple_pads(board_cfg, ARRAY_SIZE(board_cfg));

	for (i=0;i<ARRAY_SIZE(board_config_pads);i++)
	{
		gpio_request(board_config_pads[i], "SOM-Revision-GPIO");
		gpio_direction_input(board_config_pads[i]);
		revision |= gpio_get_value(board_config_pads[i]) << i;
		gpio_free(board_config_pads[i]);
	}

	*pcb_rev = ((revision) & 0x07) +1;
	*bom_rev = (((revision) & 0x78)>>3);
}

/* IWG34S: Print Board Information */
static void printboard_info(void)
{
	int bom_rev = 0, pcb_rev = 0;
	struct tag_serialnr serialnr;

	/*IWG34S : Adding CPU Unique ID read support*/
	get_board_serial(&serialnr);
	get_board_info(&pcb_rev, &bom_rev);

	printf ("Board Info:\n");
	printf ("\tBSP Version            : %s\n", BSP_VERSION);
	printf ("\tSOM Version            : iW-PRGII-AP-01-R%x.%x\n",pcb_rev,bom_rev);
	printf ("\tCPU Unique ID          : 0x%08X%08X\n",serialnr.high,serialnr.low);
	printf ("\n");
}

#ifdef CONFIG_OF_BOARD_SETUP
/* IWG34S: Display selection Implementation */
int ft_board_setup (void *blob, struct bd_info *bd)
{
	int pcb_rev, bom_rev, ret;

	if (!strcmp("lvds", env_get("disp"))){
		ret = fdt_delprop(blob, fdt_path_offset(blob, "/soc@0/bus@30800000/i2c@30a30000/lt8912@48"), "ddc-i2c-bus");
		if(ret < 0)
			return ret;
		ret = fdt_setprop_string(blob, fdt_path_offset(blob, "/sound-hdmi"), "status", "disabled");
		if(ret < 0)
			return ret;
	}

	get_board_info(&pcb_rev, &bom_rev);
	ret = fdt_setprop_u32(blob,fdt_path_offset(blob, "/iwg_common"), "pcb_rev", pcb_rev);
	if(ret < 0)
		return ret;
	ret = fdt_setprop_u32(blob,fdt_path_offset(blob, "/iwg_common"), "bom_rev", bom_rev);
	if(ret < 0)
		return ret;

#ifdef CONFIG_SDRAM_SIZE_1GB
	int i;
	uint32_t reg[4], size[2], gpu[16];
	reg[0] = cpu_to_fdt32(0);
	reg[1] = cpu_to_fdt32(0x40000000);
	reg[2] = cpu_to_fdt32(0);
	reg[3] = cpu_to_fdt32(0x40000000);
	ret= fdt_setprop(blob,fdt_path_offset(blob, "/memory@40000000"), "reg", reg, sizeof(reg));
	if(ret < 0)
		printf("Kernel Memory update failed\n");

	size[0] = cpu_to_fdt32(0);
	size[1] = cpu_to_fdt32(0x14000000);
	ret= fdt_setprop(blob,fdt_path_offset(blob, "/reserved-memory/linux,cma"), "size", size, sizeof(size));
	if(ret < 0)
		printf("Kernel FDT CMA update failed\n");

	reg[0] = cpu_to_fdt32(0);
	reg[1] = cpu_to_fdt32(0x60000000);
	reg[2] = cpu_to_fdt32(0);
	reg[3] = cpu_to_fdt32(0x40000000);
	ret= fdt_setprop(blob,fdt_path_offset(blob, "/reserved-memory/linux,cma"), "alloc-ranges", reg, sizeof(reg));
	if(ret < 0)
		printf("Kernel CMA update failed\n");

	for(i=0; i<16; i+=2)
		gpu[i] = cpu_to_fdt32(0);

	gpu[1] = cpu_to_fdt32(0x38000000); gpu[3] = cpu_to_fdt32(0x8000);
	gpu[5] = cpu_to_fdt32(0x38008000); gpu[7] = cpu_to_fdt32(0x8000);
	gpu[9] = cpu_to_fdt32(0x40000000); gpu[11] = cpu_to_fdt32(0x40000000);
	gpu[13] = cpu_to_fdt32(0); gpu[15] = cpu_to_fdt32(0x8000000);

	ret= fdt_setprop(blob,fdt_path_offset(blob, "/gpu@38000000"), "reg", gpu, sizeof(gpu));
	if(ret < 0)
		printf("GPU memory update failed\n");
#endif

/* Support for 4GB LPDDR4 Board */
#ifdef CONFIG_SDRAM_SIZE_4GB
	uint32_t reg[8];
	reg[0] = cpu_to_fdt32(0); reg[1] = cpu_to_fdt32(0x40000000);
	reg[2] = cpu_to_fdt32(0); reg[3] = cpu_to_fdt32(0xC0000000);
	reg[4] = cpu_to_fdt32(0x1); reg[5] = cpu_to_fdt32(0);
	reg[6] = cpu_to_fdt32(0); reg[7] = cpu_to_fdt32(0x40000000);
	ret= fdt_setprop(blob,fdt_path_offset(blob, "/memory@40000000"), "reg", reg, sizeof(reg));
	if(ret < 0)
		printf("Kernel Memory update failed\n");

#endif

/* Support for 8GB LPDDR4 Board */
#ifdef CONFIG_SDRAM_SIZE_8GB
	uint32_t reg[8];
	reg[0] = cpu_to_fdt32(0); reg[1] = cpu_to_fdt32(0x40000000);
	reg[2] = cpu_to_fdt32(0); reg[3] = cpu_to_fdt32(0xC0000000);
	reg[4] = cpu_to_fdt32(0x1); reg[5] = cpu_to_fdt32(0);
	reg[6] = cpu_to_fdt32(0x1); reg[7] = cpu_to_fdt32(0x40000000);
	ret= fdt_setprop(blob,fdt_path_offset(blob, "/memory@40000000"), "reg", reg, sizeof(reg));
	if(ret < 0)
		printf("Kernel Memory update failed\n");

#endif
	return ret;
}
#endif

int board_late_init(void)
{
	printboard_info();
#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "iW-RainboW-G34S-i.MX8M Mini Pico ITX SBC");
	env_set("board_rev", "iW-PRGII-AP-01-R1.X");
#endif
	return 0;
}

#ifdef CONFIG_ANDROID_SUPPORT
bool is_power_key_pressed(void) {
	return (bool)(!!(readl(SNVS_HPSR) & (0x1 << 6)));
}
#endif

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
	return 0; /* TODO */
}
#endif /* CONFIG_ANDROID_RECOVERY */
#endif /* CONFIG_FSL_FASTBOOT */
