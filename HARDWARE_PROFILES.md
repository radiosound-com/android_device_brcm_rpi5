# Raspberry Pi 5 hardware profiles

The Caramel Vanilla reference unit is a Raspberry Pi 5 with the Waveshare
10.1-inch DSI panel and an NVMe root device. It remains the default
`aosp_rpi5_car` product.

The supported lunch targets are:

| Product | Storage | Display |
| --- | --- | --- |
| `aosp_rpi5_car` | NVMe | Waveshare 10.1-inch DSI |
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
`RPI5_AUDIO=usb` routes the primary ALSA path to card 0 and disables the input
simulation flag. The USB profile defaults the ALSA `PCM Capture Source` to
`Line` when that mixer control exists, which matches the reference USB card's
line-in connection. Select a microphone input explicitly when needed:

```sh
m RPI5_AUDIO=usb RPI5_AUDIO_USB_CAPTURE_SOURCE=Mic -j8
```

The route is applied by the USB audio HAL each time the card connects, so it
survives reboot and USB re-enumeration. Unsupported cards are left unchanged
and log a warning. Storage changes select the complete first-stage and vendor
fstab together; display changes select the firmware `config.txt` used for the
boot partition. This avoids editing generated output or silently pairing an
NVMe fstab with an SD image.

The USB policy declares 44.1 kHz and 48 kHz stereo output for the dynamic USB
device port. The AOSP policy leaves that port profile-less, which caused the
reference ICUSBAUDIO7D card's output connection to be rejected even though
direct ALSA playback worked. With the product policy, `cmd audio
get-connected-output-devices` includes `USB_DEVICE` and Android media/TTS
output can use the USB speaker path.

All Caramel products include `CaramelVoiceDefaults`, which selects the bundled
offline eSpeak engine as `tts_default_synth` for an active user only when that
user has no TTS choice. This is a product-signed boot receiver, so apps such as
OsmAnd can use Android's normal default-engine path after a clean flash.

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
```

The expected properties are `usb` and `false`; `/proc/asound/cards` must list
the attached USB device. With the reference line-in setup,
`tinymix`/`dumpsys media.audio_flinger` should show the selected `Line` source;
the PTT log must show Vosk partial/final text and no `AudioRecord`/`AudioTrack`
`ENODEV` errors. A `Recognition error: 6` means `ERROR_SPEECH_TIMEOUT`, usually
because the selected mixer source contains no speech.

For the reference Pi, use `aosp_rpi5_car`. For a Waveshare unit on SD/eMMC,
use `aosp_rpi5_car_emmc`.
