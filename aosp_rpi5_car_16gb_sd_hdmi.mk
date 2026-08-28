# Copyright 2026 Radio Sound, Inc.
# SPDX-License-Identifier: Apache-2.0

RPI5_STORAGE := sd
RPI5_DISPLAY := hdmi
RPI5_AUDIO := usb
$(call inherit-product, device/brcm/rpi5/aosp_rpi5_car_16gb.mk)
PRODUCT_NAME := aosp_rpi5_car_16gb_sd_hdmi
PRODUCT_MODEL := Pi 5 16GB SD HDMI
