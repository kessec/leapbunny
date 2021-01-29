#!/bin/bash -x

# install binaries compiled with Emerald-oe repository

set -e
. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/oe-bin

mkdir -p $ROOTFS_PATH

for tar_file in `ls *.tar`; do tar -C $ROOTFS_PATH -xf $tar_file ; done

# install busybox links manually for this build
pushd $ROOTFS_PATH
while read link; do 
	if [ ! -h "$ROOTFS_PATH$link" -a ! -e "$ROOTFS_PATH$link" ]; then
		case "$link" in
			/*/*/*) to="../../bin/busybox";;
			/bin/*) to="busybox";;
			/*/*) to="../bin/busybox";;
			/*) to="/bin/busybox";;
		esac;
		ln -s $to $ROOTFS_PATH$link;
	fi;
done <$ROOTFS_PATH/etc/busybox.links
rm $ROOTFS_PATH/etc/busybox.links

# Add link for curses library
ln -sf libncurses.so usr/lib/libcurses.so

# cleanup: remove man/doc pages, locale info, any static libs, etc
pushd $ROOTFS_PATH
rm -rf usr/share/man 
rm -rf usr/share/doc
rm -rf usr/share/locale
rm -rf usr/share/sounds/alsa
find -name *.la | xargs rm
pushd usr/lib
ls *.a | xargs rm
popd
popd

popd

# Fix up /etc/vsftpd.conf
cp $PROJECT_PATH/packages/oe-bin/vsftpd.conf $ROOTFS_PATH/etc

exit 0
