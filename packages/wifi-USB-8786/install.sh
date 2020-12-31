#!/bin/bash

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

# Build Marvell 8786-USB GPL-licensed vendor driver.
pushd $PROJECT_PATH/packages/wifi-USB-8786/wlan_src
make clean build

# Install Marvell 8786-USB GPL-licensed vendor driver.
mkdir -p $ROOTFS_PATH/lib/modules/vendor-drivers
cp ../bin_usb8786/*.ko $ROOTFS_PATH/lib/modules/vendor-drivers/
# Install Marvell vendor userspace utilities
mkdir -p $ROOTFS_PATH/usr/bin
cp ../bin_usb8786/mlanconfig $ROOTFS_PATH/usr/bin/
cp ../bin_usb8786/mlan2040coex $ROOTFS_PATH/usr/bin/
# Install Marvell 8786-USB firmware imagine
mkdir -p $ROOTFS_PATH/lib/firmware/mrvl
cp ../fw/usb8786.bin $ROOTFS_PATH/lib/firmware/mrvl/
popd

exit 0
