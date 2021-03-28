#!/bin/bash

BUILD_FROM_SOURCE=1

PKG_NAME=libsdl
SRC=SDL-1.2.13.tar.gz
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

# CFLAGS="-I$PROJECT_PATH/linux-2.6/include/" CC=arm-linux-uclibcgnueabi-gcc CXX=arm-linux-uclibcgnueabi-g++ 

./configure --prefix=$ROOTFS_PATH/usr/ --build=`uname -m` --host=arm-linux --disable-video-opengl --disable-video-x11 --disable-esd --disable-video-directfb --enable-video-fbcon --enable-pulseaudio=no
make clean
make
make install

popd

popd

exit 0