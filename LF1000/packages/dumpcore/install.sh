#!/bin/bash

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/dumpcore

if [ "$CLEAN" == "1" ]; then
	make clean
fi

make
cp ./dumpcore $ROOTFS_PATH/usr/bin/
cp ./sysdump.sh $ROOTFS_PATH/usr/bin/
cp ./postmortem.sh $ROOTFS_PATH/usr/bin/
popd

exit 0
