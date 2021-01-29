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

if [ -e /sys/class/usb_device/$MDEV/ ]; then
	PRODUCT="`cat /sys/class/usb_device/$MDEV/device/idProduct`"
	VENDOR="`cat /sys/class/usb_device/$MDEV/device/idVendor`"
	MANUFACTURER="`cat /sys/class/usb_device/$MDEV/device/manufacturer`"
	echo "plug $MDEV $PRODUCT:$VENDOR \"$MANUFACTURER\"" >> $LOGFILE

	# inform application layer
	unotify
else
	echo "unplug $MDEV" >> $LOGFILE
fi
