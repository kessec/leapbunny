#!/usr/bin/perl

use lib "../../../..";
use stats;

for $f (@ARGV)
{
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

    if (exists $x{"w10"})
    {
	$dx = $x{"w02"}->mean-$x{"w10"}->mean;
	$dy = $y{"w02"}->mean-$y{"w10"}->mean;
    }
    else
    {
	$dx = $x{"w02"}->mean-$x{"w07"}->mean;
	$dy = $y{"w02"}->mean-$y{"w07"}->mean;
    }
    # Madrid m19 scaling
    $dx /= 1.1; $dy /= 1.6;
    # Explorer scaling
    # $dx /= 2.3; $dy /= 2.9;
    printf ("%-30s ", $f);
    printf ("x=%5.1f y=%5.1f d=%5.1f\n", $dx, $dy, sqrt($dx*$dx+$dy*$dy));
    undef %s;
    undef %x;
    undef %y;
    undef %p;
}
