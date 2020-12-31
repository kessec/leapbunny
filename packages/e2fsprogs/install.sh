#!/bin/bash

set -e
E2FS_SRC=e2fsprogs_1.41.3.tar.gz
ZLIB_SRC=zlib-1.2.3
LZO_SRC=lzo-2.02
ARGP_SRC=argp-standalone-1.3

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/e2fsprogs

E2FS_DIR=e2fsprogs-1.41.3

if [ "$CLEAN" == "1" -o ! -e $E2FS_DIR ]; then
	rm -rf $E2FS_DIR
	tar -xzf $E2FS_SRC
	pushd $E2FS_DIR
	popd
fi

pushd $E2FS_DIR

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

# put man and exe files in temp dir
TMPDIR=$ROOTFS_PATH/$E2FS_DIR

# configure the package
./configure --with-cc=arm-linux-gcc --host=arm-linux --build=x86-linux --prefix=$ROOTFS_PATH/usr/local --enable-shared=yes --mandir=$TMPDIR --bindir=$TMPDIR --sbindir=$TMPDIR

CROSS=$CROSS_COMPILE CFLAGS="-I$EXTRA_LINUX_HEADER_DIR -I$INCDIR -I$LZOINC -I$ARGPINC" LDFLAGS="-L$INCDIR -L$LZOLIB -L$ARGPLIB" make WITHOUT_XATTR=1 install

# remove Documentation and program files
if [ -e $TMPDIR ]; then
	rm -rf $TMPDIR
fi

# Remove locale and info pages
rm -f $ROOTFS_PATH/usr/local/share/info/libext2fs.info.gz
rmdir $ROOTFS_PATH/usr/local/share/info || true
find $ROOTFS_PATH/usr/local/share -type f -name e2fsprogs.mo -delete
# Remove any empty directories
rmdir $ROOTFS_PATH/usr/local/share/locale/*/* || true
rmdir $ROOTFS_PATH/usr/local/share/locale/* || true
rmdir $ROOTFS_PATH/usr/local/share/locale || true
rmdir $ROOTFS_PATH/usr/local/share || true

popd

exit 0
