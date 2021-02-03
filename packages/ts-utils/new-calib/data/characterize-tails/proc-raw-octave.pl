#!/usr/bin/perl

printf ("x=[");
while (<>)
{
    $_ = substr ($_, 34);
    next unless ($x1, $x2, $y1, $y2, $p1, $p2, $t1, $t2, $p) = /\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)/;
    next if $t1 > 900 || $t2 > 900;
    printf "%d,%d,%d;", ($x1+$x2+1)/2, ($y1+$y2+1)/2, ($t1+$t2+1)/2; # , $p-500;
}
printf "]\n";
