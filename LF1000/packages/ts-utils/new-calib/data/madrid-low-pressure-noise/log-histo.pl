#!/usr/bin/perl

use lib "../..";
use histo;

sub parse ($$)
{
    my $file = shift;
    my $thresh = shift;
    open (FIN, $file) || die;
    $N = 0;
    $h[$_] = new histo for (0..5);
    while (<FIN>)
    {
	$n = @v =
	    /x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+).* tnt1=\s*(\d+) tnt2=\s*(\d+)/;
	next unless $n == 6;
	next unless $v[4] <= $thresh;
	$h[$_]->add($v[$_]) for (0..5);
	$N++;
    }
    close FIN;
    printf ("%d records accepted below threshold %d\n", $N, $thresh);
}

sub show ($)
{
    my $thresh = shift;
    my @n;
    printf ("TNT Threshold=%d\n", $thresh);
    printf ("            %s\n", join ("  ", @labels));
    for $b (0 .. 1024/$W) # ($T-1)/$W, 1000/$W .. 1023/$W)
    {
	printf ("%4d-%4d:", $b*$W, $b*$W+$W-1);
	for (0..5)
	{
	    printf (" %3s", $b[$_][$b] || "");
	    if ($_ >= 4 && $b < $T/$W)
	    {
		$n[$_] += $b[$_][$b];
		printf (" (%2d%%)", 100*$n[$_]/$N[$_]) if 1;
		$p[$_] = $b+1
		    if $n[$_]/$N[$_] < $PP && ($n[$_]+$b[$_][$b+1])/$N[$_] >= $PP;
	    }
	    if ($_ < 4)
	    {
		if ($b[$_][$b])
		{
		    printf (" (%2d%%)", 100*$b[$_][$b]/$N);
		}
		else
		{
		    printf ("      ");
		}
	    }
	}
	printf ("\n");
    }
}

@labels=("x1", "x2", "y1", "y2", "t1", "t2");
$a = $ARGV[0];
$T = 1000;  # TNT Threshold for pen-down
$W = 5;  # Bucket width
$PP = 0.95; # Percentage of down TNTs we want to preserve; skimming off a few upper TNTs

parse ($a, $T);
# Collect in buckets
@{$b[$_]} = $h[$_]->buckets($W) for (0..5);
# Get totals
for $c (4..5)
{
    $N[$c] = 0;
    $N[$c] += $b[$c][$_] for (0.. ($T-1)/$W);
}
# Show
show ($T); 
# Pick a point which below 95% of all TNT trues are below
printf ("p[%d] = %d\n", $_, $p[$_]*$W) for (4..5);
$T = $p[4]*$W+W-1;
# $T = 36;
$N1=$N;

# Redo with new threshold
parse ($a, $T);
# Collect in buckets
@{$b[$_]} = $h[$_]->buckets($W) for (0..5);
# Get totals
for $c (4..5)
{
    $N[$c] = 0;
    $N[$c] += $b[$c][$_] for (0.. ($T-1)/$W);
}
show ($T);
printf ("Actual acceptance ratio $N/$N1=%.2f\n", $N/$N1); 
