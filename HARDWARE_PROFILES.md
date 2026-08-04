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

Each product has the same Caramel Vanilla voice stack and A2B audio profile.
Storage changes select the complete first-stage and vendor fstab together;
display changes select the firmware `config.txt` used for the boot partition.
This avoids editing generated output or silently pairing an NVMe fstab with an
SD image.

For the reference Pi, use `aosp_rpi5_car`. For a Waveshare unit on SD/eMMC,
use `aosp_rpi5_car_emmc`.
