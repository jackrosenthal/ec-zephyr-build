/* Copyright 2019 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.h"
#include "console.h"
#include "zephyr/zephyr.h"

static int command_zjump(int argc, char **argv)
{
	ccprints("Jump to Zephyr image");
	zephyr___reset();

	return EC_SUCCESS;
}
DECLARE_CONSOLE_COMMAND(zjump, command_zjump, "", "Jump to Zephyr image");
