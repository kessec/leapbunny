#!/bin/sh
SYS=/sys/devices/platform/lf1000-aclmtr
echo 1 > $SYS/rate
echo 10 > $SYS/average
mfgdata get aclbias > $SYS/bias

