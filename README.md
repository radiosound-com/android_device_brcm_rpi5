Raspberry Vanilla AOSP 17 device configuration for Raspberry Pi 5.

The Caramel product conditionally includes the parked-only Caramel Store app
when `vendor/radiosound/caramelstore` is present. Sync the
`android_packages_apps_Caramel_Store` source there, then build `CaramelStore`
or a Caramel product target. The app uses only the public catalog GET
endpoints; it has no import credentials, POST path, or APK proxy.
