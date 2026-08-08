# Raspberry Pi 5 hardware profiles

The Caramel Vanilla reference unit is a Raspberry Pi 5 with the Waveshare
10.1-inch DSI panel and an NVMe root device. It remains the default
`aosp_rpi5_car` product.

The supported lunch targets are:

| Product | Storage | Display |
| --- | --- | --- |
| `aosp_rpi5_car` | NVMe | Waveshare 10.1-inch DSI |
| `aosp_rpi5_car_zipformer` | NVMe | Waveshare 10.1-inch DSI; high-quality streaming INT8 Zipformer, eSpeak TTS |
| `aosp_rpi5_car_zipformer_kokoro` | NVMe | Waveshare 10.1-inch DSI; high-quality Zipformer ASR and neural Kokoro TTS |
| `aosp_rpi5_car_lgraph` | NVMe | Waveshare 10.1-inch DSI; larger Vosk graph, suitable for measured 4 GB headroom |
| `aosp_rpi5_car_lgraph_kokoro` | NVMe | Waveshare 10.1-inch DSI; larger Vosk graph + Kokoro TTS |
| `aosp_rpi5_car_16gb` | NVMe | Waveshare 10.1-inch DSI; validated high-memory Zipformer/Kokoro pair with larger beam and context budget |
| `aosp_rpi5_car_emmc` | SD/eMMC partition layout | Waveshare 10.1-inch DSI |
| `aosp_rpi5_car_hdmi` | NVMe | HDMI |
| `aosp_rpi5_car_emmc_hdmi` | SD/eMMC partition layout | HDMI |

Each product has the same Caramel Vanilla voice stack and defaults to the A2B
audio profile. For a USB microphone/speaker development unit, select the USB
audio profile at build time without changing the storage/display product:

```sh
m RPI5_AUDIO=usb -j8
```

`RPI5_AUDIO=a2b` is the default and preserves the reference behavior;
`RPI5_AUDIO=usb` routes primary output to the first USB ALSA card that exposes
playback, rather than assuming USB card 0, and disables the input simulation
flag. This matters when a capture-only USB microphone enumerates before a
USB sound card. Primary capture selection is direction-aware: it prefers a
standalone USB capture-only card (the usual USB microphone) and falls back to
the first full-duplex USB card. Android's USB audio manager owns the active
external capture connection and reselects the remaining microphone when a USB
device is added or removed. The HAL retries briefly while ALSA endpoints are
being registered, so a transient card-number race cannot strand playback on a
capture-only card. The USB profile defaults the ALSA `PCM Capture Source` to
`Mic` when that mixer control exists, which is the normal voice-assistant
configuration. Select `Line` for a wired line-in test, or choose another
supported mixer input explicitly:

```sh
m RPI5_AUDIO=usb RPI5_AUDIO_USB_CAPTURE_SOURCE=Line -j8
```

Use the Caramel release target for product images. It disables AOSP's
`wait_for_alsa_scan_results_if_has_audio_interface` Aconfig flag only for this
product, because Pi 5 USB callbacks can arrive before the corresponding ALSA
node. The older card-number wait path still provides automatic routing and
avoids the boot-time input-registration race:

```sh
lunch aosp_rpi5_car-caramel-userdebug
m RPI5_AUDIO=usb -j8
```

For the highest-quality stack validated on the actively cooled 4 GB Pi 5, use
the Zipformer/Kokoro product. It keeps the same NVMe, Waveshare, PCIe, and
USB-audio defaults:

```sh
lunch aosp_rpi5_car_zipformer_kokoro-caramel-userdebug
m RPI5_AUDIO=usb -j8
```

The recognition-only `aosp_rpi5_car_zipformer-caramel-userdebug` variant keeps
eSpeak as the lower-memory TTS fallback. For legacy compatibility with the
larger Vosk graph, keep `aosp_rpi5_car_lgraph-caramel-userdebug`; that profile
was previously measured at 2.44 GiB available RAM with approximately 447 MiB
peak host Vosk RSS while recognizing a short phrase. Keep compact products as
the default for boards with heavier unrelated workloads.

The INT8 Zipformer process held about 395--423 MiB RSS. With Kokoro, Spotify,
OsmAnd, System UI, and the AAOS services resident, the current 4 GB reference
Pi retained about 1.6 GiB available with no swap and zero sustained memory
pressure. The Vosk lgraph products remain available as compatibility choices.

The 16 GB product now selects the same latency-validated model assets with a
high-memory recognition profile (`zipformer-int8-highmem`) that increases beam and
threading headroom for better robustness on long, music-heavy commands.
Whisper `small.en` was accurate in evaluation but ran at roughly real-time on
the 4 GB Pi, so it is not used as the streaming PTT backend. The larger-RAM
product keeps room for a future optional second pass without making an
unvalidated model part of the release image.

Model capacity is a build-time choice; the product does not guess from the
board at runtime. This keeps image contents and memory behavior reproducible.

The `trunk_staging` lunch targets remain available for comparison and for
non-Caramel builds.

The Pi 5 USB gadget service is started only after the product init action has
created and permissioned its configfs gadget tree. The previous `class hal`
startup could race that action: `UsbGadget` aborted when `os_desc/b.1` was not
yet present, and a concurrent Android user switch could then crash
`system_server`. The service is now disabled for class startup and explicitly
started at the end of the configfs setup action; the HAL also treats a missing
tree as a recoverable setup error rather than aborting the process.

The route is applied by the USB audio HAL each time the card connects, so it
survives reboot and USB re-enumeration. Unsupported cards are left unchanged
and log a warning. Storage changes select the complete first-stage and vendor
fstab together; display changes select the firmware `config.txt` used for the
boot partition. This avoids editing generated output or silently pairing an
NVMe fstab with an SD image.

USB capture capabilities are intentionally discovered from the connected ALSA
endpoint. Common USB microphones such as the AK5370 expose mono capture only,
while USB line inputs may expose stereo. A fixed stereo input profile makes
AudioFlinger open an unsupported stream, leaving recognition with zero frames
and repeated HAL `-38` errors.

The ALSA producer and AudioFlinger consumer run asynchronously. Input transfer
therefore waits for a complete requested period (up to 250 ms or four ALSA
periods) instead of treating a short nonblocking MonoPipe read as real silence.
This preserves capture continuity during USB startup without allowing a missing
device to block indefinitely.

## NVMe bootloader prerequisite

The Android image supplies the NVMe fstab and, for `RPI5_STORAGE=nvme`, adds
`dtparam=pciex1` to the firmware configuration so Linux enables the PCIe link.
The reference product defaults to PCIe Gen 3 through `RPI5_PCIE_GEN=3`, which
adds `dtparam=pciex1_gen=3`. It also defaults
`RPI5_NVME_POWER_POLICY=performance`, which adds these kernel arguments:

```text
pcie_aspm.policy=performance nvme_core.default_ps_max_latency_us=0
```

The first disables PCIe Active State Power Management and Clock Power
Management. The second disables NVMe Autonomous Power State Transitions. This
keeps the always-powered automotive reference unit at full performance and
avoids controller resets observed with a Samsung SSD 960 EVO under sustained
model and filesystem reads. Restore the upstream kernel power-saving defaults
for power-sensitive hardware with `RPI5_NVME_POWER_POLICY=default`.

Select the supported Gen 2 fallback at build time with `RPI5_PCIE_GEN=2`; that
leaves the link at the Raspberry Pi default while retaining the performance
power policy:

```sh
RPI5_PCIE_GEN=2 m bootimage
```

Both build-time knobs can be combined when diagnosing an adapter:

```sh
RPI5_PCIE_GEN=2 RPI5_NVME_POWER_POLICY=default m bootimage
```

Raspberry Pi's [official PCIe documentation](https://www.raspberrypi.com/documentation/computers/configuration.html)
documents Gen 3 as uncertified on Pi 5 and warns that it may be unstable. If
the NVMe fails to enumerate, Android reboots, or storage errors appear under
load, rebuild with `RPI5_PCIE_GEN=2`. The generated Gen 3 boot
configuration contains both lines, and `rpiboot/cmdline.txt` contains the
selected power-policy arguments:

```ini
dtparam=pciex1
dtparam=pciex1_gen=3
```

The Raspberry Pi 5 bootloader configuration is separate from the image. For a
non-HAT+ PCIe/NVMe adapter, set these EEPROM values once from Raspberry Pi OS
booted on a temporary SD card (or with the Raspberry Pi bootloader utility):

```ini
[all]
BOOT_ORDER=0xf416
PCIE_PROBE=1
BOOT_UART=1
```

`BOOT_ORDER=0xf416` tries NVMe first, then SD and USB-MSD; `PCIE_PROBE=1`
allows the bootloader to probe adapters without a HAT+ EEPROM. `BOOT_UART=1`
keeps the early boot log available at 115200 baud for diagnosis. The EEPROM
settings cannot be reliably changed by copying files into an Android image,
because the bootloader must discover the NVMe before it can read that image.

The USB policy declares 44.1 kHz and 48 kHz stereo output for the USB device
port. The AOSP policy leaves that port profile-less, which caused the reference
ICUSBAUDIO7D card's output connection to be rejected even though direct ALSA
playback worked. This endpoint also rejects the TinyALSA `HW_REFINE` capability
probe used by the generic USB AIDL HAL. The product-specific USB HAL therefore
retains the validated static output profile while continuing normal dynamic
capability discovery for USB capture. With the product policy, `cmd audio
get-connected-output-devices` includes `USB_DEVICE` and Android media/TTS
output can use the USB speaker path.

All Caramel products include `CaramelVoiceDefaults`, which selects the bundled
offline eSpeak engine as `tts_default_synth` for an active user only when that
user has no TTS choice. This is a product-signed boot receiver, so apps such as
OsmAnd can use Android's normal default-engine path after a clean flash.
It also grants only the bundled `com.reecedunn.espeak` package the
`control_audio` and `control_audio_partial` app-ops. Android 16's playback
hardening otherwise treats the background-bound TTS service as background
audio and mutes it. The package-scoped allowlist preserves hardening for every
other application and is reapplied for each active user at boot and unlock.

## Display defaults

All Caramel Vanilla products initialize Android's
`Settings.System.SCREEN_OFF_TIMEOUT` to `3600000` ms (one hour) through the
SettingsProvider resource overlay. The existing plugged-in stay-awake policy
is unchanged.

Waveshare products also include the platform-signed
`CaramelWaveshareDisplayDefaults` receiver. It runs for the active Android
user at locked boot, normal boot, and user unlock, and persists
`peak_refresh_rate=60.0` and `min_refresh_rate=60.0` in SettingsProvider. This
is intentionally user-scoped rather than a boot-image edit, so the values
survive reboot and apply to the current car user (including user 10). HDMI
products do not include the receiver.

## USB audio acceptance check

The USB profile must be installed as an image and booted with the sound card
attached. `ro.boot.audio.tinyalsa.simulate_input` is read-only and latched by
the audio HAL at boot; changing only `persist.vendor.audio.device` at runtime
does not enable real microphone capture.

After booting the USB-profile image, verify the profile and hardware before
testing the assistant:

```sh
adb -s 192.168.1.56:5555 shell getprop persist.vendor.audio.device
adb -s 192.168.1.56:5555 shell getprop ro.boot.audio.tinyalsa.simulate_input
adb -s 192.168.1.56:5555 shell cat /proc/asound/cards
adb -s 192.168.1.56:5555 shell cmd audio get-connected-output-devices
# Keep the down/up pair atomic; separate ADB calls may become a long press.
adb -s 192.168.1.56:5555 shell cmd car_service inject-key -t 200 231
adb -s 192.168.1.56:5555 logcat -d -s CaramelVoice Vosk TextToSpeech AudioRecord AudioTrack
# From this repository on the host; also fails on AudioFlinger record overflow.
ANDROID_SERIAL=192.168.1.56:5555 audio/tests/usb_capture_continuity_test.sh
```

The expected properties are `usb` and `false`; `/proc/asound/cards` must list
the attached USB device. With a microphone setup,
`tinymix`/`dumpsys media.audio_flinger` should show the selected `Mic` source;
the PTT log must show Vosk partial/final text and no `AudioRecord`/`AudioTrack`
`ENODEV` errors. Caramel Voice loads and prewarms the selected Vosk model when
its interaction service becomes ready; if testing immediately after boot, play
test audio only after `Vosk model ready and prewarmed` appears in logcat. A
`Recognition error: 6` means
`ERROR_SPEECH_TIMEOUT`, usually because the selected mixer source contains no
speech or the test audio was sent before model loading completed.

`Recognition error: 5` is `ERROR_CLIENT` from Vosk failing to create
`AudioRecord`; on this hardware it normally means Android has not registered a
USB capture route yet, even though `/proc/asound/cards` lists the microphone.
Check `cmd audio get-connected-input-devices` before blaming the microphone.
After the Caramel release fix, a physical mic replug should be unnecessary;
if testing an older image, unplug/replug the mic and wait for the
`UsbAlsaManager` add event before retrying PTT.

For the reference Pi, use `aosp_rpi5_car`. For a Waveshare unit on SD/eMMC,
use `aosp_rpi5_car_emmc`.
