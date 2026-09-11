#
# Copyright (C) 2021-2023 KonstaKANG
#
# SPDX-License-Identifier: Apache-2.0
#

DEVICE_PATH := device/brcm/rpi5

# Caramel's USB audio devices can expose ALSA nodes after Android's USB host
# callback.  The product-specific release config selects the compatible Aconfig
# defaults without changing unrelated AOSP products.
PRODUCT_RELEASE_CONFIG_MAPS += \
    $(DEVICE_PATH)/release/release_config_map.textproto

# Caramel Vanilla's reference unit uses the Waveshare 10.1-inch panel and an
# NVMe root device. Explicit product variants below select other supported
# storage/display combinations without editing the device tree by hand.
RPI5_STORAGE ?= nvme
RPI5_DISPLAY ?= waveshare10_1
# All public Caramel Vanilla images use USB audio.
RPI5_AUDIO := usb
# Use Raspberry Pi 5's supported PCIe Gen 2 speed. On the reference NVMe unit,
# Gen 3 reproducibly prevents onboard WiFi association; Gen 2 connects using
# the same Android kernel, firmware and network credentials. Gen 3 remains
# an explicit, uncertified hardware-specific opt-in:
#   RPI5_PCIE_GEN=3 m bootimage
RPI5_PCIE_GEN ?= 2
# Keep the reference NVMe link out of PCIe ASPM and the controller out of
# autonomous low-power states.  This favors throughput and reliability on the
# always-powered automotive unit.  Builders can restore the upstream kernel
# defaults with RPI5_NVME_POWER_POLICY=default.
RPI5_NVME_POWER_POLICY ?= performance
CARAMEL_VANILLA_EXCLUDE_CAR_APP_HOST := true

# The manifest omits SDV services; their standalone platform tests and display
# safety service must not enter this non-SDV product's Soong dependency graph.
# This excludes only unrelated sources, not unresolved dependencies generally.
PRODUCT_SOURCE_ROOT_DIRS += \
    -platform_testing/libraries/sdv \
    -platform_testing/tests/sdv \
    -platform_testing/libraries/automotive/display_safety \
    -platform_testing/tests/automotive/display_safety \
    -packages/services/display_safety

# Physical A/B OTA. Raspberry Pi firmware selects the active boot FAT
# partition; the device boot-control service keeps that selection in sync
# with Android's update_engine slot state.
AB_OTA_UPDATER := true
AB_OTA_PARTITIONS := boot system vendor
TARGET_OTA_ALLOW_NON_AB := false
PRODUCT_BUILD_GENERIC_OTA_PACKAGE := true
PRODUCT_PACKAGES += update_engine update_engine_client update_verifier bootctl
PRODUCT_PACKAGES += caramel-vanilla-bootctl-service rpi5-ab-tryboot

# Inherit device configuration
$(call inherit-product, $(DEVICE_PATH)/device.mk)

PRODUCT_AAPT_CONFIG := normal mdpi hdpi
PRODUCT_AAPT_PREF_CONFIG := hdpi
PRODUCT_CHARACTERISTICS := automotive,nosdcard

$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)
$(call inherit-product, packages/services/Car/car_product/build/car.mk)
# Copyright (C) 2026 Radio Sound, Inc. for the Caramel Vanilla product integration.
$(call inherit-product, vendor/radiosound/osmand/caramel_vanilla_osmand.mk)
ifneq ($(wildcard vendor/radiosound/templates-host/caramel_vanilla_templates_host.mk),)
$(call inherit-product, vendor/radiosound/templates-host/caramel_vanilla_templates_host.mk)
endif
$(call inherit-product, vendor/radiosound/aurora-store/caramel_vanilla_aurora_store.mk)
$(call inherit-product, vendor/radiosound/voiceassistant/caramel_voice.mk)
$(call inherit-product, vendor/radiosound/caramelstore/caramel_store.mk)

$(call enforce-product-packages-exist,Bluetooth CaramelStore CaramelVanillaAuroraStore Keyguard Launcher2 OverviewApp RotaryIME RotaryPlayground com.android.ranging display_compat_config libnfc_ndef libvariablespeed pppd vendor_tracing_descriptors)

# android.car
PRODUCT_PACKAGES += \
    liblargeparcelablejni

# Audio
PRODUCT_PACKAGES += \
    android.hardware.automotive.audiocontrol-service.example

PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/car/car_audio_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/car_audio_configuration.xml

# Bluetooth
PRODUCT_VENDOR_PROPERTIES += \
    bluetooth.device.class_of_device=38,4,8 \
    bluetooth.profile.asha.central.enabled=false \
    bluetooth.profile.bap.broadcast.assist.enabled=false \
    bluetooth.profile.bap.unicast.client.enabled=false \
    bluetooth.profile.bas.client.enabled=false \
    bluetooth.profile.ccp.server.enabled=false \
    bluetooth.profile.csip.set_coordinator.enabled=false \
    bluetooth.profile.hap.client.enabled=false \
    bluetooth.profile.hfp.ag.enabled=false \
    bluetooth.profile.hid.device.enabled=false \
    bluetooth.profile.hid.host.enabled=false \
    bluetooth.profile.map.server.enabled=false \
    bluetooth.profile.mcp.server.enabled=false \
    bluetooth.profile.opp.enabled=false \
    bluetooth.profile.pbap.server.enabled=false \
    bluetooth.profile.sap.server.enabled=false \
    bluetooth.profile.vcp.controller.enabled=false

# Broadcast radio
PRODUCT_PACKAGES += \
    android.hardware.broadcastradio-service.default

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.broadcastradio.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.broadcastradio.xml

# Camera
ENABLE_CAMERA_SERVICE := true

# CAN
PRODUCT_PACKAGES += \
    android.hardware.automotive.can-service

PRODUCT_PACKAGES += \
    canhalctrl \
    canhaldump \
    canhalsend

# Radio Sound Salted Caramel Vanilla CAN configuration and tools.
PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/car/canbus_config.proto:$(TARGET_COPY_OUT_VENDOR)/etc/canbus_config.pb

PRODUCT_PACKAGES += \
    canhalconfigurator-aidl


# Display
PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/car/display_settings.xml:$(TARGET_COPY_OUT_VENDOR)/etc/display_settings.xml

# EVS
PRODUCT_PACKAGES += \
    android.hardware.automotive.evs-v4l2 \
    cardisplayproxyd \
    CarEvsCameraPreviewApp \
    evs_app \
    evsmanagerd

PRODUCT_COPY_FILES += \
    packages/services/Car/cpp/evs/manager/aidl/init.evs.rc:$(TARGET_COPY_OUT_SYSTEM)/etc/init/init.evs.rc

PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/car/evs_config_override.json:${TARGET_COPY_OUT_VENDOR}/etc/automotive/evs/config_override.json

include packages/services/Car/cpp/evs/apps/sepolicy/evsapp.mk

# Keylayout
PRODUCT_PACKAGES += \
    generic_keylayout_rpi

$(call soong_config_set_bool,rpi_keylayout,use_generic_keylayout_rpi,true)

# Occupant awareness
PRODUCT_PACKAGES += \
    android.hardware.automotive.occupant_awareness@1.0-service

include packages/services/Car/car_product/occupant_awareness/OccupantAwareness.mk

# Overlays
PRODUCT_PACKAGES += \
    AndroidCarRpiOverlay \
    BluetoothRpiOverlay \
    CarServiceRpiOverlay \
    CarSystemUIRpiOverlay \
    CaramelVoiceDefaults \
    SettingsProviderRpiOverlay \
    WifiRpiOverlay

# Keep framework clients and Caramel's explicit PTT path on the same
# product-selected recognizer. The base framework overlay selects Vosk; the
# higher-priority overlay is installed only by Zipformer products.
ifneq ($(filter zipformer-int8 zipformer-int8-highmem,$(CARAMEL_VOICE_ASR_MODEL)),)
PRODUCT_PACKAGES += \
    CaramelZipformerFrameworkOverlay
endif

# The Waveshare DSI panel has a 30 Hz default mode in this board's reported
# mode list. Apply the user-scoped 60 Hz settings at boot only for Waveshare
# products; HDMI variants must retain their display's own mode policy.
ifeq ($(RPI5_DISPLAY),waveshare10_1)
RPI5_DISPLAY_REFRESH_RATE ?= 60.03
PRODUCT_VENDOR_PROPERTIES += \
    ro.vendor.rpi5.display.refresh_rate=$(RPI5_DISPLAY_REFRESH_RATE)

PRODUCT_PACKAGES += \
    CaramelWaveshareDisplayDefaults
endif

# Permissions
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.software.activities_on_secondary_displays.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.activities_on_secondary_displays.xml \
    frameworks/native/data/etc/car_core_hardware.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/car_core_hardware.xml \
    $(DEVICE_PATH)/permissions/default-permissions-rpi5.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/default-permissions/default-permissions-rpi5.xml \
    $(DEVICE_PATH)/permissions/privapp-permissions-rpi5.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/permissions/privapp-permissions-rpi5.xml

# Vehicle
PRODUCT_PACKAGES += \
    android.hardware.automotive.vehicle@V4-default-service

# Device identifier. This must come after all inclusions.
PRODUCT_DEVICE := rpi5
PRODUCT_NAME := aosp_rpi5_car
PRODUCT_BRAND := Raspberry
PRODUCT_MODEL := Pi 5
PRODUCT_MANUFACTURER := Raspberry
