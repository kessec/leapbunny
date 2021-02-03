#!/bin/sh

T=$(mktemp)
cat <<EOF >$T
retr /var/log/new-calib.log
exit
EOF

$PROJECT_PATH/host_tools/dftp-client.py $(get-ip) < $T
echo "Fetched new-calib.log"
rm $T
