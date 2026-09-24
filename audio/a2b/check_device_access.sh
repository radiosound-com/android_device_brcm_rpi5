#!/system/bin/sh
# Run as root on an idle development device after boot. No I2C transfers or unmute.
set -eu

[ "$(id -u)" = 0 ] || { echo "Run with adb root" >&2; exit 1; }
profile=$(getprop persist.vendor.audio.a2b.profile)
case "$(a2bctl status)" in
    *" streams=0 "*) ;;
    *) echo "Stop playback before running this check" >&2; exit 1 ;;
esac

pid=$(pidof android.hardware.audio.service.rpi)
# Android su limits supplementary groups. Keep the HAL's UID/primary GID and
# include the profile group only if the real HAL has it; unrelated groups are omitted.
profile_gid=$(stat -c %g /data/vendor/a2b/profiles)
identity=$(awk -v profile_gid="$profile_gid" '
    /^Uid:/ { uid=$2 }
    /^Gid:/ { gid=$2 }
    /^Groups:/ { for (i=2; i<=NF; i++) if ($i==profile_gid) group="," $i }
    END { printf "%s,%s%s", uid, gid, group }
' /proc/"$pid"/status)
hal_uid=${identity%%,*}
probe=/data/vendor/a2b/profiles/.access-check.$$
trap 'rm -f "$probe"' EXIT
printf 'profile read permission probe\n' > "$probe"
chown root:audioserver "$probe"
chmod 0640 "$probe"
restorecon "$probe"

failed=0
su "$identity" sh -c 'test "$(id -u)" = "$2" && cat "$1" >/dev/null && test ! -w "$1"' sh "$probe" "$hal_uid" || failed=1
# Opening i2c-dev checks DAC access without issuing an ioctl, read or write.
su "$identity" sh -c 'test "$(id -u)" = "$1" && exec 9<> /dev/i2c-1 && exec 9>&-' sh "$hal_uid" || failed=1
# Also exercise profile loading inside the HAL's real SELinux domain.
a2bctl select "$profile" || failed=1
[ "$failed" = 0 ] || { echo "FAIL: audio HAL access" >&2; exit 1; }
echo "PASS: HAL can read root-owned profiles and open I2C; profile loaded, muted"
