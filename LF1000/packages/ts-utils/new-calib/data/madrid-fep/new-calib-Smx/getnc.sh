#!/bin/sh

pat=new-calib-S5mx
pat=new-calib-Smx
for i in 1 2 3 4 5 6 7 8 9; do
	if [ -e $pat$i.log ]; then
		continue;
	fi
	mv ~lfu/new-calib.log $pat$i.log
	sudo chown $USER:$USER *
	break;
done
echo "Created $pat$i.log"
