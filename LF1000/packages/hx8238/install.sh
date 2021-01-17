#!/bin/bash

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/hx8238

if [ "$CLEAN" == "1" ]; then
	make clean
fi

make
mkdir -p $ROOTFS_PATH/usr/bin/
cp ./hx8238 $ROOTFS_PATH/usr/bin/
cp ./hx8238.sh $ROOTFS_PATH/usr/bin/
cp ./hx8238_dump.sh $ROOTFS_PATH/usr/bin/
popd

exit 0
