#!/bin/bash

echo '-----------------------------'
echo "<<< ENTERING: $0"

# make Surgeon root file system and kernel

set -x # let me see what is being done (for now)
set -e # exit immediately if we encounter trouble
set -o pipefail # Make broken pipes of tar clog up messily so 

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

# Get frozen version number
if [ -z "$VERSION" ]; then
	echo "Can't find VERSION.  Please export or define"
	exit 1;
fi

# Create a new staging area
# SURGEONFS=/tmp/surgeon # Must be fixed! `mktemp -d /tmp/surgeon-$USER.XXXXXX`
SURGEONFS=`mktemp -d /tmp/surgeon-$USER.XXXXXX`
sudo rm -rf $SURGEONFS; mkdir -p $SURGEONFS

# Create the spine of a rootfs
ROOTFS_PATH=$SURGEONFS make_directories.sh -e

#
# Ok, White list mentality: What is the very least we need?
#
# To work in the field:
#  Busybox
#  INIT ==> /etc/init.d, /dev, stuff in /etc like passwd, inittab,fstab
#  MODULES ==> Kernel modules, g_ether.ko right now
#  RECOVERY ==> recovery,recovery-functions,recovery-dftp,show-scr,gpio-control,imager,flipbook,surgeon-usb-socket
#               var/screens/connect-pngs, usr/lib/libpng12, /usr/lib/libz
#  AVAHI ==> Lots of stuff; dbus too
#  DFTP ==> dftp-device, C++ libs, g_ether, avahi, 
#  NAND ==> flash_*, nand*, 
#  NOR  ==> flashcp
#  MFGDATA ==> mfgdata, mfgmode.sh
#
# To work for developers:
#  CONNECTIVITY: Telnetd, ftpd
#  HELPERS: relax-surgeon, update, play, iospeed, flash-fw, etc.
#
# Further, keep permission and ownership information as found in $EROOTFS.
# Tar preserves permission and link-status nicely, but unless you're root,
# changes ownership to 'you' when you untar it.
#
# Further, be defensive about how we collect these pieces.  They are likely to
# move from time to time, so if we fail to find one, complain!
#

#### Busybox and basic bins
# /bin/busybox + a bunch of symlinks to it
tar -C $EROOTFS_PATH -c bin/busybox | tar -C $SURGEONFS -xv
(cd $EROOTFS_PATH && find bin sbin usr/bin usr/sbin -type l -lname \*busybox) | \
	xargs tar -C $EROOTFS_PATH -c | tar -C $SURGEONFS -xv
# Make init symlink for initramfs to boot correctly
ln -s bin/busybox $SURGEONFS/init
# Bring in a couple of omitted busybox guys... hope we can abandon these
tar -C $EROOTFS_PATH -c bin/ls | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/{zip,unzip} | tar -C $SURGEONFS -xv
# Get the elusive libiconv and libintl libraries for Avahi and ls to work
# unfortunately, get them from a previous BRIO build
where=$LINUX_DIST_DIR/packages/base
zipspace=`mktemp -d /tmp/unzip-$USER.XXXXXX`
unzip $where/Brio-*.lfp -d $zipspace
pushd $zipspace/Brio
  libs=`ls lib/libiconv.so* lib/libintl.so*`
popd  
tar -C $zipspace/Brio -cv $libs | tar -C $SURGEONFS -xv
rm -rf $zipspace

#### INIT
# Login
tar -C $EROOTFS_PATH -c dev | sudo tar -C $SURGEONFS -x
tar -C $EROOTFS_PATH -c etc/{group,passwd,profile,inittab,mtab,fstab,default,version} | tar -C $SURGEONFS -xv
# tar -C $EROOTFS_PATH -c etc/sudoers usr/bin/sudo | tar -C $SURGEONFS -xv
# sudo chown root:root $SURGEONFS/etc/sudoers $SURGEONFS/usr/bin/sudo
# sudo chmod +s $SURGEONFS/usr/bin/sudo
# sudo chmod 440 $SURGEONFS/etc/sudoers
# Init scripts
tar -C $EROOTFS_PATH -c etc/init.d | tar -C $SURGEONFS -xv
# mdev
tar -C $EROOTFS_PATH -c etc/mdev.conf etc/mdev | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/make_dev_nodes.sh | tar -C $SURGEONFS -xv
# Modules
tar -C $EROOTFS_PATH -c lib/modules/2.6.31-leapfrog/kernel/drivers/usb/gadget/{g_ether.ko,lf1000_udc.ko} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c lib/modules/2.6.31-leapfrog/kernel/drivers/input/touchscreen/lf1000_ts2.ko | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c lib/modules/2.6.31-leapfrog/modules.{alias,ieee1394map,pcimap,inputmap,seriomap,ccwmap,isapnpmap,symbols,dep,ofmap,order,usbmap} | tar -C $SURGEONFS -xv
# if we get .bin's again, bring them in.
if [ -e $EROOTFS_PATH/lib/modules/2.6.31-leapfrog/modules.dep.bin ]; then
    tar -C $EROOTFS_PATH -c lib/modules/2.6.31-leapfrog/modules.{alias.bin,symbols.bin,dep.bin} | tar -C $SURGEONFS -xv
fi

# Recovery
tar -C $EROOTFS_PATH -c usr/bin/{recovery,recovery-functions,recovery-dftp,recovery-shutdown,show-scr,gpio-control,imager,imager-fb,flipbook,flipbook-fb,flipbook2,surgeon-usb-socket} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c var/screens/{connect-pngs,SYS_0005_no_yellow.png} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c var/screens/{Madrid-sparkle-pngs,Madrid-Boot-StaticConnectScreen-01CW.png} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/lib/libpng12.so{,.0.22.0,.0} usr/lib/libpng.so usr/bin/libpng{,12}-config | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/lib/libz.so{,.1,.1.2.3} | tar -C $SURGEONFS -xv

#### Avahi
tar -C $EROOTFS_PATH -c etc/avahi etc/dbus-1 etc/init.d/avahi-{daemon,dnsconfd} etc/init.d/dbus-1 | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/lib/libavahi-client.so{,.3,.3.2.5} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/lib/libavahi-common.so{,.3,.3.5.1} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/lib/libavahi-core.so{,.6,.6.0.0} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/lib/libavahi-glib.so{,.1,.1.0.1} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/lib/libavahi-gobject.so{,.0,.0.0.1} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/lib/libdaemon.so{,.0,.0.4.0} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/lib/libexpat.so{,.1,.1.5.0} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/sbin/avahi-dnsconfd | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/sbin/avahi-daemon | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/sbin/avahi-autoipd | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/avahi-publish-service | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/avahi-publish-address | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/avahi-browse | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/avahi-set-host-name | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/avahi-resolve | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/avahi-resolve-address | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/avahi-resolve-host-name | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/avahi-publish | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/avahi-browse-domains | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/tput  | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/share/avahi | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/share/avahi/avahi-service.dtd | tar -C $SURGEONFS -xv
# tar -C $EROOTFS_PATH -c usr/share/locale/en_GB/LC_MESSAGES/avahi.mo | tar -C $SURGEONFS -xv
# dbus
tar -C $EROOTFS_PATH -c usr/bin/dbus-cleanup-sockets | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/dbus-daemon | tar -C $SURGEONFS -xv
#tar -C $EROOTFS_PATH -c usr/bin/dbus-launch
#tar -C $EROOTFS_PATH -c usr/bin/dbus-monitor
#tar -C $EROOTFS_PATH -c usr/bin/dbus-send
tar -C $EROOTFS_PATH -c usr/bin/dbus-uuidgen | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/lib/libdbus-1.so{,.3,.3.4.0} | tar -C $SURGEONFS -xv
#tar -C $EROOTFS_PATH -c usr/libexec/dbus-1
#tar -C $EROOTFS_PATH -c usr/libexec/dbus-daemon-launch-helper
tar -C $EROOTFS_PATH -c usr/share/dbus-1/system-services/fi.epitest.hostap.WPASupplicant.service | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c var/run/dbus | tar -C $SURGEONFS -xv

#### DFTP
tar -C $EROOTFS_PATH -c etc/init.d/dftpdevice | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/dftpdevice | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c var/loglevel | tar -C $SURGEONFS -xv

#### NAND
tar -C $EROOTFS_PATH -c usr/bin/faster-nand-timing usr/bin/nand{wipebbt,get,scan,fmt,scrub,get2} \
	usr/sbin/nand{test,dump,write} usr/sbin/flash_{erase,eraseall} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/sbin/ubi{attach,detach,nfo,mkvol,rmvol} | tar -C $SURGEONFS -xv

#### NOR
tar -C $EROOTFS_PATH -c usr/sbin/flashcp | tar -C $SURGEONFS -xv

#### SD
tar -C $EROOTFS_PATH -c usr/sbin/fuse-flasher | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/lib/libfuse{.so,.so.2,.so.2.7.2} usr/lib/libulockmgr.so | tar -C $SURGEONFS -xv

#### EXT3
tar -C $EROOTFS_PATH -c sbin/{mkfs.ext3,mkfs.ext4,fsck.ext4,fsck.ext3,tune2fs,dumpe2fs} | tar -C $SURGEONFS -xv


#### MFGDATA ==> mfgdata, mfgmode.sh
tar -C $EROOTFS_PATH -c usr/bin/{mfgdata,mfgmode.sh} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/lib/libMfgData.so | tar -C $SURGEONFS -xv

#### Connectivity
# ftp
tar -C $EROOTFS_PATH -c etc/vsftpd.conf usr/sbin/vsftpd | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/{ftp-pipe-functions,ftpget,ftpput} | tar -C $SURGEONFS -xv

# telnet
tar -C $EROOTFS_PATH -c usr/bin/telnet usr/sbin/telnetd etc/init.d/vsftpd | tar -C $SURGEONFS -xv

#### Helpers
tar -C $EROOTFS_PATH -c usr/bin/{relax-surgeon,update,play,iospeed} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/{fileget,fileput,nandget,nandget2,norget,target,ubiget2,ver,ddget} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c var/{0.6.0,1.2.0}.fs | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c var/mbr2G.image | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/flash-fw | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/{get-ip,lsof,rl,getmac.sh} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/lfpkg | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/drawtext{,-fb} var/fonts/monotext8x16.rgb | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/{ts2nor,setcal,reset_lang,reset_profiles,reset_unit,partition_to_mtd,tim,evtest,new-calib,list-input-devices} | tar -C $SURGEONFS -xv
tar -C $EROOTFS_PATH -c usr/bin/i2c-ctl | tar -C $SURGEONFS -xv

#####################################################################
# Fix for uClibc 29/30 problem -- see make_payload.sh
#####################################################################

# install new OpenEmbedded uclibc
tar -C $SURGEONFS -xvf $PROJECT_PATH/packages/oe-uclibc/uclibc-0.9.30-svn.tar
# remove any remains of older uclibc 0.9.29 library
# rm -f $SURGEONFS/lib/*0.9.29*
# install new gcc 4.3.3 supporting libraries
tar -C $SURGEONFS -xvf $PROJECT_PATH/packages/oe-gcc/gcc-cross-4.3.3-r5.1.tar
# remove any remains of older gcc supporting libraries 
# rm -f $SURGEONFS/lib/libstdc++*

##############################################################################

# Compress some libs
set +x
savings=0
for i in $SURGEONFS/usr/lib/lib*.so* $SURGEONFS/lib/lib*.so* $SURGEONFS/{bin,usr/bin,sbin,usr/sbin}/*; do
	if file $i | fgrep "not stripped" > /dev/null && 
	    [ `basename "$i"` != "libc.so" -a ! -L "$i" ]; then
		before=`stat --printf=%s $i`
		arm-linux-strip $i
		after=`stat --printf=%s $i`
		STRIP_DELTA=$((before-after))
		echo "strip $i: $before -> $after = $STRIP_DELTA"
		savings=$((savings+STRIP_DELTA))
	fi
done
echo "Total savings from stripping: $savings bytes"

set -x
echo "look into $SURGEONFS..."

# Restore one last piece
mkdir -p $SURGEONFS/LF/Bulk/ProgramFiles
mkdir -p $SURGEONFS/LF/Base

# Take a look?
if [ true ]; then
	du -s $SURGEONFS/*
	tar -c $SURGEONFS | wc -c
fi

# DEBUG: hang onto what we built in case we did something wrong
# But first, remove old ones
rm -f surgeon-[0-9]*.tar
tar -C $SURGEONFS -cf surgeon-$VERSION.tar .

# Make the surgeon kernel cbf
pushd $PROJECT_PATH/linux-2.6
make lf1000_ts_surgeon_defconfig
SURGEONFS=$SURGEONFS ./install.sh
cd ../scripts
CBF=surgeon.cbf
./make_compressed_cbf.py -o $CBF

# Restore old kernel config
cd $PROJECT_PATH/linux-2.6
make lf1000_ts_defconfig
cd ../scripts

# Blow up if we exceed 8MB limit
LIMIT=$((8*1024*1024))
if [ `stat -c '%s' $CBF` -gt $LIMIT ]; then
	echo "$CBF size is > $LIMIT bytes; must be reduced to fit."
	echo "Build aborted"
	exit 1
fi

# Wrap surgeon in lfp
WHERE=Surgeon
LFPKG=../packages/lfpkg/lfpkg
rm -rf $WHERE
mkdir -p $WHERE
cp $CBF $WHERE
cat <<EOF > $WHERE/meta.inf
MetaVersion="1.0"
Device="LeapsterExplorer"
Type="System"
ProductID=0x00170028
PackageID="LST3-0x00170028-000000"
PartNumber="152-12352"
Version="$VERSION"
Locale="en-us"
Name="Surgeon"
ShortName="Surgeon"
Publisher="LeapFrog, Inc."
Developer="LeapFrog, Inc."
Hidden=1
BinFile="surgeon.cbf"
BuildDate="`date +%x`"
EOF
rm -f $WHERE-*.lfp
$LFPKG -a create $WHERE

#Create Madrid Surgeon (which is the same surgeon with a new meta.inf!)
WHERE_LPAD=Surgeon-Lpad
mv $WHERE $WHERE_LPAD
cat <<EOF > $WHERE_LPAD/meta.inf
MetaVersion="1.0"
Device="LeapPadExplorer"
Type="System"
ProductID=0x001E0011
PackageID="LPAD-0x001E0011-000000"
PartNumber="152-12546"
Version="$VERSION"
Locale="en-us"
Name="Surgeon"
ShortName="Surgeon"
Publisher="LeapFrog, Inc."
Developer="LeapFrog, Inc."
Hidden=1
BinFile="surgeon.cbf"
BuildDate="`date +%x`"
EOF
rm -f $WHERE_LPAD-*.lfp
$LFPKG -a create $WHERE_LPAD

rm -rf $WHERE_LPAD

popd

# Clean up
echo Clean up $SURGEONFS
sudo rm -rf $SURGEONFS

echo ">>> EXITING: $0"
exit 0
