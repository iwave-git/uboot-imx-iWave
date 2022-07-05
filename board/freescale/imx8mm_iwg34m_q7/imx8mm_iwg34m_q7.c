// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 iWave Systems Technologies Pvt Ltd.
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
#include <asm/setup.h>
#include <i2c.h>
#include <asm/io.h>
#include "../common/tcpc.h"
#include <usb.h>
#include <imx_sip.h>
#include <linux/arm-smccc.h>

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

#define LVDS_RESET_GPIO IMX_GPIO_NR(4,0)

static iomux_v3_cfg_t lvds_reset_pads[] = {
        IMX8MM_PAD_SAI1_RXFS_GPIO4_IO0 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

void lvds_reset(int func)
{
	imx_iomux_v3_setup_multiple_pads(lvds_reset_pads, ARRAY_SIZE(lvds_reset_pads));
	gpio_request(LVDS_RESET_GPIO, "lvds_reset");

	if (func == TURN_OFF)
		gpio_direction_output(LVDS_RESET_GPIO, 0);

	if (func == TURN_ON)
	{
		//mdelay(30);
		gpio_direction_output(LVDS_RESET_GPIO, 0);
		//mdelay(120);
		gpio_direction_output(LVDS_RESET_GPIO, 1);
	}

	gpio_free(LVDS_RESET_GPIO);
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


#define DISPMIX				9
#define MIPI				10

int board_init(void)
{
	struct arm_smccc_res res;

	if (IS_ENABLED(CONFIG_FEC_MXC))
		setup_fec();

	arm_smccc_smc(IMX_SIP_GPC, IMX_SIP_GPC_PM_DOMAIN,
		      DISPMIX, true, 0, 0, 0, 0, &res);
	arm_smccc_smc(IMX_SIP_GPC, IMX_SIP_GPC_PM_DOMAIN,
		      MIPI, true, 0, 0, 0, 0, &res);

	lvds_reset(TURN_ON);
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

#define GPIO_PAD_CFG_CTRL PAD_CTL_FSEL0

#define BCONFIG_6 IMX_GPIO_NR(2, 20)
#define BCONFIG_5 IMX_GPIO_NR(2, 19)
#define BCONFIG_4 IMX_GPIO_NR(1, 0)
#define BCONFIG_3 IMX_GPIO_NR(5, 1)
#define BCONFIG_2 IMX_GPIO_NR(4, 25)
#define BCONFIG_1 IMX_GPIO_NR(4, 26)
#define BCONFIG_0 IMX_GPIO_NR(1, 9)

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
      IMX8MM_PAD_GPIO1_IO09_GPIO1_IO9 | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
      IMX8MM_PAD_SAI2_TXD0_GPIO4_IO26 | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
      IMX8MM_PAD_SAI2_TXC_GPIO4_IO25 | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
      IMX8MM_PAD_SAI3_TXD_GPIO5_IO1 | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
      IMX8MM_PAD_GPIO1_IO00_GPIO1_IO0 | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
      IMX8MM_PAD_SD2_RESET_B_GPIO2_IO19 | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
      IMX8MM_PAD_SD2_WP_GPIO2_IO20 | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
};

int get_carrier_revision(int *probe, int *carrier_pcb, int *carrier_bom)
{
	struct udevice *bus;
	struct udevice *i2c_dev = NULL;
	int bus_seq, revision;

	bus_seq = uclass_get_device_by_seq(UCLASS_I2C, 1, &bus);

	if (bus_seq) {
		printf("%s: Can't find bus\n", __func__);
		return -EINVAL;
	}

	*probe = dm_i2c_probe(bus, 0x39, 0, &i2c_dev);
	if (*probe) {
		debug("Can't find device id=0x%x\n", *probe);
		return *probe;
	}

	else
	{
		revision = dm_i2c_reg_read(i2c_dev, 0xff);
		*carrier_pcb = (revision & 0x07)+1;
		*carrier_bom = ((revision & 0xF8)>>3);
		return 0;
	}

	return -ENODEV;
}

void get_som_revision(int *pcb_rev, int *bom_rev)
{
	int i, revision = 0;

	imx_iomux_v3_setup_multiple_pads(board_cfg, ARRAY_SIZE(board_cfg));

	for (i=0;i<ARRAY_SIZE(board_config_pads);i++)
	{
		gpio_request(board_config_pads[i], "SOM-Revision-GPIO");
		gpio_direction_input(board_config_pads[i]);
		revision |= gpio_get_value(board_config_pads[i]) << i;
	}

	*pcb_rev = (((revision) & 0x60)>>5)+1;
	*bom_rev = ((revision) & 0x1F);
}

/* IWG34M: Print Board Information */
static void printboard_info(void)
{
     int bom_rev = 0, pcb_rev = 0;
     int stmpe_i2c_reg, carrier_revision;
     int carrier_pcb, carrier_bom;

     struct tag_serialnr serialnr;

     /*IWG27M : Adding CPU Unique ID read support*/
     get_board_serial(&serialnr);

     get_som_revision(&pcb_rev, &bom_rev);

     printf ("Board Info:\n");
     printf ("\tBSP Version               : %s\n", BSP_VERSION);
     printf ("\tSOM Version               : iW-PRGNZ-AP-01-R%x.%x\n",pcb_rev,bom_rev);
     printf ("\tCPU Unique ID             : 0x%08X%08X\n",serialnr.high,serialnr.low);

     get_carrier_revision(&stmpe_i2c_reg, &carrier_pcb, &carrier_bom);
     if(stmpe_i2c_reg == 0)
     {
		     printf ("\tCarrier Board Version     : iW-PRGTD-AP-01-R%x.%x\n", carrier_pcb, carrier_bom);
     }
     else
		     printf ("\tCarrier Board Version     : iW-PRGTD-AP-01-RX.X(Custom)\n");
     printf ("\n");
}

#define LVDS_BACKLIGHT IMX_GPIO_NR(5, 4)
#define LVDS_BACKLIGHT_EN IMX_GPIO_NR(5, 5)

static iomux_v3_cfg_t lvds_backlight_pads[] = {
	IMX8MM_PAD_SPDIF_RX_GPIO5_IO4 | MUX_PAD_CTRL(NO_PAD_CTRL),
	IMX8MM_PAD_SPDIF_EXT_CLK_GPIO5_IO5 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

void lvds_bcklt_reset(int func)
{
	imx_iomux_v3_setup_multiple_pads(lvds_backlight_pads, ARRAY_SIZE(lvds_backlight_pads));
        gpio_request(LVDS_BACKLIGHT_EN, "lvds_backlight_en");
	gpio_request(LVDS_BACKLIGHT, "lvds_backlight");

	if (func == TURN_OFF)
	{
		gpio_direction_output(LVDS_BACKLIGHT_EN,0);
		gpio_direction_output(LVDS_BACKLIGHT,0);
		gpio_free(LVDS_BACKLIGHT);
	}

	if (func == TURN_ON)
	{
		//mdelay(500);
		gpio_direction_output(LVDS_BACKLIGHT_EN,1);
	}

	gpio_free(LVDS_BACKLIGHT_EN);
}

#ifdef CONFIG_OF_BOARD_SETUP
/* IWG34M: Updating carrier board revision to device tree and Display selection */
int ft_board_setup (void *blob, struct bd_info *bd)
{
        int stmpe_i2c_reg, carrier_pcb, carrier_bom, bom_rev=0, pcb_rev=0, carrier_revision=-1, ret;
        int value, offset;
        int pcb_stmp_reg[1], bom_stmp_reg[1];
        int reg_pcb[1], reg_bom[1];

        get_som_revision(&pcb_rev, &bom_rev);
        reg_pcb[0] = cpu_to_fdt32(pcb_rev);
        reg_bom[0] = cpu_to_fdt32(bom_rev);
        fdt_setprop(blob,fdt_path_offset(blob, "/iwg34m_common_q7"), "pcb_rev", reg_pcb, sizeof(reg_pcb));
        fdt_setprop(blob,fdt_path_offset(blob, "/iwg34m_common_q7"), "bom_rev", reg_bom, sizeof(reg_bom));

        get_carrier_revision(&stmpe_i2c_reg, &carrier_pcb, &carrier_bom);
        if (stmpe_i2c_reg == 0) /* Checking stmpe register, if stmpe_i2c_reg is 0 means stmpe found and otherwise not found */
        {
                pcb_stmp_reg[0] = cpu_to_fdt32(carrier_pcb);
                bom_stmp_reg[0] = cpu_to_fdt32(carrier_bom);
                value = fdt_setprop(blob,fdt_path_offset(blob, "/iwg34m_common_q7"), "carrier_pcb", pcb_stmp_reg, sizeof(pcb_stmp_reg));
                value = fdt_setprop(blob,fdt_path_offset(blob, "/iwg34m_common_q7"), "carrier_bom", bom_stmp_reg, sizeof(bom_stmp_reg));
        }
        else
	{
                pcb_stmp_reg[0] = cpu_to_fdt32(carrier_revision);
                bom_stmp_reg[0] = cpu_to_fdt32(carrier_revision);
        value = fdt_setprop(blob,fdt_path_offset(blob, "/iwg34m_common_q7"), "carrier_pcb", pcb_stmp_reg, sizeof(pcb_stmp_reg));
        value = fdt_setprop(blob,fdt_path_offset(blob, "/iwg34m_common_q7"), "carrier_bom", bom_stmp_reg, sizeof(bom_stmp_reg));
	}

	if (!strcmp("lvds", env_get("disp"))){
		fdt_delprop(blob, fdt_path_offset(blob, "/soc@0/bus@30800000/i2c@30a30000/lt8912@48"), "ddc-i2c-bus");

		offset = fdt_setprop_string(blob, fdt_path_offset(blob, "/sound-hdmi"), "status", "disabled");
		if (offset)
			return offset;
	}

	else
	{
		fdt_setprop_string(blob, fdt_path_offset(blob, "/soc@0/bus@30800000/i2c@30a30000/edt-ft5x06@38"), "status", "disabled");
	}

/* Support for 1GB LPDDR4 Board */
#ifdef CONFIG_SDRAM_SIZE_1GB
        int i;
        uint32_t reg[4], size[2], gpu[16], rpmsg[4];
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

        if (!strcmp("imx8mm-iwg34m-q7-rpmsg.dtb", env_get("fdtfile"))) {
                rpmsg[0] = cpu_to_fdt32(0);
                rpmsg[1] = cpu_to_fdt32(0x60000000);
                rpmsg[2] = cpu_to_fdt32(0);
                rpmsg[3] = cpu_to_fdt32(0x1000000);
                ret= fdt_setprop(blob,fdt_path_offset(blob, "/reserved-memory/m4@0x80000000"), "reg", rpmsg, sizeof(rpmsg));
                if(ret < 0)
                        printf("RPMSG memory update failed\n");
                rpmsg[0] = cpu_to_fdt32(0);
                rpmsg[1] = cpu_to_fdt32(0x78000000);
                rpmsg[2] = cpu_to_fdt32(0);
                rpmsg[3] = cpu_to_fdt32(0x8000);
                ret= fdt_setprop(blob,fdt_path_offset(blob, "/reserved-memory/vdev0vring0@b8000000"), "reg", rpmsg, sizeof(rpmsg));
                if(ret < 0)
                        printf("RPMSG memory update failed\n");
                rpmsg[0] = cpu_to_fdt32(0);
                rpmsg[1] = cpu_to_fdt32(0x78008000);
                rpmsg[2] = cpu_to_fdt32(0);
                rpmsg[3] = cpu_to_fdt32(0x8000);
                ret= fdt_setprop(blob,fdt_path_offset(blob, "/reserved-memory/vdev0vring1@b8008000"), "reg", rpmsg, sizeof(rpmsg));
                if(ret < 0)
                        printf("RPMSG memory update failed\n");
                rpmsg[0] = cpu_to_fdt32(0);
                rpmsg[1] = cpu_to_fdt32(0x780ff000);
                rpmsg[2] = cpu_to_fdt32(0);
                rpmsg[3] = cpu_to_fdt32(0x1000);
                ret= fdt_setprop(blob,fdt_path_offset(blob, "/reserved-memory/rsc_table@b80ff000"), "reg", rpmsg, sizeof(rpmsg));
                if(ret < 0)
                        printf("RPMSG memory update failed\n");
                rpmsg[0] = cpu_to_fdt32(0);
                rpmsg[1] = cpu_to_fdt32(0x78400000);
                rpmsg[2] = cpu_to_fdt32(0);
                rpmsg[3] = cpu_to_fdt32(0x100000);
                ret= fdt_setprop(blob,fdt_path_offset(blob, "/reserved-memory/vdevbuffer@b8400000"), "reg", rpmsg, sizeof(rpmsg));
                if(ret < 0)
                        printf("RPMSG memory update failed\n");
        }
#endif
	return 0;
}
#endif

int board_late_init(void)
{
	printboard_info();
	if (!strcmp("lvds", env_get("disp")))
		lvds_bcklt_reset(TURN_ON);
#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "iW-RainboW-G34M-i.MX8M Mini Qseven SOM");
	env_set("board_rev", "iW-PRGNZ-AP-01-R3.X");
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
