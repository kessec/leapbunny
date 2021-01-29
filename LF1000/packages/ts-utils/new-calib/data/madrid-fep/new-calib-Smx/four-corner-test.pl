#!/usr/bin/perl

for $a (@ARGV)
{
    open (FIN, $a) || die;
    $seen = 0;
    while (<FIN>)
    {
	$avg = $1, next if /Fit [CP].*[Cc]enter: (\d+)/;
	if (/At\s*\d+,\s*\d+: Real=\s*(\d+) Fit=\s*(\d+) Offset=\s*\d+/)
	{
	    next if $seen;
	    $cen=$1, $fcen=$2, $seen=1;
	}
	$cen = $1, next if /Mode  40g: P=\s*\d+ T=(\d+)/;
	# $mtd = $1, next if /Setting for max_tnt_down:\s+(\d+)/;
    }
    printf ("%45s: a=%2d c=%2d c/a=%5.2f\n", 
	    $a, $avg, $cen, $cen/$avg);
}
