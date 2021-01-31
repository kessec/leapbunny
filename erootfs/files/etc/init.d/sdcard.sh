#!/bin/sh

case "$1" in
	start)
		modprobe mes_sdhc
		modprobe mmc_block
		sleep 1
		if [ -e /dev/mmcblk0p3 ]; then
			mount /dev/mmcblk0p3 /mnt
		fi
		if [ -e /dev/mmcblk0p4 ]; then
			mount /dev/mmcblk0p4 /mnt2
		fi
		;;
	stop)
		if [ -e /dev/mmcblk0p3 ]; then
			umount /dev/mmcblk0p3 > /dev/null
		fi
		if [ -e /dev/mmcblk0p4 ]; then
			umount /dev/mmcblk0p4 > /dev/null
		fi
		modprobe -r mmc_block 2> /dev/null
		modprobe -r mes_sdhc 2> /dev/null
		;;
	*)
		echo "Usage: $0 {start|stop}"
		exit 1
		;;
esac

exit 0
