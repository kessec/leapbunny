#!/bin/bash

. $PROJECT_PATH/scripts/functions

check_vars

pushd $PROJECT_PATH/packages/oss-utils

make clean
make
mkdir -p $ROOTFS_PATH/usr/bin
mkdir -p $ROOTFS_PATH/test
cp ./oss $ROOTFS_PATH/usr/bin
cp ./vol $ROOTFS_PATH/usr/bin
cp ./vivaldi.wav $ROOTFS_PATH/test
popd

exit 0
