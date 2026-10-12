Raspberry Vanilla AOSP 17 device configuration for Raspberry Pi 5.

USB audio is the default. For A2B output, synchronous MCLK, editable node profiles
and TAS5720A bring-up, see [the A2B guide](audio/a2b/README.md).

USB input and output are selected independently using Android's
`ro.audio.multi_usb_mode=true` mode. Connecting a capture-only USB microphone
replaces the matching USB input route while retaining the sound card's output.
Removing that microphone restores the previous matching input. This applies to
USB devices on both USB-output and A2B builds; it does not change the persistent
primary output choice or the A2B profile. The property is read at framework
startup and is packaged in the system image, not the audio APEX.

The hardware regression test requires root ADB, a capture-only microphone on a
root-hub port, and a separate USB sound card. It briefly disables/enables only
the microphone port three times, checks that the playback port ID remains
unchanged, and checks microphone selection and capture fallback. It restores
the microphone port on exit and does not start media or change volume:

```sh
adb -s PI_ADDRESS:5555 root
python3 audio/tests/usb_independent_routing_test.py \
  --serial PI_ADDRESS:5555 --hardware-serial PI_HARDWARE_SERIAL \
  --mic-port 1-1 --output-card 1 --artifacts /tmp/usb-routing-test
```

The Caramel product conditionally includes the parked-only Caramel Store app
when `vendor/radiosound/caramelstore` is present. Sync the
`android_packages_apps_Caramel_Store` source there, then build `CaramelStore`
or a Caramel product target. The app uses only the public catalog GET
endpoints; it has no import credentials, POST path, or APK proxy.
