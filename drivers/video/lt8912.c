// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 iWave Systems Technologies Pvt. Ltd.
 *
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

struct lt8912b_priv {
	unsigned int addr;
	unsigned int addr_cec;
	unsigned int addr_cec2;
	unsigned int lanes;
	enum mipi_dsi_pixel_format format;
	int sink_is_hdmi;
	unsigned long mode_flags;
	struct udevice *cec_dev;
	struct udevice *cec_dev2;
	struct gpio_desc bk_lgt;
	struct gpio_desc bk_lgt_en;
};

static const struct display_timing default_timing_lvds = {
        .pixelclock.typ         = 70000000,
        .hactive.typ            = 1280,
        .hfront_porch.typ       = 70,
        .hback_porch.typ        = 70,
        .hsync_len.typ          = 20,
        .vactive.typ            = 800,
        .vfront_porch.typ       = 8,
        .vback_porch.typ        = 8,
        .vsync_len.typ          = 7,
};

static const struct display_timing default_timing_hdmi = {
	.pixelclock.typ         = 148500000,
	.hactive.typ            = 1920,
	.hfront_porch.typ       = 88,
	.hback_porch.typ        = 148,
	.hsync_len.typ          = 44,
	.vactive.typ            = 1080,
	.vfront_porch.typ       = 4,
	.vback_porch.typ        = 36,
	.vsync_len.typ          = 5,
};


static int lt8912b_i2c_reg_write(struct udevice *dev, uint addr, uint data)
{
	uint8_t valb;
	int err;

	valb = data;

	err = dm_i2c_write(dev, addr, &valb, 1);
	return err;
}

static int lt8912b_i2c_reg_read(struct udevice *dev, uint8_t addr, uint8_t *data)
{
	uint8_t valb;
	int err;

	err = dm_i2c_read(dev, addr, &valb, 1);
	if (err)
		return err;

	*data = (int)valb;
	return 0;
}

void lvds_init(struct lt8912b_priv *priv, struct udevice *dev)
{
	lt8912b_i2c_reg_write(priv->cec_dev,0x10,0x01);
	lt8912b_i2c_reg_write(priv->cec_dev,0x11,0x04);
	lt8912b_i2c_reg_write(priv->cec_dev,0x18,0x14);
	lt8912b_i2c_reg_write(priv->cec_dev,0x19,0x07);
	lt8912b_i2c_reg_write(priv->cec_dev,0x1c,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x1d,0x05);
	lt8912b_i2c_reg_write(priv->cec_dev,0x2f,0x0c);
	lt8912b_i2c_reg_write(priv->cec_dev,0x34,0xa0);
	lt8912b_i2c_reg_write(priv->cec_dev,0x35,0x05);
	lt8912b_i2c_reg_write(priv->cec_dev,0x36,0x37);
	lt8912b_i2c_reg_write(priv->cec_dev,0x37,0x03);
	lt8912b_i2c_reg_write(priv->cec_dev,0x38,0x8);
	lt8912b_i2c_reg_write(priv->cec_dev,0x39,0x0);
	lt8912b_i2c_reg_write(priv->cec_dev,0x3a,0x8);
	lt8912b_i2c_reg_write(priv->cec_dev,0x3b,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x3c,0x46);
	lt8912b_i2c_reg_write(priv->cec_dev,0x3d,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x3e,0x46);
	lt8912b_i2c_reg_write(priv->cec_dev,0x3f,0x00);

	lt8912b_i2c_reg_write(priv->cec_dev,0x4e,0xff);
	lt8912b_i2c_reg_write(priv->cec_dev,0x4f,0x56);
	lt8912b_i2c_reg_write(priv->cec_dev,0x50,0x69);
	lt8912b_i2c_reg_write(priv->cec_dev,0x51,0x80);

	lt8912b_i2c_reg_write(priv->cec_dev,0x1f,0x5e);
	lt8912b_i2c_reg_write(priv->cec_dev,0x20,0x01);
	lt8912b_i2c_reg_write(priv->cec_dev,0x21,0x2c);
	lt8912b_i2c_reg_write(priv->cec_dev,0x22,0x01);
	lt8912b_i2c_reg_write(priv->cec_dev,0x23,0xfa);
	lt8912b_i2c_reg_write(priv->cec_dev,0x24,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x25,0xc8);
	lt8912b_i2c_reg_write(priv->cec_dev,0x26,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x27,0x5e);
	lt8912b_i2c_reg_write(priv->cec_dev,0x28,0x01);
	lt8912b_i2c_reg_write(priv->cec_dev,0x29,0x2c);
	lt8912b_i2c_reg_write(priv->cec_dev,0x2a,0x01);
	lt8912b_i2c_reg_write(priv->cec_dev,0x2b,0xfa);
	lt8912b_i2c_reg_write(priv->cec_dev,0x2c,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x2d,0xc8);
	lt8912b_i2c_reg_write(priv->cec_dev,0x2e,0x00);	
	lt8912b_i2c_reg_write(priv->cec_dev,0x42,0x64);
	lt8912b_i2c_reg_write(priv->cec_dev,0x43,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x44,0x04);
	lt8912b_i2c_reg_write(priv->cec_dev,0x45,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x46,0x59);
	lt8912b_i2c_reg_write(priv->cec_dev,0x47,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x48,0xf2);
	lt8912b_i2c_reg_write(priv->cec_dev,0x49,0x06);
	lt8912b_i2c_reg_write(priv->cec_dev,0x4a,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x4b,0x72);
	lt8912b_i2c_reg_write(priv->cec_dev,0x4c,0x45);
	lt8912b_i2c_reg_write(priv->cec_dev,0x4d,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x52,0x08);
	lt8912b_i2c_reg_write(priv->cec_dev,0x53,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x54,0xb2);
	lt8912b_i2c_reg_write(priv->cec_dev,0x55,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x56,0xe4);
	lt8912b_i2c_reg_write(priv->cec_dev,0x57,0x0d);
	lt8912b_i2c_reg_write(priv->cec_dev,0x58,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x59,0xe4);
	lt8912b_i2c_reg_write(priv->cec_dev,0x5a,0x8a);
	lt8912b_i2c_reg_write(priv->cec_dev,0x5b,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x5c,0x34);
	lt8912b_i2c_reg_write(priv->cec_dev,0x1e,0x4f);
	lt8912b_i2c_reg_write(priv->cec_dev,0x51,0x00);

	lt8912b_i2c_reg_write(dev,0x03,0x7f);
	lt8912b_i2c_reg_write(dev,0x03,0xff);
	lt8912b_i2c_reg_write(dev,0x05,0xfb);
	lt8912b_i2c_reg_write(dev,0x05,0xff);

	//core pll bypass
	lt8912b_i2c_reg_write(dev,0x50, 0x24);//cp=50uA
	lt8912b_i2c_reg_write(dev,0x52, 0x04);//loopdiv=0;use second-order PLL
	lt8912b_i2c_reg_write(dev,0x51, 0x2d);//Pix_clk as reference,second order passive LPF PLL
	lt8912b_i2c_reg_write(dev,0x69, 0x0e);//CP_PRESET_DIV_RATIO
	lt8912b_i2c_reg_write(dev,0x69, 0x8e);
	lt8912b_i2c_reg_write(dev,0x6a, 0x00);
	lt8912b_i2c_reg_write(dev,0x6c, 0x94);//RGD_CP_SOFT_K_EN,RGD_CP_SOFT_K[13:8]
	lt8912b_i2c_reg_write(dev,0x6b, 0x17);

	lt8912b_i2c_reg_write(dev,0x04, 0xfb);//core pll reset
	lt8912b_i2c_reg_write(dev,0x04, 0xff);

	//scaler bypass
	lt8912b_i2c_reg_write(dev,0x7f, 0x00);//disable scaler
	//lt8912b_i2c_reg_write(dev,0xa8, 0x33);//0x13 : JEIDA, 0x33:VESA
	lt8912b_i2c_reg_write(dev,0xa8, 0x13);//0x13 : JEIDA, 0x33:VESA

	lt8912b_i2c_reg_write(dev,0x03, 0x7f);
	lt8912b_i2c_reg_write(dev,0x03, 0xff); //mipi rx reset

	lt8912b_i2c_reg_write(dev,0x05, 0xfb);
	lt8912b_i2c_reg_write(dev,0x05, 0xff); //dds reset 

	lt8912b_i2c_reg_write(dev, 0x02, 0xf7);//lvds pll reset
	lt8912b_i2c_reg_write(dev, 0x02, 0xff);
	lt8912b_i2c_reg_write(dev, 0x03, 0xcb);//scaler module reset
	lt8912b_i2c_reg_write(dev, 0x03, 0xfb);//lvds tx module reset
	lt8912b_i2c_reg_write(dev, 0x03, 0xff);

	lt8912b_i2c_reg_write(dev, 0x44, 0x30); //enbale lvds output

	/* Enable Backlight */
	dm_gpio_set_value(&priv->bk_lgt_en, true);
	dm_gpio_set_value(&priv->bk_lgt, true);
}

void hdmi_init(struct lt8912b_priv *priv, struct udevice *dev)
{
	/* DigitalClockEn */
	lt8912b_i2c_reg_write(dev, 0x08, 0xff);
	lt8912b_i2c_reg_write(dev, 0x09, 0xff);
	lt8912b_i2c_reg_write(dev, 0x0a, 0xff);
	lt8912b_i2c_reg_write(dev, 0x0b, 0x7c);
	lt8912b_i2c_reg_write(dev, 0x0c, 0xff);

	/* TxAnalog */
	lt8912b_i2c_reg_write(dev, 0x31, 0x71);
	lt8912b_i2c_reg_write(dev, 0x32, 0x71);
	lt8912b_i2c_reg_write(dev, 0x33, 0x03);
	lt8912b_i2c_reg_write(dev, 0x37, 0x00);
	lt8912b_i2c_reg_write(dev, 0x38, 0x26);
	lt8912b_i2c_reg_write(dev, 0x60, 0x82);

	/* CbusAnalog */
	lt8912b_i2c_reg_write(dev, 0x39, 0x45);
	lt8912b_i2c_reg_write(dev, 0x3a, 0x00);
	lt8912b_i2c_reg_write(dev, 0x3b, 0x00);

	/* HDMIPllAnalog */
	lt8912b_i2c_reg_write(dev, 0x44, 0x31);
	lt8912b_i2c_reg_write(dev, 0x55, 0x44);
	lt8912b_i2c_reg_write(dev, 0x57, 0x01);
	lt8912b_i2c_reg_write(dev, 0x5a, 0x02);

	/* MIPIAnalog */
	lt8912b_i2c_reg_write(dev, 0x3e, 0xce);
	lt8912b_i2c_reg_write(dev, 0x3f, 0xd4);
	lt8912b_i2c_reg_write(dev, 0x41, 0x3c);

	/* MipiBasicSet */
	lt8912b_i2c_reg_write(priv->cec_dev, 0x12, 0x04);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x13, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x14, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x15, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x1a, 0x03);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x1b, 0x03);

	/* MIPIDig */
	lt8912b_i2c_reg_write(priv->cec_dev, 0x10, 0x01);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x11, 0x0a);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x18, 0x2c);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x19, 0x05);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x1c, 0x80);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x1d, 0x07);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x2f, 0x0c);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x34, 0x98);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x35, 0x08);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x36, 0x65);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x37, 0x04);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x38, 0x24);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x39, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x3a, 0x04);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x3b, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x3c, 0x94);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x3d, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x3e, 0x58);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x3f, 0x00);
	lt8912b_i2c_reg_write(dev, 0xab, 0x01);

	/* DDSConfig */
	lt8912b_i2c_reg_write(priv->cec_dev, 0x4e, 0x6a);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x4f, 0xad);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x50, 0xf3);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x51, 0x80);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x1f, 0x5e);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x20, 0x01);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x21, 0x2c);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x22, 0x01);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x23, 0xfa);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x24, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x25, 0xc8);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x26, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x27, 0x5e);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x28, 0x01);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x29, 0x2c);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x2a, 0x01);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x2b, 0xfa);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x2c, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x2d, 0xc8);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x2e, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x42, 0x64);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x43, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x44, 0x04);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x45, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x46, 0x59);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x47, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x48, 0xf2);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x49, 0x06);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x4a, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x4b, 0x72);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x4c, 0x45);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x4d, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x52, 0x08);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x53, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x54, 0xb2);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x55, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x56, 0xe4);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x57, 0x0d);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x58, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x59, 0xe4);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x5a, 0x8a);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x5b, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x5c, 0x34);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x1e, 0x4f);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x51, 0x00);
	lt8912b_i2c_reg_write(dev, 0xb2, 0x01);

	/* Audio Disable */
	lt8912b_i2c_reg_write(priv->cec_dev2, 0x06, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev2, 0x07, 0x00);
	lt8912b_i2c_reg_write(priv->cec_dev2, 0x34, 0xd2);
	lt8912b_i2c_reg_write(priv->cec_dev2, 0x3c, 0x41);

	/* MIPIRxLogicRes */
	lt8912b_i2c_reg_write(dev, 0x03, 0x7f);
	udelay(10000);
	lt8912b_i2c_reg_write(dev, 0x03, 0xff);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x51, 0x80);
	udelay(10000);
	lt8912b_i2c_reg_write(priv->cec_dev, 0x51, 0x00);
}

static int lt8912b_enable(struct udevice *dev)
{
	struct lt8912b_priv *priv = dev_get_priv(dev);

	/* Wakeup */
	lt8912b_i2c_reg_write(dev,0x02, 0xf7); //lvds pll reset.
	lt8912b_i2c_reg_write(dev,0x08, 0xff);
	/* HPD override */
	lt8912b_i2c_reg_write(dev,0x09, 0xff);
	/* color space */
	lt8912b_i2c_reg_write(dev,0x0a, 0xff);
	/* Fixed */
	lt8912b_i2c_reg_write(dev,0x0b, 0x7c);
	/* HDCP */
	lt8912b_i2c_reg_write(dev,0x0c, 0xff);
	/*Tx Analog*/
	/* Fixed */
	lt8912b_i2c_reg_write(dev,0x31, 0xb1);
	/* V1P2 */
	lt8912b_i2c_reg_write(dev,0x32, 0xb1);
	/* Fixed */
	lt8912b_i2c_reg_write(dev,0x33, 0x0e);
	/* Fixed */
	lt8912b_i2c_reg_write(dev,0x37, 0x00);
	/* Fixed */
	lt8912b_i2c_reg_write(dev,0x38, 0x22);
	/* Fixed */
	lt8912b_i2c_reg_write(dev,0x60, 0x82);
	/*Cbus Analog*/
	/* Fixed */
	lt8912b_i2c_reg_write(dev,0x39, 0x45);
	lt8912b_i2c_reg_write(dev,0x3a, 0x00); //20180718
	lt8912b_i2c_reg_write(dev,0x3b, 0x00);
	/*HDMI Pll Analog*/
	lt8912b_i2c_reg_write(dev,0x44, 0x31);
	/* Fixed */
	lt8912b_i2c_reg_write(dev,0x55, 0x44);
	/* Fixed */
	lt8912b_i2c_reg_write(dev,0x57, 0x01);
	lt8912b_i2c_reg_write(dev,0x5a, 0x02);
	/*MIPI Analog*/
	/* Fixed */
	lt8912b_i2c_reg_write(dev,0x3e, 0xd6);  //0xde.  //0xf6 = pn swap
	lt8912b_i2c_reg_write(dev,0x3f, 0xd4);
	lt8912b_i2c_reg_write(dev,0x41, 0x3c);

	lt8912b_i2c_reg_write(priv->cec_dev,0x12,0x04);
	lt8912b_i2c_reg_write(priv->cec_dev,0x13,0x00);  //0x02 = mipi 2 lane
	lt8912b_i2c_reg_write(priv->cec_dev,0x14,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x15,0x00);
	lt8912b_i2c_reg_write(priv->cec_dev,0x1a,0x03);
	lt8912b_i2c_reg_write(priv->cec_dev,0x1b,0x03);

	if(priv->sink_is_hdmi)
		hdmi_init(priv, dev);
	else
		lvds_init(priv, dev);
	return 0;
}

static int lt8912b_enable_backlight(struct udevice *dev)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	int ret;

	ret = mipi_dsi_attach(device);
	if (ret < 0)
	{
		printf("error %d\n",ret);
		return ret;
	}

	return 0;
}

static int lt8912b_get_display_timing(struct udevice *dev,
					    struct display_timing *timings)
{
        struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
        struct mipi_dsi_device *device = plat->device;
	struct lt8912b_priv *priv = dev_get_priv(dev);

	if(priv->sink_is_hdmi)
		memcpy(timings, &default_timing_hdmi, sizeof(*timings));
	else
		memcpy(timings, &default_timing_lvds, sizeof(*timings));

	/* fill characteristics of DSI data link */
	if (device) {
		device->lanes = priv->lanes;
		device->format = priv->format;
		device->mode_flags = priv->mode_flags;
	}

	return 0;
}

static int lt8912b_probe(struct udevice *dev)
{
        struct lt8912b_priv *priv = dev_get_priv(dev);
	int ret;

	priv->format = MIPI_DSI_FMT_RGB888;

	if (strcmp("lvds", env_get("disp")))
		priv->sink_is_hdmi = 1;
	else
		priv->sink_is_hdmi = 0;

	priv->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
				MIPI_DSI_MODE_EOT_PACKET | MIPI_DSI_MODE_VIDEO_HSE;

	priv->addr  =  dev_read_addr(dev);
	if (priv->addr  == 0)
		return -ENODEV;

	ret = dev_read_u32(dev, "digi,dsi-lanes", &priv->lanes);
	if (ret) {
		dev_err(dev, "Failed to get dsi-lanes property (%d)\n", ret);
		return ret;
	}

	if (priv->lanes < 1 || priv->lanes > 4) {
		dev_err(dev, "Invalid dsi-lanes: %d\n", priv->lanes);
		return -EINVAL;
	}

	ret = dev_read_u32(dev, "lt8912,addr-cec", &priv->addr_cec);
	if (ret) {
		dev_err(dev, "Failed to get addr-cec property (%d)\n", ret);
		return -EINVAL;
	}

	ret = dm_i2c_probe(dev_get_parent(dev), priv->addr_cec, 0, &priv->cec_dev);
	if (ret) {
		dev_err(dev, "Can't find cec device id=0x%x\n", priv->addr_cec);
		return -ENODEV;
	}

	ret = dev_read_u32(dev, "lt8912,addr-cec2", &priv->addr_cec2);
	if (ret) {
		dev_err(dev, "Failed to get addr-cec property (%d)\n", ret);
		return -EINVAL;
	}

	ret = dm_i2c_probe(dev_get_parent(dev), priv->addr_cec2, 0, &priv->cec_dev2);
	if (ret) {
		dev_err(dev, "Can't find cec device id=0x%x\n", priv->addr_cec2);
		return -ENODEV;
	}

	ret = gpio_request_by_name(dev, "backlight-gpio", 0, &priv->bk_lgt,
			GPIOD_IS_OUT);

	if (ret) {
		printf("Warning: cannot get backlight GPIO\n");
		if (ret != -ENOENT)
			return ret;
	}

	ret = gpio_request_by_name(dev, "backlight-en-gpio", 0, &priv->bk_lgt_en,
			GPIOD_IS_OUT);

	if (ret) {
		printf("Warning: cannot get backlight enable GPIO\n");
		if (ret != -ENOENT)
			return ret;
	}

	lt8912b_enable(dev);

	return 0;
}

static int lt8912b_remove(struct udevice *dev)
{
	return 0;

}

static const struct panel_ops lt8912b_ops = {
	.enable_backlight = lt8912b_enable_backlight,
	.get_display_timing = lt8912b_get_display_timing,
};

static const struct udevice_id lt8912b_ids[] = {
	{ .compatible = "lontium,lt8912" },
	{ }
};

U_BOOT_DRIVER(lt8912b_mipi2hdmi) = {
	.name			  = "lt8912b_mipi2hdmi",
	.id			  = UCLASS_PANEL,
	.of_match		  = lt8912b_ids,
	.ops			  = &lt8912b_ops,
	.probe			  = lt8912b_probe,
	.remove			  = lt8912b_remove,
	.plat_auto		  = sizeof(struct mipi_dsi_panel_plat),
	.priv_auto		  = sizeof(struct lt8912b_priv),
};
