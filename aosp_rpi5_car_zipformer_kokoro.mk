# Copyright 2026 Radio Sound, Inc.
# SPDX-License-Identifier: Apache-2.0

# Highest-quality offline recognition and speech pair validated on a 4 GB
# actively cooled Pi 5.
CARAMEL_VOICE_ASR_MODEL := zipformer-int8
CARAMEL_VOICE_TTS := kokoro

$(call inherit-product, device/brcm/rpi5/aosp_rpi5_car.mk)

PRODUCT_NAME := aosp_rpi5_car_zipformer_kokoro
PRODUCT_MODEL := Pi 5 Neural Voice
