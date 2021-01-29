#!/bin/sh

# Look for previous cache of static nodes
TARFILE=/var/dev_nodes.tar
TAR_DOOM_FILE=/var/dev_nodes_doom
if [ -e $TARFILE ]; then
	if [ ! -e $TAR_DOOM_FILE ]; then
		tar -xf $TARFILE && exit 0
	fi
fi

#Make i2c node required for detecting lfp100
mknod -m 666 /dev/i2c-0 c 89 0

#Make nodes that mdev does not auto-detect
mknod -m 666 /dev/gpio c 246 0
mknod -m 666 /dev/idct c 248 0
mknod -m 666 /dev/ga3d c 249 0

#Create pseudo terminal directory for mount point
mkdir /dev/pts

#Mount sysfs and pts
mount -a

#Scan hw and create dev nodes
mdev -s

#Tar up dev nodes to speed next boot
mount -o,remount,rw /
touch $TAR_DOOM_FILE

tar -cf $TARFILE /dev
rm $TAR_DOOM_FILE

