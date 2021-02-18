#!/bin/sh
SYS=/sys/devices/platform/lf1000-aclmtr
echo 10 > $SYS/rate
echo 1 > $SYS/average
echo 0 0 0 > $SYS/bias

