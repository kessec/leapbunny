#!/bin/sh
#
#  YTu  10/16/2009 Totally rewrite
#
#
#set -x

size=$1
outfile=$2
infile=$3
exit_code=0

# Check for usage
if [ ! "$3" -o "$1" != "32" -a "$1" != "64" -a "$1" != "128" ]; then
    cat <<EOF

Usage:	mkotp.sh size Outfile game
	This utility creates a game OTP image (.bin)

Parameters:
	size: 	32, 64 or 128, indicating the size in MB of the output file.
	outfile: 	output file name, if no extension provided, .bin will be appended.
	game: 	either .zip/.lfp file that contains a game or a path to the game directory.
	
Examples:
	./mkotp.sh 64 harmonies.bin harmonies.zip
	./mkopt.sh 64 harmonies.bin ~/gamespace/harmonies
	./mkopt.sh 64 nlr.bin ~/gamespace/nlr

EOF
    exit 0
elif [ ! -e "$infile" ]; then
    echo Input file $infile does not exist
    exit 1
fi

##############################################################################
date=`date '+%Y%m%d-%H%M'`
if [ "x$who" = "x" ]; then
    who=anonymous
fi

#append .bin to outfile
if ! echo $outfile | grep '\.bin$' >/dev/null; then
	outfile=$outfile.bin
fi

echo "Building $outfile on $date by " $who
echo "Please wait ..."


# Test for input being .zip
if echo $infile | grep '\.zip$' >/dev/null; then
	iszip=1
	echo "game input from a zip file ..."
elif echo $infile | grep '\.lfp$' >/dev/null; then
	iszip=1
	echo "game input from a lfp file ..."
else
	if [ -d $infile -a -f $infile/meta.inf ]; then
		echo "game input from a directory ..."
	else
		echo "Game Input file '$infile' must be *.zip or *.lfp file or a directory containing game"
		exit 1
	fi
	iszip=0
fi

if [ -e /LF/Bulk ]; then
	cart_file=/LF/Bulk/_cart_loop_
	rm -f $cart_file
	cart_dir=/LF/Bulk/cart
	mkdir -p $cart_dir
else
	cart_file=`mktemp cart.XXXXX`
	cart_dir=`mktemp -d cart_mount.XXXXX`	
fi

# dd if=/dev/zero of=$cart_file bs=1024 count=65536
# mkfs.vfat -n "Cartridge" -F 32 -s 1 -S 512 -C $cart_file 65536
if [ -e /LF/Bulk ]; then
dd2ff $cart_file 67108864
else
./dd2ff $cart_file 67108864
fi

# find a device
loopd=`sudo losetup -f`
if [ -z loopd ]; then
	if [ ! -e /dev/loop0 ]; then
		mknod /dev/loop0 b 7 0
		if [$? == 0 ]; then
			loopd=/dev/loop0
		fi
	else
		echo "Error, no free loop device"
		rm -fr $cart_file
		rm -fr $cart_dir	
		exit 1
	fi
else
	if [ ! -e $loopd ]; then
		mknod $loopd b 7 0
		if [ $? != 0 ]; then
			echo "Can't mknod $loopd b 7 0"
			rm -fr $cart_file
			rm -fr $cart_dir	
			exit 1
		fi
	fi
fi

# setup the device
sudo losetup $loopd $cart_file
if [ $? != 0 ]; then
	echo "Error, Can't attach loop device losetup $loopd $cart_file"
	rm -fr $cart_file
	rm -fr $cart_dir	
	exit 1
fi

sudo mkfs.vfat -n "Cartridge" -F 32 -s 1 -S 512 $loopd 65536

# create a mount point
sudo mount -t vfat $loopd $cart_dir
if [ $? != 0 ]; then
	sudo losetup -d $loopd
	rm -fr $cart_file
	rm -fr $cart_dir	
	echo "Error: Can't mount $loopd $cart_dir"
	exit 1
fi

if [ $iszip -eq 1 ]; then
	echo "unzip $infile ..."
	sudo unzip -q $infile -d $cart_dir
	if [ $? != 0 ]; then
		echo "Error: Unzip $infile failed"
		echo "Error: Probably total file size for $infile is larger than $size"
		echo "Error: Please reduce file size, and retry"
		sudo umount $cart_dir
		sudo losetup -d $loopd
		rm -fr $cart_file
		rm -fr $cart_dir
		exit 1	
	fi
else
	echo "Game content from $infile ..."
	sudo cp -r $infile -d $cart_dir
	if [ $? != 0 ]; then
		echo "Error: copy $infile failed"
		echo "Error: Probably total file size for $infile is larger than $size"
		echo "Error: Reduce file size, and retry"
		sudo umount $cart_dir
		sudo losetup -d $loopd
		rm -fr $cart_file
		rm -fr $cart_dir
		exit 1
	fi
fi

# check a couple of things
ntoplevel=`ls $cart_dir | wc -l`
if [ $ntoplevel -ne 1 -a $ntoplevel -ne 2 ]; then
	echo "Error: directory structure wrong in the zip file -- "
	sudo umount $cart_dir
	sudo losetup -d $loopd
	rm -fr $cart_file
	rm -fr $cart_dir	
	exit 1
fi

used_size=`du -k -s $cart_dir/ | cut -f1`
free_size=`expr $size \* 1000 - $used_size`
echo "Game used $used_size KBytes, free $free_size KBytes"

if [ -e /LF/Bulk ]; then
	last_block=/LF/Bulk/last_block_
	ff_file=/var/ff.bin
	sudo dd if=$loopd skip=131071  of=$last_block bs=512 count=1 &> /dev/null	
else
	last_block=`mktemp lastblock.XXXXX`
	ff_file=ff.bin
	sudo dd status=noxfer if=$loopd skip=131071  of=$last_block bs=512 count=1 &> /dev/null	
fi

nonempty=`cmp -s $ff_file $last_block`
if [ ! -z $nonempty ]; then
	echo "The last page is not empty"
	echo "Please try to reduce the file size !!"
	exit_code=1
else
	echo "Verified that the last page is empty"
fi
rm -fr $last_block

sudo umount $cart_dir
sudo losetup -d $loopd
mv $cart_file $outfile
rm -fr $cart_dir

if [ $exit_code -eq '0' ]; then
	echo "Success making OTP image!!!"
else
	echo "Failure !!!"
	exit $exit_code
fi

if [ -e /LF/Bulk ]; then
	while true; do
		echo "Do you want to continue burning the image into OTP Cartridge ? (y/n)?"
		read choice
		if [ $choice = "y" ]; then
			break;
		elif [ $choice = "n" ]; then
			echo "You can manaully burning the $outfile into OTP Cartridge using :"
			echo "	otp_program.sh $outfile"
			exit 0;
		fi
	done

	otp_program.sh $outfile
	if [ $? -eq "0" ]; then
		echo "Your cartridge is programmed !"
	fi
else
	exit 0
fi


