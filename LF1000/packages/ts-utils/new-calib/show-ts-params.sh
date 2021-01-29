#!/bin/sh

SYS=/sys/devices/platform/lf1000-touchscreen
for i in averaging delay_in_us y_delay_in_us tnt_delay_in_us \
    max_tnt_down min_tnt_up tnt_mode tnt_plane pressure_curve; do
    echo $i `cat $SYS/$i`;
done
