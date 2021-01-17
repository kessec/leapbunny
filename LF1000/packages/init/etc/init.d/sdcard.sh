#!/bin/sh

case "$1" in
	start)
		modprobe lf1000_mmc
		modprobe mmc_block
		sleep 1
		if [ -e /dev/mmcblk0p1 ]; then
			mount /dev/mmcblk0p1 /mnt
		fi
		;;
	stop)
		if [ -e /dev/mmcblk0p1 ]; then
			umount /dev/mmcblk0p1 > /dev/null
		fi
		modprobe -r mmc_block 2> /dev/null
		modprobe -r lf1000_mmc 2> /dev/null
		;;
	*)
		echo "Usage: $0 {start|stop}"
		exit 1
		;;
esac

exit 0
