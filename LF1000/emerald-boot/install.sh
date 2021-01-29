#!/bin/bash

# This script builds a emerald-boot image.

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

set_standard_opts $*

pushd $PROJECT_PATH/emerald-boot/

CLEAN=1
export BUTTON_BOOT=1

if [ "$CLEAN" == "1" ]; then
	make clean
fi

SDCARD=$SDCARD SDCARD_DEBUG=$SDCARD_DEBUG UBOOT_SUPPORT=$UBOOTLOADERS KERNELDIR=$KERNELDIR CROSS_COMPILE=$CROSS_COMPILE make 

VERSION=`grep "CURRENT_MAJOR_VERSION=" ../packages/version/install.sh | cut -d = -f 2`
BUILD=`svn info $PROJECT_PATH | grep Revision | awk '{print $2}'`
for b in emerald-boot.bin madrid-boot.bin; do
	echo "" >> $b
	echo "VERSION:$VERSION.$BUILD" >> $b
	echo "" >> $b
	cp $b $TFTP_PATH/
done

popd
