#!/bin/sh
SYS=/sys/devices/platform/lf1000-touchscreen
# TSP Version=3
echo 23 > $SYS/max_tnt_down
#2 echo 34 > $SYS/max_tnt_down
#3_7 echo 20 > $SYS/max_tnt_down
echo 522 > $SYS/min_tnt_up
echo 5 > $SYS/max_delta_tnt
#3_8 echo 20 > $SYS/max_delta_tnt
echo 5 > $SYS/delay_in_us
echo 5 > $SYS/y_delay_in_us
echo 5 > $SYS/tnt_delay_in_us
echo 482 483 492 495 503 510 525 536 554 > $SYS/pressure_curve
echo 1 > $SYS/tnt_mode
echo -1 > $SYS/averaging
echo -849 -86 473086 > $SYS/tnt_plane
#2 echo 0 0 0 > $SYS/tnt_plane
