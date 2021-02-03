#!/bin/bash

BUILD_FROM_SOURCE=0

PKG_NAME=tslib
SRC=tslib-1.0.tar.bz2
SRC_URL=http://tslib?/
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
	tar -xjf $SRC

	if [ "$BUILD_FROM_SOURCE" == "1" ]; then
		# patch up configure to ignore rpl_malloc
		pushd $BUILD_DIR
		aclocal	
		autoconf
		set +e
		./autogen.sh
		set -e
		./autogen.sh
		patch -p1 < ../no_rpl_malloc.patch
		popd
	fi
fi

pushd $BUILD_DIR

if [ "$BUILD_FROM_SOURCE" == "1" ]; then
	CFLAGS="-O3 -fPIC -mcpu=arm926ej-s" ./configure --host=arm-linux --target=arm-linux --build=x86-linux --prefix=$ROOTFS_PATH/usr --enable-shared --disable-static

	# make clean
	make
	# make install
	cp -a src/.libs/libts*.so* $ROOTFS_PATH/usr/lib
fi

cp -v src/tslib.h $ROOTFS_PATH/usr/include

popd

cp -v ts.conf $ROOTFS_PATH/etc

popd

exit 0
