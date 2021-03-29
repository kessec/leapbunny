#!/bin/bash

BUILD_FROM_SOURCE=1

PKG_NAME=sdldoom
SRC=sdldoom-1.0
set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/$PKG_NAME

BUILD_DIR=`echo "$SRC" | cut -d '.' -f -3`

if [ "$CLEAN" == "1" -o ! -e $BUILD_DIR ]; then
	rm -rf $BUILD_DIR
	tar -xzf $SRC
fi

pushd $BUILD_DIR
./configure
make -j5

cp ./doom $ROOTFS_PATH/usr/bin
cd ../
mkdir $ROOTFS_PATH/LF/Base/doom
cp ./doom1.wad $ROOTFS_PATH/LF/Base/doom
pushd $BUILD_DIR
rm ./doom
popd

popd

exit 0
