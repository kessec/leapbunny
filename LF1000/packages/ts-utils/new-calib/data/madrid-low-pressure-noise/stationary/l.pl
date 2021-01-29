#!/usr/bin/perl

use lib "../../../..";
use stats;

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
		$s{$try}=new stats;
		$x{$try}=new stats;
		$y{$try}=new stats;
		$p{$try}=new stats;
	    }
	    next
	}
	$n = @v =
	    /x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+).* tnt1=\s*(\d+) tnt2=\s*(\d+).*P=\s*(\d+)/;
	next unless $n == 7;
	$tnt = ($v[4]+$v[5])/2;
	$s{$try}->add($tnt) if $tnt_mode == 1;
	$x{$try}->add(($v[0]+$v[1])/2);
	$y{$try}->add(($v[2]+$v[3])/2);
	$p{$try}->add($v[6]);
    }

    for (sort keys %s)
    {
	next if /w00/;
	if (0)
	{
	    # Human Readable
	    printf "%10s ", $_;
	    $t0 = $s{"w10"}->mean;
	    printf ("TNT N=%d Mean=%5.1f Stdev=%7.2f ",
		    $s{$_}->N, $s{$_}->mean-$t0, $s{$_}->stdev);
	    $x0 = $x{"w10"}->mean;
	    printf ("X N=%d Mean=%5.1f Stdev=%7.2f ",
		    $x{$_}->N, $x{$_}->mean-$x0, $x{$_}->stdev);
	    $y0 = $y{"w10"}->mean;
	    printf ("Y Mean=%5.1f Stdev=%7.2f",
		    $y{$_}->mean-$y0, $y{$_}->stdev);
	    $p0 = $p{"w10"}->mean;
	    printf ("P Mean=%6.1f Stdev=%7.2f\n",
		    $p{$_}->mean-$p0, $p{$_}->stdev);
	}
	else
	{
	    # Latex
	    ($n) = /w(\d+)/;
	    printf "%d & ", $n;
	    $t0 = $s{"w07"}->mean;
	    printf ("%.0f & %.1f & %.2f & ",
		    $s{$_}->mean, $s{$_}->mean-$t0, $s{$_}->stdev);
	    $x0 = $x{"w07"}->mean;
	    printf ("%.0f & %.1f & %.2f & ",
		    $x{$_}->mean, $x{$_}->mean-$x0, $x{$_}->stdev);
	    $y0 = $y{"w07"}->mean;
	    printf ("%.0f & %.1f & %.2f & ",
		    $y{$_}->mean, $y{$_}->mean-$y0, $y{$_}->stdev);
	    $p0 = $p{"w07"}->mean;
	    printf ("%.0f & %.1f & %.2f \\\\ \\hline\n",
		    $p{$_}->mean, $p{$_}->mean-$p0, $p{$_}->stdev);
	}
    }
    undef %s;
    undef %x;
    undef %y;
    undef %p;
}
