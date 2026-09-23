#!/usr/bin/env python3
"""Create a board-specific profile without editing/rebuilding C++.

GPIO assignment is opt-in; confirm board wiring and reset-state pull-down first.
"""
import argparse
import json
import pathlib
import re


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("--id", required=True)
    parser.add_argument("--amp-address", type=lambda s: int(s, 0), choices=(0x6C, 0x6D), required=True)
    parser.add_argument("--channel", choices=("left", "right"), default="left")
    parser.add_argument("--clkout", type=int, choices=(1, 2), default=2)
    parser.add_argument("--shutdown-io", type=int, choices=(4,), help="DTX1/IO4, after checking J4/D9 wiring")
    args = parser.parse_args()
    if not re.fullmatch(r"[a-z0-9_-]{1,64}", args.id):
        parser.error("ID must be 1..64 lowercase letters, digits, _ or -")
    profile = json.loads((pathlib.Path(__file__).parent / "profiles/tas5720a_1node.json").read_text())
    profile["id"] = args.id
    profile["description"] = f"TAS5720A {args.channel} channel; CLKOUT{args.clkout}; SPK_SD " + (
        "on IO4 (requires verified board modification)" if args.shutdown_io else "not controlled by GPIO")
    profile["peripherals"]["amp"]["address"] = args.amp_address
    sequences = profile["sequences"]
    for operations in sequences.values():
        for op in operations:
            if op.get("target") == "amp" and op.get("reg") == 6:
                op["value"] = 0x9B if args.channel == "left" else 0x99
                op["note"] = f"25 dBV, PWM 8fs, {args.channel} channel"
            if op.get("target") == "node0" and op.get("reg") in (0x59, 0x5A):
                # Init writes both clock registers; health checks the enabled one.
                if op["op"] == "verify":
                    op["reg"] = 0x58 + args.clkout
                op["value"] = 0x81 if op["reg"] == 0x58 + args.clkout else 0
                op["note"] = "12.288 MHz on selected CLKOUT; other output disabled"
    if args.shutdown_io is not None:
        def update(reg, value, note):
            return dict(op="update", target="node0", reg=reg, mask=16, value=value, note=note)
        def write(reg, note):
            return dict(op="write", target="node0", reg=reg, value=16, note=note)
        init = sequences["init"]
        position = next(i + 1 for i, op in enumerate(init)
                        if op.get("target") == "node0" and op.get("reg") == 3)
        init[position:position] = [
            write(0x4C, "Drive SPK_SD low before enabling IO4 output"),
            update(0x80, 0, "Disable GPIO forwarding for IO4"),
            update(0x4E, 0, "Disable IO4 input"),
            update(0x4D, 16, "Enable IO4 output; DTX1 remains disabled"),
        ]
        position = next(i for i, op in enumerate(init) if op.get("target") == "amp"
                        and op.get("reg") == 1 and op.get("value") == 1)
        init[position:position] = [write(0x4B, "Release physical SPK_SD after configuration"),
                                   dict(op="delay", ms=10)]
        sequences["shutdown"].append(write(0x4C, "Assert physical SPK_SD"))
    args.output.write_text(json.dumps(profile, indent=2) + "\n")


if __name__ == "__main__":
    main()
