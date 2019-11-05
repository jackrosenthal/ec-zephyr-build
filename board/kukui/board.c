/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "adc_chip.h"
#include "button.h"
#include "charge_manager.h"
#include "charge_ramp.h"
#include "charge_state.h"
#include "charger.h"
#include "chipset.h"
#include "common.h"
#include "console.h"
#include "driver/accelgyro_bmi160.h"
#include "driver/als_tcs3400.h"
#include "driver/bc12/pi3usb9201.h"
#include "driver/sync.h"
#include "driver/usb_mux/it5205.h"
#include "extpower.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "i2c.h"
#include "lid_switch.h"
#include "power.h"
#include "power_button.h"
#include "pwm.h"
#include "pwm_chip.h"
#include "registers.h"
#include "spi.h"
#include "system.h"
#include "task.h"
#include "tcpm.h"
#include "timer.h"
#include "usb_charge.h"
#include "usb_mux.h"
#include "usb_pd_policy.h"
#include "usb_pd_tcpm.h"
#include "util.h"

#define CPRINTS(format, args...) cprints(CC_USBCHARGE, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_USBCHARGE, format, ## args)

#include "gpio_list.h"

/******************************************************************************/
/* I2C ports */
const struct i2c_port_t i2c_ports[] = {
	{"typec", 0, 400, GPIO_I2C1_SCL, GPIO_I2C1_SDA},
	{"other", 1, 400, GPIO_I2C2_SCL, GPIO_I2C2_SDA},
};
const unsigned int i2c_ports_used = ARRAY_SIZE(i2c_ports);

#define BC12_I2C_ADDR_FLAGS PI3USB9201_I2C_ADDR_3_FLAGS

/* power signal list.  Must match order of enum power_signal. */
const struct power_signal_info power_signal_list[] = {
	{GPIO_AP_IN_SLEEP_L,   POWER_SIGNAL_ACTIVE_LOW,  "AP_IN_S3_L"},
	{GPIO_PMIC_EC_RESETB,  POWER_SIGNAL_ACTIVE_HIGH, "PMIC_PWR_GOOD"},
};
BUILD_ASSERT(ARRAY_SIZE(power_signal_list) == POWER_SIGNAL_COUNT);

/******************************************************************************/
/* SPI devices */
const struct spi_device_t spi_devices[] = {
};
const unsigned int spi_devices_used = ARRAY_SIZE(spi_devices);

/******************************************************************************/
void board_set_dp_mux_control(int output_enable, int polarity)
{
	if (board_get_version() >= 5)
		return;

	gpio_set_level(GPIO_USB_C0_DP_OE_L, !output_enable);
	if (output_enable)
		gpio_set_level(GPIO_USB_C0_DP_POLARITY, polarity);
}

static int force_discharge;

int board_set_active_charge_port(int charge_port)
{
	CPRINTS("New chg p%d", charge_port);

	/* ignore all request when discharge mode is on */
	if (force_discharge)
		return EC_SUCCESS;

	switch (charge_port) {
	case CHARGE_PORT_USB_C:
		/* Don't charge from a source port */
		if (board_vbus_source_enabled(charge_port))
			return -1;
		gpio_set_level(GPIO_EN_POGO_CHARGE_L, 1);
		gpio_set_level(GPIO_EN_USBC_CHARGE_L, 0);
		break;
#if CONFIG_DEDICATED_CHARGE_PORT_COUNT > 0
	case CHARGE_PORT_POGO:
		gpio_set_level(GPIO_EN_USBC_CHARGE_L, 1);
		gpio_set_level(GPIO_EN_POGO_CHARGE_L, 0);
		break;
#endif
	case CHARGE_PORT_NONE:
		/*
		 * To ensure the fuel gauge (max17055) is always powered
		 * even when battery is disconnected, keep VBAT rail on but
		 * set the charging current to minimum.
		 */
		gpio_set_level(GPIO_EN_POGO_CHARGE_L, 1);
		gpio_set_level(GPIO_EN_USBC_CHARGE_L, 1);
		charger_set_current(0);
		break;
	default:
		panic("Invalid charge port\n");
		break;
	}

	return EC_SUCCESS;
}

int board_discharge_on_ac(int enable)
{
	int ret, port;

	if (enable) {
		port = CHARGE_PORT_NONE;
	} else {
		/* restore the charge port state */
		port = charge_manager_get_override();
		if (port == OVERRIDE_OFF)
			port = charge_manager_get_active_charge_port();
	}

	ret = board_set_active_charge_port(port);
	if (ret)
		return ret;
	force_discharge = enable;

	return charger_discharge_on_ac(enable);
}

int extpower_is_present(void)
{
	/*
	 * The charger will indicate VBUS presence if we're sourcing 5V,
	 * so exclude such ports.
	 */
	int usb_c_extpower_present;

	if (board_vbus_source_enabled(CHARGE_PORT_USB_C))
		usb_c_extpower_present = 0;
	else
		usb_c_extpower_present = tcpm_get_vbus_level(CHARGE_PORT_USB_C);

	return usb_c_extpower_present || gpio_get_level(GPIO_POGO_VBUS_PRESENT);
}

int pd_snk_is_vbus_provided(int port)
{
	return 1;
}

#if defined(BOARD_KUKUI) || defined(BOARD_KODAMA)
/* dummy interrupt function for kukui */
void pogo_adc_interrupt(enum gpio_signal signal)
{
}
#endif

static void board_init(void)
{
	/* If the reset cause is external, pulse PMIC force reset. */
	if (system_get_reset_flags() == EC_RESET_FLAG_RESET_PIN) {
		gpio_set_level(GPIO_PMIC_FORCE_RESET_ODL, 0);
		msleep(100);
		gpio_set_level(GPIO_PMIC_FORCE_RESET_ODL, 1);
	}

	return;
}
DECLARE_HOOK(HOOK_INIT, board_init, HOOK_PRIO_DEFAULT);

static void board_rev_init(void)
{
	/* Board revision specific configs. */

	/*
	 * It's a P1 pin BOOTBLOCK_MUX_OE, also a P2 pin BC12_DET_EN.
	 * Keep this pin defaults to P1 setting since that eMMC enabled with
	 * High-Z stat.
	 */
	if (IS_ENABLED(BOARD_KUKUI) && board_get_version() == 1)
		gpio_set_flags(GPIO_BC12_DET_EN, GPIO_ODR_HIGH);

	if (board_get_version() == 2) {
		/* configure PI3USB9201 to USB Path ON Mode */
		i2c_write8(I2C_PORT_BC12, BC12_I2C_ADDR_FLAGS,
			   PI3USB9201_REG_CTRL_1,
			   (PI3USB9201_USB_PATH_ON <<
			    PI3USB9201_REG_CTRL_1_MODE_SHIFT));
	}

	if (board_get_version() < 5) {
		gpio_set_flags(GPIO_USB_C0_DP_OE_L, GPIO_OUT_HIGH);
		gpio_set_flags(GPIO_USB_C0_DP_POLARITY, GPIO_OUT_LOW);
	}
}
DECLARE_HOOK(HOOK_INIT, board_rev_init, HOOK_PRIO_INIT_ADC + 1);

/* Motion sensors */
/* Mutexes */
#ifndef VARIANT_KUKUI_NO_SENSORS
static struct mutex g_lid_mutex;

static struct bmi160_drv_data_t g_bmi160_data;

/* TCS3400 private data */
static struct als_drv_data_t g_tcs3400_data = {
	.als_cal.scale = 1,
	.als_cal.uscale = 0,
	.als_cal.offset = 0,
	.als_cal.channel_scale = {
		.k_channel_scale = ALS_CHANNEL_SCALE(1.0), /* kc */
		.cover_scale = ALS_CHANNEL_SCALE(1.0),     /* CT */
	},
};

static struct tcs3400_rgb_drv_data_t g_tcs3400_rgb_data = {
	/*
	 * TODO(b:139366662): calculates the actual coefficients and scaling
	 * factors
	 */
	.rgb_cal[X] = {
		.offset = 0,
		.scale = {
			.k_channel_scale = ALS_CHANNEL_SCALE(1.0), /* kr */
			.cover_scale = ALS_CHANNEL_SCALE(1.0)
		},
		.coeff[TCS_RED_COEFF_IDX] = FLOAT_TO_FP(0),
		.coeff[TCS_GREEN_COEFF_IDX] = FLOAT_TO_FP(0),
		.coeff[TCS_BLUE_COEFF_IDX] = FLOAT_TO_FP(0),
		.coeff[TCS_CLEAR_COEFF_IDX] = FLOAT_TO_FP(0),
	},
	.rgb_cal[Y] = {
		.offset = 0,
		.scale = {
			.k_channel_scale = ALS_CHANNEL_SCALE(1.0), /* kg */
			.cover_scale = ALS_CHANNEL_SCALE(1.0)
		},
		.coeff[TCS_RED_COEFF_IDX] = FLOAT_TO_FP(0),
		.coeff[TCS_GREEN_COEFF_IDX] = FLOAT_TO_FP(0),
		.coeff[TCS_BLUE_COEFF_IDX] = FLOAT_TO_FP(0),
		.coeff[TCS_CLEAR_COEFF_IDX] = FLOAT_TO_FP(0.1),
	},
	.rgb_cal[Z] = {
		.offset = 0,
		.scale = {
			.k_channel_scale = ALS_CHANNEL_SCALE(1.0), /* kb */
			.cover_scale = ALS_CHANNEL_SCALE(1.0)
		},
		.coeff[TCS_RED_COEFF_IDX] = FLOAT_TO_FP(0),
		.coeff[TCS_GREEN_COEFF_IDX] = FLOAT_TO_FP(0),
		.coeff[TCS_BLUE_COEFF_IDX] = FLOAT_TO_FP(0),
		.coeff[TCS_CLEAR_COEFF_IDX] = FLOAT_TO_FP(0),
	},
	.saturation.again = TCS_DEFAULT_AGAIN,
	.saturation.atime = TCS_DEFAULT_ATIME,
};

/* Matrix to rotate accelerometer into standard reference frame */
#ifdef BOARD_KUKUI
static const mat33_fp_t lid_standard_ref = {
	{FLOAT_TO_FP(1), 0, 0},
	{0, FLOAT_TO_FP(1), 0},
	{0, 0, FLOAT_TO_FP(1)}
};
#else
static const mat33_fp_t lid_standard_ref = {
	{FLOAT_TO_FP(-1), 0, 0},
	{0, FLOAT_TO_FP(-1), 0},
	{0, 0, FLOAT_TO_FP(1)}
};
#endif /* BOARD_KUKUI */

#ifdef CONFIG_MAG_BMI160_BMM150
/* Matrix to rotate accelrator into standard reference frame */
static const mat33_fp_t mag_standard_ref = {
	{0, FLOAT_TO_FP(-1), 0},
	{FLOAT_TO_FP(-1), 0, 0},
	{0, 0, FLOAT_TO_FP(-1)}
};
#endif /* CONFIG_MAG_BMI160_BMM150 */

struct motion_sensor_t motion_sensors[] = {
	/*
	 * Note: bmi160: supports accelerometer and gyro sensor
	 * Requirement: accelerometer sensor must init before gyro sensor
	 * DO NOT change the order of the following table.
	 */
	[LID_ACCEL] = {
	 .name = "Accel",
	 .active_mask = SENSOR_ACTIVE_S0_S3,
	 .chip = MOTIONSENSE_CHIP_BMI160,
	 .type = MOTIONSENSE_TYPE_ACCEL,
	 .location = MOTIONSENSE_LOC_LID,
	 .drv = &bmi160_drv,
	 .mutex = &g_lid_mutex,
	 .drv_data = &g_bmi160_data,
	 .port = I2C_PORT_ACCEL,
	 .i2c_spi_addr_flags = BMI160_ADDR0_FLAGS,
	 .rot_standard_ref = &lid_standard_ref,
	 .default_range = 4,  /* g */
	 .min_frequency = BMI160_ACCEL_MIN_FREQ,
	 .max_frequency = BMI160_ACCEL_MAX_FREQ,
	 .config = {
		 /* Enable accel in S0 */
		 [SENSOR_CONFIG_EC_S0] = {
			 .odr = 10000 | ROUND_UP_FLAG,
			 .ec_rate = 100 * MSEC,
		 },
	 },
	},
	[LID_GYRO] = {
	 .name = "Gyro",
	 .active_mask = SENSOR_ACTIVE_S0_S3,
	 .chip = MOTIONSENSE_CHIP_BMI160,
	 .type = MOTIONSENSE_TYPE_GYRO,
	 .location = MOTIONSENSE_LOC_LID,
	 .drv = &bmi160_drv,
	 .mutex = &g_lid_mutex,
	 .drv_data = &g_bmi160_data,
	 .port = I2C_PORT_ACCEL,
	 .i2c_spi_addr_flags = BMI160_ADDR0_FLAGS,
	 .default_range = 1000, /* dps */
	 .rot_standard_ref = &lid_standard_ref,
	 .min_frequency = BMI160_GYRO_MIN_FREQ,
	 .max_frequency = BMI160_GYRO_MAX_FREQ,
	},
#ifdef CONFIG_MAG_BMI160_BMM150
	[LID_MAG] = {
	 .name = "Lid Mag",
	 .active_mask = SENSOR_ACTIVE_S0_S3,
	 .chip = MOTIONSENSE_CHIP_BMI160,
	 .type = MOTIONSENSE_TYPE_MAG,
	 .location = MOTIONSENSE_LOC_LID,
	 .drv = &bmi160_drv,
	 .mutex = &g_lid_mutex,
	 .drv_data = &g_bmi160_data,
	 .port = I2C_PORT_ACCEL,
	 .i2c_spi_addr_flags = BMI160_ADDR0_FLAGS,
	 .default_range = BIT(11), /* 16LSB / uT, fixed */
	 .rot_standard_ref = &mag_standard_ref,
	 .min_frequency = BMM150_MAG_MIN_FREQ,
	 .max_frequency = BMM150_MAG_MAX_FREQ(SPECIAL),
	},
#endif /* CONFIG_MAG_BMI160_BMM150 */
	[CLEAR_ALS] = {
	 .name = "Clear Light",
	 .active_mask = SENSOR_ACTIVE_S0_S3,
	 .chip = MOTIONSENSE_CHIP_TCS3400,
	 .type = MOTIONSENSE_TYPE_LIGHT,
	 .location = MOTIONSENSE_LOC_LID,
	 .drv = &tcs3400_drv,
	 .drv_data = &g_tcs3400_data,
	 .port = I2C_PORT_ALS,
	 .i2c_spi_addr_flags = TCS3400_I2C_ADDR_FLAGS,
	 .rot_standard_ref = NULL,
	 .default_range = 0x10000, /* scale = 1x, uscale = 0 */
	 .min_frequency = TCS3400_LIGHT_MIN_FREQ,
	 .max_frequency = TCS3400_LIGHT_MAX_FREQ,
	 .config = {
		 /* Run ALS sensor in S0 */
		[SENSOR_CONFIG_EC_S0] = {
			.odr = 1000,
		},
	 },
	},
	[RGB_ALS] = {
	.name = "RGB Light",
	 .active_mask = SENSOR_ACTIVE_S0_S3,
	 .chip = MOTIONSENSE_CHIP_TCS3400,
	 .type = MOTIONSENSE_TYPE_LIGHT_RGB,
	 .location = MOTIONSENSE_LOC_LID,
	 .drv = &tcs3400_rgb_drv,
	 .drv_data = &g_tcs3400_rgb_data,
	 /*.port = I2C_PORT_ALS,*/ /* Unused. RGB channels read by CLEAR_ALS. */
	 .rot_standard_ref = NULL,
	 .default_range = 0x10000, /* scale = 1x, uscale = 0 */
	 .min_frequency = 0, /* 0 indicates we should not use sensor directly */
	 .max_frequency = 0, /* 0 indicates we should not use sensor directly */
	},
	[VSYNC] = {
	 .name = "Camera vsync",
	 .active_mask = SENSOR_ACTIVE_S0,
	 .chip = MOTIONSENSE_CHIP_GPIO,
	 .type = MOTIONSENSE_TYPE_SYNC,
	 .location = MOTIONSENSE_LOC_CAMERA,
	 .drv = &sync_drv,
	 .default_range = 0,
	 .min_frequency = 0,
	 .max_frequency = 1,
	},
};
const unsigned int motion_sensor_count = ARRAY_SIZE(motion_sensors);
const struct motion_sensor_t *motion_als_sensors[] = {
	&motion_sensors[CLEAR_ALS],
};
#endif /* VARIANT_KUKUI_NO_SENSORS */

void usb_charger_set_switches(int port, enum usb_switch setting)
{
}

/*
 * Return if VBUS is sagging too low
 */
int board_is_vbus_too_low(int port, enum chg_ramp_vbus_state ramp_state)
{
	/*
	 * Though we have a more tolerant range (3.9V~13.4V), setting 4400 to
	 * prevent from a bad charger crashed.
	 *
	 * TODO(b:131284131): mt6370 VBUS reading is not accurate currently.
	 * Vendor will provide a workaround solution to fix the gap between ADC
	 * reading and actual voltage.  After the workaround applied, we could
	 * try to raise this value to 4600.  (when it says it read 4400, it is
	 * actually close to 4600)
	 */
	return charger_get_vbus_voltage(port) < 4400;
}

__override int board_charge_port_is_sink(int port)
{
	/* TODO(b:128386458): Check POGO_ADC_INT_L */
	return 1;
}

__override int board_charge_port_is_connected(int port)
{
	return gpio_get_level(GPIO_POGO_VBUS_PRESENT);
}

__override
void board_fill_source_power_info(int port,
				  struct ec_response_usb_pd_power_info *r)
{
	r->meas.voltage_now = 3300;
	r->meas.voltage_max = 3300;
	r->meas.current_max = 1500;
	r->meas.current_lim = 1500;
	r->max_power = r->meas.voltage_now * r->meas.current_max;
}

__override int board_has_virtual_mux(void)
{
	return board_get_version() < 5;
}
