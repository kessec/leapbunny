#!/bin/sh
SYS=/sys/devices/platform/lf1000-touchscreen
# TSP Version=3
echo 18 > $SYS/max_tnt_down
echo 521 > $SYS/min_tnt_up
echo 20 > $SYS/max_delta_tnt
echo 5 > $SYS/delay_in_us
echo 5 > $SYS/y_delay_in_us
echo 5 > $SYS/tnt_delay_in_us
echo '439 443 459 466 481 495 524 546 582' > $SYS/pressure_curve
echo 1 > $SYS/tnt_mode
echo -1 > $SYS/averaging
echo -1012 -175 619138 > $SYS/tnt_plane
exit 0 # success
