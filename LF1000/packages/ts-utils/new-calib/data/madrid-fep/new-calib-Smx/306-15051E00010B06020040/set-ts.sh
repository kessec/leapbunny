#!/bin/sh
SYS=/sys/devices/platform/lf1000-touchscreen
# TSP Version=3
echo 23 > $SYS/max_tnt_down
echo 522 > $SYS/min_tnt_up
echo 5 > $SYS/max_delta_tnt
echo 5 > $SYS/delay_in_us
echo 5 > $SYS/y_delay_in_us
echo 5 > $SYS/tnt_delay_in_us
echo '488 489 496 500 506 513 526 536 552' > $SYS/pressure_curve
echo 1 > $SYS/tnt_mode
echo -1 > $SYS/averaging
echo -908 168 355235 > $SYS/tnt_plane
exit 0 # success
