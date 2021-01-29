#!/bin/sh
# $1 should be Flight Recorder directory
# $2 should be the program name that crashed
# $3 should be the PID of the crash process

# skip coredump by default, unless in developer mode
if [ ! -e /flags/coredump ]; then
	echo "skipping coredump" 
	exit 0
fi

exec 1>/dev/console
exec 2>/dev/console

fr_dir=$1
core_prog=$2
core_pid=$3

_print_header()
{
	printf '\n[---core-info: %s---]\n' "$@"
}

_print_file_with_header()
{
	if [ -f $1 ]; then
		_print_header $1
		cat $1
	fi
}

_print_command_with_header()
{
	_cmd=$1
	shift
	if [ -x ${_cmd} ]; then
		_print_header ${_cmd##*/}
		{ ${_cmd} $@ 2>&1; }
	fi
}

_section_date()
{ 
	_print_command_with_header /bin/date
}

_section_linux_version()
{ 
	_print_file_with_header /proc/version
}

_section_firmware_version()
{ 
	_print_file_with_header /etc/version
}

_section_game_version()
{ 
	nandgame=`cat /proc/${core_pid}/smaps | grep -m 1 -o -e "/LF/Bulk/ProgramFiles/.*/"`
	meta=meta.inf
	nandgamemeta=$nandgame$meta
	if [ -e $nandgamemeta ]; then
		_print_header $nandgamemeta	
		cat $nandgamemeta
	fi

	cartgame=`cat /proc/${core_pid}/smaps | grep -m 1 -o -e "/LF/Cart/.*/"`
	meta=meta.inf
	cartgamemeta=$cartgame$meta
	if [ -e $cartgamemeta ]; then
		_print_header $cartgamemeta		
		cat $cartgamemeta
	fi
	
}

_section_cmdline()
{
	_print_file_with_header /proc/${core_pid}/cmdline
}

_section_proc_fd()
{ 
	_print_header fd
	export LD_LIBRARY_PATH=/LF/Base/Brio/lib
	ls -l /proc/${core_pid}/fd/
}

_section_df()
{
	_print_command_with_header /bin/df
}

_section_mount()
{
	_print_command_with_header /bin/mount
}

_section_meminfo()
{
	_print_file_with_header /proc/meminfo
} 


(
logger -s -t "Emerald Base" -p local4.notice "$core_prog $core_pid crashed !!!"
_section_date
_section_linux_version
_section_firmware_version
_section_game_version
_section_cmdline
_section_df
_section_proc_fd
_section_mount
_section_meminfo
) > $fr_dir/core_info

echo Generating coredump ...
# dumpcore $1 $2 2>&1
cd $fr_dir
/sbin/lzop -v -f -o core_$2.lzo
if [ $? -eq "0" ]; then
	echo done !
else
	rm -f core_$2.lzo
	logger -s -t "Emerald Base" -p local4.notice "Failed to generate coredump core_$2.lzo!!!"
fi


