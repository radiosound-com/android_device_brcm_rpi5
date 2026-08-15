#
# Copyright (C) 2026 Radio Sound, Inc.
# SPDX-License-Identifier: Apache-2.0
#
# Waveshare/NVMe Caramel Vanilla product for 4 GB Pi 5 boards that have enough
# live headroom for Vosk's larger graph model. The compact product remains the
# conservative default; this product only changes the offline ASR profile.
#

CARAMEL_VOICE_ASR_MODEL := lgraph

$(call inherit-product, device/brcm/rpi5/aosp_rpi5_car.mk)

PRODUCT_NAME := aosp_rpi5_car_lgraph
PRODUCT_MODEL := Pi 5 Large ASR
