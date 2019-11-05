/* Copyright 2019 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Kukui board configuration */

#ifndef __CROS_EC_BASEBOARD_H
#define __CROS_EC_BASEBOARD_H

#undef CONFIG_WATCHDOG

/*
 * Define this flag if board controls dp mux via gpio pins USB_C0_DP_OE_L and
 * USB_C0_DP_POLARITY.
 *
 * board must provide function board_set_dp_mux_control(output_enable, polarity)
 *
 * #define VARIANT_KUKUI_DP_MUX_GPIO
 */

/* Optional modules */
#undef  CONFIG_ADC
#undef  CONFIG_ADC_WATCHDOG
#define CONFIG_CHIPSET_MT8183
#undef  CONFIG_EMULATED_SYSRQ
#undef  CONFIG_HIBERNATE
#define CONFIG_I2C
#define CONFIG_I2C_MASTER
#define CONFIG_LED_COMMON
#define CONFIG_LOW_POWER_IDLE
#define CONFIG_POWER_COMMON
#undef  CONFIG_SPI
#undef  CONFIG_SPI_MASTER
#define CONFIG_STM_HWTIMER32
#undef  CONFIG_SWITCH
#undef  CONFIG_WATCHDOG_HELP

#define CONFIG_SYSTEM_UNLOCKED /* Allow dangerous commands for testing */

#undef  CONFIG_UART_CONSOLE
#define CONFIG_UART_CONSOLE 1
#define CONFIG_UART_RX_DMA

/* Bootblock */
#ifdef SECTION_IS_RO
#define CONFIG_BOOTBLOCK

#define EMMC_SPI_PORT 2
#endif

/* Optional features */
#define CONFIG_BOARD_PRE_INIT
#define CONFIG_BOARD_VERSION_CUSTOM
#define CONFIG_CHARGER_ILIM_PIN_DISABLED
#define CONFIG_FORCE_CONSOLE_RESUME

/* Required for FAFT */
#undef  CONFIG_CMD_BUTTON
#undef  CONFIG_CMD_CHARGEN

/* By default, set hcdebug to off */
#undef CONFIG_HOSTCMD_DEBUG_MODE
#define CONFIG_HOSTCMD_DEBUG_MODE HCDEBUG_OFF
#define CONFIG_SUPPRESSED_HOST_COMMANDS \
	EC_CMD_CONSOLE_SNAPSHOT, EC_CMD_CONSOLE_READ, EC_CMD_MOTION_SENSE_CMD

#define CONFIG_LTO
#undef  CONFIG_POWER_BUTTON
#define CONFIG_POWER_TRACK_HOST_SLEEP_STATE
#define CONFIG_SOFTWARE_PANIC
#define CONFIG_VBOOT_HASH

/* Increase tx buffer size, as we'd like to stream EC log to AP. */
#undef CONFIG_UART_TX_BUF_SIZE
#define CONFIG_UART_TX_BUF_SIZE 4096

/* To be able to indicate the device is in tablet mode. */
#define PD_OPERATING_POWER_MW 15000
#define PD_MAX_POWER_MW       ((PD_MAX_VOLTAGE_MV * PD_MAX_CURRENT_MA) / 1000)
#define PD_MAX_CURRENT_MA     3000

/*
 * The Maximum input voltage is 13.5V, need another 5% tolerance.
 * 12.85V * 1.05 = 13.5V
 */
#define PD_MAX_VOLTAGE_MV     12850

#define PD_POWER_SUPPLY_TURN_ON_DELAY  30000  /* us */
#define PD_POWER_SUPPLY_TURN_OFF_DELAY 50000  /* us */
#define PD_VCONN_SWAP_DELAY 5000 /* us */

/* Timer selection */
#define TIM_CLOCK32  2

/* 48 MHz SYSCLK clock frequency */
#define CPU_CLOCK 48000000

/* Optional for testing */
#undef CONFIG_PECI
#undef CONFIG_PSTORE

/* Modules we want to exclude */
#undef CONFIG_CMD_BATTFAKE
#undef CONFIG_CMD_FLASH
#undef CONFIG_CMD_HASH
#undef CONFIG_CMD_MD
#undef CONFIG_CMD_POWERINDEBUG
#undef CONFIG_CMD_TIMERINFO

#undef CONFIG_CMD_APTHROTTLE
#undef CONFIG_CMD_MMAPINFO
#undef CONFIG_CMD_PWR_AVG
#undef CONFIG_CMD_REGULATOR
#undef CONFIG_CMD_RW
#undef CONFIG_CMD_SHMEM
#undef CONFIG_CMD_SLEEPMASK
#undef CONFIG_CMD_SLEEPMASK_SET
#undef CONFIG_CMD_SYSLOCK
#undef CONFIG_HOSTCMD_FLASHPD
#undef CONFIG_HOSTCMD_RWHASHPD

#ifndef __ASSEMBLER__
#ifdef VARIANT_KUKUI_DP_MUX_GPIO
void board_set_dp_mux_control(int output_enable, int polarity);
#endif /* VARIANT_KUKUI_DP_MUX_GPIO */
#endif /* !__ASSEMBLER__ */

#endif /* __CROS_EC_BASEBOARD_H */
