#!/bin/bash

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/unotify

if [ "$CLEAN" == "1" ]; then
	make clean
fi

make
mkdir -p $ROOTFS_PATH/sbin/
cp ./unotify $ROOTFS_PATH/sbin/
cp ./cnotify $ROOTFS_PATH/sbin/

popd

exit 0
