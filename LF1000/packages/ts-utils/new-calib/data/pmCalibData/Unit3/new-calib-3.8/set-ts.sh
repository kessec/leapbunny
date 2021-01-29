#!/bin/sh
SYS=/sys/devices/platform/lf1000-touchscreen
# TSP Version=3
echo 25 > $SYS/max_tnt_down
#2 echo 37 > $SYS/max_tnt_down
echo 523 > $SYS/min_tnt_up
echo 20 > $SYS/max_delta_tnt
echo 5 > $SYS/delay_in_us
echo 5 > $SYS/y_delay_in_us
echo 5 > $SYS/tnt_delay_in_us
echo 405 431 557 614 726 839 1064 1233 1514 > $SYS/pressure_curve
echo 1 > $SYS/tnt_mode
echo -1 > $SYS/averaging
echo -809 -178 516595 > $SYS/tnt_plane
#2 echo 0 0 0 > $SYS/tnt_plane
