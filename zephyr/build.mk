# Copyright 2019 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EC_DIR = $(shell pwd)
ZEPHYR_DIR = $(EC_DIR)/../zephyr
ZEPHYR_BUILD_DIR = $(ZEPHYR_DIR)/build

ZEPHYR_ARCHIVES = $(patsubst ./%,%,$(shell cd $(ZEPHYR_BUILD_DIR) && find -name "*.a"))
ZEPHYRFILES = $(patsubst %,%.zephyr,$(ZEPHYR_ARCHIVES))

%.zcopy:
	$(Q)echo -e "  ZCOPY   $@"
	$(Q)mkdir -p "$(dir $@)"
	$(Q)cp $(ZEPHYR_BUILD_DIR)/$(shell sed -e 's#build/[^/]*/[^/]*/zephyr/\(.*\)$$#\1#' <<<$(basename $@)) $@

%.zephyr: %.zcopy
	$(Q)echo -e "  Z       $@"
	$(Q)$(OBJCOPY) --redefine-sym __reset=zephyr___reset \
	               --redefine-sym memset=zephyr_memset \
	               --redefine-sym __start=zephyr___start \
	               "$<" "$@"

zephyr-y+=zjump.o
zephyr-y+=llsr.o
zephyr-y+=$(ZEPHYRFILES)
