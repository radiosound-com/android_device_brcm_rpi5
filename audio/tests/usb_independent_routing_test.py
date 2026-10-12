#!/usr/bin/env python3
"""Exercise real USB mic hotplug without disconnecting the playback device.

Requires root ADB, a capture-only USB microphone directly on a root-hub port,
and a separate USB sound card. The mic port is briefly disabled through sysfs
and always reenabled on exit. This generates real USB host disconnect events.
No media is started and no volume settings are changed.
"""

import argparse
import pathlib
import re
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--serial", required=True, help="ADB serial/address")
    parser.add_argument("--hardware-serial", required=True)
    parser.add_argument("--mic-port", required=True, help="USB sysfs port, e.g. 1-1")
    parser.add_argument("--output-card", required=True, type=int)
    parser.add_argument("--artifacts", required=True, type=pathlib.Path)
    args = parser.parse_args()
    if not re.fullmatch(r"\d+-\d+", args.mic_port):
        parser.error("mic-port must name a device directly on a root hub, e.g. 1-1")
    args.artifacts.mkdir(parents=True, exist_ok=True)

    def shell(command):
        return subprocess.check_output(
            ["adb", "-s", args.serial, "shell", command], text=True, timeout=20
        ).strip()

    assert shell("getprop ro.serialno") == args.hardware_serial, "Wrong device"
    assert shell("id -u") == "0", "Run adb root first"
    mic_path = "/sys/bus/usb/devices/" + args.mic_port
    bus, port = args.mic_port.split("-")
    port_disable = f"/sys/bus/usb/devices/usb{bus}/{bus}-0:1.0/usb{bus}-port{port}/disable"
    mic_card = int(shell("cat " + mic_path + "/*/sound/card*/number"))
    assert mic_card != args.output_card, "Mic and playback must be separate cards"
    pcm = shell("cat /proc/asound/pcm")
    mic_lines = [line for line in pcm.splitlines() if line.startswith(f"{mic_card:02d}-")]
    assert any("capture" in line for line in mic_lines), "Mic has no capture endpoint"
    assert not any("playback" in line for line in mic_lines), "Mic is not capture-only"
    assert any(line.startswith(f"{args.output_card:02d}-") and "playback" in line
               for line in pcm.splitlines()), "Output card has no playback endpoint"
    assert shell("cat " + mic_path + "/authorized") == "1", "Mic must start connected"
    assert shell("cat " + port_disable) == "0", "Mic port must start enabled"

    def ports(dump, direction):
        section = dump.split(" Available " + direction + " devices", 1)[1]
        section = re.split(r"\n (?:Available |Hardware )", section, maxsplit=1)[0]
        return {(int(card), int(device)): int(port) for port, card, device in re.findall(
            r"Port ID: (\d+);[^\n]*AUDIO_DEVICE_(?:OUT|IN)_USB_[^\n]*"
            r"card=(\d+);device=(\d+)", section)}

    def wait_input(card, present, name):
        deadline = time.monotonic() + 12
        while True:
            dump = shell("dumpsys media.audio_policy")
            if ((card, 0) in ports(dump, "input")) == present:
                (args.artifacts / (name + ".txt")).write_text(dump)
                return dump
            if time.monotonic() >= deadline:
                (args.artifacts / (name + ".txt")).write_text(dump)
                raise AssertionError("USB capture route did not settle: " + name)
            time.sleep(0.25)

    try:
        # Establish the working sound-card-only state before adding the mic.
        shell("echo 1 > " + port_disable)
        baseline = wait_input(mic_card, False, "sound-card-only")
        expected = ports(baseline, "output").get((args.output_card, 0))
        assert expected is not None, "Sound card is not available for playback"
        for cycle in range(1, 4):
            shell("echo 0 > " + port_disable)
            both = wait_input(mic_card, True, f"mic-attached-{cycle}")
            actual = ports(both, "output").get((args.output_card, 0))
            assert actual == expected, (
                f"Mic attachment displaced USB playback: port {expected} -> {actual}")
            assert (args.output_card, 0) not in ports(both, "input"), (
                "USB microphone did not replace the sound card's capture route")
            print(f"PASS cycle {cycle}: sound card playback port {actual}, mic capture", flush=True)
            shell("echo 1 > " + port_disable)
            removed = wait_input(mic_card, False, f"mic-removed-{cycle}")
            assert ports(removed, "output").get((args.output_card, 0)) == expected, (
                "Mic removal displaced USB playback")
            assert (args.output_card, 0) in ports(removed, "input"), (
                "Sound card capture did not recover after microphone removal")
        print("PASS: three mic attach/remove cycles retained the same playback port", flush=True)
    finally:
        shell("echo 0 > " + port_disable)


if __name__ == "__main__":
    main()
