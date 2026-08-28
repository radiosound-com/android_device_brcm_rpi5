#
# Copyright (C) 2021-2022 KonstaKANG
#
# SPDX-License-Identifier: Apache-2.0
#

DEVICE_PATH := device/brcm/rpi5
KERNEL_PATH := device/brcm/rpi5-kernel

RPI_BOOT_OUT := $(PRODUCT_OUT)/rpiboot
RPI_BOOT_INPUTS := $(wildcard $(DEVICE_PATH)/boot/*)
RPI5_DISPLAY ?= waveshare10_1
$(RPI_BOOT_OUT): $(INSTALLED_RAMDISK_TARGET) $(RPI_BOOT_INPUTS)
	mkdir -p $(RPI_BOOT_OUT)
	mkdir -p $(RPI_BOOT_OUT)/overlays
	cp $(DEVICE_PATH)/boot/* $(RPI_BOOT_OUT)
	if [ "$(RPI5_DISPLAY)" = "waveshare10_1" ]; then :; \
	elif [ "$(RPI5_DISPLAY)" = "hdmi" ]; then cp $(DEVICE_PATH)/boot/config.txt.hdmi $(RPI_BOOT_OUT)/config.txt; \
	else echo "Unsupported RPI5_DISPLAY '$(RPI5_DISPLAY)'; use waveshare10_1 or hdmi"; exit 1; fi
	if [ "$(RPI5_STORAGE)" = "nvme" ]; then \
		if [ "$(RPI5_PCIE_GEN)" = "2" ]; then :; \
		elif [ "$(RPI5_PCIE_GEN)" = "3" ]; then :; \
		else echo "Unsupported RPI5_PCIE_GEN '$(RPI5_PCIE_GEN)'; use 2 or 3"; exit 1; fi; \
		if [ "$(RPI5_NVME_POWER_POLICY)" = "performance" ]; then :; \
		elif [ "$(RPI5_NVME_POWER_POLICY)" = "default" ]; then :; \
		else echo "Unsupported RPI5_NVME_POWER_POLICY '$(RPI5_NVME_POWER_POLICY)'; use performance or default"; exit 1; fi; \
		echo "dtparam=pciex1" >> $(RPI_BOOT_OUT)/config.txt; \
		if [ "$(RPI5_PCIE_GEN)" = "3" ]; then echo "dtparam=pciex1_gen=3" >> $(RPI_BOOT_OUT)/config.txt; fi; \
	fi
	cp $(KERNEL_PATH)/Image $(RPI_BOOT_OUT)
	cp $(KERNEL_PATH)/bcm2712*-rpi-*.dtb $(RPI_BOOT_OUT)
	cp $(KERNEL_PATH)/overlays/* $(RPI_BOOT_OUT)/overlays
	cp $(PRODUCT_OUT)/ramdisk.img $(RPI_BOOT_OUT)
	if [ "$(RPI5_STORAGE)" = "nvme" ] && [ "$(RPI5_NVME_POWER_POLICY)" = "performance" ]; then \
		RPI5_OTA_CMDLINE="$(BOARD_KERNEL_CMDLINE) androidboot.boot_devices=axi/1000110000.pcie pcie_aspm.policy=performance nvme_core.default_ps_max_latency_us=0"; \
	elif [ "$(RPI5_STORAGE)" = "nvme" ]; then \
		RPI5_OTA_CMDLINE="$(BOARD_KERNEL_CMDLINE) androidboot.boot_devices=axi/1000110000.pcie"; \
	else \
		RPI5_OTA_CMDLINE="$(BOARD_KERNEL_CMDLINE)"; \
	fi; \
	printf '%s androidboot.slot_suffix=_a\n' "$$RPI5_OTA_CMDLINE" > $(RPI_BOOT_OUT)/cmdline_a.txt; \
	printf '%s androidboot.slot_suffix=_b\n' "$$RPI5_OTA_CMDLINE" > $(RPI_BOOT_OUT)/cmdline_b.txt; \
	rm -f $(RPI_BOOT_OUT)/cmdline.txt; \
	{ \
		echo '[all]'; \
		echo 'cmdline=cmdline_a.txt'; \
		echo '[boot_partition=2]'; \
		echo 'cmdline=cmdline_b.txt'; \
	} >> $(RPI_BOOT_OUT)/config.txt; \
	{ \
		echo '[all]'; \
		echo 'tryboot_a_b=1'; \
		echo 'boot_partition=1'; \
		echo '[tryboot]'; \
		echo 'boot_partition=2'; \
	} > $(RPI_BOOT_OUT)/autoboot.txt

$(INSTALLED_BOOTIMAGE_TARGET): $(RPI_BOOT_OUT)
	$(call pretty,"Target boot image: $@")
	dd if=/dev/zero of=$@ bs=1M count=128
	mkfs.fat -F 32 -n "boot" $@
	mcopy -s -i $@ $(RPI_BOOT_OUT)/* ::
