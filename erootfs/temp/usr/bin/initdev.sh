#!/bin/sh

# Initialize development system.  Have system start telnetd

# enable read/write of file system
mount -o remount,rw /

# create device node and directory for telnet consoles
mknod /dev/ptmx c 5 2
chmod 666 /dev/ptmx
mkdir /dev/pts
chmod 666 /dev/pts

# change startup scripts
cd /etc/rc.d
rm S06mass_storage
ln -s ../init.d/usbether S06usbether
ln -s ../init.d/nfs      S10nfs
ln -s ../init.d/telnetd  S50telnetd
ln -s ../init.d/usbether K50telnetd
ln -s ../init.d/nfs      K90nfs
ln -s ../init.d/usbether K95usbether

# have Brio ignore usb connect / disconnect
echo 0 > /flags/vbus

# set initial system volume
echo 400 > /flags/volume
