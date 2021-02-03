#!/usr/bin/perl

use lib '../../../';
use stats;

for $a (@ARGV)
{
    $sp = new stats;
    $sx = new stats;
    $sy = new stats;
    $sz1 = new stats;
    $sz2= new stats;
    open (FIN, $a) || die;
    while (<FIN>)
    {
	s/^...................................//;
	my @A = split;
	next unless @A>8;
	$sp->add($A[8]);
	$sx->add(($A[0]+$A[1])/2);
	$sy->add(($A[2]+$A[3])/2);
	$sz2->add($A[4]); ## appear reversed
	$sz1->add($A[5]);
    }
    $x=$sx->mean;
    $y=$sy->mean;
    $z1=$sz1->mean;
    $z2=$sz2->mean;
    printf ("a=%s", $a);
    printf ("x=%.0f y=%.0f z1=%.0f z2=%.0f p=%.0f ",
	    $x, $y, $z1, $z2, $sp->mean);
    printf ("x/z1-1=%8.3f y/1K-1=%8.3f (z2/z1-1)x=%8.3f\n",
    	    $x/$z1-1, $y/1024-1,
    	    ($z2/$z1-1)*$x/1024);

    # printf ("z2/z1-1=%8.3f ", $z2/$z1-1);
    # printf ("x*=%8.3f ", $x*($z2/$z1-1));
    # printf ("y*=%8.3f ", $y*($z2/$z1-1));
    # printf ("1-x*=%8.3f ", (1024-$x)*($z2/$z1-1));
    # printf ("1-y*=%8.3f ", (1024-$y)*($z2/$z1-1));
    # printf ("\n");
}

# R=Rx(X/Z1-1)+Ry(Y/1024-1)
# R=Rx(Z2/Z1-1)(X/1024)
