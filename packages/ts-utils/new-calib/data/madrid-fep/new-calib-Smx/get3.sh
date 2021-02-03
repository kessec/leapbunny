#!/bin/sh

mv ~lfu/new-calib.log .
mv ~lfu/set-ts.sh .
mv ~lfu/pointercal .
sudo chown $USER:$USER *
echo "done "

