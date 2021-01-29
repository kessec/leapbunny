#!/bin/sh
SYS=/sys/devices/platform/lf1000-touchscreen
# TSP Version=3
echo 25 > $SYS/max_tnt_down
#2 echo 37 > $SYS/max_tnt_down
#3_7 echo 31 > $SYS/max_tnt_down
echo 523 > $SYS/min_tnt_up
echo 5 > $SYS/max_delta_tnt
#3_8 echo 20 > $SYS/max_delta_tnt
echo 5 > $SYS/delay_in_us
echo 5 > $SYS/y_delay_in_us
echo 5 > $SYS/tnt_delay_in_us
echo 456 478 587 636 733 830 1024 1169 1412 > $SYS/pressure_curve
echo 1 > $SYS/tnt_mode
echo -1 > $SYS/averaging
echo -668 -575 620390 > $SYS/tnt_plane
#2 echo 0 0 0 > $SYS/tnt_plane
