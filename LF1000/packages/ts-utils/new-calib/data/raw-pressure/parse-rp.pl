#!/usr/bin/perl

use lib '../../../';
use stats;

for $a (@ARGV)
{
    $sp = new stats;
    $sp1 = new stats;
    open (FIN, $a) || die;
    while (<FIN>)
    {
	s/^...................................//;
	my @A = split;
	next unless @A>8;
	$sp->add($A[8]);
	$x = ($A[0]+$A[1])/2;
	$y = ($A[2]+$A[3])/2;
	$z1 = $A[4];
	$z2 = $A[5];
	$p1 = $x * ($z2/$z1-1);
	$sp1->add($p1);
    }
    printf ("a=%s p=%.0f/%.0f/%.0f SD=%6.3f %.3f/%.3f/%.3f SD=%6.3f\n",
	    $a, $sp->min, $sp->mean, $sp->max, $sp->stdev,
	    $sp1->min, $sp1->mean, $sp1->max, $sp1->stdev);
}

