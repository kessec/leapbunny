#!/usr/bin/perl

$N = 33;
$N2 = $N/2;
@T = (0) x $N;
@X = (0) x $N;
@Y = (0) x $N;
while (<>)
{
    for (split /;/)
    {
	next unless /(\d+),(\d+),(\d+),(\d+)/;
	shift @X;
	push @X, $1;
	shift @Y;
	push @Y, $2;
	shift @T;
	push @T, $3;
	printf ("%d,%d,%d,9;\\\n", (sort(@X))[$N2], (sort(@Y))[$N2], (sort(@T))[$N2]);
    }
}

