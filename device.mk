#
# Copyright (C) 2021-2023 KonstaKANG
#
# SPDX-License-Identifier: Apache-2.0
#

DEVICE_PATH := device/brcm/rpi5

# The reference unit uses the Salted Caramel A2B/I2S path. A USB audio profile
# is available for development units whose microphone and speaker are a USB
# sound card. Keep this a product-time option so the default image remains
# unchanged:
#
#   m RPI5_AUDIO=usb -j8
RPI5_AUDIO ?= a2b
ifeq ($(RPI5_AUDIO),a2b)
RPI5_AUDIO_DEVICE := rpi
RPI5_SIMULATE_INPUT := true
else ifeq ($(RPI5_AUDIO),usb)
RPI5_AUDIO_DEVICE := usb
RPI5_SIMULATE_INPUT := false
else
$(error Unsupported RPI5_AUDIO '$(RPI5_AUDIO)'; use a2b or usb)
endif

PRODUCT_VENDOR_PROPERTIES += \
    persist.vendor.audio.device=$(RPI5_AUDIO_DEVICE) \
    ro.boot.audio.tinyalsa.simulate_input=$(RPI5_SIMULATE_INPUT)

$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit_only.mk)
$(call inherit-product, frameworks/native/build/tablet-7in-xhdpi-2048-dalvik-heap.mk)

# APEX
$(call inherit-product, $(SRC_TARGET_DIR)/product/updatable_apex.mk)
OVERRIDE_PRODUCT_COMPRESSED_APEX := false

# API level
PRODUCT_SHIPPING_API_LEVEL := 36

# Audio
PRODUCT_PACKAGES += \
    com.android.hardware.audio.rpi

PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/audio/audio_effects_config.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_effects_config.xml \
    $(DEVICE_PATH)/audio/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_configuration.xml \
    $(DEVICE_PATH)/audio/usb_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/usb_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml \
    frameworks/av/services/audiopolicy/config/bluetooth_audio_policy_configuration_7_0.xml:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth_audio_policy_configuration_7_0.xml \
    frameworks/av/services/audiopolicy/config/default_volume_tables.xml:$(TARGET_COPY_OUT_VENDOR)/etc/default_volume_tables.xml \
    frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/r_submix_audio_policy_configuration.xml

# Bluetooth
PRODUCT_PACKAGES += \
    com.android.hardware.bluetooth.rpi

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.bluetooth.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth.xml \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth_le.xml

# Camera
PRODUCT_PACKAGES += \
    com.android.hardware.camera.external.rpi

PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/camera/external/external_camera_config.xml:$(TARGET_COPY_OUT_VENDOR)/etc/external_camera_config.xml

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.camera.external.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.external.xml

PRODUCT_PACKAGES += \
    com.android.hardware.camera.libcamera

$(call soong_config_set,libcamera,ipa,pisp)

PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/camera/libcamera/camera_hal.yaml:$(TARGET_COPY_OUT_VENDOR)/etc/libcamera/camera_hal.yaml

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.camera.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.xml \
    frameworks/native/data/etc/android.hardware.camera.concurrent.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.concurrent.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.camera.full.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.full.xml \
    frameworks/native/data/etc/android.hardware.camera.raw.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.raw.xml

PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/camera/media_profiles_V1_0.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_profiles_V1_0.xml

# CEC
PRODUCT_PACKAGES += \
    com.android.hardware.tv.hdmi.cec.rpi \
    com.android.hardware.tv.hdmi.connection.rpi

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.hdmi.cec.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.hdmi.cec.xml

# DRM
PRODUCT_PACKAGES += \
    com.android.hardware.drm.clearkey

# Emergency info
PRODUCT_PACKAGES += \
    EmergencyInfo

# Ethernet
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.ethernet.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.ethernet.xml

# FFmpeg
PRODUCT_PACKAGES += \
    com.android.hardware.media.c2.ffmpeg

PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/media/media_codecs_ffmpeg_c2.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_ffmpeg_c2.xml

# Gatekeeper
PRODUCT_PACKAGES += \
    com.android.hardware.gatekeeper.nonsecure

# GPS
PRODUCT_PACKAGES += \
    android.hardware.gnss@1.0 \
    android.hardware.gnss@1.0-impl \
    android.hardware.gnss@1.0-service \
    gps.default

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.location.gps.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.location.gps.xml

# Graphics
PRODUCT_PACKAGES += \
    com.android.hardware.egl.mesa \
    com.android.hardware.graphics.allocator.minigbm_gbm_mesa \
    com.android.hardware.graphics.composer.drm_hwcomposer \
    com.android.hardware.vulkan.broadcom

# Health
PRODUCT_PACKAGES += \
    com.android.hardware.health.rpi

# GNSS
# The implementation is consumed directly from Mark777a's Apache-2.0 AIDL v6
# HAL repository by the Caramel Vanilla manifest.
PRODUCT_PACKAGES += \
    android.hardware.gnss-service.rpi

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.location.gps.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.location.gps.xml

# Kernel
PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)-kernel/Image:$(PRODUCT_OUT)/kernel

# Keymint
PRODUCT_PACKAGES += \
    com.android.hardware.keymint.rust_nonsecure

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.keystore.app_attest_key.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.keystore.app_attest_key.xml

# Lights
PRODUCT_PACKAGES += \
    com.android.hardware.light.rpi

# Media
PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/media/media_codecs.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_c2_audio.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_google_c2_audio.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_c2_tv.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_google_c2_tv.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_c2_video.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_google_c2_video.xml

# Power
PRODUCT_PACKAGES += \
    com.android.hardware.power

# Ramdisk
RPI5_STORAGE ?= emmc
ifeq ($(RPI5_STORAGE),nvme)
RPI5_FSTAB := $(DEVICE_PATH)/ramdisk/fstab.rpi5.nvme
else ifeq ($(RPI5_STORAGE),emmc)
RPI5_FSTAB := $(DEVICE_PATH)/ramdisk/fstab.rpi5.emmc
else
$(error Unsupported RPI5_STORAGE '$(RPI5_STORAGE)'; use nvme or emmc)
endif

PRODUCT_COPY_FILES += \
    $(RPI5_FSTAB):$(TARGET_COPY_OUT_RAMDISK)/fstab.rpi5 \
    $(RPI5_FSTAB):$(TARGET_COPY_OUT_VENDOR)/etc/fstab.rpi5 \
    $(DEVICE_PATH)/ramdisk/init.rpi5.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.rpi5.rc \
    $(DEVICE_PATH)/ramdisk/init.rpi5.usb.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.rpi5.usb.rc \
    $(DEVICE_PATH)/ramdisk/ueventd.rpi5.rc:$(TARGET_COPY_OUT_VENDOR)/etc/ueventd.rc

# Seccomp
PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/seccomp_policy/mediacodec.policy:$(TARGET_COPY_OUT_VENDOR)/etc/seccomp_policy/mediacodec.policy \
    $(DEVICE_PATH)/seccomp_policy/mediaswcodec.policy:$(TARGET_COPY_OUT_VENDOR)/etc/seccomp_policy/mediaswcodec.policy

# Soong
PRODUCT_SOONG_NAMESPACES += \
    $(DEVICE_PATH) \
    external/mark777a/AOSP-AIDL-v6-GNSS-HAL/vendor/brcm/interfaces/gnss/aidl

# Storage
$(call inherit-product, $(SRC_TARGET_DIR)/product/emulated_storage.mk)

# Suspend
PRODUCT_PACKAGES += \
    com.android.hardware.suspend_blocker.rpi

# Thermal
PRODUCT_PACKAGES += \
    com.android.hardware.thermal

# Touchscreen
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml

# USB
PRODUCT_PACKAGES += \
    com.android.hardware.usb \
    com.android.hardware.usb.gadget.rpi

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.software.midi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.midi.xml

# Virtualization
$(call inherit-product, packages/modules/Virtualization/apex/product_packages.mk)

# Wifi
PRODUCT_PACKAGES += \
    com.android.hardware.wifi \
    com.android.hardware.wifi.hostapd.rpi \
    com.android.hardware.wifi.supplicant.rpi \
    libwpa_client \
    wificond

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.wifi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.xml
