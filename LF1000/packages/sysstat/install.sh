#!/bin/bash
#

BUILD_FROM_SOURCE=1

SRC=sysstat-9.0.3.tar.gz
SRC_URL=http://pagesperso-orange.fr/sebastien.godard
set -x

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/sysstat

if [ ! -e $SRC ]; then
	wget $SRC_URL/$SRC
fi

BUILD_DIR=sysstat-9.0.3

if [ "$CLEAN" == "1" -o ! -e $BUILD_DIR ]; then
	rm -rf $BUILD_DIR
	tar -xzf $SRC
fi

pushd $BUILD_DIR

#MANDIR=/tmp/sysstat
#./configure --prefix=$ROOTFS_PATH/usr/ CC=arm-linux-gcc --host=arm-linux --mandir=$MANDIR
./configure --prefix=$ROOTFS_PATH/usr/ CC=arm-linux-gcc --host=arm-linux
make

# ubifs doesn't goes through block I/O, so limited usage for Emerald for now.
cp ./sysstat $ROOTFS_PATH/usr/bin/
# cp ./iostat $ROOTFS_PATH/usr/local/bin/
cp ./mpstat $ROOTFS_PATH/usr/bin/


popd

popd

exit 0
