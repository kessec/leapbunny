#!/bin/bash

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

# pushd $PROJECT_PATH/packages/init/fling
# ./install.sh $*
# popd

pushd $PROJECT_PATH/packages/init/rl
./install.sh $*
popd

pushd $PROJECT_PATH/packages/init/syncd
make
install -m 0755 syncd $ROOTFS_PATH/usr/bin/
popd

pushd $PROJECT_PATH/packages/init

# Copy symlinks correctly
tar -c etc usr var --exclude .svn | tar -C $ROOTFS_PATH/ -x

# SVN doesn't save group and other permission information!
chmod 666 $ROOTFS_PATH/var/loglevel

pushd $ROOTFS_PATH
find . -name ".svn" | xargs rm -rf
popd

pushd $ROOTFS_PATH/etc
rm -f mtab
ln -s /proc/mounts mtab
popd

popd

exit 0
