#!/bin/sh

grep screen_module=HX8238 /proc/cmdline > /dev/null
if [ $? -ne 0 ]; then
	echo "Didn't detect HX8238";
	exit 1;
fi

hx8238 /dev/gpio 0004
hx8238 /dev/gpio 0005
hx8238 /dev/gpio 001E
hx8238 /dev/gpio 0030
hx8238 /dev/gpio 0031
hx8238 /dev/gpio 0032
hx8238 /dev/gpio 0033
hx8238 /dev/gpio 0034
hx8238 /dev/gpio 0035
hx8238 /dev/gpio 0036
hx8238 /dev/gpio 0037
hx8238 /dev/gpio 003A
hx8238 /dev/gpio 003B
