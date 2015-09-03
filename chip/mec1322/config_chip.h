/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_CONFIG_CHIP_H
#define __CROS_EC_CONFIG_CHIP_H

/* CPU core BFD configuration */
#include "core/cortex-m/config_core.h"

/* Number of IRQ vectors on the NVIC */
#define CONFIG_IRQ_COUNT	93

/* Use a bigger console output buffer */
#undef CONFIG_UART_TX_BUF_SIZE
#define CONFIG_UART_TX_BUF_SIZE	2048

/* Interval between HOOK_TICK notifications */
#define HOOK_TICK_INTERVAL_MS	250
#define HOOK_TICK_INTERVAL	(HOOK_TICK_INTERVAL_MS * MSEC)

/* Maximum number of deferrable functions */
#define DEFERRABLE_MAX_COUNT	8

/*
 * Number of I2C controllers. Controller 0 has 2 ports, so the chip has one
 * additional port.
 */
#define CONFIG_I2C_MULTI_PORT_CONTROLLER

#define I2C_CONTROLLER_COUNT	4
#define I2C_PORT_COUNT		5

/****************************************************************************/
/* Memory mapping */

/*
 * The memory region for RAM is actually 0x00100000-0x00120000.
 * RAM for Loader = 2k
 * RAM for RO/RW = 24k
 * CODE size of the Loader is 4k
 * As per the above configuartion  the upper 26k
 * is used to store data.The rest is for code.
 * the lower 100K is flash[ 4k Loader and 96k RO/RW],
 * and the higher 26K is RAM shared by loader and RO/RW.
 */

/****************************************************************************/
/* Define our RAM layout. */

#define CONFIG_MEC_SRAM_BASE_START	0x00100000
#define CONFIG_MEC_SRAM_BASE_END	0x00120000
#define CONFIG_MEC_SRAM_SIZE		(CONFIG_MEC_SRAM_BASE_END - \
					CONFIG_MEC_SRAM_BASE_START)

/* 2k RAM for Loader */
#define CONFIG_RAM_SIZE_LOADER		0x00000800
/* 24k RAM for RO /RW */
#define CONFIG_RAM_SIZE_RORW		0x00006000

#define CONFIG_RAM_SIZE_TOTAL		(CONFIG_RAM_SIZE_LOADER + \
					CONFIG_RAM_SIZE_RORW)
#define CONFIG_RAM_BASE_RORW		(CONFIG_MEC_SRAM_BASE_END - \
					CONFIG_RAM_SIZE_TOTAL)
#define CONFIG_RAM_BASE			CONFIG_RAM_BASE_RORW
#define CONFIG_RAM_SIZE			CONFIG_RAM_SIZE_TOTAL

/* System stack size */
#define CONFIG_STACK_SIZE		4096

/* non-standard task stack sizes */
#define IDLE_TASK_STACK_SIZE		512
#define LARGER_TASK_STACK_SIZE		640

#define CHARGER_TASK_STACK_SIZE		640
#define HOOKS_TASK_STACK_SIZE		640
#define CONSOLE_TASK_STACK_SIZE		640
#define HOST_CMD_TASK_STACK_SIZE	640

/* Default task stack size */
#define TASK_STACK_SIZE			512

/****************************************************************************/
/* Define our flash layout. */

/* Protect bank size 4K bytes */
#define CONFIG_FLASH_BANK_SIZE		0x00001000
/* Sector erase size 4K bytes */
#define CONFIG_FLASH_ERASE_SIZE		0x00001000
/* Minimum write size */
#define CONFIG_FLASH_WRITE_SIZE		0x00000004

/* One page size for write */
#define CONFIG_FLASH_WRITE_IDEAL_SIZE	256

/* Independent of the Flash Physical size of the board
256KB Max size used. Located at the top most segment */
#define CONFIG_FLASH_PHYSICAL_SIZE	0x00040000

/* Program memory base address */
#define CONFIG_FLASH_BASE		0x00100000

#define CONFIG_CDRAM_BASE		0x00100000
#define CONFIG_CDRAM_SIZE		0x00020000

#include "config_flash_layout.h"

/****************************************************************************/
/* Customize the build */
/* Optional features present on this chip */
#if 0
#define CONFIG_ADC
#define CONFIG_PECI
#define CONFIG_MPU
#endif
#define CONFIG_DMA
#define CONFIG_I2C
#define CONFIG_LPC
#define CONFIG_SPI
#define CONFIG_SWITCH

#define GPIO_PIN(index) (index / 10), (1 << (index % 10))
#define GPIO_PIN_MASK(pin, mask) (pin), (mask)

#endif  /* __CROS_EC_CONFIG_CHIP_H */
