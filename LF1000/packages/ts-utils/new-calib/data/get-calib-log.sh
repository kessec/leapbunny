#!/bin/bash

HT=$PROJECT_PATH/host_tools
T=`mktemp /tmp/gcl.dftp.XXXXXXXXXX`
cat <<EOF > $T
retr /var/log/new-calib.log
EOF
$HT/dftp-client.py < $T `$HT/get-ip`
x=1;
while z=new-calib-`printf %04d $x`.log && [ -e $z ]; do
	x=$((++x))
done
mv new-calib.log $z
echo $z
