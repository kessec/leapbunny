#!/usr/bin/perl

use lib '../..';
# use stats;
$l=0;
$l=$l<length($_)?length($_):$l for (@ARGV);

for $a (@ARGV)
{
    # $sp = new stats;
    $tn = 0;
    open (FIN, $a) || die;
    while (<FIN>)
    {
	$tn++ if /target state/i;
    }
    printf ("%${l}s too noisy=%2d\n", $a, $tn-4);
    close FIN;
}
