#!/usr/bin/perl

use lib "../..";
use histo;

$T = 100;
$h1 = new histo;
$h2 = new histo;
$h3 = new histo;
while (<>)
{
    $n = @v =
	/x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+).* tnt1=\s*(\d+) tnt2=\s*(\d+)/;
    next unless $n == 6;
    next unless $v[4] <= $T;
    $h1->add($v[4]);
    $h2->add($v[5]);
    $h3->add(($v[4]+$v[5])/2);
}

$W = 2;
@b1 = $h1->buckets ($W);
@b2 = $h2->buckets ($W);
@b3 = $h3->buckets ($W);
for $b (0 .. $T/$W)
{
    printf ("%4d-%4d:", $b*$W, $b*$W+$W-1);
    printf ("%4d %4d %4d\n", $b1[$b], $b2[$b], $b3[$b]);
}
