#!/bin/sh
SYS=/sys/devices/platform/lf1000-touchscreen
# TSP Version=3
echo 26 > $SYS/max_tnt_down
#2 echo 39 > $SYS/max_tnt_down
#3_7 echo 30 > $SYS/max_tnt_down
echo 523 > $SYS/min_tnt_up
echo 5 > $SYS/max_delta_tnt
#3_8 echo 20 > $SYS/max_delta_tnt
echo 5 > $SYS/delay_in_us
echo 5 > $SYS/y_delay_in_us
echo 5 > $SYS/tnt_delay_in_us
echo 467 485 572 610 687 765 919 1035 1228 > $SYS/pressure_curve
echo 1 > $SYS/tnt_mode
echo -1 > $SYS/averaging
echo -225 -713 457234 > $SYS/tnt_plane
#2 echo 0 0 0 > $SYS/tnt_plane
