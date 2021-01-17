#!/bin/sh

grep screen_module=NT39016 /proc/cmdline > /dev/null
if [ $? -ne 0 ]; then
	echo "Didn't detect NT39016";
	exit 1;
fi

threewire /dev/gpio w E  68
