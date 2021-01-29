#!/bin/sh
SYS=/sys/devices/platform/lf1000-touchscreen
# TSP Version=3
echo 22 > $SYS/max_tnt_down
echo 521 > $SYS/min_tnt_up
echo 5 > $SYS/max_delta_tnt
echo 5 > $SYS/delay_in_us
echo 5 > $SYS/y_delay_in_us
echo 5 > $SYS/tnt_delay_in_us
echo '426 429 444 451 465 479 506 526 560' > $SYS/pressure_curve
echo 1 > $SYS/tnt_mode
echo -1 > $SYS/averaging
echo -1061 85 528270 > $SYS/tnt_plane
exit 0 # success
