#!/bin/sh

N=1
t=1;
echo "tnt_mode=$t"
echo $t > /sys/devices/platform/lf1000-touchscreen/tnt_mode
for w in `seq 10 30`; do
        echo "point=$w"
        cat
        sleep .1
        echo $((11+16*N*100)) > /sys/devices/platform/lf1000-touchscreen/report_events
        sleep $N.5
	echo "done"
done

