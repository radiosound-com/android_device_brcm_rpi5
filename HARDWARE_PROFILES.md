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
simulation flag. Storage changes select the complete first-stage and vendor
fstab together; display changes select the firmware `config.txt` used for the
boot partition. This avoids editing generated output or silently pairing an
NVMe fstab with an SD image.

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
# Keep the down/up pair atomic; separate ADB calls may become a long press.
adb -s 192.168.1.56:5555 shell cmd car_service inject-key -t 200 231
adb -s 192.168.1.56:5555 logcat -d -s CaramelVoice Vosk TextToSpeech AudioRecord AudioTrack
```

The expected properties are `usb` and `false`; `/proc/asound/cards` must list
the attached USB device. The PTT log must show an active voice session and
Vosk model readiness without `AudioRecord`/`AudioTrack` `ENODEV` errors.

For the reference Pi, use `aosp_rpi5_car`. For a Waveshare unit on SD/eMMC,
use `aosp_rpi5_car_emmc`.
