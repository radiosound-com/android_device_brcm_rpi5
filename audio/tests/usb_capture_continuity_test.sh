#!/bin/sh
# Copyright (C) 2026 Radio Sound, Inc.
# SPDX-License-Identifier: Apache-2.0

# Integration regression test for the USB-input MonoPipe handoff. The test
# intentionally exercises the real VoiceInteraction -> AudioRecord -> USB HAL
# path because the scheduling race only appears with a live ALSA producer.

set -eu

adb_bin=${ADB:-adb}
adb_server_port=${ADB_SERVER_PORT:-5037}
device_serial=${ANDROID_SERIAL:-192.168.1.56:5555}
capture_seconds=${CAPTURE_SECONDS:-2}

adb_cmd() {
    "$adb_bin" -P "$adb_server_port" -s "$device_serial" "$@"
}

cleanup() {
    adb_cmd shell cmd voiceinteraction hide >/dev/null 2>&1 || true
}
trap cleanup EXIT HUP INT TERM

adb_cmd get-state >/dev/null
adb_cmd logcat -c
adb_cmd shell cmd voiceinteraction show >/dev/null
sleep "$capture_seconds"

short_reads=$(adb_cmd logcat -d -v brief AHAL_StreamAlsa:W '*:S' \
    | awk '/incomplete data received/ { count++ } END { print count + 0 }')
record_overflows=$(adb_cmd logcat -d -v brief AudioFlinger:W '*:S' \
    | awk '/RecordThread: buffer overflow/ { count++ } END { print count + 0 }')

if [ "$short_reads" -ne 0 ]; then
    echo "FAIL: USB capture inserted silence for $short_reads short pipe reads" >&2
    exit 1
fi
if [ "$record_overflows" -ne 0 ]; then
    echo "FAIL: AudioRecord overflowed $record_overflows times" >&2
    exit 1
fi

echo "PASS: no USB capture underruns or AudioRecord overflows during ${capture_seconds}s recognition window"
