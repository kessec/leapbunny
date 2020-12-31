#!/bin/sh

grep screen_module=NT39016 /proc/cmdline > /dev/null
if [ $? -ne 0 ]; then
	echo "Didn't detect NT39016";
	exit 1;
fi 

threewire /dev/gpio r 0
threewire /dev/gpio r 1
threewire /dev/gpio r 2
threewire /dev/gpio r 3
threewire /dev/gpio r 4
threewire /dev/gpio r 5
threewire /dev/gpio r 6
threewire /dev/gpio r 7
threewire /dev/gpio r 8
threewire /dev/gpio r 9
threewire /dev/gpio r A
threewire /dev/gpio r B
threewire /dev/gpio r C
threewire /dev/gpio r D
threewire /dev/gpio r E
threewire /dev/gpio r F
threewire /dev/gpio r 10
threewire /dev/gpio r 11
threewire /dev/gpio r 12
threewire /dev/gpio r 1E
threewire /dev/gpio r 20
