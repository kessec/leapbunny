#!/usr/bin/perl

use lib '../..';
# use stats;
$l=0;
$l=$l<length($_)?length($_):$l for (@ARGV);

for $a (@ARGV)
{
    # $sp = new stats;
    open (FIN, $a) || die;
    while (<FIN>)
    {
	$max_tnt_down = $1 if /Setting for max_tnt_down:\s*(\d+)/;
	$min_tnt_up = $1 if /Setting for min_tnt_up:\s*(\d+)/;
	$delay_in_us = $1 if /Setting for delay_in_us:\s*(\d+)/;
	$y_delay_in_us = $1 if /Setting for y_delay_in_us:\s*(\d+)/;
	$tnt_delay_in_us = $1 if /Setting for tnt_delay_in_us:\s*(\d+)/;
    }
    printf ("%${l}s max_tnt=%3d min_tnt=%4d delay=%2d y_delay=%2d tnt_delay=%2d\n", 

	    $a, $max_tnt_down, $min_tnt_up, $delay_in_us, $y_delay_in_us,
	    $tnt_delay_in_us);
    close FIN;
}
