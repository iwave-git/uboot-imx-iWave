/*
 * Copyright (C) 2014 Gateworks Corporation
 * Tim Harvey <tharvey@gateworks.com>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <i2c.h>
#include <power/pmic.h>
#include <power/bd71837.h>

static const char bd71837_name[] = "BD71837";
int power_bd71837_init (unsigned char bus) {
	struct pmic *p = pmic_alloc();

	if (!p) {
		printf("%s: POWER allocation error!\n", __func__);
		return -ENOMEM;
	}

	p->name = bd71837_name;
	p->interface = PMIC_I2C;
	p->number_of_regs = BD718XX_MAX_REGISTER;
	p->hw.i2c.addr = 0x4b;
	p->hw.i2c.tx_num = 1;
	p->bus = bus;

#if defined (CONFIG_TARGET_IMX8MM_IWG34M) || (CONFIG_TARGET_IMX8MM_IWG34S) || (CONFIG_TARGET_IMX8MM_IWG34M_Q7)
	/*IWG34: changing pmic number*/
	printf("power_bd71847_init\n");
#else
	printf("power_bd71837_init\n");
#endif

	return 0;
}
