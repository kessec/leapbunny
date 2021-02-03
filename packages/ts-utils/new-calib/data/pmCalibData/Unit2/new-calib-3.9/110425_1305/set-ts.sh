#!/bin/sh
SYS=/sys/devices/platform/lf1000-touchscreen
# TSP Version=3
echo 24 > $SYS/max_tnt_down
#2 echo 36 > $SYS/max_tnt_down
#3_7 echo 34 > $SYS/max_tnt_down
echo 522 > $SYS/min_tnt_up
echo 5 > $SYS/max_delta_tnt
#3_8 echo 20 > $SYS/max_delta_tnt
echo 5 > $SYS/delay_in_us
echo 5 > $SYS/y_delay_in_us
echo 5 > $SYS/tnt_delay_in_us
echo 472 496 613 665 769 873 1081 1238 1498 > $SYS/pressure_curve
echo 1 > $SYS/tnt_mode
echo -1 > $SYS/averaging
echo -459 -367 431681 > $SYS/tnt_plane
#2 echo 0 0 0 > $SYS/tnt_plane
