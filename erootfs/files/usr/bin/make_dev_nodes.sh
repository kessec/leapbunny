#!/bin/sh

# Look for previous cache of static nodes
TARFILE=/var/dev_nodes.tar
if [ -e $TARFILE ]; then
	tar -xf $TARFILE && exit 0
fi

# Otherwise, make up static nodes, and save for later
cd /dev
mknod -m 644 video0 c 81 0

# The console is funny
ln -sf ttyS0 console

# These are devices for the custom lf1000 devices
mknod -m 666 gpio c 246 0
#mknod -m 666 buttons c 247 0
mknod -m 666 mlc c 247 0
# mknod -m 666 power c 239 0

if [ "$MACH" == "CONFIG_MACH_ME_LF1000" ]; then
	M_LAYERS=2
else
	M_LAYERS=3
fi
for i in `seq 0 $M_LAYERS`; do
	mknod -m 666 layer$i c 245 $i
done

mknod -m 666 dpc c 252 0
mknod -m 666 ga3d c 249 0

# Add device node for /dev/idct, IDCT Macro Block Decoder for Video.
mknod -m 666 idct c 248 0

# Add devices for the mtd subsystem.
# for m in `seq 0 9`; do
# 	mknod -m 666 mtd$m c 90 $((2*$m))
# 	# mknod -m 666 mtdr$m c 90 $((2*$m+1));
# 	mknod -m 666 mtdblock$m b 31 $m;
# done

# make device node for OSS audio
mknod -m 666 dsp c 14 3

# make device node for i2c:
# (do this if you want to use the i2c chardev driver, i2c-dev, otherwise it is
#  not needed)
#
mknod -m 666 i2c-0 c 89 0

# make device node for SPI:
# (do this if you want to use the SPI driver from user space, otherwise it is 
# not needed)
#
# Note: I stole 243 for the ubi layer.  Please don't use.
# mknod -m 666 spi c 243 0

# make device node for /dev/rtc0
# mknod -m 666 rtc0 c 254 0

# make device node for /dev/ram0.  This is the ramdisk for USB
# mknod -m 666 ram0 b 1 0

#make device node for /dev/power.  This driver monitors the battery
mknod -m 666 power c 239 0

# make device nodes for ubi.  When are we going to fix this stuff?
# mknod -m 666 ubi0 c 240 0;
# mknod -m 666 ubi1 c 241 0;
# mknod -m 666 ubi2 c 242 0;
# mknod -m 666 ubi3 c 243 0;
mknod -m 666 ubi_ctrl c 10 63;

# keyboard event interface (plus touchscreen)
# Create quite a few device nodes in case HID devices get plugged in.  In the
# future, we'll hotplug instead and this won't be needed.
# mkdir -p ./input
# for i in `seq 0 19`; do
# 	mknod -m 666 input/event$i c 13 $((64+$i))
# done

# make device nodes for Ethernet over USB / Telnet support if needed.
mknod -m 666 ptmx c 5 2
mkdir -p pts
chmod 777 pts

# Finally, tar this mess up to speed up next boot
tar -cf $TARFILE /dev
