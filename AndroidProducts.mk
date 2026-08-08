#
# Copyright (C) 2021-2023 KonstaKANG
#
# SPDX-License-Identifier: Apache-2.0
#

PRODUCT_MAKEFILES := \
    $(LOCAL_DIR)/aosp_rpi5.mk \
    $(LOCAL_DIR)/aosp_rpi5_car.mk \
    $(LOCAL_DIR)/aosp_rpi5_car_zipformer.mk \
    $(LOCAL_DIR)/aosp_rpi5_car_zipformer_kokoro.mk \
    $(LOCAL_DIR)/aosp_rpi5_car_lgraph.mk \
    $(LOCAL_DIR)/aosp_rpi5_car_lgraph_kokoro.mk \
    $(LOCAL_DIR)/aosp_rpi5_car_16gb.mk \
    $(LOCAL_DIR)/aosp_rpi5_car_emmc.mk \
    $(LOCAL_DIR)/aosp_rpi5_car_hdmi.mk \
    $(LOCAL_DIR)/aosp_rpi5_car_emmc_hdmi.mk \
    $(LOCAL_DIR)/aosp_rpi5_tv.mk

COMMON_LUNCH_CHOICES := \
    aosp_rpi5-trunk_staging-userdebug \
    aosp_rpi5_car-trunk_staging-userdebug \
    aosp_rpi5_car-caramel-userdebug \
    aosp_rpi5_car_zipformer-caramel-userdebug \
    aosp_rpi5_car_zipformer_kokoro-caramel-userdebug \
    aosp_rpi5_car_lgraph-caramel-userdebug \
    aosp_rpi5_car_lgraph_kokoro-caramel-userdebug \
    aosp_rpi5_car_16gb-caramel-userdebug \
    aosp_rpi5_car_emmc-trunk_staging-userdebug \
    aosp_rpi5_car_emmc-caramel-userdebug \
    aosp_rpi5_car_hdmi-trunk_staging-userdebug \
    aosp_rpi5_car_hdmi-caramel-userdebug \
    aosp_rpi5_car_emmc_hdmi-trunk_staging-userdebug \
    aosp_rpi5_car_emmc_hdmi-caramel-userdebug \
    aosp_rpi5_tv-trunk_staging-userdebug
