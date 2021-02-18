#!/bin/sh

# set -x

CART=/LF/Cart
cart_mounted=`mount | grep $CART`

if [ $# != "1" ]; then
	echo "Program the OTP cartridge with the specified bin file"
	echo "Usage:"
	echo "$0 bin_file"
	exit 1
fi

if [ ! -e $1 ]; then
	echo "Error: file $1 doesn't exist !"
	exit 1
fi


if [ -n "$cart_mounted" ]; then
	echo "Error: cartridge is already mounted, and has content inside !!"
	exit 1;
fi

cart_mtd=`cat /proc/mtd | grep -e "\bCartridge" | cut -d: -f1`
if [ -z $cart_mtd ]; then
	echo 1 > /sys/devices/platform/lf1000-nand/cart_hotswap
	ff1=`cat  /sys/devices/platform/lf1000-nand/cart_hotswap | cut -f1`
	ff2=`cat  /sys/devices/platform/lf1000-nand/cart_hotswap | cut -f2`
	if [ "$ff1" != "1" ]; then
		echo "Cartridge init failure, please reinsert, and try again !!"
		exit 1;
	fi
	if [ "$ff2" != "0" ]; then
		echo "Cartridge has some Didj content inside, please reinsert a different cartridge !!"
		exit 1;
	fi
fi

#flush mtd read cache
cart_mtd=`cat /proc/mtd | grep -e "\bCartridge" | cut -d: -f1`
nanddump -s 1026 -l 512 -n -o /dev/$cart_mtd 2>&1 > /dev/null


mtd_size=`cat /sys/class/mtd/$cart_mtd/size`
bin_size=`stat -t $1 | cut -d" " -f2`
if [ $mtd_size -ne $bin_size ]; then
	echo "Cartridge size $mtd_size doesn't match bin file size $bin_size"
	exit 1;
fi

count=`nanddump -s 0 -l 512 -p -n -o /dev/$cart_mtd 2>&1 | grep -c "ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"`
if [ $count -ne "32" ]; then
	while true; do
		echo "Cartridge is programmed, do you want to continue (y/n)?"
		read choice
		if [ $choice = "y" ]; then
			break;
		elif [ $choice = "n" ]; then
			exit 1;
		fi
	done
fi

if [ ! -e /flags/poweron ]; then
	touch /flags/poweron
	echo "Overwrite IDLE power off option !"
	add_poweron=1
else
	add_poweron=0
fi

# don't add s/n, as sandisk S/N proposal is killed as of 11/25/09
# addsn $1
otpwrite /dev/$cart_mtd $1
otpwrite_ret=$?

if [ $add_poweron -eq "1" ]; then
	echo "Restore IDLE power off option !"
	rm -f /flags/poweron
fi

if [ $otpwrite_ret -eq "0" ]; then
	echo "Done !"
	exit 0
else
	exit 1
fi

