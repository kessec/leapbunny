#!/usr/bin/perl

use lib '../../../new-calib';
use histo;

$state = '';
while (<>)
{
# 7:             p1 p2 t1 t2 Pr     524  524  517  517  800  370   17   14  608
    $_ = substr ($_, 63);
    ($t1, $t2, $p) = /\s+(\d+)\s+(\d+)\s+(\d+)/;
    # printf ("%4d %4d %4d\n", $t1, $t2, $p);
    if ($t1 > 0 && $t1 < 100)
    {
	if (!$state)
	{
	    $state = 1;
	    $s = new histo;
	}
	$s->add($p);
    }
    if ($t1 >= 100)
    {
	if ($state)
	{
	    $state = 0;
	    if ($s->N >= 10)
	    {
		printf ("%3d: %4d/%4d/%4d Median=%4d",
			$s->N, $s->min, $s->mean, $s->max, $s->median);
		# printf (" %4d", $s->median_pct($_/10)) for (0..10);
		printf (" mode=%4d,%3d", $s->mode, $s->modeN);
		printf ("\n");
	    }
	    undef $s;
	}
    }
}
