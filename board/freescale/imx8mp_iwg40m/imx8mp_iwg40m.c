// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021 iWave System Technologies Pvt Ltd. 
 */

#include <common.h>
#include <errno.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/io.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <asm/arch/imx8mp_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch/clock.h>
#include <spl.h>
#include <asm/mach-imx/dma.h>
#include <power/pmic.h>
#include "../common/tcpc.h"
#include <usb.h>
#include <dwc3-uboot.h>
#include <mmc.h>
#include <asm/setup.h>
#include <asm/bootm.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

static iomux_v3_cfg_t const uart_pads[] = {
	MX8MP_PAD_UART4_RXD__UART4_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX8MP_PAD_UART4_TXD__UART4_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	MX8MP_PAD_GPIO1_IO02__WDOG1_WDOG_B  | MUX_PAD_CTRL(WDOG_PAD_CTRL),
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

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	return 0;
}
#endif

#ifdef CONFIG_MXC_SPI
#define SPI_PAD_CTRL    (PAD_CTL_DSE2 | PAD_CTL_HYS)
static iomux_v3_cfg_t const ecspi1_pads[] = {
        MX8MP_PAD_ECSPI1_SCLK__ECSPI1_SCLK | MUX_PAD_CTRL(SPI_PAD_CTRL),
        MX8MP_PAD_ECSPI1_MOSI__ECSPI1_MOSI | MUX_PAD_CTRL(SPI_PAD_CTRL),
        MX8MP_PAD_ECSPI1_MISO__ECSPI1_MISO | MUX_PAD_CTRL(SPI_PAD_CTRL),
        MX8MP_PAD_ECSPI1_SS0__GPIO5_IO09 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_spi(void)
{
        imx_iomux_v3_setup_multiple_pads(ecspi1_pads, ARRAY_SIZE(ecspi1_pads));

        init_clk_ecspi(0);
}

int board_spi_cs_gpio()
{
        return IMX_GPIO_NR(5, 9);
}
#endif

#ifdef CONFIG_FEC_MXC
#define FEC_RST_PAD IMX_GPIO_NR(1, 6)
static iomux_v3_cfg_t const fec1_rst_pads[] = {
	MX8MP_PAD_GPIO1_IO06__GPIO1_IO06 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_iomux_fec(void)
{
	imx_iomux_v3_setup_multiple_pads(fec1_rst_pads,
					 ARRAY_SIZE(fec1_rst_pads));

	gpio_request(FEC_RST_PAD, "fec1_rst");
	gpio_direction_output(FEC_RST_PAD, 0);
	mdelay(15);
	gpio_direction_output(FEC_RST_PAD, 1);
	mdelay(100);
}

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	setup_iomux_fec();

	/* Enable RGMII TX clk output */
	setbits_le32(&gpr->gpr[1], BIT(22));

	//return set_clk_enet(ENET_125MHZ);
	return 0;
}
#endif

#ifdef CONFIG_DWC_ETH_QOS

#define EQOS_RST_PAD IMX_GPIO_NR(1, 7)
static iomux_v3_cfg_t const eqos_rst_pads[] = {
	MX8MP_PAD_GPIO1_IO07__GPIO1_IO07 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_iomux_eqos(void)
{
	imx_iomux_v3_setup_multiple_pads(eqos_rst_pads,
					 ARRAY_SIZE(eqos_rst_pads));

	gpio_request(EQOS_RST_PAD, "eqos_rst");
	gpio_direction_output(EQOS_RST_PAD, 0);
	mdelay(15);
	gpio_direction_output(EQOS_RST_PAD, 1);
	mdelay(100);
}

static int setup_eqos(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	setup_iomux_eqos();

	/* set INTF as RGMII, enable RGMII TXC clock */
	clrsetbits_le32(&gpr->gpr[1],
			IOMUXC_GPR_GPR1_GPR_ENET_QOS_INTF_SEL_MASK, BIT(16));
	setbits_le32(&gpr->gpr[1], BIT(19) | BIT(21));

	return set_clk_eqos(ENET_125MHZ);
}
#endif

#if defined(CONFIG_FEC_MXC) || defined(CONFIG_DWC_ETH_QOS)
int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);
	return 0;
}
#endif

static iomux_v3_cfg_t const usb_vbus_pads[] = {
        MX8MP_PAD_GPIO1_IO13__GPIO1_IO13 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static int init_usb_vbus(void)
{
	int ret;
	imx_iomux_v3_setup_multiple_pads(usb_vbus_pads, ARRAY_SIZE(usb_vbus_pads));
	gpio_request(IMX_GPIO_NR(1, 13), "usb_vbus");
	gpio_direction_output(IMX_GPIO_NR(1, 13), 0);
	mdelay(10);
	return ret;
}

#define USB_TYPEC_SEL IMX_GPIO_NR(4, 2)

static iomux_v3_cfg_t ss_mux_gpio[] = {
	MX8MP_PAD_SAI1_RXD0__GPIO4_IO02 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

void ss_mux_select(enum typec_cc_polarity pol)
{
	if (pol == TYPEC_POLARITY_CC1)
		gpio_direction_output(USB_TYPEC_SEL, 0);
	else
		gpio_direction_output(USB_TYPEC_SEL, 1);
}

static int setup_typec(void)
{
	int ret;

        imx_iomux_v3_setup_multiple_pads(ss_mux_gpio, ARRAY_SIZE(ss_mux_gpio));
        gpio_request(USB_TYPEC_SEL, "typec_sel");
	gpio_direction_output(USB_TYPEC_SEL, 0);

}

#ifdef CONFIG_USB_DWC3

#define USB_PHY_CTRL0			0xF0040
#define USB_PHY_CTRL0_REF_SSP_EN	BIT(2)

#define USB_PHY_CTRL1			0xF0044
#define USB_PHY_CTRL1_RESET		BIT(0)
#define USB_PHY_CTRL1_COMMONONN		BIT(1)
#define USB_PHY_CTRL1_ATERESET		BIT(3)
#define USB_PHY_CTRL1_VDATSRCENB0	BIT(19)
#define USB_PHY_CTRL1_VDATDETENB0	BIT(20)

#define USB_PHY_CTRL2			0xF0048
#define USB_PHY_CTRL2_TXENABLEN0	BIT(8)

#define USB_PHY_CTRL6			0xF0058

#define HSIO_GPR_BASE                               (0x32F10000U)
#define HSIO_GPR_REG_0                              (HSIO_GPR_BASE)
#define HSIO_GPR_REG_0_USB_CLOCK_MODULE_EN_SHIFT    (1)
#define HSIO_GPR_REG_0_USB_CLOCK_MODULE_EN          (0x1U << HSIO_GPR_REG_0_USB_CLOCK_MODULE_EN_SHIFT)


static struct dwc3_device dwc3_device_data = {
#ifdef CONFIG_SPL_BUILD
	.maximum_speed = USB_SPEED_HIGH,
#else
	.maximum_speed = USB_SPEED_SUPER,
#endif
	.base = USB1_BASE_ADDR,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 0,
	.power_down_scale = 2,
};

int usb_gadget_handle_interrupts(void)
{
	dwc3_uboot_handle_interrupt(0);
	return 0;
}

static void dwc3_nxp_usb_phy_init(struct dwc3_device *dwc3)
{
	u32 RegData;

	/* enable usb clock via hsio gpr */
	RegData = readl(HSIO_GPR_REG_0);
	RegData |= HSIO_GPR_REG_0_USB_CLOCK_MODULE_EN;
	writel(RegData, HSIO_GPR_REG_0);

	/* USB3.0 PHY signal fsel for 100M ref */
	RegData = readl(dwc3->base + USB_PHY_CTRL0);
	RegData = (RegData & 0xfffff81f) | (0x2a<<5);
	writel(RegData, dwc3->base + USB_PHY_CTRL0);

	RegData = readl(dwc3->base + USB_PHY_CTRL6);
	RegData &=~0x1;
	writel(RegData, dwc3->base + USB_PHY_CTRL6);

	RegData = readl(dwc3->base + USB_PHY_CTRL1);
	RegData &= ~(USB_PHY_CTRL1_VDATSRCENB0 | USB_PHY_CTRL1_VDATDETENB0 |
			USB_PHY_CTRL1_COMMONONN);
	RegData |= USB_PHY_CTRL1_RESET | USB_PHY_CTRL1_ATERESET;
	writel(RegData, dwc3->base + USB_PHY_CTRL1);

	RegData = readl(dwc3->base + USB_PHY_CTRL0);
	RegData |= USB_PHY_CTRL0_REF_SSP_EN;
	writel(RegData, dwc3->base + USB_PHY_CTRL0);

	RegData = readl(dwc3->base + USB_PHY_CTRL2);
	RegData |= USB_PHY_CTRL2_TXENABLEN0;
	writel(RegData, dwc3->base + USB_PHY_CTRL2);

	RegData = readl(dwc3->base + USB_PHY_CTRL1);
	RegData &= ~(USB_PHY_CTRL1_RESET | USB_PHY_CTRL1_ATERESET);
	writel(RegData, dwc3->base + USB_PHY_CTRL1);
}
#endif

#if defined(CONFIG_USB_DWC3) || defined(CONFIG_USB_XHCI_IMX8M)

int board_usb_init(int index, enum usb_init_type init)
{
        int ret = 0;
        uint8_t vbus_high = 0;
        struct udevice *bus;
        struct udevice **i2c_dev;
        uint8_t const value[3] = {0x07, 0x02, 0x03};
        uint8_t buf[1];
        uint8_t tog_ss = 0;

        uclass_get_device_by_seq(UCLASS_I2C, 1, &bus);
        i2c_get_chip(bus, 0x22, 1, i2c_dev);
        dm_i2c_write(*i2c_dev, 0x0B, value, 1);
        dm_i2c_write(*i2c_dev, 0x08, value+1, 1);
        dm_i2c_write(*i2c_dev, 0x08, value+2, 1);
        mdelay(100);
        dm_i2c_read(*i2c_dev, 0x3d, buf, 1);
        tog_ss = (buf[0] & 0x38) >> 3;
        dm_i2c_read(*i2c_dev, 0x40, buf, 1);
        vbus_high = (buf[0] & 0x80) >> 7;
        if (tog_ss % 2)
                ss_mux_select(TYPEC_POLARITY_CC1);
        else
                ss_mux_select(TYPEC_POLARITY_CC2);

        if (index == 0 && init == USB_INIT_DEVICE) {
                imx8m_usb_power(index, true);
		gpio_direction_output(IMX_GPIO_NR(1, 13), 0);
                dwc3_nxp_usb_phy_init(&dwc3_device_data);
                return dwc3_uboot_init(&dwc3_device_data);
        } else if (index == 0 && init == USB_INIT_HOST) {
                /*Since vbus is shared between type-c port and usb 3.0 upper port in carrier card,
                 *don't drive the vbus if host connected to type-c port
                 *is already driving the vbus high.
                 */
                if (!vbus_high)
                        gpio_direction_output(IMX_GPIO_NR(1, 13), 1);
                return ret;
        }

        return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;
	if (index == 0 && init == USB_INIT_DEVICE) {
		dwc3_uboot_exit(index);
		imx8m_usb_power(index, false);
	} else if (index == 0 && init == USB_INIT_HOST) {
		gpio_direction_output(IMX_GPIO_NR(1, 13), 0);
	}
	return ret;
}
#endif

#define FSL_SIP_GPC			0xC2000000
#define FSL_SIP_CONFIG_GPC_PM_DOMAIN	0x3
#define DISPMIX				13
#define MIPI				15

int board_init(void)
{
        setup_typec();

#ifdef CONFIG_MXC_SPI
        setup_spi();
#endif

#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

#ifdef CONFIG_DWC_ETH_QOS
	/* clock, pin, gpr */
	setup_eqos();
#endif

#if defined(CONFIG_USB_DWC3) || defined(CONFIG_USB_XHCI_IMX8M)
	init_usb_clk();
	init_usb_vbus();
#endif

	/* enable the dispmix & mipi phy power domain */
	call_imx_sip(FSL_SIP_GPC, FSL_SIP_CONFIG_GPC_PM_DOMAIN, DISPMIX, true, 0);
	call_imx_sip(FSL_SIP_GPC, FSL_SIP_CONFIG_GPC_PM_DOMAIN, MIPI, true, 0);

	return 0;
}

#define BCONFIG_0 IMX_GPIO_NR(2, 8)
#define BCONFIG_1 IMX_GPIO_NR(2, 9)
#define BCONFIG_2 IMX_GPIO_NR(2, 11)
#define BCONFIG_3 IMX_GPIO_NR(2, 10)
#define BCONFIG_4 IMX_GPIO_NR(1, 1)

int board_config_pads[] = {
        BCONFIG_0,
        BCONFIG_1,
        BCONFIG_2,
        BCONFIG_3,
        BCONFIG_4,
};

static iomux_v3_cfg_t const board_cfg[] = {
        MX8MP_PAD_SD1_DATA6__GPIO2_IO08 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX8MP_PAD_SD1_DATA7__GPIO2_IO09 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX8MP_PAD_SD1_STROBE__GPIO2_IO11| MUX_PAD_CTRL(NO_PAD_CTRL),
	MX8MP_PAD_SD1_RESET_B__GPIO2_IO10 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX8MP_PAD_GPIO1_IO01__GPIO1_IO01 | MUX_PAD_CTRL(NO_PAD_CTRL),
};


static void print_board_info(void)
{
        struct tag_serialnr serialnr;
        int i ,bom_rev, pcb_rev;;
        
	/*IWG40M : Adding CPU Unique ID read support*/
        get_board_serial(&serialnr);

        imx_iomux_v3_setup_multiple_pads(board_cfg,
                                         ARRAY_SIZE(board_cfg));

        for (i=0;i<ARRAY_SIZE(board_config_pads);i++) {
                if(i<2) {
                        gpio_request(board_config_pads[i], "SOM-Revision-GPIO");
                        gpio_direction_input(board_config_pads[i]);
                        pcb_rev |= (gpio_get_value(board_config_pads[i]) << i);
                } else {
                        gpio_request(board_config_pads[i], "SOM-Revision-GPIO");
                        gpio_direction_input(board_config_pads[i]);
                        bom_rev |= (gpio_get_value(board_config_pads[i]) << (i-2));
                }
        }

        printf ("Board Info:\n");
        printf ("\tBSP Version     : %s\n", BSP_VERSION);
        printf ("\tSOM Version     : iW-PRGQZ-AP-01-R%x.%x\n",pcb_rev+1,bom_rev);
        printf ("\tCPU Unique ID   : 0x%08X%08X\n",serialnr.high,serialnr.low);
        printf ("\n");
}

int checkboard(void)
{
       puts("Board: i.MX8MPlus SMARC SOM\n");
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

void iwg40m_fdt_update(void *fdt)
{
        uint32_t reg[2];
        int ret=0;

	if (!strcmp("enabled", env_get("usb3.0_top"))){
		fdt_setprop_string(fdt, fdt_path_offset(fdt, "/usb@32f10100/dwc3@38100000"), "dr_mode", "host");
		fdt_setprop_string(fdt, fdt_path_offset(fdt, "/vcc5v0-typec-regulator"), "regulator-boot-on", NULL);
		fdt_setprop_string(fdt, fdt_path_offset(fdt, "/vcc5v0-typec-regulator"), "regulator-always-on", NULL);
		fdt_delprop(fdt, fdt_path_offset(fdt, "/usb@32f10100/dwc3@38100000"), "hnp-disable");
		fdt_delprop(fdt, fdt_path_offset(fdt, "/usb@32f10100/dwc3@38100000"), "srp-disable");
		fdt_delprop(fdt, fdt_path_offset(fdt, "/usb@32f10100/dwc3@38100000"), "adp-disable");
		fdt_delprop(fdt, fdt_path_offset(fdt, "/usb@32f10100/dwc3@38100000"), "usb-role-switch");
	}

#ifdef CONFIG_SDRAM_SIZE_2GB
        reg[0] = cpu_to_fdt32(0x0);
        reg[1] = cpu_to_fdt32(0x1E000000);
        ret= fdt_setprop(fdt,fdt_path_offset(fdt, "/reserved-memory/linux,cma"), "size", reg, sizeof(reg));
        if(ret<0){
                printf("Kernel FTD CMA memory range update is failed\n");
        }
#endif

}

int board_late_init(void)
{
	print_board_info();
#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif
#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
        env_set("board_name", "iW-RainboW-G40M-i.MX8MP SMARC");
        env_set("board_rev", "iW-PRGQZ-AP-01-R2.X");
#endif

	return 0;
}

#ifdef CONFIG_IMX_BOOTAUX
ulong board_get_usable_ram_top(ulong total_size)
{
	/* Reserve 16M memory used by M core vring/buffer, which begins at 16MB before optee */
	if (rom_pointer[1])
		return gd->ram_top - SZ_16M;

	return gd->ram_top;
}
#endif

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

#ifdef CONFIG_SPL_MMC_SUPPORT

#define UBOOT_RAW_SECTOR_OFFSET 0x40
unsigned long spl_mmc_get_uboot_raw_sector(struct mmc *mmc)
{
	u32 boot_dev = spl_boot_device();
	switch (boot_dev) {
		case BOOT_DEVICE_MMC2:
			return CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR - UBOOT_RAW_SECTOR_OFFSET;
		default:
			return CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR;
	}
}
#endif
