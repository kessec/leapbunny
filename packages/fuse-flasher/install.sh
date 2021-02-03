#!/bin/bash

PKG_NAME=fuse-flasher
set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/$PKG_NAME

if [ "$CLEAN" == "1" ]; then
	make clean
fi

make
make install

popd

exit 0
