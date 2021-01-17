#!/bin/bash

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/kbtool

make

mkdir -p $ROOTFS_PATH/usr/bin
cp ./kbtool $ROOTFS_PATH/usr/bin/

popd

exit 0
