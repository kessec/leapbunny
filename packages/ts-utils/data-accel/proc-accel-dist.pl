#!/usr/bin/perl

use bias;

while (<>)
{
    $n = ($x, $y, $z) = /D\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)/;
    next unless $n;
    next if $x == -1000 || $y == -1000 || $z == -1000;
    $x-=$xbias; $y-=$ybias; $z-=$zbias;
    $d = sqrt($x*$x+$y*$y+$z*$z);
    printf ("%5.1f %5.1f %5.1f: %5.1f\n", $x, $y, $z, $d);
}
