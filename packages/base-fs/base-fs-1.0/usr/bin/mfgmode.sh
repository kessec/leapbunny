#!/bin/sh

SYS=/sys/devices/platform/lf1000-nand/nor_write_addr_threshold

usage ()
{
	echo "Usage"
	echo "   $0 0 # Write protect entire NOR"
	echo "   $0 1 # Write enable scratch area only"
	echo "   $0 2 # Write enable MfgData and scratch area"
	echo "Current address: `cat $SYS`.  See /proc/mtd"
	exit 1
}

get_addr ()
{
	printf "%d" 0x`grep $1 /proc/mtd | cut -d ' ' -f 2`
}

if [ -z "$1" ]; then
	usage
fi
if [ ! -e $SYS ]; then
	echo "Trouble.  No $SYS"
	exit 1
fi

case "$1" in
	0)
		addr=0x7fff0000
		;;
	1)
		addr=$((`get_addr NOR_Boot` + `get_addr MfgData1`))
		;;
	2)
		addr=`get_addr NOR_Boot`
		;;
	9)
		addr=0
		;;
	*)
		usage
		;;
esac
echo $addr > $SYS
printf "%s set to 0x%x\n" `basename $SYS` $addr
exit 0
