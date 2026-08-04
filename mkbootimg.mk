#
# Copyright (C) 2021-2022 KonstaKANG
#
# SPDX-License-Identifier: Apache-2.0
#

DEVICE_PATH := device/brcm/rpi5
KERNEL_PATH := device/brcm/rpi5-kernel

RPI_BOOT_OUT := $(PRODUCT_OUT)/rpiboot
RPI5_DISPLAY ?= waveshare10_1
$(RPI_BOOT_OUT): $(INSTALLED_RAMDISK_TARGET)
	mkdir -p $(RPI_BOOT_OUT)
	mkdir -p $(RPI_BOOT_OUT)/overlays
	cp $(DEVICE_PATH)/boot/* $(RPI_BOOT_OUT)
	if [ "$(RPI5_DISPLAY)" = "waveshare10_1" ]; then :; \
	elif [ "$(RPI5_DISPLAY)" = "hdmi" ]; then cp $(DEVICE_PATH)/boot/config.txt.hdmi $(RPI_BOOT_OUT)/config.txt; \
	else echo "Unsupported RPI5_DISPLAY '$(RPI5_DISPLAY)'; use waveshare10_1 or hdmi"; exit 1; fi
	cp $(KERNEL_PATH)/Image $(RPI_BOOT_OUT)
	cp $(KERNEL_PATH)/bcm2712*-rpi-*.dtb $(RPI_BOOT_OUT)
	cp $(KERNEL_PATH)/overlays/* $(RPI_BOOT_OUT)/overlays
	cp $(PRODUCT_OUT)/ramdisk.img $(RPI_BOOT_OUT)
	echo $(BOARD_KERNEL_CMDLINE) > $(RPI_BOOT_OUT)/cmdline.txt

$(INSTALLED_BOOTIMAGE_TARGET): $(RPI_BOOT_OUT)
	$(call pretty,"Target boot image: $@")
	dd if=/dev/zero of=$@ bs=1M count=128
	mkfs.fat -F 32 -n "boot" $@
	mcopy -s -i $@ $(RPI_BOOT_OUT)/* ::
