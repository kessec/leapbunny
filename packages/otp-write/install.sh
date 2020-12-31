#!/bin/bash

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/otp-write

if [ "$CLEAN" == "1" ]; then
	make clean
fi

make
cp ./otpwrite $ROOTFS_PATH/usr/bin/
cp ./otp_program.sh $ROOTFS_PATH/usr/bin/otp_program.sh
cp ./dd2ff $ROOTFS_PATH/usr/bin/dd2ff
cp ./addsn $ROOTFS_PATH/usr/bin/addsn
cp ./mkotp.sh $ROOTFS_PATH/usr/bin/mkotp.sh
cp ./packageGame $ROOTFS_PATH/usr/bin/packageGame
cp ./ff.bin $ROOTFS_PATH/var/ff.bin

popd

exit 0
