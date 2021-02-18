#!/bin/sh
#
# set kTouchModeDefault values
#

echo 100 > /sys/devices/platform/lf1000-touchscreen/sample_rate_in_hz
echo   4 > /sys/devices/platform/lf1000-touchscreen/debounce_in_samples_down
echo   1 > /sys/devices/platform/lf1000-touchscreen/debounce_in_samples_up

