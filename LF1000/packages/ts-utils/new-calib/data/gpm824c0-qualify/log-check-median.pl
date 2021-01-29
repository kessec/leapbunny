#!/usr/bin/perl

use lib "../../..";
use stats;
use median;

@label=qw(x1 x2 y1 y2 t1 t2 p);

$try = "wtf";
$s{$try}[$_] = new stats for (0..6);
$t{$try}[$_] = new stats for (0..6);
for $f (@ARGV)
{
    printf ("%s\n", $f);
    my $F = $f;
    $F = sprintf "zcat %s |", $f if $f =~ /\.gz/;
    open (FIN, $F) || die;
    while (<FIN>)
    {
	$weight = $1, next if /weight=(.*)/;
	if (/tnt_mode=(\d+)/)
	{
	    $tnt_mode = $1;
	    # $try = "t$tnt_mode-w$weight";
	    $try = sprintf ("w%02d", $weight);
	    if (!exists $s{$try})
	    {
		$s{$try}[$_] = new stats for (0..6);
		$t{$try}[$_] = new stats for (0..6);
		@v = ();
		@w = ();
		@x = ();
	    }
	    next
	}
	$n = @v =
	    /x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+).* tnt1=\s*(\d+) tnt2=\s*(\d+).*P=\s*(\d+)/;
	next unless $n == 7;
	$s{$try}[$_]->add($v[$_]) for (0..6);
	if (@x > 0)
	{
	    $t{$try}[$_]->add(median3($v[$_],$w[$_],$x[$_])) for (0..6);
	    # printf ("%4d:%4d ", $v[$_], median3($v[$_],$w[$_],$x[$_]))
	    #	for (0..6);  printf ("\n");
	}
	@x = @w;
	@w = @v;
    }

    for $k (sort keys %s)
    {
	for (0..6)
	{
	    printf ("%s:%2s N=%4d Mean=%7.2f Stdev=%7.2f ",
		    $k, $label[$_], $s{$k}[$_]->N,
		    $s{$k}[$_]->mean, $s{$k}[$_]->stdev);
	    printf ("Median3 N=%4d Mean=%7.2f Stdev=%7.2f\n",
		    $t{$k}[$_]->N,
		    $t{$k}[$_]->mean, $t{$k}[$_]->stdev);
	}
    }
    undef %s;
    undef %t;
}
