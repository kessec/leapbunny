#!/bin/sh

SYS=/sys/devices/platform/lf1000-touchscreen;

getit()
{
	echo 1 > $SYS/us_in_driver;
	sleep 1;
	G=`cat $SYS/us_in_driver | cut -d/ -f2`
}

for a in 1 2 4 8; do
  echo -n "A=$a "
  for d in 0 10 20 40; do
#    for t in 0 10 20 40; do
     t=40
      echo $a > $SYS/averaging
      echo $d > $SYS/delay_in_us
      echo $t > $SYS/tnt_delay_in_us
      # echo -n "A=$a D=$d T=$t ";
      getit
      printf "%4d " "$G"
#    done
  done
  echo ""
done
