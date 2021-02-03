#!/usr/bin/perl

while (<>)
{
    for (split /;/)
    {
	next unless ($x, $y, $t, $p) = /(\d+),(\d+),(\d+),(\d+)/;
	$dx=$x-$lastx;
	$dy=$y-$lasty;
	$lastx=$x;
	$lasty=$y;
	next if $t>50;
	$d=sqrt($dx*$dx+$dy*$dy);
	$D+=$d;
	$d=int($d);
	$d=10 if ($d>10);
	$h[$d]++;
	$n++;
    }
}

for (0..10)
{
    printf ("%5.1f%% ", 100*$h[$_]/$n) if $h[$_];
}
printf ("  N=%5d D=%6d\n", $n, $D);

