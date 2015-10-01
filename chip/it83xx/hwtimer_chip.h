/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* External timers control module for IT83xx. */

#ifndef __CROS_EC_HWTIMER_CHIP_H
#define __CROS_EC_HWTIMER_CHIP_H

#define FREE_EXT_TIMER_L     EXT_TIMER_3
#define FREE_EXT_TIMER_H     EXT_TIMER_4
#define FAN_CTRL_EXT_TIMER   EXT_TIMER_5
#define EVENT_EXT_TIMER      EXT_TIMER_6
#define WDT_EXT_TIMER        EXT_TIMER_7

enum ext_timer_clock_source {
	EXT_PSR_32P768K_HZ = 0,
	EXT_PSR_1P024K_HZ  = 1,
	EXT_PSR_32_HZ      = 2,
	EXT_PSR_8M_HZ      = 3
};

/*
 * 24-bit timers: external timer 3, 5, and 7
 * 32-bit timers: external timer 4, 6, and 8
 */
enum ext_timer_sel {
	/* timer 3 and 4 combine mode for free running timer */
	EXT_TIMER_3 = 0,
	EXT_TIMER_4,
	/* For fan control */
	EXT_TIMER_5,
	/* timer 6 for event timer */
	EXT_TIMER_6,
	/* For WDT capture important state information before being reset */
	EXT_TIMER_7,
	/* reserved */
	EXT_TIMER_8,
	EXT_TIMER_COUNT,
};

struct ext_timer_ctrl_t {
	volatile uint8_t *mode;
	volatile uint8_t *polarity;
	uint8_t mask;
	uint8_t irq;
};

extern const struct ext_timer_ctrl_t et_ctrl_regs[];
void ext_timer_start(enum ext_timer_sel ext_timer, int en_irq);
void ext_timer_stop(enum ext_timer_sel ext_timer, int dis_irq);
void fan_ext_timer_interrupt(void);
void update_exc_start_time(void);
/**
 * Config a external timer.
 *
 * @param raw	(!=0) timer count equal to param "ms" no conversion.
 */
int ext_timer_ms(enum ext_timer_sel ext_timer,
		enum ext_timer_clock_source ext_timer_clock,
		int start,
		int et_int,
		int32_t ms,
		int first_time_enable,
		int raw);

#endif /* __CROS_EC_HWTIMER_CHIP_H */
