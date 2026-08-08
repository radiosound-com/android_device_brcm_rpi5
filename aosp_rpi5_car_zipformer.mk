#
# Copyright (C) 2026 Radio Sound, Inc.
# SPDX-License-Identifier: Apache-2.0
#
# Waveshare/NVMe Caramel Vanilla product for 4 GB Pi 5 boards using the
# streaming INT8 Zipformer backend. This is the highest-quality profile that
# stays within the 4 GB Pi's practical RAM budget in current benchmarks.
#
CARAMEL_VOICE_ASR_MODEL := zipformer-int8

$(call inherit-product, device/brcm/rpi5/aosp_rpi5_car.mk)

PRODUCT_NAME := aosp_rpi5_car_zipformer
PRODUCT_MODEL := Pi 5 Zipformer Voice
