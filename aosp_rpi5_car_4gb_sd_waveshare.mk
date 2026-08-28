# Copyright 2026 Radio Sound, Inc.
# SPDX-License-Identifier: Apache-2.0

RPI5_STORAGE := sd
RPI5_DISPLAY := waveshare10_1
RPI5_AUDIO := usb
$(call inherit-product, device/brcm/rpi5/aosp_rpi5_car_zipformer.mk)
PRODUCT_NAME := aosp_rpi5_car_4gb_sd_waveshare
PRODUCT_MODEL := Pi 5 4GB SD Waveshare
