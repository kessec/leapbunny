#!/bin/bash
# Create a release.  Put it in RELEASE_PATH

set -e
. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse the args
set_standard_opts $*

if [ $HELP == 1 ]; then
	echo "Usage $0 [options]"
	echo ""
	echo "Options:"
	echo "-h		Print help then quit"
	echo "-b		Add Brio to the release"
	echo "-e		Build the embedded release (jffs2 image)"
	echo "-u		Add u-boot to the release"
	# echo "-w		Add A/B buttons to choose 'which' kernel boots"
	exit 0
fi

if [ "X$RELEASE_PATH" == "X" ]; then
	export RELEASE_PATH=/tmp/lightning-release/
	echo "RELEASE_PATH not set.  Using default $RELEASE_PATH"
fi

# clear "buildcomplete" token, set "buildstart" token
rm -rf ~/buildcomplete ~/buildstart
touch ~/buildstart

# create the release dir
mkdir -p $RELEASE_PATH

# Get the version number from the version package
ROOTFS_PATH=$RELEASE_PATH $PROJECT_PATH/packages/version/install.sh
VERSION=`cat $RELEASE_PATH/etc/version`
LFP_VERSION=`echo $VERSION | tr - .`
rm -rf $RELEASE_PATH/etc/
LINUX_DIST_NAME=LinuxDist-Emerald-$VERSION
LINUX_DIST_DIR=$RELEASE_PATH/$LINUX_DIST_NAME
rm -rf $LINUX_DIST_DIR $LINUX_DIST_DIR.tar.gz
mkdir -p $LINUX_DIST_DIR

echo "********* Creating Release $LINUX_DIST_NAME *********"

# make the rootfs and package it up
KERNEL_CONFIG_PATH=$PROJECT_PATH/linux-2.6/.config

# move user's .config out of the way (we build defconfig)
KERNEL_CONFIG_MOVED=0
if [ -e $KERNEL_CONFIG_PATH ]; then
	mv $KERNEL_CONFIG_PATH $KERNEL_CONFIG_PATH.current
	KERNEL_CONFIG_MOVED=1
fi

# We only need one nfsroot image for any LF1000 target
ROOTFS_RELEASE=$VERSION-$TARGET_MACH
ROOTFS_PATH=$LINUX_DIST_DIR/nfsroot-$VERSION $PROJECT_PATH/scripts/make_rootfs.sh $*
ROOTFS_PATH=$LINUX_DIST_DIR/erootfs-$ROOTFS_RELEASE $PROJECT_PATH/scripts/make_rootfs.sh $* -e

# move user's .config back, if we had moved it out of the way
if [ $KERNEL_CONFIG_MOVED -eq 1 ]; then
	mv $KERNEL_CONFIG_PATH.current $KERNEL_CONFIG_PATH
fi

# build u-boot if needed
if [ "$UBOOTLOADERS" == "1" ]; then
	$PROJECT_PATH/u-boot-1.1.6-lf1000/install.sh
fi

# Build the bootstrap
$PROJECT_PATH/emerald-boot/install.sh $*

# Wrap the kernel zImage with a CBF header
$PROJECT_PATH/scripts/make_cbf.py $*

# Create lfp deliverables of Brio and Base binaries
mkdir -p $LINUX_DIST_DIR/packages
mkdir -p $LINUX_DIST_DIR/packages/base
mkdir -p $LINUX_DIST_DIR/packages/bulk
mkdir -p $LINUX_DIST_DIR/packages/demos
mkdir -p $LINUX_DIST_DIR/packages/demos/themes
mkdir -p $LINUX_DIST_DIR/packages/demos/trailers
mkdir -p $LINUX_DIST_DIR/packages/experimental
mkdir -p $LINUX_DIST_DIR/packages/unlockers
#mkdir -p $LINUX_DIST_DIR/packages/gamepatches
if [ "$BRIO" == "1" ]; then
	if [ 0 -lt `ls $LINUX_DIST_DIR/nfsroot-$VERSION/*.lfp | grep -c .lfp` ]; then
		mv -f $LINUX_DIST_DIR/nfsroot-$VERSION/*.lfp $LINUX_DIST_DIR/packages/base
	fi
	if [ 0 -lt `ls $LINUX_DIST_DIR/nfsroot-$VERSION/demos/*.lfp | grep -c .lfp` ]; then
		mv -f $LINUX_DIST_DIR/nfsroot-$VERSION/demos/*.lfp $LINUX_DIST_DIR/packages/demos
	fi
	if [ 0 -lt `ls $LINUX_DIST_DIR/nfsroot-$VERSION/themes/*.lfp | grep -c .lfp` ]; then
		mv -f $LINUX_DIST_DIR/nfsroot-$VERSION/themes/*.lfp $LINUX_DIST_DIR/packages/demos/themes
	fi
	if [ 0 -lt `ls $LINUX_DIST_DIR/nfsroot-$VERSION/trailers/*.lfp | grep -c .lfp` ]; then
		mv -f $LINUX_DIST_DIR/nfsroot-$VERSION/trailers/*.lfp $LINUX_DIST_DIR/packages/demos/trailers
	fi
else
	# Get from PREV_LINUX_DIST_DIR
	if [ -e "$PREV_LINUX_DIST_DIR" ]; then
		cp -p $PREV_LINUX_DIST_DIR/packages/base/*.lfp $LINUX_DIST_DIR/packages/base
		cp -p $PREV_LINUX_DIST_DIR/packages/demos/*.lfp $LINUX_DIST_DIR/packages/demos
		cp -p $PREV_LINUX_DIST_DIR/packages/demos/themes/*.lfp $LINUX_DIST_DIR/packages/demos/themes
		cp -p $PREV_LINUX_DIST_DIR/packages/demos/trailers/*.lfp $LINUX_DIST_DIR/packages/demos/trailers
		cp -p $PREV_LINUX_DIST_DIR/packages/bulk/*.lfp $LINUX_DIST_DIR/packages/bulk
	fi
fi

#move items into bulk that belong there
if [ ! -e "$PREV_LINUX_DIST_DIR" ]; then
	cp -f $PROJECT_PATH/scripts/packages_from_1.0.8/PetPad*.lfp $LINUX_DIST_DIR/packages/bulk
	cp -f $PROJECT_PATH/scripts/packages_from_1.0.8/ConnectMovieApp*.lfp $LINUX_DIST_DIR/packages/bulk
	cp -f $PROJECT_PATH/scripts/packages_from_1.0.8/LST3-0x00170032-00000*-1.0.0.0.lfp $LINUX_DIST_DIR/packages/bulk
	touch $LINUX_DIST_DIR/packages/bulk/LST3-0x00170032-000004-1.0.0.0.lfp
	touch $LINUX_DIST_DIR/packages/bulk/LST3-0x00170032-000003-1.0.0.0.lfp
	touch $LINUX_DIST_DIR/packages/bulk/LST3-0x00170032-000002-1.0.0.0.lfp
	touch $LINUX_DIST_DIR/packages/bulk/LST3-0x00170032-000001-1.0.0.0.lfp

	if [ 0 -lt `ls $LINUX_DIST_DIR/packages/base/AccessoryTeaser*.lfp | grep -c .lfp` ]; then
		mv -f $LINUX_DIST_DIR/packages/base/AccessoryTeaser*.lfp $LINUX_DIST_DIR/packages/bulk
	fi
	if [ 0 -lt `ls $LINUX_DIST_DIR/packages/base/Language*.lfp | grep -c .lfp` ]; then
		mv -f $LINUX_DIST_DIR/packages/base/Language*.lfp $LINUX_DIST_DIR/packages/bulk
	fi
	#put file to unlock PetPad into packages/bulk ; move LFConnect nag unlocker to same folder
	cp -p $PROJECT_PATH/scripts/packages_from_1.0.8/LST3-0x00180009-000009.lfp $LINUX_DIST_DIR/packages/bulk
	cp -p $PROJECT_PATH/scripts/packages_external/*.lfp $LINUX_DIST_DIR/packages/bulk
fi

# add accessory nag unlock to 'unlockers' in addtion to connect nag unlock
if [ 0 -lt `ls $LINUX_DIST_DIR/packages/demos/trailers/LFConnect*.lfp | grep -c .lfp` ]; then
	mv -f $LINUX_DIST_DIR/packages/demos/trailers/LFConnect*.lfp $LINUX_DIST_DIR/packages/unlockers
	cp -p $PROJECT_PATH/scripts/experimental_camera/LST3-0x00170038-100001-1.0.0.0.lfp $LINUX_DIST_DIR/packages/unlockers
fi

#copy latest EmeraldMfgTest to demos
cp -p $PROJECT_PATH/packages/EmeraldMfgTest/EmeraldMfgTest/package/EmeraldMfgTest.lfp $LINUX_DIST_DIR/packages/demos

# Make payload directory of UBI and other binary NAND images
LINUX_DIST_DIR=$LINUX_DIST_DIR VERSION=$LFP_VERSION $PROJECT_PATH/scripts/make_payload.sh $*

# Make the Firmware Transplant that the Surgeon will use; deliver LFP
# echo "Going into transplant: VERSION=$VERSION LFP_VERSION=$LFP_VERSION"
LINUX_DIST_DIR=$LINUX_DIST_DIR VERSION=$LFP_VERSION $PROJECT_PATH/scripts/make_transplant.sh $*
mv -v Firmware-*$LFP_VERSION.lfp $LINUX_DIST_DIR/packages
# mv -v NORBoot-*$LFP_VERSION.lfp $LINUX_DIST_DIR/packages

# Make the surgeon; deliver LFP
VERSION=$LFP_VERSION LINUX_DIST_DIR=$LINUX_DIST_DIR $PROJECT_PATH/scripts/make_surgeon.sh $*
mv -v Surgeon-$LFP_VERSION.lfp $LINUX_DIST_DIR/packages

# Patch nfsroot image for uClibc 29/30
if [ -e $LINUX_DIST_DIR/nfsroot-$VERSION ]; then
	ROOTFS_PATH=$LINUX_DIST_DIR/nfsroot-$VERSION  $PROJECT_PATH/scripts/make_nfsuclibc_patch.sh $*
fi

# Create MD5 and SHA1 checksums for packages
pushd $LINUX_DIST_DIR/packages
md5sum *.lfp > md5sum.txt
sha1sum *.lfp > sha1sum.txt
popd
if [ "$BRIO" == "1" ]; then
	pushd $LINUX_DIST_DIR/packages/base
	md5sum *.lfp > md5sum.txt
	sha1sum *.lfp > sha1sum.txt
	popd
	pushd $LINUX_DIST_DIR/packages/bulk
	md5sum *.lfp > md5sum.txt
	sha1sum *.lfp > sha1sum.txt
	popd
	#not needed for demos, not shipping them
	#pushd $LINUX_DIST_DIR/packages/demos
	#md5sum *.lfp > md5sum.txt
	#sha1sum *.lfp > sha1sum.txt
	#popd
	#pushd $LINUX_DIST_DIR/packages/gamepatches
	#md5sum *.lfp > md5sum.txt
	#sha1sum *.lfp > sha1sum.txt
	#popd
fi

# Move the release notes over
cp $PROJECT_PATH/RELEASE-NOTES $LINUX_DIST_DIR/LinuxDist_RELEASE-NOTES.txt

# Move the host tools (and NAND Flash memory map) over
pushd $PROJECT_PATH

if [ ! -n $SVNUSER ]; then
	MYSVNUSER=$USER
else
	MYSVNUSER=$SVNUSER
fi

echo "GSTEST: make_release.sh doing svn export 1 as $MYSVNUSER"
#cp packages/lfpkg/lfpkg $LINUX_DIST_DIR/host_tools
svn export --username $MYSVNUSER --force host_tools $LINUX_DIST_DIR/host_tools
# Overcome bug in lightning-release's svn export...
if [ ! -e $LINUX_DIST_DIR/host_tools/otp/lfpkg-external ]; then
	svn export --username $MYSVNUSER --force host_tools/otp/lfpkg-external $LINUX_DIST_DIR/host_tools/otp/lfpkg-external
fi

scripts/make_map.py > $LINUX_DIST_DIR/host_tools/flash.map
popd

# Move over UART bootstrapping tools
UART_BOOT=0
if [ $UART_BOOT -eq 1 ]; then

mkdir -p $LINUX_DIST_DIR/uart_bootstrap
if [ "$TARGET_MACH" == "LF_MP2530F" ]; then
	cp $PROJECT_PATH/images/boot-u.nb0 $LINUX_DIST_DIR/uart_bootstrap/
else
	cp $PROJECT_PATH/images/UARTBOOT.bin $LINUX_DIST_DIR/uart_bootstrap/
fi

# provide u-boot for UART bootstrap
if [ "$UBOOTLOADERS" == "1" ]; then
	cp $PROJECT_PATH/u-boot-1.1.6-lf1000/u-boot.bin $LINUX_DIST_DIR/uart_bootstrap/u-boot-$VERSION-$TARGET_MACH.bin
fi

echo "This directory contains images needed for bootstrapping a bricked (or brand new) board via the UART boot process.  They may be used with lf1000_bootstrap.py or mp2530_bootstrap.py (see the host_tools README for instructions).  You do NOT need these images when doing normal firmware upgrades or installing releases." > $LINUX_DIST_DIR/uart_bootstrap/README

fi # UART_BOOT

# Move ATAP manufacturing cartridge tools over
ATAP=0
if [ $ATAP -eq 1 ]; then

pushd $PROJECT_PATH
mkdir -p $LINUX_DIST_DIR/mfg-cart
mkdir -p $LINUX_DIST_DIR/mfg-cart/ATAP
echo "GSTEST: make_release.sh doing svn export 2 as $MYSVNUSER"
svn export --username $MYSVNUSER --force  packages/mfg-cart/ATAP $LINUX_DIST_DIR/mfg-cart/ATAP
#cp -Rf packages/mfg-cart/ATAP/* $LINUX_DIST_DIR/mfg-cart/ATAP
#rm -rf $LINUX_DIST_DIR/mfg-cart/ATAP/.svn
cp -f $LINUX_DIST_DIR/packages/*.lfp $LINUX_DIST_DIR/mfg-cart/ATAP/FW_packages
if [ "$BRIO" == "1" ]; then
	cp -f $LINUX_DIST_DIR/packages/base/*.lfp $LINUX_DIST_DIR/mfg-cart/ATAP/Packages
fi
cp -f $LINUX_DIST_DIR/host_tools/lfpkg $LINUX_DIST_DIR/mfg-cart/ATAP
cp -f $LINUX_DIST_DIR/nfsroot-$VERSION/usr/bin/mkbase.sh $LINUX_DIST_DIR/mfg-cart/ATAP
#tar -czf ATAP-$VERSION.tar.gz $LINUX_DIST_DIR/mfg-cart/ATAP
#mv ATAP*.gz $LINUX_DIST_DIR/mfg-cart
echo "GSTEST: make_release.sh doing svn export 3 as $MYSVNUSER"
svn export --username $MYSVNUSER --force  packages/mfg-cart/Base2Cart $LINUX_DIST_DIR/mfg-cart/Base2Cart
cp -f $LINUX_DIST_DIR/host_tools/lfpkg $LINUX_DIST_DIR/mfg-cart/Base2Cart
echo "GSTEST: make_release.sh doing svn export 4 as $MYSVNUSER"
svn export --username $MYSVNUSER --force  packages/mfg-cart/Base2ATAP $LINUX_DIST_DIR/mfg-cart/Base2ATAP
cp -f $LINUX_DIST_DIR/host_tools/lfpkg $LINUX_DIST_DIR/mfg-cart/Base2ATAP
cp -f $LINUX_DIST_DIR/nfsroot-$VERSION/usr/bin/mkbase.sh $LINUX_DIST_DIR/mfg-cart/Base2ATAP
CUR_PATH="`pwd`"
cd $LINUX_DIST_DIR/mfg-cart
../host_tools/lfpkg -a create Base2Cart
../host_tools/lfpkg -a create Base2ATAP
rm -rf Base2Cart
rm -rf Base2ATAP
#mv Base2*.lfp $LINUX_DIST_DIR/mfg-cart
cd $CUR_PATH
popd

fi

#Run unit tests
#$PROJECT_PATH/scripts/run_tests.sh

# quarantine what's needed in experimental
cp $PROJECT_PATH/scripts/README_experimental.txt $LINUX_DIST_DIR/packages/experimental
cp $PROJECT_PATH/scripts/experimental_camera/* $LINUX_DIST_DIR/packages/experimental
UNIX2DOS=unix2dos
which $UNIX2DOS || UNIX2DOS=todos
which $UNIX2DOS && $UNIX2DOS $LINUX_DIST_DIR/packages/experimental/README_experimental.txt

#remove packages/themes  FIXME I am not sure what is making this
rm -rf $LINUX_DIST_DIR/packages/themes

# Tar up the nfsroot image instead of leaving it expanded
# fix up .txt files for use on Windows
pushd $LINUX_DIST_DIR
sudo tar -czvf nfsroot-$VERSION.tar.gz nfsroot-$VERSION
sudo rm -rf nfsroot-$VERSION
sudo unix2dos -du *.txt
popd

# Tar up the emuroot image if it was built for LightningCore emulation
if [ -d $HOME/emuroot/ ]; then
	tar -C $HOME -czvf $LINUX_DIST_DIR/emuroot-$VERSION.tar.gz emuroot/
fi

# tar up the release
pushd $RELEASE_PATH
sudo tar -czf $LINUX_DIST_NAME.tar.gz $LINUX_DIST_NAME
popd

# clear "buildstart" token, set "buildcomplete" token
rm -rf ~/buildstart
echo $VERSON > ~/buildcomplete
echo $LINUX_DIST_DIR >> ~/buildcomplete

exit 0
