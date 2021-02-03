#!/usr/bin/perl

use lib "..";
use stats;

$sx=new stats;
$sy=new stats;
$sz=new stats;
$l=0;
$l=$l<length($_)?length($_):$l for (@ARGV);
for $a (@ARGV)
{
    open FIN, $a || die;
    while (<FIN>)
    {
	$n = ($x, $y, $z) = /\s*([-.0-9]+)\s+([-.0-9]+)\s+([-.0-9]+)/;
	next unless $n==3;

	$sx->add($x);
	$sy->add($y);
	$sz->add($z);
	$d = sqrt($x*$x+$y*$y+$z*$z);
	printf ("d=%7.2f ", $d);
	print;
    }
    close FIN;
}
# Expect mean sz to be -31/2
$ez = -31/2;
printf ("%6.2f (%5.2f) %6.2f (%5.2f) %6.2f (%5.2f) N=%.0f\n",
	$sx->mean, $sx->stdev,
	$sy->mean, $sy->stdev,
	$sz->mean-$ez, $sz->stdev, $sx->N);
for $a (@ARGV)
{
    open FIN, $a || die;
    while (<FIN>)
    {
	$n = ($x, $y, $z) = /\s*([-.0-9]+)\s+([-.0-9]+)\s+([-.0-9]+)/;
	next unless $n==3;
	$x-=$sx->mean;
	$y-=$sy->mean;
	$z-=$sz->mean-$ez;
	$d = sqrt($x*$x+$y*$y+$z*$z);
	printf ("d=%6.2f x=%6.2f y=%6.2f z=%6.2f\n", $d, $x, $y, $z);
    }
    close FIN;
}
