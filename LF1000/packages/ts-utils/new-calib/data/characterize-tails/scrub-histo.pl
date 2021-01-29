#!/usr/bin/perl

$B=5;
$TT=50;
$BD=1;
$ND=10;
while (<>)
{
    for (split /;/)
    {
	next unless ($x, $y, $t, $p) = /(\d+),(\d+),(\d+),(\d+)/;
	$dx=$x-$lastx;
	$dy=$y-$lasty;
	$lastx=$x;
	$lasty=$y;
	# next if $t>100;
	$d=sqrt($dx*$dx+$dy*$dy);
	$D=int($d/$BD);
	$D=$ND if $D>$ND;
	$T=$t;
	$T=$TT if $t>$TT;
	$T/=$B;
	$H[$D][$T]++;
	$N[$D]++;
	$n++;
    }
}

if (0)
{
    # Human
    for $t (10/$B .. $TT/$B)
    {
	printf ("TNT %2d ", $t*$B);
	for $d (0..$ND)
	{
	    $n = $N[$d];
	    printf ("%2d%% ", 100*$H[$d][$t]/$n);
	}
	printf ("\n");
    }
}
else
{
    # Octave
    printf ("x=[%s];\n", join(',', (10/$B .. $TT/$B)));
    printf ("y=[%s];\n", join(',', (0..$ND)));
    printf ("s=[");
    $semi='';
    for $t (10/$B .. $TT/$B)
    {
	printf ("%s", $semi);
	$semi=';';
	$comma='';
	for $d (0..$ND)
	{
	    printf ("%s%d", $comma, 100*$H[$d][$t]/$n);
	    $comma=',';
	}
    }
    printf ("];\n");
}
