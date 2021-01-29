#!/usr/bin/perl

use lib "../..";
use histo;

sub parse ($$)
{
    my $file = shift;
    my $thresh = shift;
    open (FIN, $file) || die;
    $N = 0;
    $hi[$_] = new histo for (0..5);
    $lo[$_] = new histo for (0..5);
    while (<FIN>)
    {
	$n = @v =
	    /x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+).* tnt1=\s*(\d+) tnt2=\s*(\d+)/;
	next unless $n == 6;
	if ($v[4] <= $thresh)
	{
	    $lo[$_]->add($v[$_]) for (0..5);
	    $Nl++;
	}
	else
	{
	    $hi[$_]->add($v[$_]) for (0..5);
	    $Nh++;
	}
    }
    close FIN;
    printf ("%d records accepted below threshold %d\n", $Nl, $thresh);
    printf ("%d records accepted above threshold %d\n", $Nh, $thresh);
}

@labels=("x1", "x2", "y1", "y2", "t1", "t2");
$T = 1000;  # TNT Threshold for pen-down
$W = 5;  # Bucket width
$PP = 0.95; # Percentage of down TNTs we want to preserve; skimming off a few upper TNTs

$a = $ARGV[0];
parse ($a, $T);

for (0..5)
{
    printf ("lo %s: %4d/%4d/%4d ", $labels[$_],
	    $lo[$_]->min, $lo[$_]->mean, $lo[$_]->max);
    for my $x (1..9)
    {
	printf (" %d(%4d)", $x, $lo[$_]->median_pct($x/10));
    }
    printf ("\n");
}
for (0..5)
{
    printf ("hi %s: %4d/%4d/%4d median=%4d\n", $labels[$_],
	    $hi[$_]->min, $hi[$_]->mean, $hi[$_]->max, $hi[$_]->median);
}
