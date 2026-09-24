# Raspberry Pi A2B audio

One HAL supports USB and A2B playback. The A2B prototype is Pi 5 → AD2428MINI
main → one AD2428MINI subordinate → one TAS5720A amplifier/speaker. Transport is
48 kHz stereo I²S with two 32-bit slots; the mono amplifier selects one channel.
The profile is an initial bench configuration, not a hardware-qualified release.

## Build

```sh
source build/envsetup.sh
lunch aosp_rpi5_car_4gb_nvme_waveshare-caramel-userdebug
# AD2428MINI subordinate: IO4 shutdown / CLKOUT1 bench wiring described below:
m -j8 RPI5_AUDIO=a2b RPI5_AUDIO_INPUT=usb RPI5_A2B_PROFILE=tas5720a_io4_clk1
```

| Option | Values | Default |
| --- | --- | --- |
| `RPI5_AUDIO` | `usb`, `a2b` | `usb` |
| `RPI5_AUDIO_INPUT` | `usb`, `none` (simulated input) | `usb` |
| `RPI5_A2B_PROFILE` | packaged JSON filename without extension | `tas5720a_1node` |

The build compiles `rpi-ad242x-overlay.dts` with AOSP's host `dtc` and packages
`overlays/rpi-ad242x.dtbo` in the boot filesystem. Only the A2B option enables
`dtoverlay=rpi-ad242x` in `config.txt`. The overlay exposes playback PCM device 0,
card ID `ad242x`. Selection uses that ID, never the first enumerated sound card.
USB microphone selection is independent of playback. Existing Android Sound and
status-bar volume controls continue to control the PCM stream's software gain.
These options select the primary speaker path. Android can still select an
external USB/Bluetooth output through normal audio policy. For A2B bench tests,
use the Speaker route and disconnect other playback endpoints; the USB microphone
can remain connected. `none` simulates primary input, not a global capture ban.

The HAL/APEX, CLI, SELinux policy and default profiles require an initial image
build/deployment. The vendor audio APEX is not an independently updatable APK.
Later profile changes do not require a HAL rebuild. Build properties are defaults:
an existing persistent output/profile selection survives an image update.

## Profiles and runtime changes

Default files live in `/vendor/etc/a2b/profiles/`. Root-owned overrides live in
`/data/vendor/a2b/profiles/` and survive reboot/OTA. An existing invalid override
is an error, never a reason to silently choose the vendor copy. Remove or repair
it explicitly. This maintenance CLI is for `userdebug`/`eng` images with `adb root`.

```sh
# On the development machine, generate a board-specific candidate:
python3 device/brcm/rpi5/audio/a2b/make_tas_profile.py speaker.json \
  --id speaker --amp-address 0x6c --channel left --clkout 1
# Add --shutdown-io 4 only AFTER verifying and making the board modification.

adb root
adb push speaker.json /data/local/tmp/speaker.json
adb shell 'a2bctl validate - < /data/local/tmp/speaker.json'
adb shell 'a2bctl install - < /data/local/tmp/speaker.json'
adb shell a2bctl select speaker
adb shell a2bctl status
# Play a low-level test signal, verify clocks/routing and then:
adb shell a2bctl unmute
```

`install` validates and atomically replaces the override, retaining the previous
override as `.previous`. `select ID` validates again and selects the profile. With
playback active it mutes, shuts down, initializes and verifies the new network
while the PCM thread maintains clocks with silence. It always returns muted;
use `unmute` explicitly. Without playback it stages the selection: hardware
initialization/readback happens when the next output stream starts.

A failed active reload attempts to restore the old in-memory profile, still
muted. The installed file remains available for diagnosis. `a2bctl rollback ID`
atomically restores `.previous` and selects it. Do this before restarting the
HAL if the override failed: a new HAL process reads the installed file afresh.
The first override has no `.previous`; keep your source copy or remove that
override to restore the vendor default. File edits alone are staging until
`select`, or until a fresh HAL process starts.

Other commands:

```sh
adb shell a2bctl mute
adb shell a2bctl shutdown     # Mute + software shutdown; prevent new activation
adb shell a2bctl resume       # Reinitialize, remain muted; unmute separately
adb shell a2bctl output usb   # Quiesce then restart audio; playback is interrupted
adb shell a2bctl output a2b   # Requires an enabled overlay and card ad242x
```

An A2B image retains USB support. A USB image contains the overlay file but needs
it enabled in the boot configuration and a reboot before A2B can be selected.
Do not use raw `i2cset` alongside the HAL: NODEADR/CHIP selection is shared state.
The HAL owns all bus operations, including control requests and periodic health
checks. The socket checks peer UID 0 and has mode 0600. Override files must be
root-owned regular files, not group/world writable. The HAL has read-only access.
The CLI transitions to a dedicated vendor SELinux domain. Shell input is passed
by descriptor (`-`); the CLI cannot open/traverse `/data/local/tmp` itself.

## Schema version 1

Use JSON integers (decimal; e.g. `108` is `0x6c`). Unknown fields, duplicate keys,
invalid targets and out-of-range values are rejected. `note` is an optional
human-readable comment on an operation. Maximum file size is 256 KiB, 2048 total
operations, 500 ms per delay, 1000 ms per poll and 10 seconds of waits per phase.

`nodes` lists transceiver vendor/product IDs in physical chain order: `node0` is
nearest the main. `peripherals` maps a name to a node index and 7-bit I²C address.
The executor translates these names to NODEADR, CHIP and PERI operations and
clears routing after every remote operation, including failures. Direct writes to those
routing registers are rejected. Discovery sequences must match the physical
topology: adding an entry to `nodes` alone does not discover another device.
For another topology, export/adapt its discovery and slot configuration into a
new profile, with explicit verification at each discovered node. Identification
checks identify the transceiver model, not the board or its serial number.

All five sequences are required:

| Sequence | Purpose |
| --- | --- |
| `init` | Discovery, clocks, slots, peripheral setup; leave amplifier muted |
| `mute` | Mute with settling delay while clocks remain present |
| `unmute` | Check faults and release mute |
| `shutdown` | Disable amplifier before PCM clocks stop |
| `health` | Read-only verification while a stream is active |

Operations: `write`, `update`, `verify`, `poll`, `delay`. Register operations
specify `target`, `reg`, `value`; all except `write` also require `mask`.
`update` preserves bits outside its mask; `verify`/`poll` compare the masked read.
`poll` adds `timeout_ms`; `delay` uses `ms` instead of register fields. Init/unmute
abort on failure. Mute/shutdown attempt remaining operations so a GPIO can still
disable the amp after a failed peripheral access. Health allows only `verify`.

No writes can prove arbitrary hardware configuration safe: root profile edits
are trusted engineering input. Readbacks prove transport/register behavior,
not signal integrity, phase, output power or board compatibility.

## TAS5720A and clock settings

The generic `tas5720a_1node` profile uses address **0x6c**, **left** channel and
**CLKOUT2**; confirm these against the actual wiring/straps. No shutdown GPIO is
assigned in that profile. Select one of the IO4 bench profiles below explicitly
to control physical shutdown; the generic profile is not interchangeable with them.
`start_muted: true` requires an explicit unmute after HAL startup. A root mute
selection is retained across stream standby within that HAL process.

| Register | Value | Meaning |
| --- | --- | --- |
| TAS `0x02` | `0x04` | 0 dB digital boost, single speed, I²S |
| TAS `0x04`, `0x05` | `0xce` | Both channel volumes −0.5 dB |
| TAS `0x06` | `0x9b` / `0x99` | Left / right, 25 dBV analog gain, PWM 8 × LRCLK |
| AD2428 node `0x5a` | `0x81` | CLKOUT2 12.288 MHz, non-inverted |

At 48 kHz, the AD2428 PLL is 2048fs = 98.304 MHz. CLKOUT's divide-by-2 prescaler
and `DIV=1` divide by another `2 × (1 + 1)`, producing 256fs = 12.288 MHz.
For CLKOUT1 use `0x59=0x81`, `0x5a=0`; both outputs share the recovered clock
domain with BCLK/LRCLK. CLKOUT overrides the corresponding IO1/IO2 GPIO function.
The Pi supplies 48 kHz LRCLK and 3.072 MHz BCLK; scope all three clocks before
unmuting. The TAS gain target is approximately 20 W into 3.5 Ω at adequate supply
voltage, **not a power limiter**. Start with a low-level signal and measure.

TAS register `0x03` bits 1:0 mute the two channels; bit 7 controls fade. Register
`0x01` bit 0 is active-low software shutdown and bit 1 requests sleep. The loader
uses masked writes to retain digital-clipper bits 7:2. Physical SPK_SD must also
be high to play. There is no separate MUTE pin. SPK_SLEEP/ADR is an address strap
in software mode. The default profile checks error register `0x08` and relevant
configuration once per second. A fault or PCM write failure silences PCM and
attempts shutdown; correct the cause, then `resume` and `unmute`. Hardware fault
protection remains necessary for loss of host power, clocks or communication.

## IO4 bench profiles

Two profiles package IO4 shutdown for one amplifier. Choose the MCLK output
that matches the physical wiring. Pin numbers below are IC pins, not
evaluation-board header positions.

| Profile | MCLK output | Clock registers |
| --- | --- | --- |
| `tas5720a_io4_clk1` | ADR1/IO1/CLKOUT1, pin 7 | `CLK1CFG=0x81`, `CLK2CFG=0` |
| `tas5720a_io4_clk2` | ADR2/IO2/CLKOUT2, pin 8 | `CLK1CFG=0`, `CLK2CFG=0x81` |

| Connection | AD2428 subordinate | TAS5720A |
| --- | --- | --- |
| Shutdown, low = disabled | DTX1/IO4, pin 13 | SPK_SD, pin 7; 10 kΩ pull-down |
| MCLK, 12.288 MHz | Selected CLKOUT pin above | MCLK, pin 14 |
| BCLK, 3.072 MHz | BCLK, pin 10 | SCLK, pin 15 |
| LRCLK, 48 kHz | SYNC, pin 11 | LRCK, pin 17 |
| Stereo I²S data | DTX0/IO3, pin 12 | SDIN, pin 16 |
| I²C | SDA pin 5 / SCL pin 4 | FREQ/SDA pin 8 / HW/SCL pin 9 |

Pull both TAS gain pins high for software control. Pull SPK_SLEEP/ADR (pin 13)
low for 7-bit address `0x6c`. Both profiles select the left channel; right-channel
selection remains a JSON/profile change. Both keep DTX1 disabled. The CLKOUT2
profile dedicates IO2 to MCLK: it must not also drive a power-control net such as
`12VSW_ON`. The IO4 pull-down holds shutdown during reset; initialization preloads
IO4 low, configures clocks and the muted amplifier, then releases IO4. Shutdown
attempts to clear IO4 even if the amplifier's I²C shutdown write fails. Health
checks verify the output latch, output enable and GPIO mux settings; these are
register readbacks, not measurements of the voltage at SPK_SD.

Regenerate the CLKOUT1 profile (use `--clkout 2` with the CLKOUT2 ID/path for
the other wiring, or substitute `--channel right` and a new ID):

```sh
python3 device/brcm/rpi5/audio/a2b/make_tas_profile.py \
  device/brcm/rpi5/audio/a2b/profiles/tas5720a_io4_clk1.json \
  --id tas5720a_io4_clk1 --amp-address 0x6c --channel left --clkout 1 --shutdown-io 4
# After initial deployment, select the packaged profile without rebuilding:
adb root
adb shell a2bctl select tas5720a_io4_clk1
adb shell a2bctl status
```

These profiles assume an externally powered amplifier. They do not sequence
a complete module's power controls or its local microcontroller. IO5 remains
undriven; IO2 is undriven with CLKOUT1, or a clock output with CLKOUT2. Keep the
local MCU from driving SPK_SD or
initiating I²C transactions concurrently with the A2B controller. An optional
SPK_FAULT connection to IO6 remains unused by this profile; faults are checked
through TAS register `0x08`. Supply the amplifier's power before initialization.

## AD2428MINI subordinate: current bench wiring

Use `tas5720a_io4_clk1`. It enables CLKOUT1 at 12.288 MHz, disables CLKOUT2,
and retains IO4 shutdown. This is the same register profile already packaged in
the CLKOUT1 image; changing evaluation boards does not require a new clock profile.
An existing persistent CLKOUT2 selection must be changed with `a2bctl select`
even after installing an image whose build default is CLKOUT1.

The [MINI manual, sections 3-5 through 3-9](https://www.analog.com/media/en/technical-documentation/user-guides/adzs-ad2428mini_manual.pdf#page=14)
documents these P2 connections:

| MINI P2 | TAS connection |
| --- | --- |
| 12, IO1/CLKOUT1 | MCLK, pin 14 |
| 3, BCLK | SCLK, pin 15 |
| 7, SYNC | LRCK, pin 17 |
| 27, I2C_SCL | HW/SCL, pin 9 |
| 31, I2C_SDA | FREQ/SDA, pin 8 |
| 1 or 2, GND | Amplifier logic ground |

**Data and shutdown pickup points still need confirmation:** the manual labels
P2 pins 11/15/19/23 as SIO0/1/2/3 without mapping them to DTX/DRX. Check the MINI
schematic or continuity to AD2428 DTX0 (IC pin 12) for TAS SDIN, and DTX1/IO4
(IC pin 13) for SPK_SD. Do not transfer the WB1BZ J4 pinout to P2.

Set P3 2-3 for 3.3 V I/O and P4 2-3 for AD2428 address `0x68` on both MINIs;
reserve `0x6c` on the subordinate I²C bus for the TAS. IO1 also drives LED DS3:
check its loading and measure MCLK at the amplifier. Connect main P10 to
subordinate P9. Power jumpers are:

| Mode | P5 | P6 | P7 | P8 |
| --- | --- | --- | --- | --- |
| Local power, 12 V at P2-32 | Removed | 2-3 | Removed | 1-2 |
| Bus-powered subordinate | 1-2 | 1-2 | 1-2 | Removed |

Choose the subordinate's power mode to match the actual supply/ground arrangement
before bring-up. These jumper options are not a hardware validation of either
configuration with this profile. In bus-power mode, account for the subordinate
ground offset when connecting an external amplifier supply or test equipment.
Supply speaker power separately; P2 regulator outputs are not speaker supplies.

## EVAL-AD2428WB1BZ modification (previous bench option)

ADI's EE-419 identifies **DTX1/IO4** on the unpopulated J4 footprint and its
connection to LED D9. It is a candidate SPK_SD output because this profile uses
DTX0 only (`I2SCFG=0x09`, TX1 disabled). The exact J4 pin number and D9 resistor/
load must be checked against the board revision's schematic/continuity before
soldering; do not infer pin numbering from the AD2428MINI P2 header. IO0 has the
pushbutton/IRQ circuit; IO1/IO2 are clock-output candidates. Leave them alone.

For verified IO4 wiring, the helper preloads GPIODATCLR `0x4c=0x10`, disables
GPIO forwarding/input for that bit, enables GPIOOEN `0x4d` bit 4, and later uses
GPIODATSET `0x4b=0x10` to release SPK_SD after clocks/configuration. Shutdown clears
it. Provide an appropriate hardware pull-down so reset/high impedance means
shutdown, and check logic voltage compatibility and LED loading.

J3 exposes SDA/SCL/GND for the amplifier's I²C connection; J4 exposes BCLK, SYNC
and DTX0 for I²S. CLKOUT must be picked up from the selected IO1/IO2 net: J4 does
not list MCLK. Confirm the board's existing codec/clock connections before
altering that net. The on-board ADAU1761 and EEPROM use 0x39 and 0x50 and are not
configured by these speaker profiles. Both IO4 profiles expect a low TAS
address strap (`0x6c`); confirm the physical modification matches it.

**Power/ground:** this is a bus-powered subordinate board. EE-419 explicitly
warns that its ground can differ from the main/local-power ground. Arrange an
isolated/floating amplifier supply referenced to the subordinate ground, or
engineer the appropriate isolation/local-power modification; do not bridge it
to Pi/bench earth inadvertently. The TAS speaker power comes from its own supply,
not the board's A2B bus supply. A direct Pi GPIO would additionally cross this
ground boundary, so use the remote GPIO for this board. This implementation does
not claim a generic Pi GPIO character-device control service.

## Verification

```sh
m rpi_a2b_profile_test
A2B_PROFILE_DIR=device/brcm/rpi5/audio/a2b/profiles \
  out/host/linux-x86/nativetest64/rpi_a2b_profile_test/rpi_a2b_profile_test
```

Tests cover malformed profiles, peripheral routing/cleanup, masked updates,
bounded polling, node mismatch, discovery failure and the bundled TAS lifecycles,
including IO4 sequencing, both CLKOUT selections and shutdown after a peripheral NACK.
Image builds check the HAL, CLI, init/SELinux policy and overlay packaging.
Hardware acceptance still needs: wiring/address/power confirmation; successful
discovery/readback; clock frequency/phase measurement; muted boot, unmute, volume,
standby/resume, reload/rollback and fault recovery; USB playback/mic regression.

Sources: [AD242x TRM Rev 1.4](https://www.analog.com/media/en/technical-documentation/user-guides/ad242x-trm.pdf),
[TAS5720A-Q1 datasheet](https://www.ti.com/lit/ds/symlink/tas5720a-q1.pdf),
[EVAL-AD2428WB1BZ EE-419](https://www.analog.com/media/en/technical-documentation/application-notes/EE419v01.pdf),
[ADI Pi overlay example](https://wiki.analog.com/resources/tools-software/a2bv2/a2braspberrypi/dtsoverlay).
Discovery baseline: Radio Sound Salted Caramel `device/brcm/rpi5` commit
`a154ca98e37c14f0dafb3fd1df77b43b21e6856e`, one-node MRData sequence; adapted here
for two-slot I²S, synchronous MCLK and TAS control. The old compiled routines are
retained as reference only and are no longer selected by a preprocessor flag.
