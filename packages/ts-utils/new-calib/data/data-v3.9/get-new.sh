#!/bin/sh

NAME=$1
if [ -z "$NAME" ]; then
	echo "You must use me: $0 NAME";
	exit 1;
fi
if [ -e "set-ts.sh-$NAME" ]; then
	echo "set-ts.sh-$NAME already exists"
	exit 1;
fi	

mv ~lfu/new-calib.log new-calib.log-$NAME
mv ~lfu/set-ts.sh set-ts.sh-$NAME
mv ~lfu/mb_0000.argb .
sudo chown rdowling:rdowling new-calib.log-$NAME set-ts.sh-$NAME mb_0000.argb
convert -size 480x272 -depth 8 -alpha off rgba:mb_0000.argb mb_0000-$NAME.png
rm mb_0000.argb
