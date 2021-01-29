#!/usr/bin/perl

use lib '../..';
use stats;
for $a (@ARGV)
{
    $sp = new stats;
    $pc = '';
    open (FIN, $a) || die;
    while (<FIN>)
    {
	# if (/Entering calib_delay/../Entering get_four_points/)
	if (/Entering calib_delay/../Entering calib_pressure/)
	{
	    ($x1, $x2, $y1, $y2, $p1, $p2, $t1, $t2, $p, $d) =
		/X=\s*(\d+)\s+X2=\s*(\d+)\s+Y=\s*(\d+)\s+Y2=\s*(\d+)\s+P1=\s*(\d+)\s+P2=\s*(\d+)\s+T1=\s*(\d+)\s+T2=\s*(\d+)\s+P=\s*(\d+)\s+D=\s*(\d+)/;
	    next unless $t1;
	    if ($t1 < 500 && $t2 < 500)
	    {
		$sp->add($p);
	    }
	}
	if (/calib_pressure: P=\s*(\d+)/)
	{
	    $sp->add($1);
	}
	$pc=$1 if /Setting for pressure_curve:\s+(.*)/;
    }
    printf ("%20s %4d: p=%4d/%4d/%4d %6.2f pc=%s\n",
	    $a, $sp->N, $sp->min, $sp->mean, $sp->max, $sp->stdev, $pc);
    close FIN;
}
