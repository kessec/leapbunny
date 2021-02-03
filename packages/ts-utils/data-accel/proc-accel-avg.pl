#!/usr/bin/perl

use lib "..";
use stats;

$SX=new stats;
$SY=new stats;
$SZ=new stats;
$l=0;
$l=$l<length($_)?length($_):$l for (@ARGV);
for $a (@ARGV)
{
    $sx=new stats;
    $sy=new stats;
    $sz=new stats;
    open FIN, $a || die;
    while (<FIN>)
    {
	$n = ($x, $y, $z) = /D\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)/;
	next unless $n;
	next if $x == -1000 || $y == -1000 || $z == -1000;
	$sx->add($x);
	$sy->add($y);
	$sz->add($z);
	# $d = sqrt($x*$x+$y*$y+$z*$z);
	# printf ("%3d %3d %3d : %5.1f\n", $x, $y, $z, $d);
    }
    # printf ("x: %.0f/%.2f/%.0f %.2f\n", $sx->min, $sx->mean, $sx->max, $sx->stdev);
    # printf ("y: %.0f/%.2f/%.0f %.2f\n", $sy->min, $sy->mean, $sy->max, $sy->stdev);
    # printf ("z: %.0f/%.2f/%.0f %.2f\n", $sz->min, $sz->mean, $sz->max, $sz->stdev);
    printf ("%${l}s %6.2f (%5.2f) %6.2f (%5.2f) %6.2f (%5.2f)\n", $a,
	    $sx->mean, $sx->stdev,
	    $sy->mean, $sy->stdev,
	    $sz->mean, $sz->stdev);
    $SX->add($sx->mean);
    $SY->add($sy->mean);
    $SZ->add($sz->mean);
}
printf ("%${l}s %6.2f (%5.2f) %6.2f (%5.2f) %6.2f (%5.2f)\n", "Total",
	$SX->mean, $SX->stdev,
	$SY->mean, $SY->stdev,
	$SZ->mean, $SZ->stdev);
