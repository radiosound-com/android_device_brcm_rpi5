#
# Copyright (C) 2026 Radio Sound, Inc.
# SPDX-License-Identifier: Apache-2.0
#
# Waveshare/NVMe Caramel Vanilla product for 4 GB Pi 5 boards using the
# larger Vosk graph and the neural Kokoro TTS engine.
#

CARAMEL_VOICE_ASR_MODEL := lgraph
CARAMEL_VOICE_TTS := kokoro

$(call inherit-product, device/brcm/rpi5/aosp_rpi5_car.mk)

PRODUCT_NAME := aosp_rpi5_car_lgraph_kokoro
PRODUCT_MODEL := Pi 5 Neural Voice
