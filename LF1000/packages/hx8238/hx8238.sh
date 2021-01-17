#!/bin/sh

grep screen_module=HX8238 /proc/cmdline > /dev/null
if [ $? -ne 0 ]; then
	echo "Didn't detect HX8238";
	exit 1;
fi

hx8238 /dev/gpio 0004 0447
hx8238 /dev/gpio 0005 BCC4
