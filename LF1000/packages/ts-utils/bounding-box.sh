#!/bin/sh

SYS=/sys/devices/platform/lf1000-touchscreen

MINX1=1024; MAXX1=0
MINY1=1024; MAXY1=0
MINX2=1024; MAXX2=0
MINY2=1024; MAXY2=0

while true; do
	in=`tr ' ' ';' < $SYS/raw_adc`;
	eval $in;
	if [ $MINX1 -gt $x1 ]; then MINX1=$x1; fi
	if [ $MAXX1 -lt $x1 ]; then MAXX1=$x1; fi
	if [ $MINY1 -gt $y1 ]; then MINY1=$y1; fi
	if [ $MAXY1 -lt $y1 ]; then MAXY1=$y1; fi
	if [ $MINX2 -gt $x2 ]; then MINX2=$x2; fi
	if [ $MAXX2 -lt $x2 ]; then MAXX2=$x2; fi
	if [ $MINY2 -gt $y2 ]; then MINY2=$y2; fi
	if [ $MAXY2 -lt $y2 ]; then MAXY2=$y2; fi
	echo "$x1 $y1 $MINX1-$MAXX1,$MINY1-$MAXY1 $MINX2-$MAXX2,$MINY2-$MAXY2";
	sleep .1;
done
