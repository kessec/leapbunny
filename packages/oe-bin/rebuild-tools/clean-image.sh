#!/bin/sh

set -x

pushd image

BLACKLIST=( "usr/include" "usr/share/man" "usr/share/info" "usr/share/aclocal" )

for DIR in ${BLACKLIST[@]}
do
	rm -rf "$DIR"
done

BLACKLIST=( "*.a" "*.la" "*.h" ".svn" ".debug" )

for TYPE in ${BLACKLIST[@]}
do
	find -name "$TYPE" | xargs rm -rf
done

find | xargs md5sum > "../image-new.txt"
find -type d >> "../image-new.txt"

popd

pushd image-old

find | xargs md5sum > "../image-old.txt"
find -type d >> "../image-old.txt"

meld "../image-old.txt" "../image-new.txt"
