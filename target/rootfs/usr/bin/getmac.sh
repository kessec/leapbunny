#!/bin/sh
#
# getmac.sh: return IPV4 MAC (Media Access Control) address for device.
#
# $1=device, supported values are 'GADGET' and 'HOST'
#
# The 48 bit MAC address uses the Emerald unit's serial number as part of the
# device ID in the lower 24 bits.  If a # serial number is not present, then
# generate a random serial number.
#
# 'GADGET' devices have BYTE4 values from 0x00 to 0x7F
# 'HOST' devices have BYTE4 values from 0x80 to 0xFF
#
RANDOM_COMPANY_MAC='00:00:45'	# for random MAC values use Ford Aerospace
LEAPFROG_COMPANY_MAC='00:1C:B4'	# until we get one, use Iridium Satellite

# convert byte into padded hex value
format_rand() {
    if [ $1 -lt 16 ]
    then
        echo -n 0
    fi
    echo -n `dc 16 o $1 p`
}

# get device serial number
SN=`mfgdata get sn`

if [ "X$SN" == 'X' ]
then # no serial number, make one up
    RAND=$RANDOM	# get next number
    if [ "X$RAND" == 'X' ]
    then
        echo "shell does not have \$RANDOM enabled, can't generate number." 
        exit 1
    fi
    BYTE4=`dc $RAND 128 % p`

    if [ "X$1" == 'XHOST' ] || [ "X$1" == 'Xhost' ]
    then
	BYTE4=`dc $BYTE4 128 + p`
    fi
    BYTE4=`format_rand $BYTE4`
    RAND=$RANDOM	# get next number
    BYTE5=`dc $RAND 256 % p`
    BYTE5=`format_rand $BYTE5`
    RAND=$RANDOM	# get next number
    BYTE6=`dc $RAND 256 % p`
    BYTE6=`format_rand $BYTE6`
    echo $RANDOM_COMPANY_MAC':'$BYTE4':'$BYTE5':'$BYTE6
else
    echo 'SN='$SN
    BYTE4=`dc $SN 65536 / 128 % p`
    if [ "X$1" == 'XHOST' ] || [ "X$1" == 'Xhost' ]
    then
	BYTE4=`dc $BYTE4 128 + p`
    fi
    BYTE4=`format_rand $BYTE4`
    BYTE5=`dc $SN 256 / 256 % p`
    BYTE5=`format_rand $BYTE5`
    BYTE6=`dc $SN 256 % p`
    BYTE6=`format_rand $BYTE6`
    echo $LEAPFROG_COMPANY_MAC':'$BYTE4':'$BYTE5':'$BYTE6
fi 
