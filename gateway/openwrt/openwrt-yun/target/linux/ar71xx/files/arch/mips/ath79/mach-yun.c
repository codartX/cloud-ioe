/*
 *  Arduino Yun support
 *
 *  Copyright (C) 2011-2012 Gabor Juhos <juhosg@openwrt.org>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-spi.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"
#include <asm/mach-ath79/ar71xx_regs.h>
#include <asm/mach-ath79/ath79.h>
#include "common.h"
#include "gpio.h"
#include "linux/gpio.h"

// Uncomment to use for DS1 board
//#define DS1
// Uncomment to have reset on gpio18 instead of gipo7
#define DS2_B

#define DS_GPIO_LED_WLAN		0
#define DS_GPIO_LED_USB			1

#define DS_GPIO_OE			21
#define DS_GPIO_AVR_RESET		18

// Maintained to have the console in the previous version of DS2 working
#define DS_GPIO_AVR_RESET_DS2           7
#define DS2_PREV_RESET_PIN

#ifdef DS1
#define DS_GPIO_OE2			23
#else
#define DS_GPIO_OE2                     22
#define DS_GPIO_UART_ENA		23
#define DS_GPIO_CONF_BTN		20
#endif

#define DS_KEYS_POLL_INTERVAL		20	/* msecs */
#define DS_KEYS_DEBOUNCE_INTERVAL	(3 * DS_KEYS_POLL_INTERVAL)

#define DS_MAC0_OFFSET			0x0000
#define DS_MAC1_OFFSET			0x0006
#define DS_CALDATA_OFFSET		0x1000
#define DS_WMAC_MAC_OFFSET		0x1002

static struct gpio_led ds_leds_gpio[] __initdata = {
	{
		.name		= "ds:green:usb",
		.gpio		= DS_GPIO_LED_USB,
		.active_low	= 0,
	},
	{
		.name		= "ds:green:wlan",
		.gpio		= DS_GPIO_LED_WLAN,
		.active_low	= 0,
	},
};

static struct gpio_keys_button ds_gpio_keys[] __initdata = {
#ifndef DS1
	{
		.desc		= "configuration button",
		.type		= EV_KEY,
		.code		= KEY_WPS_BUTTON,
		.debounce_interval = DS_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= DS_GPIO_CONF_BTN,
		.active_low	= 1,
	},
#endif
};

static void __init ds_common_setup(void)
{
	static u8 mac[6];

	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);
	ath79_register_m25p80(NULL);
#if 0
	ath79_register_wmac(art + DS_CALDATA_OFFSET,
			    art + DS_WMAC_MAC_OFFSET);

	ath79_init_mac(ath79_eth0_data.mac_addr, art + DS_MAC0_OFFSET, 0);
	ath79_init_mac(ath79_eth1_data.mac_addr, art + DS_MAC1_OFFSET, 0);
#else
        if (ar93xx_wmac_read_mac_address(mac)) {
                  ath79_register_wmac(NULL, NULL);
          } else {
                  ath79_register_wmac(art + DS_CALDATA_OFFSET,
                                      art + DS_WMAC_MAC_OFFSET);
                  memcpy(mac, art + DS_WMAC_MAC_OFFSET, sizeof(mac));
          }

          mac[3] |= 0x08;
          ath79_init_mac(ath79_eth0_data.mac_addr, mac, 0);

          mac[3] &= 0xF7;
          ath79_init_mac(ath79_eth1_data.mac_addr, mac, 0);
#endif
	ath79_register_mdio(0, 0x0);

	/* LAN ports */
	ath79_register_eth(1);

	/* WAN port */
	ath79_register_eth(0);
}

static void __init ds_setup(void)
{
#ifdef DS2_PREV_RESET_PIN
	u32 t;
#endif
	ds_common_setup();

	ath79_register_leds_gpio(-1, ARRAY_SIZE(ds_leds_gpio),
				 ds_leds_gpio);
	ath79_register_gpio_keys_polled(-1, DS_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(ds_gpio_keys),
					ds_gpio_keys);
	ath79_register_usb();

	//Disable the Function for some pins to have GPIO functionality active
	// GPIO6-7-8 and GPIO11
	ath79_gpio_function_setup(AR933X_GPIO_FUNC_JTAG_DISABLE | AR933X_GPIO_FUNC_I2S_MCK_EN, 0);

	ath79_gpio_function2_setup(AR933X_GPIO_FUNC2_JUMPSTART_DISABLE, 0);

	printk("Setting DogStick2 GPIO\n");
#ifdef DS2_PREV_RESET_PIN
	t = ath79_reset_rr(AR933X_RESET_REG_BOOTSTRAP);
	t |= AR933X_BOOTSTRAP_MDIO_GPIO_EN;
	ath79_reset_wr(AR933X_RESET_REG_BOOTSTRAP, t);

	// Put the avr reset to high
	if (gpio_request_one(DS_GPIO_AVR_RESET_DS2,
                 GPIOF_OUT_INIT_LOW | GPIOF_EXPORT_DIR_FIXED,
                 "OE-1") != 0)
                printk("Error setting GPIO OE\n");
	gpio_unexport(DS_GPIO_AVR_RESET_DS2);
	gpio_free(DS_GPIO_AVR_RESET_DS2);
#endif

	// enable OE of level shifter
	if (gpio_request_one(DS_GPIO_OE,
		 GPIOF_OUT_INIT_LOW | GPIOF_EXPORT_DIR_FIXED,
		 "OE-1") != 0)
		printk("Error setting GPIO OE\n");

#ifdef DS1
	if (gpio_request_one(DS_GPIO_OE2,
		 GPIOF_OUT_INIT_HIGH | GPIOF_EXPORT_DIR_FIXED,
		 "OE-2") != 0)
		printk("Error setting GPIO OE2\n");
#else
        if (gpio_request_one(DS_GPIO_UART_ENA,
                 GPIOF_OUT_INIT_LOW | GPIOF_EXPORT_DIR_FIXED,
                 "UART-ENA") != 0)
                printk("Error setting GPIO Uart Enable\n");

	// enable OE of level shifter
        if (gpio_request_one(DS_GPIO_OE2,
                 GPIOF_OUT_INIT_LOW | GPIOF_EXPORT_DIR_FIXED,
                 "OE-2") != 0)
                printk("Error setting GPIO OE2\n");
#endif
}

MIPS_MACHINE(ATH79_MACH_YUN, "YUN", "Arduino Yun",
	     ds_setup);
