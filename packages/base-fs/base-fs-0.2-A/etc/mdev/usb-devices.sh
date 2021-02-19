#!/bin/sh
#
# usb-devices.sh -- simple USB device hotplug detector.  
#
# This script is called by /sbin/mdev so that we can inform the app layer of
# USB device hotplug events.

# Log a message about any plug/unplug event to a file in case whoever is doing
# firmware updates needs to know about what devices have been plugged in to
# this unit.  That way we can push firmware packages that support the devices 
# the owner wishes to use.
LOGFILE=/var/log/devices

# Keep track of which devices have been inserted during this boot so we can
# perform cleanup upon removal.
TMPFILE=/tmp

if [ -e /sys/class/usb_device/$MDEV/ ]; then
	PRODUCT="`cat /sys/class/usb_device/$MDEV/device/idProduct`"
	VENDOR="`cat /sys/class/usb_device/$MDEV/device/idVendor`"
	MANUFACTURER="`cat /sys/class/usb_device/$MDEV/device/manufacturer`"
	echo "plug $MDEV $PRODUCT:$VENDOR \"$MANUFACTURER\"" >> $LOGFILE

	# track plugged-in devices
	echo "$VENDOR:$PRODUCT" > $TMPFILE/$MDEV

	# load drivers as appropriate
	if [ $VENDOR = "093a" -a $PRODUCT = "2801" ]; then
		# Camera widget
		modprobe uvcvideo
		modprobe snd-usb-audio index=-2
		# set mic gain to -50 dB
		amixer -D hw:1 cset name='Mic Capture Volume' 2
		# make sure camera firmware is up-to-date
		# disabled for now because we haven't released any updates
		# TODO: test thoroughly if you enable this!
		#FW_DIR="/usr/local/lib/pixart"
		#pixart /dev/video0 $FW_DIR/FW.hex $FW_DIR/UI.hex
	elif [ $VENDOR = "1286" -a $PRODUCT = "203c" ]; then
		# WiFi widget: Marvell USB 8786
		modprobe firmware_class
		insmod /lib/modules/vendor-drivers/mlan.ko
		insmod /lib/modules/vendor-drivers/usb8786.ko
	fi

	# inform application layer (except for host hub enum)
	if [ ! $VENDOR = "1d6b" -o ! $PRODUCT = "0001" ]; then
		unotify
	fi
else
	VENDOR="`cut -d: -f 1 $TMPFILE/$MDEV`"
	PRODUCT="`cut -d: -f 2 $TMPFILE/$MDEV`"

	echo "unplug $MDEV" >> $LOGFILE

	rm $TMPFILE/$MDEV

	# inform application layer (except for host hub enum)
	if [ ! $VENDOR = "1d6b" -o ! $PRODUCT = "0001" ]; then
		unotify
	fi

	# unload drivers as appropriate
	if [ $VENDOR = "093a" -a $PRODUCT = "2801" ]; then
		# Camera widget
		modprobe -r uvcvideo
		modprobe -r snd-usb-audio
	elif [ $VENDOR = "1286" -a $PRODUCT = "203c" ]; then
		# WiFi widget: Marvell USB 8786
		rmmod usb8xxx
		rmmod mlan
		modprobe -r firmware_class
	fi
fi
