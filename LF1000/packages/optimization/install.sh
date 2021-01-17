#!/bin/bash

set -e -x

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/optimization

GCC=gcc

$CROSS_COMPILE$GCC $INCLUDES -o iospeed iospeed.c
$CROSS_COMPILE$GCC $INCLUDES -o cpuspeed cpuspeed.c
$CROSS_COMPILE$GCC $INCLUDES -o cpulog cpulog.c
$CROSS_COMPILE$GCC $INCLUDES -o tim tim.c

cp -pv iospeed cpuspeed cpulog tim faster-nand-timing nand-speed-timing-test \
	$ROOTFS_PATH/usr/bin

popd

exit 0
