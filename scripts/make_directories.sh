#!/bin/bash

# assume all variables are set.  Create directory stucture under ROOTFS_PATH
set -e
set -x

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

echo "GS: make_directories.sh"
echo $0

mkdir -p $ROOTFS_PATH
pushd $ROOTFS_PATH
mkdir -p bin dev etc lib proc sbin tmp usr var sys boot mnt mnt2 opt flags
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p LF LF/Base LF/Bulk LF/Cart
mkdir -p LF/Base/FR 
mkdir -p LF/Bulk/{LanguagePack,ProgramFiles,Downloads}
mkdir -p LF/Bulk/LanguagePack_en/Tutorials
mkdir -p LF/Bulk/Data/Uploads/{0,1,2,All}
mkdir -p LF/Bulk/Data/Local/{0,1,2,All}
mkdir -p mfgdata
mkdir -p www
# Chmod mount points
chmod 777 LF/Bulk LF/Cart
# Chmod R/W points for games
chmod 777 LF/Base/FR mfgdata flags www
# Chmod Game Folders
chmod -R 777 LF/Bulk/Data # Games can write anything in here
chmod 777 LF/Base # So AppManager can write settings.cfg there

# Make compatibility links
L=usr/bin/compatibility-links.sh
cat <<EOF > $L
#!/bin/sh
echo "These links are for allowing legacy tutorial folder structures to work"
echo "Installing..."
ln -s /LF/Bulk/LanguagePack_en/Tutorials LF/Base/Tutorials
echo "To remove compatibility links type:"
echo "   rm LF/Base/Tutorials"
EOF
chmod +x $L
# run this now:
$L

if [ $EMBEDDED -eq 0 ]; then
	mkdir -p usr/include
	mkdir -p usr/local/include
	mkdir -p usr/local/bin
fi

mkdir -p var/lib var/lock var/log var/run var/tmp
chmod 2777 var/tmp
chmod a+rwx $ROOTFS_PATH
popd

exit 0
