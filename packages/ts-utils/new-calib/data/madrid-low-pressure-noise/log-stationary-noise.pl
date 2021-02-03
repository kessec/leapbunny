#!/usr/bin/perl

use lib "../../..";
use stats;

@ts=(40, 30, 25, 20, 18, 16, 14);
for (0..$#ts)
{
    $xs[$_] = new stats;
    $ys[$_] = new stats;
}
while (<>)
{
    $n = @v =
	/x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+).* tnt1=\s*(\d+) tnt2=\s*(\d+)/;
    next unless $n == 6;
    $x = ($v[0]+$v[1])/2;
    $y = ($v[2]+$v[3])/2;
    $tnt = ($v[4]+$v[5])/2;
    for (0..$#ts)
    {
	if ($tnt <= $ts[$_])
	{
	    $xs[$_]->add($x);
	    $ys[$_]->add($y);
	}
    }
}

for (0..$#ts)
{
    printf ("%4d ", $ts[$_]);
    printf ("X %4d/%4d/%4d Span=%3d %7.2f ", 
	    $xs[$_]->min, $xs[$_]->mean, $xs[$_]->max,
	    $xs[$_]->max-$xs[$_]->min,
	    $xs[$_]->stdev);
    printf ("Y %4d/%4d/%4d Spen=%3d %7.2f N=%d\n", 
	    $ys[$_]->min, $ys[$_]->mean, $ys[$_]->max,
	    $xs[$_]->max-$xs[$_]->min,
	    $ys[$_]->stdev,
	    $ys[$_]->N);
}
