#!/bin/sh

N=1
for w in 6 7; do
        echo "weight=$w"
        for c in UL UR LR LL CC NO; do
                echo "corner=$c"
                cat
                for t in 0 1 2 3; do
                        echo "tnt_mode=$t"
                        echo $t > /sys/devices/platform/lf1000-touchscreen/tnt_mode
                        sleep 1
                        echo $((11+16*N*100)) > /sys/devices/platform/lf1000-touchscreen/report_events
                        sleep $((1+N))
                done
        done
done

