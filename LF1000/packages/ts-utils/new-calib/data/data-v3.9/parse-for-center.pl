#!/usr/bin/perl

for $a (@ARGV)
{
    open FIN, "zcat $a |" || die;
    while (<FIN>)
    {
	if (/Mode  40g: P=\s*(\d+) T=\s*(\d+)/)
	{
	    $pc40=$1;
	    $tc40=$2;
	    next;
	}
	if (/Fit Corners/)
	{
	    ($t1, $t2, $t3, $t4, $tc) =/Fit Corners:\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)\s*Center:\s*(\d+)/;
	    next;
	}
    }
    printf "%30s: Avg(%2d %2d %2d %2d)=%2d center=%2d Mode at center=%2d Diff=%3d\n",
	$a, $t1, $t2, $t3, $t4, int(($t1+$t2+$t3+$t4+2)/4), $tc, $tc40,
	    $tc40-$tc;
}
