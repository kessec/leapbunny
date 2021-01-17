#!/bin/bash

# make patches using a diff command like this:
# diff -Naur mtd-utils/flashcp.c mtd-utils/flashcp.c.new > flashcp.c.patch

set -e
MTD_SRC=mtd-utils.tar.gz
ZLIB_SRC=zlib-1.2.3
LZO_SRC=lzo-2.02
ARGP_SRC=argp-standalone-1.3
E2FS_SRC=e2fsprogs-1.41.3
MANDIR=/tmp/mtd-utils

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/mtd-utils-2

MTD_DIR=mtd-utils

if [ "$CLEAN" == "1" -o ! -e $MTD_DIR ]; then
	rm -rf $MTD_DIR
	tar -xzf $MTD_SRC
	patch -p0 < flash_erase2.patch
	patch -p0 < mtd-bbt-ioctls.patch
	patch -p0 < flashcp.c.patch
	patch -p0 < scan_accepts_errors.patch
	# patch -p0 < md5-option-for-nanddump.patch
	patch -p0 < nandtest_dont_stop_on_errors.patch
	patch -p0 < nandtest_ECC_failed.patch
	pushd $MTD_DIR
	# we need to be able to set the include path using CFLAGS
	popd
fi

pushd $MTD_DIR

INCDIR=$PROJECT_PATH/packages/zlib/$ZLIB_SRC/
if [ ! -e $INCDIR ]; then
	echo "Can't find zlib at $INCDIR.  Did you build zlib?"
	exit 1
fi

LZOINC=$PROJECT_PATH/packages/lzo/$LZO_SRC/include
LZOLIB=$PROJECT_PATH/packages/lzo/$LZO_SRC/src/.libs/
if [ ! -e $LZOINC ]; then
	echo "Can't find lzo at $LZODIR.  Did you build lzo?"
	exit 1
fi

ARGPINC=$PROJECT_PATH/packages/argp/$ARGP_SRC/
ARGPLIB=$PROJECT_PATH/packages/argp/$ARGP_SRC/
if [ ! -e $ARGPINC ]; then
	echo "Can't find argp at $ARGPDIR.  Did you build argp?"
	exit 1
fi

E2FSINC=$PROJECT_PATH/packages/e2fsprogs/$E2FS_SRC/
E2FSLIB=$PROJECT_PATH/packages/e2fsprogs/$E2FS_SRC/lib
if [ ! -e $E2FSINC ]; then
	echo "Can't find e2fsprogs at $E2FSDIR.  Did you build e2fsprogs?"
	exit 1
fi

CROSS=$CROSS_COMPILE CFLAGS="-I$KERNELDIR/include -I./src -I$INCDIR -I$LZOINC -I$ARGPINC -I$E2FSINC/lib" LDFLAGS="-L$INCDIR -L$LZOLIB -L$ARGPLIB -L$E2FSLIB" make WITHOUT_XATTR=1 DESTDIR=$ROOTFS_PATH MANDIR=$MANDIR install

popd

# Create profnand.c and put in /usr/bin
GCC=gcc
INCLUDES="$INCLUDE -Imtd-utils/include -Imtd-utils/ubi-utils/src"
# -Imtd-utils/ubi-utils/new-utils/include \
$CROSS_COMPILE$GCC $INCLUDES -o profnand profnand.c
$CROSS_COMPILE$GCC $INCLUDES -o nandscrub nandscrub.c
$CROSS_COMPILE$GCC $INCLUDES -o nandwipebbt nandwipebbt.c
$CROSS_COMPILE$GCC $INCLUDES -o nandscan nandscan.c
$CROSS_COMPILE$GCC $INCLUDES -o xor xor.c
$CROSS_COMPILE$GCC $INCLUDES -o oobdump oobdump.c
$CROSS_COMPILE$GCC $INCLUDES \
	-UDEBUG -UFAKE \
	-o libnandfmt.o -c libnandfmt.c
AR=ar
$CROSS_COMPILE$AR -rv libnandfmt.a libnandfmt.o \
	mtd-utils/arm-linux/ubi-utils/new-utils/libmtd.o \
	mtd-utils/arm-linux/ubi-utils/new-utils/libscan.o \
	mtd-utils/arm-linux/ubi-utils/new-utils/crc32.o
$CROSS_COMPILE$GCC $INCLUDES \
	-o nandfmt nandfmt.c libnandfmt.a \

cp -pv profnand nandscrub nandscan nandwipebbt oobdump xor nandfmt \
	$ROOTFS_PATH/usr/bin

# remove man files
if [ -e $ROOTFS_PATH/$MANDIR ]; then
	rm -rf $ROOTFS_PATH/$MANDIR
fi

popd

exit 0
