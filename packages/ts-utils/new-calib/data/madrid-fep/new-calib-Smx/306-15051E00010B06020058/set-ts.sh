#!/bin/sh
SYS=/sys/devices/platform/lf1000-touchscreen
# TSP Version=3
echo 24 > $SYS/max_tnt_down
#2 echo 36 > $SYS/max_tnt_down
#3_7 echo -1 > $SYS/max_tnt_down
echo 521 > $SYS/min_tnt_up
echo 5 > $SYS/max_delta_tnt
#3_8 echo -1 > $SYS/max_delta_tnt
echo 1 > $SYS/delay_in_us
echo 1 > $SYS/y_delay_in_us
echo 1 > $SYS/tnt_delay_in_us
echo 468 471 487 495 509 524 553 575 612 > $SYS/pressure_curve
echo 1 > $SYS/tnt_mode
echo -1 > $SYS/averaging
echo -858 -89 512293 > $SYS/tnt_plane
#2 echo 0 0 0 > $SYS/tnt_plane
