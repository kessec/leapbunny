#!/usr/bin/perl

use lib "../../../..";
use stats;

my $f = shift;
my $F = $f;
$F = sprintf "zcat %s |", $f if $f =~ /\.gz/;
open (FIN, $F) || die;
while (<FIN>)
{
    $corner = $1, next if /corner=(.*)/;
    if (/tnt_mode=(\d+)/)
    {
	$try = "t$1-c$corner";
	if (!exists $s{$try})
	{
	    $s{$try}=new stats;
	}
	next
    }
    $n = @v =
	/x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+).* tnt1=\s*(\d+) tnt2=\s*(\d+)/;
    next unless $n == 6;
    $tnt = ($v[4]+$v[5])/2;
    $s{$try}->add($tnt);
}

for (sort keys %s)
{
    printf "%s ", $_;
    printf ("TNT N=%d Mean=%4d Stdev=%7.2f\n",
	    $s{$_}->N, $s{$_}->mean, $s{$_}->stdev);
}
