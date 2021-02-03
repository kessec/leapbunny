#!/bin/bash

IPERF_VERSION=2.0.4
IPERF_SRC=iperf-$IPERF_VERSION.tar.gz

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/iperf

if [ ! -e $IPERF_SRC ]; then
	wget http://downloads.sourceforge.net/project/iperf/iperf/$IPERF_VERSION%20source/$IPERF_SRC?use_mirror=cdnetworks-us-2
fi

IPERF_DIR=iperf-$IPERF_VERSION

if [ "$CLEAN" == "1" -o ! -e $IPERF_DIR ]; then
	rm -rf $IPERF_DIR
	tar -xf $IPERF_SRC
fi

pushd $IPERF_DIR

export CC=arm-linux-gcc
export CXX=arm-linux-g++
export LD=arm-linux-ld
export LDFLAGS=-L/opt/RidgeRun/arm-eabi-uclibc/lib
export CPPFLAGS=-I/opt/RidgeRun/arm-eabi-uclibc/include

./configure --host=arm --prefix=$ROOTFS_PATH --exec-prefix=$ROOTFS_PATH

patch -p0 < ../use-gnu-malloc.patch

make

mkdir -p $ROOTFS_PATH/usr/bin
cp src/iperf $ROOTFS_PATH/usr/bin

popd

popd

exit 0
