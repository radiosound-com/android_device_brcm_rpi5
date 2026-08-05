#!/bin/bash

#
# Copyright (C) 2021-2022 KonstaKANG
#
# SPDX-License-Identifier: Apache-2.0
#

exit_with_error() {
  echo $@
  exit 1
}

if [ -z ${TARGET_PRODUCT} ]; then
  exit_with_error "TARGET_PRODUCT environment variable is not set. Run lunch first."
fi

if [ -z ${ANDROID_PRODUCT_OUT} ]; then
  exit_with_error "ANDROID_PRODUCT_OUT environment variable is not set. Run lunch first."
fi

for PARTITION in "boot" "system" "vendor"; do
  if [ ! -f ${ANDROID_PRODUCT_OUT}/${PARTITION}.img ]; then
    exit_with_error "Partition image not found. Run 'make ${PARTITION}image' first."
  fi
done

VERSION=RaspberryVanillaAOSP16
DATE=$(date +%Y%m%d)
TARGET=$(echo ${TARGET_PRODUCT} | sed 's/^aosp_//')
IMGNAME=${VERSION}-${DATE}-${TARGET}.img

# Keep the historical 15.3 GB default, but allow a reproducible image to be
# sized for the actual target medium. The last primary partition (userdata)
# consumes all remaining sectors after the fixed boot/system/vendor/metadata
# layout, so a larger image directly yields a larger userdata filesystem.
DEFAULT_IMAGE_SIZE_BYTES=15360000000
IMGSIZE=${RPI5_IMAGE_SIZE_BYTES:-${DEFAULT_IMAGE_SIZE_BYTES}}
if ! [[ ${IMGSIZE} =~ ^[0-9]+$ ]]; then
  exit_with_error "RPI5_IMAGE_SIZE_BYTES must be a decimal byte count"
fi

# Leave room for the fixed partitions and fdisk alignment. This catches a
# typo before fallocate/truncate creates an unusable image.
MIN_IMAGE_SIZE_BYTES=7700000000
if [ ${IMGSIZE} -lt ${MIN_IMAGE_SIZE_BYTES} ]; then
  exit_with_error "RPI5_IMAGE_SIZE_BYTES is too small for the RPi 5 layout"
fi

BOOT_PARTITION_SIZE=128
SYSTEM_PARTITION_SIZE=3072
VENDOR_PARTITION_SIZE=384
METADATA_PARTITION_SIZE=16
EXTENDED_PARTITION_SIZE=$((${SYSTEM_PARTITION_SIZE}+${VENDOR_PARTITION_SIZE}+${METADATA_PARTITION_SIZE}+4))

if [ -f ${ANDROID_PRODUCT_OUT}/${IMGNAME} ]; then
  exit_with_error "${ANDROID_PRODUCT_OUT}/${IMGNAME} already exists!"
fi

echo "Creating ${IMGSIZE}-byte image file ${ANDROID_PRODUCT_OUT}/${IMGNAME}..."
# A sparse file avoids allocating all unused userdata blocks on the build
# host; the final raw image still has the requested full logical capacity.
truncate -s ${IMGSIZE} ${ANDROID_PRODUCT_OUT}/${IMGNAME}
sync

echo "Creating partitions..."
(
echo o

echo n
echo p
echo 1
echo
echo +${BOOT_PARTITION_SIZE}M

echo n
echo e
echo 2
echo
echo +${EXTENDED_PARTITION_SIZE}M

echo n
echo l
echo
echo +${SYSTEM_PARTITION_SIZE}M

echo n
echo l
echo
echo +${VENDOR_PARTITION_SIZE}M

echo n
echo l
echo
echo +${METADATA_PARTITION_SIZE}M

echo n
echo p
echo 3
echo
echo

echo t
echo 1
echo c
echo a
echo 1

echo w
) | sudo fdisk ${ANDROID_PRODUCT_OUT}/${IMGNAME}
sync

LOOPDEV=$(sudo kpartx -av ${ANDROID_PRODUCT_OUT}/${IMGNAME} | awk 'NR==1{ sub(/p[0-9]$/, "", $3); print $3 }')
if [ -z ${LOOPDEV} ]; then
  exit_with_error "Unable to find loop device!"
fi
echo "Image mounted as /dev/${LOOPDEV}"
sleep 1

echo "Copying boot..."
sudo dd if=${ANDROID_PRODUCT_OUT}/boot.img of=/dev/mapper/${LOOPDEV}p1 bs=1M
echo "Copying system..."
sudo dd if=${ANDROID_PRODUCT_OUT}/system.img of=/dev/mapper/${LOOPDEV}p5 bs=1M
echo "Copying vendor..."
sudo dd if=${ANDROID_PRODUCT_OUT}/vendor.img of=/dev/mapper/${LOOPDEV}p6 bs=1M
echo "Creating metadata..."
sudo mkfs.ext4 /dev/mapper/${LOOPDEV}p7 -I 512 -L metadata
echo "Creating userdata..."
sudo mkfs.ext4 /dev/mapper/${LOOPDEV}p3 -I 512 -L userdata
sync

sudo kpartx -d "/dev/${LOOPDEV}"
sudo losetup -d "/dev/${LOOPDEV}"
sudo chown ${USER}:${USER} ${ANDROID_PRODUCT_OUT}/${IMGNAME}

echo "Done, created ${ANDROID_PRODUCT_OUT}/${IMGNAME}!"
exit 0
