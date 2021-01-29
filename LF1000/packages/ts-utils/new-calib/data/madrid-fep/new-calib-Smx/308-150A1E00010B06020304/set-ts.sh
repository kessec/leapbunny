#!/bin/sh
SYS=/sys/devices/platform/lf1000-touchscreen
# TSP Version=3
echo 20 > $SYS/max_tnt_down
echo 520 > $SYS/min_tnt_up
echo 5 > $SYS/max_delta_tnt
echo 5 > $SYS/delay_in_us
echo 5 > $SYS/y_delay_in_us
echo 5 > $SYS/tnt_delay_in_us
echo '427 428 435 438 444 450 462 471 486' > $SYS/pressure_curve
echo 1 > $SYS/tnt_mode
echo -1 > $SYS/averaging
echo -733 -93 413880 > $SYS/tnt_plane
exit 0 # success
