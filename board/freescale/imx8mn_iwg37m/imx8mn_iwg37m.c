// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 iWave Systems Technologies Pvt Ltd.
 */

#include <common.h>
#include <env.h>
#include <init.h>
#include <asm/global_data.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <asm/arch/imx8mn_pins.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/setup.h>
#include <i2c.h>
#include <asm/io.h>
#include <usb.h>
#include <imx_sip.h>
#include <linux/arm-smccc.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)
#define RESET_PAD_CTRL  (PAD_CTL_DSE6 | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

static iomux_v3_cfg_t const uart_pads[] = {
	IMX8MN_PAD_UART4_RXD__UART4_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MN_PAD_UART4_TXD__UART4_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MN_PAD_GPIO1_IO02__WDOG1_WDOG_B  | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

static iomux_v3_cfg_t const reset_pads[] = {
        IMX8MN_PAD_SAI3_MCLK__GPIO5_IO2  | MUX_PAD_CTRL(RESET_PAD_CTRL),
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

#define CONFG_GPIO_PAD_CTRL PAD_CTL_FSEL0

#define BCONFIG_0 IMX_GPIO_NR(1,9)
#define BCONFIG_1 IMX_GPIO_NR(5,1)
#define BCONFIG_2 IMX_GPIO_NR(4,25)
#define BCONFIG_3 IMX_GPIO_NR(4,26)
#define BCONFIG_4 IMX_GPIO_NR(4,27)
#define BCONFIG_5 IMX_GPIO_NR(2,19)
#define BCONFIG_6 IMX_GPIO_NR(2,20)

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
	IMX8MN_PAD_GPIO1_IO09__GPIO1_IO9 | MUX_PAD_CTRL(CONFG_GPIO_PAD_CTRL),
	IMX8MN_PAD_SAI3_TXD__GPIO5_IO1 | MUX_PAD_CTRL(CONFG_GPIO_PAD_CTRL),
	IMX8MN_PAD_SAI2_TXC__GPIO4_IO25	| MUX_PAD_CTRL(CONFG_GPIO_PAD_CTRL),
	IMX8MN_PAD_SAI2_TXD0__GPIO4_IO26 | MUX_PAD_CTRL(CONFG_GPIO_PAD_CTRL),
	IMX8MN_PAD_SAI2_MCLK__GPIO4_IO27 | MUX_PAD_CTRL(CONFG_GPIO_PAD_CTRL),
	IMX8MN_PAD_SD2_RESET_B__GPIO2_IO19 | MUX_PAD_CTRL(CONFG_GPIO_PAD_CTRL),
	IMX8MN_PAD_SD2_WP__GPIO2_IO20 | MUX_PAD_CTRL(CONFG_GPIO_PAD_CTRL),
};

static print_board_info(void) {
	int i, bom_rev=0, pcb_rev=0;
	int stmp_i2c_reg, carrier_revision;

	struct tag_serialnr serialnr;

	imx_iomux_v3_setup_multiple_pads(board_cfg, ARRAY_SIZE(board_cfg));

	get_board_serial(&serialnr);
	
	get_som_revision(&pcb_rev, &bom_rev);

	printf ("Board Info:\n");
	printf ("\tBSP Version               : %s\n", BSP_VERSION);
	printf ("\tSOM Version               : iW-PRGJZ-AP-01-R%x.%x\n",pcb_rev,bom_rev);
	printf ("\tCPU Unique ID             : 0x%08X%08X\n",serialnr.high,serialnr.low);

	get_carrier_revision(&stmp_i2c_reg , &carrier_revision);
	if(stmp_i2c_reg == 0) { 
		printf ("\tCarrier revision	  : iW-PREVD-AP-01-R2.%x\n", carrier_revision);
	}
	else {
		printf ("\tCarrier revision	  : iW-PREVD-AP-01-R2.X(custom)\n");
	}
	printf("\n");
}

int get_som_revision (int *pcb_rev, int *bom_rev){
	int i, revision = 0;

	imx_iomux_v3_setup_multiple_pads(board_cfg, ARRAY_SIZE(board_cfg));
        for(i=0;i<ARRAY_SIZE(board_config_pads);i++) {
                        gpio_request(board_config_pads[i], "SOM-revision-gpio");
                        gpio_direction_input(board_config_pads[i]);
                        revision |= gpio_get_value(board_config_pads[i]) << i;

	}
	*pcb_rev = ((revision) & 0x07) + 1;
	*bom_rev = ((revision & 0x78) >> 3);
	
	return (*pcb_rev,*bom_rev);
}

int get_carrier_revision (int *probe, int *status) {
	struct udevice *bus;
	struct udevice *i2c_dev = NULL;
	uint8_t valb;
	int bus_seq, read_clock, read_reg, carrier_rev, lsb_state, read_alternative_reg, read_alter_reg;

	bus_seq = uclass_get_device_by_seq(UCLASS_I2C, 1, &bus);
	if (bus_seq) {
		printf("%s: Can't find bus\n", __func__);
		return -EINVAL;
	}

	*probe = dm_i2c_probe(bus, 0x44, 0, &i2c_dev);
        if (*probe) {
                debug("%s: Can't find device id=0x%x\n",
                                *probe);
                return *probe;
        }

	else
	{
		read_clock = dm_i2c_reg_read(i2c_dev, 0x04);
		read_reg = read_clock & 0xB;
			dm_i2c_reg_write(i2c_dev, 0x04, read_reg); /*Writing clock control register*/

		read_alternative_reg = dm_i2c_reg_read(i2c_dev, 0x17);
		read_alter_reg = read_alternative_reg | 0xD;
		dm_i2c_reg_write(i2c_dev, 0x17, read_alter_reg); /*Writing Alternate function register*/

		carrier_rev = dm_i2c_reg_read(i2c_dev, 0x12);
		carrier_rev &= 0xD;
		if(carrier_rev & 1)
			lsb_state = 1;
		else
			lsb_state = 0;
		carrier_rev >>=1;
		*status = carrier_rev | lsb_state;

		return *status;
	}

	return -ENODEV;

}

static void reset_enable(void)
{
        imx_iomux_v3_setup_multiple_pads(reset_pads, ARRAY_SIZE(reset_pads));

        struct gpio_desc reset_desc;
        int ret;

        ret = dm_gpio_lookup_name("GPIO5_2", &reset_desc);
        if (ret) {
                printf("%s lookup GPIO5_2 failed ret = %d\n", ret);
                return;
        }

        ret = dm_gpio_request(&reset_desc, "reset_gpio");
        if (ret) {
                printf("%s request reset gpio failed ret = %d\n", ret);
                return;
        }

        dm_gpio_set_dir_flags(&reset_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
        dm_gpio_set_value(&reset_desc, 1);
}

#ifdef CONFIG_OF_BOARD_SETUP
/*Update kernel dts entry for printing carrier revision*/
int ft_board_setup(void *blob, struct bd_info *bd) 
{
	int stmpe_i2c_reg, carrier_revision, bom_rev=0, pcb_rev=0, ret;
	int value;
	int regst[1];
	int reg_pcb[1], reg_bom[1];

	get_som_revision(&pcb_rev, &bom_rev);
	reg_pcb[0] = cpu_to_fdt32(pcb_rev);
	reg_bom[0] = cpu_to_fdt32(bom_rev);
	fdt_setprop(blob,fdt_path_offset(blob, "/iwg37m_common"), "pcb_rev", reg_pcb, sizeof(reg_pcb));
	fdt_setprop(blob,fdt_path_offset(blob, "/iwg37m_common"), "bom_rev", reg_bom, sizeof(reg_bom));

	get_carrier_revision(&stmpe_i2c_reg, &carrier_revision);
	if (stmpe_i2c_reg != 0) /* Checking stmpe register, if stmpe_i2c_reg is 0 means stmpe found and otherwise not found */
	{
		regst[0] = cpu_to_fdt32(stmpe_i2c_reg);
		value = fdt_setprop(blob,fdt_path_offset(blob, "/iwg37m_common"), "carrier_bom", regst, sizeof(regst));
	}
	else
		regst[0] = cpu_to_fdt32(carrier_revision);
	value = fdt_setprop(blob,fdt_path_offset(blob, "/iwg37m_common"), "carrier_bom", regst, sizeof(regst));

/* Support for 1GB LPDDR4 Board */
#ifdef CONFIG_SDRAM_SIZE_1GB
	int i;
	uint32_t reg[4], size[2], gpu[12], rpmsg[4];
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
	reg[1] = cpu_to_fdt32(0x40000000);
	reg[2] = cpu_to_fdt32(0);
	reg[3] = cpu_to_fdt32(0x40000000);
	ret= fdt_setprop(blob,fdt_path_offset(blob, "/reserved-memory/linux,cma"), "alloc-ranges", reg, sizeof(reg));
	if(ret < 0)
		printf("Kernel CMA update failed\n");

	for(i=0; i<=10; i+=2)
		gpu[i] = cpu_to_fdt32(0);

	gpu[1] = cpu_to_fdt32(0x38000000); gpu[3] = cpu_to_fdt32(0x40000);
	gpu[5] = cpu_to_fdt32(0x40000000); gpu[7] = cpu_to_fdt32(0x40000000);
	gpu[9] = cpu_to_fdt32(0); gpu[11] = cpu_to_fdt32(0x4000000);

	ret= fdt_setprop(blob,fdt_path_offset(blob, "/gpu@38000000"), "reg", gpu, sizeof(gpu));
	if(ret < 0)
		printf("GPU memory update failed\n");

	if (!strcmp("imx8mm-iwg37m-rpmsg.dtb", env_get("fdtfile"))) {
		rpmsg[0] = cpu_to_fdt32(0);
		rpmsg[1] = cpu_to_fdt32(0x60000000);
		rpmsg[2] = cpu_to_fdt32(0);
		rpmsg[3] = cpu_to_fdt32(0x1000000);
		ret= fdt_setprop(blob,fdt_path_offset(blob, "/reserved-memory/m_core@0x80000000"), "reg", rpmsg, sizeof(rpmsg));
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
	return ret;
}
#endif

int board_late_init(void)
{
	print_board_info();
	reset_enable();
#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "iW-RainboW-G37M-i.MX8M Nano SODIMM SOM");
	env_set("board_rev", "iW-PRGJZ-AP-01-R1.X");
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
