#!/usr/bin/perl

# Raw data:
# X (0-480) is scaled by 0.75 (ADC: 200-840)
# Y (0-272) is scaled by 0.6  (ADC: 290-740)

sub median (@)
{
    my @a = @_;
    return (sort {$a <=> $b} @a)[(scalar @a)/2];
}

while (<>)
{
    next unless ($x, $y, $t, $p) = /([ 0-9]{4}) ([ 0-9]{4}) ([ 0-9]{4}) ([ 0-9]{4})/;
    if ($t > 30)
    {
	# pen up
	if ($len > 0)
	{
	    printf ("Gesture length=%d\n", $len);
	    $ax = $sx/$len;
	    $ay = $sy/$len;
	    $ap = $sp/$len;
	    $mx = median(@x);
	    $my = median(@y);
	    $mp = median(@p);
	    printf ("      %4d %4d %4d\n", $ax, $ay, $ap);
	    printf ("      %4d %4d %4d\n", $mx, $my, $mp);
	    for (0..$len-1)
	    {
		printf ("%4d: %4d %4d %4d\n", 
			$t[$_],
			$x[$_]-$mx,
			$y[$_]-$my,
			$p[$_]-200); # $mp);
	    }
	    $len = 0;
	    $sx = $sy = $sp = 0;
	    @x = ();
	    @y = ();
	    @p = ();
	    @t = ();
	}
    }
    else
    {
	$len++;
	$sx += $x;
	$sy += $y;
	$sp += $p;
	push @x, $x;
	push @y, $y;
	push @p, $p;
	push @t, $t;
	push @P, $p;
    }
}

@P = sort { $a <=> $b } @P;
printf "P=%d/%d/%d\n", $P[0], median(@P), $P[-1];
