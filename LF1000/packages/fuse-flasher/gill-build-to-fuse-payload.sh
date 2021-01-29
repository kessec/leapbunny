#!/bin/sh

LFP_SRC=/mnt/s/Leapster3/Interim_Build_1.2.10-1718/packages
RAW_SRC=/mnt/s/Leapster3/Interim_Build_1.2.10-1718/payload

STAGE=`mktemp -d /tmp/fuse-payload-XXXXXX`
echo $STAGE
# ls -l $LFP_SRC
# ls -l $RAW_SRC
mkdir -p \
	$STAGE/NORBoot \
	$STAGE/sd/partition \
	$STAGE/sd/raw/1 \
	$STAGE/sd/raw/2 \
	$STAGE/sd/ext3/3 \
	$STAGE/sd/ext3/4

cp -p mbr2G.image $STAGE/sd/partition
cp -p $RAW_SRC/madrid-boot.bin $STAGE/NORBoot
cp -p $RAW_SRC/FIRST_Lpad.32.rle $STAGE/sd/raw/1
cp -p $RAW_SRC/kernel.cbf $STAGE/sd/raw/2
gunzip -c $RAW_SRC/erootfs-lpd.tar.gz > $STAGE/sd/ext3/3/rfs
gunzip -c $RAW_SRC/bulk_lpad.tar.gz > $STAGE/sd/ext3/4/Bulk
find $STAGE -type f | xargs ls -ld

NAME="firmware"
tar -Hustar -C $STAGE -cf "$NAME.tar" NORBoot sd/{partition,raw,ext3}
bzip2 -9c "$NAME.tar"  > "$NAME.lf2"

rm -rf $STAGE
