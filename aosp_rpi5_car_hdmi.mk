# Copyright 2026 Radio Sound, Inc.
# SPDX-License-Identifier: Apache-2.0

RPI5_STORAGE := nvme
RPI5_DISPLAY := hdmi
$(call inherit-product, device/brcm/rpi5/aosp_rpi5_car.mk)
PRODUCT_NAME := aosp_rpi5_car_hdmi
