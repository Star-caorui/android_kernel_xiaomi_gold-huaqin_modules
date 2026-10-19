# SPDX-License-Identifier: GPL-2.0
# Huaqin's drivers for gold: their own external modules, built after kernel_device_modules and against its
# symbols, and against those given in KBUILD_EXTRA_SYMBOLS (the ones of stock's modules without source)
include $(wildcard $(KERNEL_SRC)/$(DEVICE_MODULES_REL_DIR)/Makefile.include)

modules modules_install headers_install clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS) $(KBUILD_EXTRA_SYMBOLS)" $(@)
