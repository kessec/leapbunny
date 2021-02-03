#!/bin/bash

# check if we can use sparse
which sparse > /dev/null
if [ "$?" = "0" ]; then
	SPARSE_CHECK="C=1"
else
	echo "**** WARNING: you do not have sparse installed"
	echo "**** run: \"sudo apt-get install sparse\" to install it"
	echo ""
	SPARSE_CHECK=""
fi

# count number of CPUs for make

if [ -z "$CPUS" ]
then
  export CPUS=$[$(echo /sys/devices/system/cpu/cpu[0-9]* | wc -w)+0]
  [ "$CPUS" -lt 1 ] && CPUS=1
fi

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/linux-2.6/

# check for .config.  Use defconfig if it's not there.
if [ ! -e ".config" ]; then
	if  [ "$TARGET_MACH" == "LF_TS_LF1000" ]; then
		echo "*** Using default LF1000 form factor Board with touchscreen configuration."
		make -j $CPUS lf1000_ts_defconfig
	else
		echo "Unsupported machine 'TARGET_MACH=$TARGET_MACH', expected one of the following:"
		echo "   LF_TS_LF1000        -- Normal Form Factor board with Touchscreen"
		exit 1
	fi
fi

# Apply patches; -N ignore already applied, -r - don't make rejects file
set +e
patch -r - -p0 -N < default-log-level.patch &> /dev/null
set -e

make $SPARSE_CHECK -j $CPUS zImage
cp arch/arm/boot/zImage $TFTP_PATH

MODS=`grep CONFIG_MODULES include/config/auto.conf`
if [ "$MODS" != "" ]; then
	make -j $CPUS modules
	echo INSTALL_MOD_PATH=$ROOTFS_PATH
	make -j $CPUS modules_install INSTALL_MOD_PATH=$ROOTFS_PATH
fi

# create linux headers
make headers_install ARCH=arm INSTALL_HDR_PATH=$ROOTFS_PATH/usr

if [ $EMBEDDED -eq 0 ]; then
	cp vmlinux $ROOTFS_PATH
fi

popd

#### Don't do most device node creation now ###
#### See /etc/init.d/rcS and /usr/bin/make_device_nodes.sh; 
#### We do most of this at run time now.  
#### Exception is /dev/console, which init wants to grab before rcS runs, so
#### if we want to watch it boot, we'll have to have a /dev/console before
#### /dev is mounted
mkdir -p $ROOTFS_PATH/dev
pushd $ROOTFS_PATH/dev
rm -f ttyS0
sudo mknod -m 600 ttyS0 c 4 64
ln -sf ttyS0 console
popd

# stash copies of this build
echo "Saving build specific copy as $TFTP_PATH/zImage.ff_bd"
cp $TFTP_PATH/zImage $TFTP_PATH/zImage.ff_bd
