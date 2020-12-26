/*
 * Copyright 2020 iWave System Technologies Pvt Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <cpu_func.h>
#include <env.h>
#include <errno.h>
#include <init.h>
#include <linux/libfdt.h>
#include <fdt_support.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <asm/arch/snvs_security_sc.h>
#include <usb.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/lpcg.h>
#include <asm/setup.h>

DECLARE_GLOBAL_DATA_PTR;

int bom_rev, pcb_rev;

#define ENET_INPUT_PAD_CTRL	((SC_PAD_CONFIG_OD_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_18V_10MA << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ENET_NORMAL_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_18V_10MA << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))


#define GPIO_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define GPIO_PAD_CFG_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_NONE << PADRING_PULL_SHIFT))

#define UART_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

static iomux_cfg_t uart4_pads[] = {
	SC_P_M40_GPIO0_00 | MUX_PAD_CTRL(UART_PAD_CTRL),
	SC_P_M40_GPIO0_01 | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx8_iomux_setup_multiple_pads(uart4_pads, ARRAY_SIZE(uart4_pads));
}

static iomux_cfg_t wifi_pads[] = {
	SC_P_USDHC1_RESET_B | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
        SC_P_ESAI0_TX4_RX1  | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

static void wifi_pwr_seq(void)
{
	struct gpio_desc pdn_en_desc, core_en_desc;
	int ret;

	imx8_iomux_setup_multiple_pads(wifi_pads, ARRAY_SIZE(wifi_pads));

	ret = dm_gpio_lookup_name("GPIO4_7", &pdn_en_desc);
	if (ret) {
		printf("%s lookup GPIO4_7 failed ret = %d\n", __func__, ret);
		return;
	}

	ret = dm_gpio_request(&pdn_en_desc, "wifi_pdn_en_gpio");
	if (ret) {
		printf("%s request wifi pdn_en_gpio gpio failed ret = %d\n", __func__, ret);
		return;
	}

	dm_gpio_set_dir_flags(&pdn_en_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);

	ret = dm_gpio_lookup_name("GPIO2_30", &core_en_desc);
	if (ret) {
		printf("%s lookup GPIO2_30 failed ret = %d\n", __func__, ret);
		return;
	}

	ret = dm_gpio_request(&core_en_desc, "wifi_core_en_gpio");
	if (ret) {
		printf("%s request wifi core_en_gpio gpio failed ret = %d\n", __func__, ret);
		return;
	}

	dm_gpio_set_dir_flags(&core_en_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);

	dm_gpio_set_value(&pdn_en_desc, 1);
	mdelay(5);
	dm_gpio_set_value(&core_en_desc, 1);
}

int board_early_init_f(void)
{
	sc_pm_clock_rate_t rate = SC_80MHZ;
	int ret;

	/* When start u-boot in XEN VM, directly return */
	if (IS_ENABLED(CONFIG_XEN)) {
		writel(0xF53535F5, (void __iomem *)0x80000000);
		return 0;
	}

	/* Set UART4 clock root to 80 MHz */
	ret = sc_pm_setup_uart(SC_R_UART_4, rate);
	if (ret)
		return ret;

	setup_iomux_uart();

/* Dual bootloader feature will require CAAM access, but JR0 and JR1 will be
 * assigned to seco for imx8, use JR3 instead.
 */
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_DUAL_BOOTLOADER)
	sc_pm_set_resource_power_mode(-1, SC_R_CAAM_JR3, SC_PM_PW_MODE_ON);
	sc_pm_set_resource_power_mode(-1, SC_R_CAAM_JR3_OUT, SC_PM_PW_MODE_ON);
#endif

	return 0;
}

#if IS_ENABLED(CONFIG_FEC_MXC)
#include <miiphy.h>

#ifndef CONFIG_DM_ETH
static iomux_cfg_t pad_enet1[] = {
	SC_P_ENET1_RGMII_RX_CTL | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET1_RGMII_RXD0 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET1_RGMII_RXD1 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET1_RGMII_RXD2 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET1_RGMII_RXD3 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET1_RGMII_RXC | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET1_RGMII_TX_CTL | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET1_RGMII_TXD0 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET1_RGMII_TXD1 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET1_RGMII_TXD2 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET1_RGMII_TXD3 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET1_RGMII_TXC | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET1_REFCLK_125M_25M | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),

	SC_P_ENET1_MDC | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET1_MDIO | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_MIPI_DSI1_I2C0_SCL | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
        SC_P_MIPI_DSI0_I2C0_SCL | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
        SC_P_MIPI_DSI0_I2C0_SDA | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

static iomux_cfg_t pad_enet0[] = {
	SC_P_ENET0_RGMII_RX_CTL | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXD0 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXD1 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXD2 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXD3 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXC | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_TX_CTL | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXD0 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXD1 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXD2 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXD3 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXC | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),

	SC_P_ENET0_MDC | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_MDIO | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_REFCLK_125M_25M | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
        SC_P_PCIE_CTRL1_WAKE_B | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
        SC_P_PCIE_CTRL1_CLKREQ_B | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
        SC_P_PCIE_CTRL0_CLKREQ_B | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

static void setup_iomux_fec(void)
{
	if (0 == CONFIG_FEC_ENET_DEV)
		imx8_iomux_setup_multiple_pads(pad_enet0, ARRAY_SIZE(pad_enet0));
	else
		imx8_iomux_setup_multiple_pads(pad_enet1, ARRAY_SIZE(pad_enet1));
}

int board_eth_init(bd_t *bis)
{
	int ret;
	struct power_domain pd;

	printf("[%s] %d\n", __func__, __LINE__);

	if (CONFIG_FEC_ENET_DEV) {
		if (!power_domain_lookup_name("conn_enet1", &pd))
			power_domain_on(&pd);
	} else {
		if (!power_domain_lookup_name("conn_enet0", &pd))
			power_domain_on(&pd);
	}

	setup_iomux_fec();

	ret = fecmxc_initialize_multi(bis, CONFIG_FEC_ENET_DEV,
		CONFIG_FEC_MXC_PHYADDR, IMX_FEC_BASE);
	if (ret)
		printf("FEC1 MXC: %s:failed\n", __func__);

	return ret;
}
#endif

int board_phy_config(struct phy_device *phydev)
{
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

static iomux_cfg_t otg_pwr_pads[] = {
	SC_P_USB_SS3_TC0 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

static void usb_otg_pwr_enable(void)
{
	struct gpio_desc usb_otg_desc;
	int ret;

	imx8_iomux_setup_multiple_pads(otg_pwr_pads, ARRAY_SIZE(otg_pwr_pads));

	ret = dm_gpio_lookup_name("GPIO4_3", &usb_otg_desc);
	if (ret) {
		printf("%s lookup GPIO4_3 failed ret = %d\n", __func__, ret);
		return;
	}

	ret = dm_gpio_request(&usb_otg_desc, "usb_otg_gpio");
	if (ret) {
		printf("%s request usb otg gpio failed ret = %d\n", __func__, ret);
		return;
	}

	dm_gpio_set_dir_flags(&usb_otg_desc, GPIOD_IS_IN);
}

static iomux_cfg_t hub_pwr_pads[] = {
	SC_P_MIPI_DSI1_I2C0_SDA | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

static void usb_hub_pwr_enable(void)
{
	imx8_iomux_setup_multiple_pads(hub_pwr_pads, ARRAY_SIZE(hub_pwr_pads));

	struct gpio_desc hub_reset_desc;
	int ret;

	ret = dm_gpio_lookup_name("GPIO1_21", &hub_reset_desc);
	if (ret) {
		printf("%s lookup GPIO1_21 failed ret = %d\n", __func__, ret);
		return;
	}

	ret = dm_gpio_request(&hub_reset_desc, "hub_reset_gpio");
	if (ret) {
		printf("%s request hub_reset gpio failed ret = %d\n", __func__, ret);
		return;
	}

	dm_gpio_set_dir_flags(&hub_reset_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
	dm_gpio_set_value(&hub_reset_desc, 1);
}

#define BCONFIG_0 IMX_GPIO_NR(1, 5)
#define BCONFIG_1 IMX_GPIO_NR(2, 6)
#define BCONFIG_2 IMX_GPIO_NR(2, 10)
#define BCONFIG_3 IMX_GPIO_NR(1, 11)
#define BCONFIG_4 IMX_GPIO_NR(2, 11)
#define BCONFIG_5 IMX_GPIO_NR(2, 12)
#define BCONFIG_6 IMX_GPIO_NR(2, 13)

int board_config_pads[] = {
	BCONFIG_0,
	BCONFIG_1,
	BCONFIG_2,
	BCONFIG_3,
	BCONFIG_4,
	BCONFIG_5,
	BCONFIG_6,
};

static iomux_cfg_t board_cfg[] = {
	SC_P_LVDS0_GPIO01 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
	SC_P_ESAI1_SCKR | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
	SC_P_ESAI1_TX2_RX3 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
	SC_P_LVDS1_GPIO01 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
	SC_P_ESAI1_TX3_RX2 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
	SC_P_ESAI1_TX4_RX1 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
	SC_P_ESAI1_TX5_RX0 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CFG_CTRL),
};

void get_board_info(void)
{
	int i;

	imx8_iomux_setup_multiple_pads(board_cfg, ARRAY_SIZE(board_cfg));

	for (i=0;i<ARRAY_SIZE(board_config_pads);i++) {
		if(i<=3) {
			gpio_request(board_config_pads[i], "SOM-Revision-GPIO");
			gpio_direction_input(board_config_pads[i]);
			bom_rev |= (gpio_get_value(board_config_pads[i]) << i);
		} else {
			gpio_request(board_config_pads[i], "SOM-Revision-GPIO");
			gpio_direction_input(board_config_pads[i]);
			pcb_rev |= (gpio_get_value(board_config_pads[i]) << (i-4));
		}
	}
}

static void print_board_info(void)
{
	struct tag_serialnr serialnr;

	/*IWG27S : Adding CPU Unique ID read support*/
	get_board_serial(&serialnr);

	printf ("\n");
	printf ("Board Info:\n");
	printf ("\tBSP Version     : %s\n", BSP_VERSION);
	printf ("\tSOM Version     : iW-PRGEE-AP-01-R%x.%x\n",pcb_rev+1,bom_rev);
	printf ("\tCPU Unique ID   : 0X%08X%08X\n",serialnr.high,serialnr.low);
	printf ("\n");
}

int checkboard(void)
{
	puts("Board: 8QM/QP IWG27S\n");

	build_info();
	print_bootinfo();

	return 0;
}

#ifdef CONFIG_FSL_HSIO

#define PCIE_PAD_CTRL	((SC_PAD_CONFIG_OD_IN << PADRING_CONFIG_SHIFT))
static iomux_cfg_t board_pcie_pins[] = {
	SC_P_PCIE_CTRL1_PERST_B | MUX_MODE_ALT(3) | MUX_PAD_CTRL(PCIE_PAD_CTRL),
	SC_P_PCIE_CTRL0_WAKE_B | MUX_MODE_ALT(3) | MUX_PAD_CTRL(PCIE_PAD_CTRL),
	SC_P_PCIE_CTRL0_PERST_B | MUX_MODE_ALT(3) | MUX_PAD_CTRL(PCIE_PAD_CTRL),
};

static void imx8qm_hsio_initialize(void)
{
	struct power_domain pd;
	int ret;

	if (!power_domain_lookup_name("hsio_sata0", &pd)) {
		ret = power_domain_on(&pd);
		if (ret)
			printf("hsio_sata0 Power up failed! (error = %d)\n", ret);
	}

	if (!power_domain_lookup_name("hsio_pcie0", &pd)) {
		ret = power_domain_on(&pd);
		if (ret)
			printf("hsio_pcie0 Power up failed! (error = %d)\n", ret);
	}

	if (!power_domain_lookup_name("hsio_pcie1", &pd)) {
		ret = power_domain_on(&pd);
		if (ret)
			printf("hsio_pcie1 Power up failed! (error = %d)\n", ret);
	}

	if (!power_domain_lookup_name("hsio_gpio", &pd)) {
		ret = power_domain_on(&pd);
		if (ret)
			 printf("hsio_gpio Power up failed! (error = %d)\n", ret);
	}

	lpcg_all_clock_on(HSIO_PCIE_X2_LPCG);
	lpcg_all_clock_on(HSIO_PCIE_X1_LPCG);
	lpcg_all_clock_on(HSIO_PHY_X2_LPCG);
	lpcg_all_clock_on(HSIO_PHY_X1_LPCG);
	lpcg_all_clock_on(HSIO_PCIE_X2_CRR2_LPCG);
	lpcg_all_clock_on(HSIO_PCIE_X1_CRR3_LPCG);
	lpcg_all_clock_on(HSIO_MISC_LPCG);
	lpcg_all_clock_on(HSIO_GPIO_LPCG);

	imx8_iomux_setup_multiple_pads(board_pcie_pins, ARRAY_SIZE(board_pcie_pins));
}

void pci_init_board(void)
{
	/* test the 1 lane mode of the PCIe A controller */
	mx8qm_pcie_init();
}
#endif

#ifdef CONFIG_USB

int board_usb_init(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 1) {
		if (init == USB_INIT_HOST) {
#ifdef CONFIG_USB_CDNS3_GADGET
		} else {
#endif
		}
	}

	return ret;

}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 1) {
		if (init == USB_INIT_HOST) {
		}
	}

	return ret;
}
#endif

#define SATA_SEL IMX_GPIO_NR(0, 4)

void iwg27s_fdt_update(void *fdt)
{
	uint32_t reg[2];
	int ret=0;

	if (pcb_rev >= 1) {
		/* IWG27S: Enable Voltage switching only for C1 PMIC supported R2.x board */
		fdt_delprop(fdt, fdt_path_offset(fdt, "/bus@5b000000/mmc@5b020000"), "no-1-8-v");
	}

	/*IWG27S: CMA: Assigning 640MB CMA value for 2GB memory size */
#ifdef CONFIG_SDRAM_SIZE_2GB
	reg[0] = cpu_to_fdt32(0x0);
	reg[1] = cpu_to_fdt32(0x28000000);
	ret= fdt_setprop(fdt,fdt_path_offset(fdt, "/reserved-memory/linux,cma"), "size", reg, sizeof(reg));
	if(ret<0){
		printf("Kernel FTD CMA update is failed");
	}
#endif
}

int board_init(void)
{

#ifdef CONFIG_SNVS_SEC_SC_AUTO
	{
		int ret = snvs_security_sc_init();

		if (ret)
			return ret;
	}
#endif

	return 0;
}

void board_quiesce_devices(void)
{
	const char *power_on_devices[] = {
		"dma_lpuart4",
	};

	if (IS_ENABLED(CONFIG_XEN)) {
		/* Clear magic number to let xen know uboot is over */
		writel(0x0, (void __iomem *)0x80000000);
		return;
	}

	power_off_pd_devices(power_on_devices, ARRAY_SIZE(power_on_devices));
}

/*
 * Board specific reset that is system reset.
 */
void reset_cpu(ulong addr)
{
	sc_pm_reboot(-1, SC_PM_RESET_TYPE_COLD);
	while(1);
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	return 0;
}
#endif

int board_mmc_get_env_dev(int devno)
{
	/* Use EMMC */
	if (IS_ENABLED(CONFIG_XEN))
		return 0;

	return devno;
}

int mmc_map_to_kernel_blk(int dev_no)
{
	/* Use EMMC */
	if (IS_ENABLED(CONFIG_XEN))
		return 0;

	return dev_no;
}

extern uint32_t _end_ofs;
int board_late_init(void)
{
	print_board_info();
	/* IWG27S: WIFI: Correcting WIFI Power On Sequence */
	wifi_pwr_seq();
	usb_otg_pwr_enable();
	usb_hub_pwr_enable();

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "iW-RainboW-G27S-i.MX8QM/QP Pico ITX SBC");
	env_set("board_rev", "iW-PRGEE-AP-01-R1.X");
#endif

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif

	/* IWG27S: Updating iMX8QM/QP FDT file based on CPU */
	if(is_imx8qp())
		env_set("fdt_file","imx8qp-iwg27s.dtb");
	else
		env_set("fdt_file","imx8qm-iwg27s.dtb");

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
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
