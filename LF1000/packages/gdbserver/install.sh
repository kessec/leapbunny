#!/bin/bash

PKG_NAME=gdb

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

# install gdbserver from gdb binary path
pushd $PROJECT_PATH/packages/$PKG_NAME/
cp binary/usr/local/bin/gdbserver $ROOTFS_PATH/usr/bin/
popd

exit 0
