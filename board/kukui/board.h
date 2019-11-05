/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Configuration for Kukui */

#ifndef __CROS_EC_BOARD_H
#define __CROS_EC_BOARD_H

#ifdef BOARD_KRANE
#define VARIANT_KUKUI_BATTERY_MM8013
#define VARIANT_KUKUI_POGO_KEYBOARD
#define VARIANT_KUKUI_POGO_DOCK
#else
#define VARIANT_KUKUI_BATTERY_MAX17055
#endif

#define VARIANT_KUKUI_CHARGER_MT6370
#define VARIANT_KUKUI_DP_MUX_GPIO

#define VARIANT_KUKUI_NO_SENSORS

#include "baseboard.h"

/* Battery */
#ifdef BOARD_KRANE
#define BATTERY_DESIRED_CHARGING_CURRENT    3500  /* mA */
#else
#define BATTERY_DESIRED_CHARGING_CURRENT    2000  /* mA */
#endif /* BOARD_KRANE */

#ifdef BOARD_KRANE
#define CONFIG_CHARGER_MT6370_BACKLIGHT
#endif /* BOARD_KRANE */

#ifdef BOARD_KUKUI
/* kukui doesn't have BC12_DET_EN pin */
#undef CONFIG_CHARGER_MT6370_BC12_GPIO
#endif

/* I2C ports */
#define I2C_PORT_CHARGER  0
#define I2C_PORT_TCPC0    0
#define I2C_PORT_USB_MUX  0
#define I2C_PORT_BATTERY  1
#define I2C_PORT_VIRTUAL_BATTERY I2C_PORT_BATTERY
#define I2C_PORT_ACCEL    1
#define I2C_PORT_BC12     1
#define I2C_PORT_ALS      1

/* Route sbs host requests to virtual battery driver */
#define VIRTUAL_BATTERY_ADDR_FLAGS 0x0B

#ifndef __ASSEMBLER__

/* power signal definitions */
enum power_signal {
	AP_IN_S3_L,
	PMIC_PWR_GOOD,

	/* Number of signals */
	POWER_SIGNAL_COUNT,
};

/* Motion sensors */
enum sensor_id {
	SENSOR_COUNT,
};

#undef CONFIG_LID_SWITCH

enum charge_port {
	CHARGE_PORT_USB_C,
#if CONFIG_DEDICATED_CHARGE_PORT_COUNT > 0
	CHARGE_PORT_POGO,
#endif
};

#include "gpio_signal.h"
#include "registers.h"

void board_reset_pd_mcu(void);
int board_get_version(void);
int board_is_sourcing_vbus(int port);
void pogo_adc_interrupt(enum gpio_signal signal);
int board_discharge_on_ac(int enable);

#endif /* !__ASSEMBLER__ */

#endif /* __CROS_EC_BOARD_H */
