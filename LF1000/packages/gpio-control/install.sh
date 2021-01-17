#!/bin/bash

. $PROJECT_PATH/scripts/functions

check_vars

pushd $PROJECT_PATH/packages/gpio-control

if [ "$CLEAN" == "1" ]; then
	make clean
fi
make install

popd

exit 0
