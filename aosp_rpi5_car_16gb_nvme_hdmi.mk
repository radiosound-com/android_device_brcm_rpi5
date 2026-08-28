# Copyright 2026 Radio Sound, Inc.
# SPDX-License-Identifier: Apache-2.0

RPI5_STORAGE := nvme
RPI5_DISPLAY := hdmi
RPI5_AUDIO := usb
$(call inherit-product, device/brcm/rpi5/aosp_rpi5_car_16gb.mk)
PRODUCT_NAME := aosp_rpi5_car_16gb_nvme_hdmi
PRODUCT_MODEL := Pi 5 16GB NVMe HDMI
