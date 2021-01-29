#!/usr/bin/perl

# Raw data:
# X (0-480) is scaled by 0.75 (ADC: 200-840)
# Y (0-272) is scaled by 0.6  (ADC: 290-740)

sub median (@)
{
    my @a = @_;
    return (sort {$a <=> $b} @a)[(scalar @a)/2];
}

sub dot(\@\@)
{
    my ($ra,$rb) = @_;
    my @a = @{$ra};
    my @b = @{$rb};
    my $s = 0;
    for (0..$#a)
    {
	$s += $a[$_] * $b[$_];
    }
    $s;
}

sub magnitude2 (@)
{
    my @a = @_;
    my $s = 0;
    $s += $_*$_ for (@a);
    $s;
}

sub magnitude (@)
{
    sqrt(magnitude2(@_));
}

while (<>)
{
    next unless ($x, $y, $t, $p) = /([ 0-9]{4}) ([ 0-9]{4}) ([ 0-9]{4}) ([ 0-9]{4})/;
    if ($t > 30)
    {
	# pen up
	if ($len > 0)
	{
	    printf ("Length=%d\n", $len);
	    $odx = $x[-1]-$x[0];
	    $ody = $y[-1]-$y[0];
	    @o = ($odx, $ody);
	    $om = magnitude(@o);
	    printf ("Overall dx,y=%3d,%3d Mag=%3d\n", @o, $om);
	    for (1..$len-1)
	    {
		@d = ($x[$_]-$x[$_-1], $y[$_]-$y[$_-1]);
		$mxy = magnitude2 (@d);
		$mxyp = magnitude2 (@d, $p[$_]-$p[$_-1]);
		if ($mxyp<20 && $_ < $len-1)
		{
		    $n++;
		}
		else
		{
		    if ($mxy < 10 || $om < 10)
		    {
			$d = 0;
		    }
		    else
		    {
			my $q = magnitude(@o)*magnitude(@d);
			$d = dot (@o, @d) / $q;
		    }
		    printf ("%4d %4d %4d: mxy=%6d mxyp=%6d (%2d) d=%3.1f\n",
			    $x[$_]-$x[$_-1],
			    $y[$_]-$y[$_-1],
			    $p[$_]-$p[$_-1],
			    $mxy, $mxyp, $n, $d);
		    $n = 0;
		}
	    }
	    $len = 0;
	    @x = ();
	    @y = ();
	    @p = ();
	    @t = ();
	}
    }
    else
    {
	$len++;
	push @x, $x;
	push @y, $y;
	push @p, $p;
	push @t, $t;
    }
}
