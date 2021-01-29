#!/usr/bin/perl

use lib "../../../..";
use stats;

for $a (@ARGV)
{
    open (FIN, $a) || die;
    $s = new stats;
    $n25 = 0;
    while (<FIN>)
    {
	for (split)
	{
	    $s->add($_);
	    $n25++ if $_ > 25;
	}
    }
    printf ("%25s #>25=%2d %7.3f/%7.3f/%7.3f stdev=%7.3f\n", 
	    $a, $n25, $s->min, $s->mean, $s->max, $s->stdev);
}
