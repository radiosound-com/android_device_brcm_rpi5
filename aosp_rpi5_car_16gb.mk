#
# Copyright (C) 2026 Radio Sound, Inc.
# SPDX-License-Identifier: Apache-2.0
#
# Waveshare/NVMe Caramel Vanilla product for Raspberry Pi 5 boards with 16 GB
# RAM. This variant selects a higher-capacity Zipformer context profile to improve
# command recovery on longer/complex utterances. Storage/display/audio settings and
# PCIe overrides remain independently configurable.
#

CARAMEL_VOICE_ASR_MODEL := zipformer-int8-highmem
CARAMEL_VOICE_TTS := kokoro

$(call inherit-product, device/brcm/rpi5/aosp_rpi5_car.mk)

PRODUCT_NAME := aosp_rpi5_car_16gb
PRODUCT_MODEL := Pi 5 16GB Neural Voice
