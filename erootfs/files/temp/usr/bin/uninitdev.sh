#!/bin/sh

# remove development system links

# enable read/write of file system
mount -o remount,rw /

# create device node and directory for telnet consoles
mknod /dev/ptmx c 5 2
chmod 666 /dev/ptmx
mkdir /dev/pts
chmod 666 /dev/pts

# change startup scripts
cd /etc/rc.d
rm S06usbether
rm S10nfs
rm S50telnet
rm K50telnet
rm K90nfs
rm K95usbether
ln -s ../init.d/mass_storage S06mass_storage
cd /

# have Brio watch vbus flag
rm /flags/vbus

# set initial system volume
echo 400 > /flags/volume
