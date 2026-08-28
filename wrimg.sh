#!/bin/bash

# Copyright (C) 2026 Radio Sound, Inc.
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

die() { echo "$*" >&2; exit 1; }
[[ -n "${ANDROID_PRODUCT_OUT:-}" ]] || die "ANDROID_PRODUCT_OUT is not set. Run lunch first."
[[ $# -ge 1 && $# -le 2 ]] || die "Usage: $0 /dev/<whole-disk> [boot|system|vendor|wipe]"

DISK=$1
ACTION=${2:-all}
[[ -b "$DISK" ]] || die "Not a block device: $DISK"
[[ "$DISK" != *[0-9] ]] || die "Pass the whole disk, not a partition: $DISK"

part() {
  case "$DISK" in
    *nvme*|*mmcblk*) echo "${DISK}p$1" ;;
    *) echo "${DISK}$1" ;;
  esac
}

for number in 1 2 3 4 5 6 7 8; do
  [[ -b "$(part "$number")" ]] || die "Missing A/B partition $(part "$number")"
done

echo "Target: $DISK"
lsblk -o NAME,LABEL,SIZE "$DISK"
read -r -p "Type WRITE to continue: " confirmation
[[ "$confirmation" == WRITE ]] || die "Nothing written"

partition_image() {
  local image=$1
  local exact="${ANDROID_PRODUCT_OUT}/obj/PACKAGING/target_files_intermediates/${TARGET_PRODUCT}-target_files/IMAGES/${image}.img"
  if [[ -f "${ANDROID_PRODUCT_OUT}/${image}.img" ]]; then
    printf '%s\n' "${ANDROID_PRODUCT_OUT}/${image}.img"
  elif [[ -f "$exact" ]]; then
    printf '%s\n' "$exact"
  else
    find "${ANDROID_PRODUCT_OUT}/obj/PACKAGING/target_files_intermediates" \
        -type f -path "*/IMAGES/${image}.img" -print -quit
  fi
}

write_image() {
  local image=$1 number=$2
  local source
  source=$(partition_image "$image")
  [[ -n "$source" && -f "$source" ]] || die "Missing ${image}.img; build target-files-package first"
  sudo umount "$(part "$number")" 2>/dev/null || true
  sudo dd if="$source" of="$(part "$number")" bs=1M conv=fsync status=progress
}

case "$ACTION" in
  all)
    write_image boot 1; write_image boot 2
    write_image system 3; write_image system 4
    write_image vendor 5; write_image vendor 6
    ;;
  boot) write_image boot 1; write_image boot 2 ;;
  system) write_image system 3; write_image system 4 ;;
  vendor) write_image vendor 5; write_image vendor 6 ;;
  wipe)
    sudo umount "$(part 7)" "$(part 8)" 2>/dev/null || true
    sudo wipefs -a "$(part 7)" "$(part 8)"
    sudo dd if=/dev/zero of="$(part 7)" bs=1M count=16 conv=fsync status=none
    sudo mkfs.ext4 -F -I 512 -L userdata "$(part 8)" >/dev/null
    ;;
  *) die "Usage: $0 /dev/<whole-disk> [boot|system|vendor|wipe]" ;;
esac

sync
echo "Done"
