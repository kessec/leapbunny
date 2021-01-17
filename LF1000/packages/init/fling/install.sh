#!/bin/bash

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/init/fling

GCC=gcc

$CROSS_COMPILE$GCC $INCLUDES -o fling fling.c

cp -pv fling ../usr/bin
cp -pv fling $ROOTFS_PATH/usr/bin

popd

exit 0
