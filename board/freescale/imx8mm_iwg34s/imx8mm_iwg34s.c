// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 iWave Systems Technologies Pvt. Ltd.
 */

#include <common.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <asm/arch/imx8mm_pins.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <i2c.h>
#include <asm/io.h>
#include <usb.h>
#include <asm/setup.h>
#include <fdt_support.h>
#include <linux/libfdt.h>

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
#define FEC_RST_PAD IMX_GPIO_NR(4,28)
static iomux_v3_cfg_t const fec1_rst_pads[] = {
	IMX8MM_PAD_SAI3_RXFS_GPIO4_IO28 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_iomux_fec(void)
{
       imx_iomux_v3_setup_multiple_pads(fec1_rst_pads,
                                        ARRAY_SIZE(fec1_rst_pads));

       gpio_request(FEC_RST_PAD, "fec1_rst");
       gpio_direction_output(FEC_RST_PAD, 0);
       udelay(500);
       gpio_direction_output(FEC_RST_PAD, 1);
}

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	setup_iomux_fec();

	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(&gpr->gpr[1], 0x2000, 0);

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
        /* enable rgmii rxc skew and phy mode select to RGMII copper */
        phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
        phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);

        phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x00);
        phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x82ee);
        phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
        phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

        if (phydev->drv->config)
                phydev->drv->config(phydev);
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

int board_ehci_usb_phy_mode(struct udevice *dev)
{
	return USB_INIT_DEVICE;
}

#define FSL_SIP_GPC			0xC2000000
#define FSL_SIP_CONFIG_GPC_PM_DOMAIN	0x3
#define DISPMIX				9
#define MIPI				10

int board_init(void)
{
	if (IS_ENABLED(CONFIG_FEC_MXC))
		setup_fec();

	call_imx_sip(FSL_SIP_GPC, FSL_SIP_CONFIG_GPC_PM_DOMAIN, DISPMIX, true, 0);
	call_imx_sip(FSL_SIP_GPC, FSL_SIP_CONFIG_GPC_PM_DOMAIN, MIPI, true, 0);

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

#define GPIO_PAD_CFG_CTRL PAD_CTL_DSE0

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

/* IWG34M: Print Board Information */
static void printboard_info(void)
{
     int i, bom_rev = 0, pcb_rev = 0, revision = 0;
     
     struct tag_serialnr serialnr;
       
     /*IWG34S : Adding CPU Unique ID read support*/
     get_board_serial(&serialnr);
     
     imx_iomux_v3_setup_multiple_pads(board_cfg, ARRAY_SIZE(board_cfg));

     for (i=0;i<ARRAY_SIZE(board_config_pads);i++) 
     {
          gpio_request(board_config_pads[i], "SOM-Revision-GPIO");
          gpio_direction_input(board_config_pads[i]);
          revision |= gpio_get_value(board_config_pads[i]) << i;
     }

     pcb_rev = ((revision) & 0x07) +1;
     bom_rev = (((revision) & 0x78)>>3);

     printf ("\n");
     printf ("Board Info:\n");
     printf ("\tBSP Version            : %s\n", BSP_VERSION);
     printf ("\tSOM Version            : iW-PRGII-AP-01-R%x.%x\n",pcb_rev,bom_rev);
     printf ("\tCPU Unique ID          : 0x%08X%08X\n",serialnr.high,serialnr.low);
     printf ("\n");
}

void iwg34s_fdt_update(void *fdt)
{
        if (!strcmp("lvds", env_get("disp"))){
	fdt_delprop(fdt, fdt_path_offset(fdt, "/soc@0/bus@30800000/i2c@30a30000/lt8912@48"), "ddc-i2c-bus");
	}
}

int board_late_init(void)
{
	printboard_info();
#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "IWG34S");
	env_set("board_rev", "iMX8MM");
#endif
	return 0;
}

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
	return 0; /*TODO*/
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/

#ifdef CONFIG_ANDROID_SUPPORT
bool is_power_key_pressed(void) {
	return (bool)(!!(readl(SNVS_HPSR) & (0x1 << 6)));
}
#endif
