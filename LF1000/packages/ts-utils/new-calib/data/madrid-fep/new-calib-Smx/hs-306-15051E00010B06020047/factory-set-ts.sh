#!/bin/sh
SYS=/sys/devices/platform/lf1000-touchscreen
# TSP Version=3
echo 24 > $SYS/max_tnt_down
echo 522 > $SYS/min_tnt_up
echo 5 > $SYS/max_delta_tnt
echo 5 > $SYS/delay_in_us
echo 5 > $SYS/y_delay_in_us
echo 5 > $SYS/tnt_delay_in_us
echo '496 498 505 509 516 523 537 548 566' > $SYS/pressure_curve
echo 1 > $SYS/tnt_mode
echo -1 > $SYS/averaging
echo -736 83 325530 > $SYS/tnt_plane
exit 0 # success

