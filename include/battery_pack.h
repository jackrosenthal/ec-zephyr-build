/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Common API for battery pack vendor provided charging profile
 */
#ifndef __CROS_EC_BATTERY_PACK_H
#define __CROS_EC_BATTERY_PACK_H

#define CELSIUS_TO_DECI_KELVIN(temp_c) ((temp_c) * 10 + 2731)

/* Battery parameters */
struct batt_params {
	int temperature;      /* Temperature in 0.1 K */
	int state_of_charge;  /* State of charge (percent, 0-100) */
	int voltage;          /* Battery voltage (mV) */
	int current;          /* Battery current (mA) */
	int desired_voltage;  /* Charging voltage desired by battery (mV) */
	int desired_current;  /* Charging current desired by battery (mA) */
};

/* Battery constants */
struct battery_info {
	/* Design voltage in mV */
	int voltage_max;
	int voltage_normal;
	int voltage_min;
	/* Working temperature range in 0.1 K increments */
	int temp_charge_min;
	int temp_charge_max;
	int temp_discharge_min;
	int temp_discharge_max;
	/* Pre-charge current in mA */
	int precharge_current;
};

/**
 * Return vendor-provided battery constants.
 */
const struct battery_info *battery_get_info(void);

/**
 * Modify battery parameters to match vendor charging profile.
 *
 * @param batt		Battery parameters to modify
 */
void battery_vendor_params(struct batt_params *batt);

#endif
