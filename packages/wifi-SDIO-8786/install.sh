#!/bin/bash

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

# Build Marvell 8786-SDIO GPL-licensed vendor driver.
pushd $PROJECT_PATH/packages/wifi-SDIO-8786/wlan_src
make clean build

# Install Marvell 8786-SDIO GPL-licensed vendor driver.
mkdir -p $ROOTFS_PATH/lib/modules/vendor-drivers
cp ./mlan.ko $ROOTFS_PATH/lib/modules/vendor-drivers/mlan_sd.ko
cp ./sd8xxx.ko $ROOTFS_PATH/lib/modules/vendor-drivers/sd8786.ko
# Install Marvell vendor userspace utilities
# mkdir -p $ROOTFS_PATH/usr/bin
# cp ../bin_sd8786/mlanconfig $ROOTFS_PATH/usr/bin/
# cp ../bin_sd8786/mlan2040coex $ROOTFS_PATH/usr/bin/
# Install Marvell 8786-SDIO firmware imagine
mkdir -p $ROOTFS_PATH/lib/firmware/mrvl
cp ../fw/sd8786_uapsta.bin $ROOTFS_PATH/lib/firmware/mrvl/
popd

exit 0
