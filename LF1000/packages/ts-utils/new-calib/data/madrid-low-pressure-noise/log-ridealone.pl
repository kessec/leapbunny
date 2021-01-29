#!/usr/bin/perl

use lib '../..';
use histo;

@labels=("x1", "x2", "y1", "y2", "t1", "t2", "d40", "d30", "d25", "d20", "d18", "d16", "d14");
@labels=("x", "y", "t1", "t2", "d40", "d30", "d25", "d20", "d18", "d16", "d14");
@ts=(40, 30, 25, 20, 18, 16, 14);
@state = ('u')x scalar @ts;
$h[$_] = new histo for (0..$#ts);

printf ("%4s ", $labels[$_]) for (0..$#labels); printf ("\n");
while (<>)
{
    $n = @v =
	/x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+).* tnt1=\s*(\d+) tnt2=\s*(\d+)/;
    next unless $n == 6;
    $x = ($v[0]+$v[1])/2;
    $y = ($v[2]+$v[3])/2;
    # printf ("%4d ", $v[$_]) for (0..5);
    printf ("%4d %4d %4d %4d ", $x, $y, $v[4], $v[5]);
    for (0..$#ts)
    {
	$pen = $v[4]+$v[5] <= 2*$ts[$_] ? 'd' :
	       $v[4]+$v[5] > 2*500 ? 'u' : 'b';
	$up = $dn = 0;
	if ($state[$_] eq 'u')
	{
	    $state[$_] = 'd', $dn=1 if $pen eq 'd';
	    $state[$_] = 'v' if $pen eq 'b';
	}
	elsif ($state[$_] eq 'v')
	{
	    $state[$_] = 'u' if $pen eq 'u';
	    $state[$_] = 'd', $dn=1 if $pen eq 'd';
	}
	elsif ($state[$_] eq '^')
	{
	    $state[$_] = 'd' if $pen eq 'd';
	    $state[$_] = 'u', $up=1 if $pen eq 'u';
	}
	elsif ($state[$_] eq 'd')
	{
	    $state[$_] = 'u', $up=1 if $pen eq 'u';
	    $state[$_] = '^' if $pen eq 'b';
	}

	$add = '';
	if ($state[$_] eq 'd')
	{
	    if (!$dn)
	    {
		$dx = $x-$lx[$_];
		$dy = $y-$ly[$_];
		$l = sqrt ($dx*$dx + $dy*$dy);
		$arclen[$_] += $l;
		$h[$_]->add ($l);
		$add = sprintf ("+%2d", $l);
	    }
	    $lx[$_] = $x;
	    $ly[$_] = $y;
	}
	printf ("%s%s%-4s  ", $state[$_],
	       $dn || $up ? '!' : ' ', $add);
    }
    # printf ("%4s ", $v[4]+$v[5] <= 2*$ts[$_] ? $ts[$_] : " ") for (0..$#ts);
    printf ("\n");
}

for (0..$#ts)
{
    printf ("Thresh=%d arclen=%6d N=%4d mean=%2d", 
	    $ts[$_], $arclen[$_], $h[$_]->N, $h[$_]->mean);
    for $x (1..10)
    {
	printf (" %3d:%d", $h[$_]->median_pct($x/10), $x);
    }
    printf ("\n");
}

