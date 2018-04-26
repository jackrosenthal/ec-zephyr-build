/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * TI bq25703 battery charger driver.
 */

#include "battery_smart.h"
#include "bq25703.h"
#include "charger.h"
#include "common.h"
#include "console.h"
#include "i2c.h"

/* Sense resistor configurations and macros */
#define DEFAULT_SENSE_RESISTOR 10

#define INPUT_RESISTOR_RATIO \
	((CONFIG_CHARGER_SENSE_RESISTOR_AC) / DEFAULT_SENSE_RESISTOR)
#define REG_TO_INPUT_CURRENT(REG) ((REG + 1) * 50 / INPUT_RESISTOR_RATIO)
#define INPUT_CURRENT_TO_REG(CUR) (((CUR) * INPUT_RESISTOR_RATIO / 50) - 1)

#define CHARGING_RESISTOR_RATIO \
	((CONFIG_CHARGER_SENSE_RESISTOR) / DEFAULT_SENSE_RESISTOR)
#define REG_TO_CHARGING_CURRENT(REG) ((REG) / CHARGING_RESISTOR_RATIO)
#define CHARGING_CURRENT_TO_REG(CUR) ((CUR) * CHARGING_RESISTOR_RATIO)


/* Console output macros */
#define CPRINTF(format, args...) cprintf(CC_CHARGER, format, ## args)

/* Charger parameters */
static const struct charger_info bq25703_charger_info = {
	.name         = "bq25703",
	.voltage_max  = 19200,
	.voltage_min  = 1024,
	.voltage_step = 16,
	.current_max  = 8128 / CHARGING_RESISTOR_RATIO,
	.current_min  = 64 / CHARGING_RESISTOR_RATIO,
	.current_step = 64 / CHARGING_RESISTOR_RATIO,
	.input_current_max  = 6400 / INPUT_RESISTOR_RATIO,
	.input_current_min  = 50 / INPUT_RESISTOR_RATIO,
	.input_current_step = 50 / INPUT_RESISTOR_RATIO,
};

static inline int raw_read8(int offset, int *value)
{
	return i2c_read8(I2C_PORT_CHARGER, BQ25703_I2C_ADDR1, offset, value);
}

static inline int raw_write8(int offset, int value)
{
	return i2c_write8(I2C_PORT_CHARGER, BQ25703_I2C_ADDR1, offset, value);
}

static inline int raw_read16(int offset, int *value)
{
	return i2c_read16(I2C_PORT_CHARGER, BQ25703_I2C_ADDR1, offset, value);
}

static inline int raw_write16(int offset, int value)
{
	return i2c_write16(I2C_PORT_CHARGER, BQ25703_I2C_ADDR1, offset, value);
}


/* Charger interfaces */

const struct charger_info *charger_get_info(void)
{
	return &bq25703_charger_info;
}

int charger_post_init(void)
{
	/*
	 * Note: bq25703 power on reset state is:
	 *	watch dog timer     = 175 sec
	 *	input current limit = ~1/2 maximum setting
	 *	charging voltage    = 0 mV
	 *	charging current    = 0 mA
	 *	discharge on AC     = disabled
	 */

	/* Set charger input current limit */
	return charger_set_input_current(CONFIG_CHARGER_INPUT_CURRENT);
}

int charger_get_status(int *status)
{
	int rv;
	int option;

	rv = charger_get_option(&option);
	if (rv)
		return rv;

	/* Default status */
	*status = CHARGER_LEVEL_2;

	if (option & BQ25793_CHARGE_OPTION_0_CHRG_INHIBIT)
		*status |= CHARGER_CHARGE_INHIBITED;

	return EC_SUCCESS;
}

int charger_set_mode(int mode)
{
	int rv;
	int option;

	rv = charger_get_option(&option);
	if (rv)
		return rv;

	if (mode & CHARGER_CHARGE_INHIBITED)
		option |= BQ25793_CHARGE_OPTION_0_CHRG_INHIBIT;
	else
		option &= ~BQ25793_CHARGE_OPTION_0_CHRG_INHIBIT;

	return charger_set_option(option);
}

int charger_enable_otg_power(int enabled)
{
	/* This is controlled with the EN_OTG pin. Support not added yet. */
	return EC_ERROR_UNIMPLEMENTED;
}

int charger_set_otg_current_voltage(int output_current, int output_voltage)
{
	/* Add when needed. */
	return EC_ERROR_UNIMPLEMENTED;
}

int charger_is_sourcing_otg_power(int port)
{
	/* Add when needed. */
	return EC_ERROR_UNIMPLEMENTED;
}

int charger_get_current(int *current)
{
	int rv, reg;

	rv = raw_read16(BQ25703_REG_CHARGE_CURRENT, &reg);
	if (!rv)
		*current = REG_TO_CHARGING_CURRENT(reg);

	return rv;
}

int charger_set_current(int current)
{
	return raw_write16(BQ25703_REG_CHARGE_CURRENT,
		CHARGING_CURRENT_TO_REG(current));
}

/* Get/set charge voltage limit in mV */
int charger_get_voltage(int *voltage)
{
	return raw_read16(BQ25703_REG_MAX_CHARGE_VOLTAGE, voltage);
}
int charger_set_voltage(int voltage)
{
	return raw_write16(BQ25703_REG_MAX_CHARGE_VOLTAGE, voltage);
}

/* Discharge battery when on AC power. */
int charger_discharge_on_ac(int enable)
{
	int rv, option;

	rv = charger_get_option(&option);
	if (rv)
		return rv;

	if (enable)
		option |= BQ25793_CHARGE_OPTION_0_EN_LEARN;
	else
		option &= ~BQ25793_CHARGE_OPTION_0_EN_LEARN;

	return charger_set_option(option);
}

int charger_set_input_current(int input_current)
{
	return raw_write8(BQ25703_REG_IIN_HOST,
		INPUT_CURRENT_TO_REG(input_current));
}

int charger_get_input_current(int *input_current)
{
	int rv, reg;

	rv = raw_read8(BQ25703_REG_IIN_HOST, &reg);
	if (!rv)
		*input_current = REG_TO_INPUT_CURRENT(reg);

	return rv;
}

int charger_manufacturer_id(int *id)
{
	return raw_read8(BQ25703_REG_MANUFACTURER_ID, id);
}
int charger_device_id(int *id)
{
	return raw_read8(BQ25703_REG_DEVICE_ADDRESS, id);
}

int charger_get_option(int *option)
{
	/* There are 4 option registers, but we only need the first for now. */
	return raw_read16(BQ25703_REG_CHARGE_OPTION_0, option);
}

int charger_set_option(int option)
{
	/* There are 4 option registers, but we only need the first for now. */
	return raw_write16(BQ25703_REG_CHARGE_OPTION_0, option);
}