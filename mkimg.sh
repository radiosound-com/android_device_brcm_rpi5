#!/bin/bash

# Copyright (C) 2026 Radio Sound, Inc.
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

die() {
  echo "$*" >&2
  exit 1
}

[[ -n "${TARGET_PRODUCT:-}" ]] || die "TARGET_PRODUCT environment variable is not set. Run lunch first."
[[ -n "${ANDROID_PRODUCT_OUT:-}" ]] || die "ANDROID_PRODUCT_OUT environment variable is not set. Run lunch first."

[[ -f "${ANDROID_PRODUCT_OUT}/boot.img" ]] || \
  die "Partition image not found: boot.img. Run 'make bootimage' first."
SYSTEM_IMAGE="${ANDROID_PRODUCT_OUT}/system.img"
if [[ ! -f "$SYSTEM_IMAGE" ]]; then
  exact_system_image="${ANDROID_PRODUCT_OUT}/obj/PACKAGING/target_files_intermediates/${TARGET_PRODUCT}-target_files/IMAGES/system.img"
  if [[ -f "$exact_system_image" ]]; then
    SYSTEM_IMAGE="$exact_system_image"
  else
    SYSTEM_IMAGE=$(find "${ANDROID_PRODUCT_OUT}/obj/PACKAGING/target_files_intermediates" \
        -type f -path '*/IMAGES/system.img' -print -quit 2>/dev/null || true)
  fi
fi
[[ -f "$SYSTEM_IMAGE" ]] || \
  die "Partition image not found: system.img; build target-files-package first"
VENDOR_IMAGE="${ANDROID_PRODUCT_OUT}/vendor.img"
if [[ ! -f "$VENDOR_IMAGE" ]]; then
  exact_vendor_image="${ANDROID_PRODUCT_OUT}/obj/PACKAGING/target_files_intermediates/${TARGET_PRODUCT}-target_files/IMAGES/vendor.img"
  if [[ -f "$exact_vendor_image" ]]; then
    VENDOR_IMAGE="$exact_vendor_image"
  else
    VENDOR_IMAGE=$(find "${ANDROID_PRODUCT_OUT}/obj/PACKAGING/target_files_intermediates" \
        -type f -path '*/IMAGES/vendor.img' -print -quit 2>/dev/null || true)
  fi
fi
[[ -f "$VENDOR_IMAGE" ]] || \
  die "Partition image not found: vendor.img; build target-files-package first"

VERSION=RaspberryVanillaAOSP17
DATE=$(date +%Y%m%d)
TARGET=${TARGET_PRODUCT#aosp_}
IMGNAME=${VERSION}-${DATE}-${TARGET}-ab.img
DEFAULT_IMAGE_SIZE_BYTES=$((10 * 1024 * 1024 * 1024))
IMGSIZE=${RPI5_IMAGE_SIZE_BYTES:-$DEFAULT_IMAGE_SIZE_BYTES}
[[ "$IMGSIZE" =~ ^[0-9]+$ ]] || die "RPI5_IMAGE_SIZE_BYTES must be a decimal byte count"

# A/B fixed partitions consume about 7.1 GiB before userdata. Keep a small but
# useful userdata seed so the image is flashable on a 16 GiB card;
# expand-rpi5-userdata grows partition 9 after flashing to a larger card or NVMe
# drive.
MIN_IMAGE_SIZE_BYTES=$((9 * 1024 * 1024 * 1024))
(( IMGSIZE >= MIN_IMAGE_SIZE_BYTES )) || \
  die "RPI5_IMAGE_SIZE_BYTES is too small for the A/B layout (minimum 9 GiB)"

BOOT_MIB=128
SYSTEM_MIB=3072
VENDOR_MIB=384
MISC_MIB=16
METADATA_MIB=64

IMG=${ANDROID_PRODUCT_OUT}/${IMGNAME}
[[ ! -e "$IMG" ]] || die "$IMG already exists"

echo "Creating sparse ${IMGSIZE}-byte GPT A/B image: ${IMG}"
truncate -s "$IMGSIZE" "$IMG"

sudo sgdisk --zap-all "$IMG" >/dev/null
sudo sgdisk \
  -n 1:0:+${BOOT_MIB}M -c 1:boot_a -t 1:0700 \
  -n 2:0:+${BOOT_MIB}M -c 2:boot_b -t 2:0700 \
  -n 3:0:+${SYSTEM_MIB}M -c 3:system_a -t 3:8300 \
  -n 4:0:+${SYSTEM_MIB}M -c 4:system_b -t 4:8300 \
  -n 5:0:+${VENDOR_MIB}M -c 5:vendor_a -t 5:8300 \
  -n 6:0:+${VENDOR_MIB}M -c 6:vendor_b -t 6:8300 \
  -n 7:0:+${MISC_MIB}M -c 7:misc -t 7:8300 \
  -n 8:0:+${METADATA_MIB}M -c 8:metadata -t 8:8300 \
  -n 9:0:0 -c 9:userdata -t 9:8300 \
  -A 1:set:2 -A 2:set:2 "$IMG" >/dev/null
sync

LOOPDEV=$(sudo kpartx -av "$IMG" | awk 'NR == 1 { sub(/p[0-9]+$/, "", $3); print $3 }')
[[ -n "$LOOPDEV" ]] || die "Unable to create partition mappings"
cleanup() {
  sudo kpartx -d "/dev/${LOOPDEV}" >/dev/null 2>&1 || true
  sudo losetup -d "/dev/${LOOPDEV}" >/dev/null 2>&1 || true
}
trap cleanup EXIT
sleep 1

copy_partition() {
  local source=$1 target=$2
  echo "Writing ${source} to ${target}"
  sudo dd if="$source" of="/dev/mapper/${LOOPDEV}p${target}" bs=1M conv=fsync status=none
}

copy_partition "${ANDROID_PRODUCT_OUT}/boot.img" 1
copy_partition "${ANDROID_PRODUCT_OUT}/boot.img" 2
copy_partition "$SYSTEM_IMAGE" 3
copy_partition "$SYSTEM_IMAGE" 4
copy_partition "$VENDOR_IMAGE" 5
copy_partition "$VENDOR_IMAGE" 6

echo "Creating raw misc, metadata, and userdata filesystems"
sudo dd if=/dev/zero of="/dev/mapper/${LOOPDEV}p7" bs=1M count=${MISC_MIB} conv=fsync status=none
sudo mkfs.ext4 -F -I 512 -L metadata "/dev/mapper/${LOOPDEV}p8" >/dev/null
sudo mkfs.ext4 -F -I 512 -L userdata "/dev/mapper/${LOOPDEV}p9" >/dev/null
sync
sudo chown "${USER}:${USER}" "$IMG"
trap - EXIT
cleanup
echo "Created ${IMG}"
