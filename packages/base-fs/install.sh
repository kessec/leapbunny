#!/bin/bash -x

# Setup the base filesystem for the SD card.
set -e
. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user isn't root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

# parse args
set_standard_opts $*
pushd $PROJECT_PATH/packages/base-fs
tar -xf base-fs-0.2.1.tar.gz -C ./

# Copy the contents of the base filesystem to target.
cp -r ./base-fs-0.2.1/* $PROJECT_PATH/target/rootfs
echo "Successfully copied base-fs"
chmod +rwx $PROJECT_PATH/target/rootfs/*

exit 0
