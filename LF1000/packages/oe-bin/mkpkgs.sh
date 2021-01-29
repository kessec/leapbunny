#!/bin/sh

set -e
set -x

DEST=`pwd`

cleanup() {
	find -type d -name ".debug" -print0 | xargs -0 -r rm -rf
	find -name "*.m4" -print0 | xargs -0 -r rm
	find -name "*.la" -print0 | xargs -0 -r rm
	find -name "*.pc" -print0 | xargs -0 -r rm
	find -type d -name "man" -print0 | xargs -0 -r rm -rf
	find -type d -name "include" -print0 | xargs -0 -r rm -rf
}

OE_PKGS=( alsa-lib alsa-utils avahi busybox dbus \
  expat gdb gdbserver gnutls lfd \
  libdaemon libgcrypt libgpg-error \
  libusb ncurses openssh openssl sudo \
  tslib vsftpd wireless-tools wpa-supplicant )

for PKG in ${OE_PKGS[@]}; do
	pushd $OE_HOME/../tmp/work/arm*/${PKG}*/image
	cleanup
	pushd ..
	TARNAME="`pwd | xargs basename`.tar"
	popd
	tar -cf $TARNAME *
	mv $TARNAME $DEST
	popd
done

#Special handling for zip(unzip) and coreutils-partial(ls)
