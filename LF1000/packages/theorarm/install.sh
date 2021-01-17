#!/bin/bash

# Build theorarm libs from source
THEORA_LIB_DIR=theorarm-merge-branch
THEORA_LIB_SRC=$THEORA_LIB_DIR.tar.gz

set -e
set +x

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/theorarm/

if [ ! -e $THEORA_LIB_SRC ]; then
	svn export http://svn.xiph.org/branches/theorarm-merge-branch
fi

if [ "$CLEAN" == "1" -o ! -e $THEORA_LIB_DIR ]; then
	rm -rf $THEORA_LIB_DIR
	tar -xzf $THEORA_LIB_SRC

	# FIXME: manual fixups needed as follows:
	# Convert ARM .s files in ./lib/arm to gcc-compatible files with arm2gcc.pl script.
	# Modify ARMoptions.s with ARMV6 and ARM_HAS_NEON options disabled (0).
	# Modify makefiles to add "-Xassembler -Iarm/" options to CCASFLAGS.
	# Not all assembler function replacements work, so OC_ARM_ASM should not be defined in decode.c.
	pushd $THEORA_LIB_DIR
	set +e
	./autogen.sh --host=arm-linux --build=x86-linux
	set -e
	./autogen.sh --host=arm-linux --build=x86-linux
	cp ../patches/*.s lib/arm/
	patch -p1 < ../patches/ARMstate.patch
	patch -p1 < ../patches/decode.patch
	popd
fi

# build and copy shared libs to rootfs
pushd $THEORA_LIB_DIR

OGG_CFLAGS="-I${ROOTFS_PATH}/usr/local/include" OGG_LIBS="-L${ROOTFS_PATH}/usr/local/lib" CCASFLAGS="-Xassembler -Iarm/" ./configure --host=arm-linux --build=x86-linux --prefix=$ROOTFS_PATH/usr/local --enable-shared=yes --disable-float --disable-encode -disable-examples --with-ogg=$ROOTFS_PATH/usr/local --with-vorbis=$ROOTFS_PATH/usr/local --disable-oggtest --disable-vorbistest --disable-sdltest

make
make install
#cp -a ./lib/.libs/libtheora.so* $ROOTFS_PATH/usr/local/lib/
#cp -R ./include/theora $ROOTFS_PATH/usr/local/include/

popd

popd

exit 0
