#!/bin/sh

N=1
for w in `seq 0 10`; do
        echo "weight=$w"
        cat
        for t in 0 1 2 3; do
                echo "tnt_mode=$t"
                echo $t > /sys/devices/platform/lf1000-touchscreen/tnt_mode
                sleep 1
                echo $((11+16*N*100)) > /sys/devices/platform/lf1000-touchscreen/report_events
                sleep $((1+N))
        done
done

