#!/bin/sh

pwd=$PWD
for d in ???-1*; do
	bn=$(echo $d | cut -c 1-4,22-24)-before.png
	an=$(echo $d | cut -c 1-4,22-24)-after.png
	rm /tmp/$bn /tmp/$an
	ln -s $pwd/$d/mb_0000.png /tmp/$bn
	ln -s $pwd/$d/mb_0001.png /tmp/$an
done
