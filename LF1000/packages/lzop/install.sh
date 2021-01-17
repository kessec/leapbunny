#!/bin/bash

BUILD_FROM_SOURCE=1

PKG_NAME=lzop
SRC=lzop-1.01.tar.gz
SRC_URL=http://www.lzop.org/download
set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/$PKG_NAME

if [ ! -e $SRC ]; then
	wget $SRC_URL/$SRC
fi

BUILD_DIR=`echo "$SRC" | cut -d '.' -f -2`

if [ "$CLEAN" == "1" -o ! -e $BUILD_DIR ]; then
	rm -rf $BUILD_DIR
	tar -xzf $SRC
fi

pushd $BUILD_DIR

set +e
patch -p1 -N -i ../lzop-lzo2.patch
set -e

autoconf
CPPFLAGS=-I$ROOTFS_PATH/usr/include LDFLAGS=-L$ROOTFS_PATH/usr/lib ./configure --host=arm-linux --target=arm-linux
make
mkdir -p $ROOTFS_PATH/sbin
cp ./src/lzop $ROOTFS_PATH/sbin
#make install DESTDIR=$ROOTFS_PATH

popd

popd

exit 0
