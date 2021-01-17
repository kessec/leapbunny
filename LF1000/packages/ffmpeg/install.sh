#!/bin/bash

# Build ffmpeg libs from source
FFMPEG_LIB_DIR=ffmpeg-0.5.1
FFMPEG_LIB_SRC=ffmpeg-0.5.1.tar.bz2

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/ffmpeg/

if [ ! -e $FFMPEG_LIB_SRC ]; then
	wget http://www.ffmpeg.org/releases/$FFMPEG_LIB_SRC
fi

if [ "$CLEAN" == "1" -o ! -e $FFMPEG_LIB_DIR ]; then
	rm -rf $FFMPEG_LIB_DIR
	tar -xjf $FFMPEG_LIB_SRC
	# apply patch for internal header to avoid static inline with ARM gcc 4.1.
	patch -p0 < ffmpeg-internal-header.patch
fi

# build and copy shared libs to rootfs
pushd $FFMPEG_LIB_DIR

# cull unwanted codecs from config
./configure --enable-cross-compile --cross-prefix=arm-linux- --target-os=linux --arch=arm --cpu=arm926ej-s --prefix=$ROOTFS_PATH/usr/local --enable-shared  --disable-pthreads --disable-armvfp --disable-debug --disable-stripping --disable-optimizations --disable-filters --disable-muxers --disable-demuxers --disable-encoders --disable-decoders --enable-muxer=avi --enable-demuxer=avi --enable-muxer=rawvideo --enable-demuxer=rawvideo --enable-encoder=rawvideo --enable-decoder=rawvideo --enable-muxer=mjpeg --enable-demuxer=mjpeg --enable-encoder=mjpeg --enable-decoder=mjpeg --enable-muxer=pcm_s16le --enable-demuxer=pcm_s16le --enable-encoder=pcm_s16le --enable-decoder=pcm_s16le --disable-parsers --enable-parser=mjpeg --enable-parser=ac3

make
make install

popd

popd

exit 0



