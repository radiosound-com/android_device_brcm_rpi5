#
# Copyright (C) 2026 Radio Sound, Inc.
# SPDX-License-Identifier: Apache-2.0
#
# Waveshare/NVMe Caramel Vanilla product for Raspberry Pi 5 boards with 16 GB
# RAM. This is a model-capacity variant, not a different boot or storage
# layout; RPI5_STORAGE, RPI5_DISPLAY, RPI5_PCIE_GEN, and RPI5_AUDIO remain
# independently overridable at build time.
#

CARAMEL_VOICE_ASR_MODEL := lgraph
CARAMEL_VOICE_TTS := kokoro

$(call inherit-product, device/brcm/rpi5/aosp_rpi5_car.mk)

PRODUCT_NAME := aosp_rpi5_car_16gb
PRODUCT_MODEL := Pi 5 16GB
