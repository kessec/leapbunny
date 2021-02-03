#!/bin/bash

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/pixart/src

if [ "$CLEAN" == "1" ]; then
	make clean
fi

make
cp ./pixart $ROOTFS_PATH/usr/bin/

popd

pushd $PROJECT_PATH/packages/pixart
mkdir -p $ROOTFS_PATH/usr/local/lib/pixart
cp -a ./*.hex $ROOTFS_PATH/usr/local/lib/pixart
popd

pushd $ROOTFS_PATH/usr/local/lib/pixart
ln -s *FW.hex FW.hex
ln -s *UI.hex UI.hex
popd

exit 0
