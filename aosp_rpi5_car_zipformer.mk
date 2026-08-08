# Copyright 2026 Radio Sound, Inc.
# SPDX-License-Identifier: Apache-2.0

# High-quality streaming recognition profile validated on a 4 GB Pi 5. This
# keeps eSpeak as the lower-memory TTS default.
CARAMEL_VOICE_ASR_MODEL := zipformer-int8

$(call inherit-product, device/brcm/rpi5/aosp_rpi5_car.mk)

PRODUCT_NAME := aosp_rpi5_car_zipformer
PRODUCT_MODEL := Pi 5 Zipformer Voice
