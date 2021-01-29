#!/bin/sh

T=$(mktemp)
cat <<EOF >$T
retr $1
exit
EOF

$PROJECT_PATH/host_tools/dftp-client.py $(get-ip) < $T
echo "Fetched $1"
rm $T
