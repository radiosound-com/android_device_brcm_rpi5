#!/system/bin/sh

# The Pi firmware consumes this one-shot flag and clears it before starting
# Android. If the trial slot cannot reach boot completion, the next ordinary
# boot follows the previous slot in autoboot.txt.
sleep 1
reboot "0 tryboot"
