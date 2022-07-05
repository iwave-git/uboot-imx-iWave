/*
 * Copyright (C) 2022-23 iWave Systems Technologies Pvt Ltd.
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
#include <dm.h>
#include <dm/device_compat.h>
#include <mipi_dsi.h>
#include <panel.h>
#include <asm/gpio.h>
#include <i2c.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <env.h>

/* I2C registers of the Atmel microcontroller. */
enum REG_ADDR {
       REG_ID = 0x80,
       REG_PORTA, // BIT(2) for horizontal flip, BIT(3) for vertical flip
       REG_PORTB,
       REG_PORTC,
       REG_PORTD,
       REG_POWERON,
       REG_PWM,
       REG_DDRA,
       REG_DDRB,
       REG_DDRC,
       REG_DDRD,
       REG_TEST,
       REG_WR_ADDRL,
       REG_WR_ADDRH,
       REG_READH,
       REG_READL,
       REG_WRITEH,
       REG_WRITEL,
       REG_ID2,
};

/* DSI D-PHY Layer Registers */
#define D0W_DPHYCONTTX 0x0004
#define CLW_DPHYCONTRX 0x0020
#define D0W_DPHYCONTRX 0x0024
#define D1W_DPHYCONTRX 0x0028
#define COM_DPHYCONTRX 0x0038
#define CLW_CNTRL 0x0040
#define D0W_CNTRL 0x0044
#define D1W_CNTRL 0x0048
#define DFTMODE_CNTRL 0x0054

/* DSI PPI Layer Registers */
#define PPI_STARTPPI 0x0104
#define PPI_BUSYPPI 0x0108
#define PPI_LINEINITCNT 0x0110
#define PPI_LPTXTIMECNT 0x0114
#define PPI_CLS_ATMR 0x0140
#define PPI_D0S_ATMR 0x0144
#define PPI_D1S_ATMR 0x0148
#define PPI_D0S_CLRSIPOCOUNT 0x0164
#define PPI_D1S_CLRSIPOCOUNT 0x0168
#define CLS_PRE 0x0180
#define D0S_PRE 0x0184
#define D1S_PRE 0x0188
#define CLS_PREP 0x01A0
#define D0S_PREP 0x01A4
#define D1S_PREP 0x01A8
#define CLS_ZERO 0x01C0
#define D0S_ZERO 0x01C4
#define D1S_ZERO 0x01C8
#define PPI_CLRFLG 0x01E0
#define PPI_CLRSIPO 0x01E4
#define HSTIMEOUT 0x01F0
#define HSTIMEOUTENABLE 0x01F4

/* DSI Protocol Layer Registers */
#define DSI_STARTDSI 0x0204
#define DSI_BUSYDSI 0x0208
#define DSI_LANEENABLE 0x0210
#define DSI_LANEENABLE_CLOCK BIT(0)
#define DSI_LANEENABLE_D0 BIT(1)
#define DSI_LANEENABLE_D1 BIT(2)

#define DSI_LANESTATUS0 0x0214
#define DSI_LANESTATUS1 0x0218
#define DSI_INTSTATUS 0x0220
#define DSI_INTMASK 0x0224
#define DSI_INTCLR 0x0228
#define DSI_LPTXTO 0x0230
#define DSI_MODE 0x0260
#define DSI_PAYLOAD0 0x0268
#define DSI_PAYLOAD1 0x026C
#define DSI_SHORTPKTDAT 0x0270
#define DSI_SHORTPKTREQ 0x0274
#define DSI_BTASTA 0x0278
#define DSI_BTACLR 0x027C

/* DSI General Registers */
#define DSIERRCNT 0x0300
#define DSISIGMOD 0x0304

/* DSI Application Layer Registers */
#define APLCTRL 0x0400
#define APLSTAT 0x0404
#define APLERR 0x0408
#define PWRMOD 0x040C
#define RDPKTLN 0x0410
#define PXLFMT 0x0414
#define MEMWRCMD 0x0418

/* LCDC/DPI Host Registers */
#define LCDCTRL 0x0420
#define HSR 0x0424
#define HDISPR 0x0428
#define VSR 0x042C
#define VDISPR 0x0430
#define VFUEN 0x0434

/* DBI-B Host Registers */
#define DBIBCTRL 0x0440

/* SPI Master Registers */
#define SPICMR 0x0450
#define SPITCR 0x0454

/* System Controller Registers */
#define SYSSTAT 0x0460
#define SYSCTRL 0x0464
#define SYSPLL1 0x0468
#define SYSPLL2 0x046C
#define SYSPLL3 0x0470
#define SYSPMCTRL 0x047C

/* GPIO Registers */
#define GPIOC 0x0480
#define GPIOO 0x0484
#define GPIOI 0x0488

/* I2C Registers */
#define I2CCLKCTRL 0x0490

/* Chip/Rev Registers */
#define IDREG 0x04A0

/* Debug Registers */
#define WCMDQUEUE 0x0500

struct rpi_panel_priv {
	struct gpio_desc reset;
	struct udevice *i2c_dev;
	unsigned int lanes;
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;
};

static const struct display_timing default_timing = {
#define PIXEL_CLOCK ((2000000000 / 3) / 24)
#define VREFRESH    60
#define VTOTAL      (480 + 7 + 2 + 21)
#define HACT        800
#define HSW         5
#define HBP         46
#define HFP         ((PIXEL_CLOCK / (VTOTAL * VREFRESH)) - (HACT + HSW + HBP))

        .pixelclock.typ         = 33300000,
        .hactive.typ            = HACT,
        .hfront_porch.typ       = HFP,
        .hback_porch.typ        = HBP,
        .hsync_len.typ          = HSW,
        .vactive.typ            = 480,
        .vfront_porch.typ       = 7,
        .vback_porch.typ        = 21,
        .vsync_len.typ          = 2,
};

static int rpi_touchscreen_write_eic(struct mipi_dsi_device *dsi, u16 reg,
				     u32 val)
{
	u8 msg[] = {
		reg, reg >> 8, val, val >> 8, val >> 16, val >> 24,
	};

	mipi_dsi_generic_write(dsi, msg, sizeof(msg));

	return 0;
}

static void rpi_touchscreen_i2c_write_eic(struct udevice *i2c_dev,
                                     u8 reg, u8 val)
{
	int ret;
	ret = dm_i2c_reg_write(i2c_dev, reg, val);
}

static u8 rpi_touchscreen_i2c_read(struct udevice *i2c_dev, u8 reg)
{
	int ret;
	uint8_t valb;
       	ret = dm_i2c_read(i2c_dev, reg, &valb, 1);
        if (ret) {
                return -EIO;
        }
	return valb;
}

static int rpi_enable(struct udevice *dev)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *dsi = plat->device;
	struct rpi_panel_priv *priv = dev_get_priv(dev);
	int i;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;
	rpi_touchscreen_i2c_write_eic(priv->i2c_dev, REG_POWERON, 1);
        /* Wait for nPWRDWN to go low to indicate poweron is done. */
        for (i = 0; i < 100; i++) {
                if (rpi_touchscreen_i2c_read(priv->i2c_dev, REG_PORTB) & 1)
                        break;
        	}

	rpi_touchscreen_write_eic(dsi, DSI_LANEENABLE,
				  DSI_LANEENABLE_CLOCK | DSI_LANEENABLE_D0);
	rpi_touchscreen_write_eic(dsi, PPI_D0S_CLRSIPOCOUNT, 0x05);
	rpi_touchscreen_write_eic(dsi, PPI_D1S_CLRSIPOCOUNT, 0x05);
	rpi_touchscreen_write_eic(dsi, PPI_D0S_ATMR, 0x00);
	rpi_touchscreen_write_eic(dsi, PPI_D1S_ATMR, 0x00);
	rpi_touchscreen_write_eic(dsi, PPI_LPTXTIMECNT, 0x03);

	rpi_touchscreen_write_eic(dsi, SPICMR, 0x00);
	rpi_touchscreen_write_eic(dsi, LCDCTRL, 0x00100150);
	rpi_touchscreen_write_eic(dsi, SYSCTRL, 0x040f);
	mdelay(100);

	rpi_touchscreen_write_eic(dsi, PPI_STARTPPI, 0x01);
	rpi_touchscreen_write_eic(dsi, DSI_STARTDSI, 0x01);
	mdelay(100);
	rpi_touchscreen_i2c_write_eic(priv->i2c_dev, REG_PWM, 255);
	rpi_touchscreen_i2c_write_eic(priv->i2c_dev, REG_PORTA, BIT(2));
	return 0;
}

static int rpi_panel_enable_backlight(struct udevice *dev)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	int ret;

	ret = mipi_dsi_attach(device);
	if (ret < 0)
		return ret;

	return rpi_enable(dev);
}

static int rpi_panel_get_display_timing(struct udevice *dev,
					    struct display_timing *timings)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	struct rpi_panel_priv *priv = dev_get_priv(dev);

	memcpy(timings, &default_timing, sizeof(*timings));

	/* fill characteristics of DSI data link */
	if (device) {
		device->lanes = priv->lanes;
		device->format = priv->format;
		device->mode_flags = priv->mode_flags;
	}

	return 0;
}

static int rpi_panel_probe(struct udevice *dev)
{
	struct rpi_panel_priv *priv = dev_get_priv(dev);
	int ret;
	u32 video_mode;

	priv->format = MIPI_DSI_FMT_RGB888;
	priv->mode_flags =  MIPI_DSI_MODE_VSYNC_FLUSH | MIPI_DSI_MODE_VIDEO |
		MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
		MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_EOT_PACKET;

	ret = dev_read_u32(dev, "video-mode", &video_mode);
	if (!ret) {
		switch (video_mode) {
		case 0:
			/* burst mode */
			priv->mode_flags |= MIPI_DSI_MODE_VIDEO_BURST;
			break;
		case 1:
			/* non-burst mode with sync event */
			break;
		case 2:
			/* non-burst mode with sync pulse */
			priv->mode_flags |= MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
			break;
		default:
			dev_warn(dev, "invalid video mode %d\n", video_mode);
			break;
		}
	}

	ret = dev_read_u32(dev, "dsi-lanes", &priv->lanes);
	if (ret) {
		printf("Failed to get dsi-lanes property (%d)\n", ret);
		return ret;
	}

	ret = dm_i2c_probe(dev_get_parent(dev), 0x45, 0, &priv->i2c_dev);
	if (ret) {
		dev_err(dev, "Can't find i2c device id=0x45\n");
		return -ENODEV;
	}

	/* Make register 0 for power on sequence*/
	rpi_touchscreen_i2c_write_eic(priv->i2c_dev, REG_POWERON, 0);
	return 0;
}

static int rpi_panel_disable(struct udevice *dev)
{
	struct rpi_panel_priv *priv = dev_get_priv(dev);
	rpi_touchscreen_i2c_write_eic(priv->i2c_dev, REG_PWM, 0);
	rpi_touchscreen_i2c_write_eic(priv->i2c_dev, REG_POWERON, 0);
	return 0;
}

static const struct panel_ops rpi_panel_ops = {
	.enable_backlight = rpi_panel_enable_backlight,
	.get_display_timing = rpi_panel_get_display_timing,
};

static const struct udevice_id rpi_panel_ids[] = {
	{ .compatible = "rpi,panelv1" },
	{ }
};

U_BOOT_DRIVER(rpi_panel) = {
	.name			  = "rpi_panel",
	.id			  = UCLASS_PANEL,
	.of_match		  = rpi_panel_ids,
	.ops			  = &rpi_panel_ops,
	.probe			  = rpi_panel_probe,
	.remove			  = rpi_panel_disable,
	.plat_auto		  = sizeof(struct mipi_dsi_panel_plat),
	.priv_auto		  = sizeof(struct rpi_panel_priv),
};
