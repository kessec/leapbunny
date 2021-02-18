#!/bin/sh

# define some paths
APPMANAGER_PATH=/LF/Base/bin/AppManager
CORE_LZO_PATH=/LF/Base/FR/core_AppManager.lzo
CORE_DIR_PATH=/LF/Bulk/Data/Uploads/All/FR/
CORE_FILE_PATH=${CORE_DIR_PATH}core_AppManager

mkdir -p $CORE_DIR_PATH

# remove the existing core file
if [ -e $CORE_FILE_PATH ]; then
     echo "Removing existing core file: $CORE_FILE_PATH"
     rm $CORE_FILE_PATH
fi

# uncompress the core file
echo "Expanding core file: $CORE_FILE_PATH"
lzop -o $CORE_FILE_PATH -d $CORE_LZO_PATH

# launch gdb
gdb $APPMANAGER_PATH $CORE_FILE_PATH
