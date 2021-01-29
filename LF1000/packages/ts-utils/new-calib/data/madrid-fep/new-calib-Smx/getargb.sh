#!/bin/sh

for i in ~lfu/mb_00[0-9][0-9].argb; do
	n=$(basename $i)
	mv $i .
	sudo chown $USER:$USER $n
	convert -size 480x272 -depth 8 -alpha off rgba:$n ${n/argb/png}
	rm $n;
	echo "done ${n/argb/png}"
done
