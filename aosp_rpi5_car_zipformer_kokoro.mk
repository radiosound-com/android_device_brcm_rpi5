#
# Copyright (C) 2026 Radio Sound, Inc.
# SPDX-License-Identifier: Apache-2.0
#
# Waveshare/NVMe Caramel Vanilla product for 4 GB Pi 5 boards using the
# streaming INT8 Zipformer backend plus Kokoro speech for natural TTS.
#
CARAMEL_VOICE_ASR_MODEL := zipformer-int8
CARAMEL_VOICE_TTS := kokoro

$(call inherit-product, device/brcm/rpi5/aosp_rpi5_car.mk)

PRODUCT_NAME := aosp_rpi5_car_zipformer_kokoro
PRODUCT_MODEL := Pi 5 Neural Voice
