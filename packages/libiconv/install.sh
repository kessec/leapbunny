#!/bin/bash

BUILD_FROM_SOURCE=1

PKG_NAME=libiconv
SRC=libiconv-1.13.1.tar.gz
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

CFLAGS="-I$PROJECT_PATH/packages/$PKG_NAME/libiconv-1.13.1/ -O3 -fPIC -mcpu=arm926ej-s" LDFLAGS=-L$PROJECT_PATH/packages/$PKG_NAME/libiconv-1.13.1/ ./configure --host=arm-linux --target=arm-linux --prefix=$ROOTFS_PATH/usr --enable-shared
make clean
make
make install

popd

popd

exit 0