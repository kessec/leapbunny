#!/usr/bin/perl

use lib "../../../..";
use stats;

my $f = shift;
my $F = $f;
$F = sprintf "zcat %s |", $f if $f =~ /\.gz/;
open (FIN, $F) || die;
while (<FIN>)
{
    $weight = $1, next if /weight=(.*)/;
    if (/tnt_mode=(\d+)/)
    {
	$try = "t$1-w$weight";
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
    $s{$try}->add($tnt);
    $x{$try}->add(($v[0]+$v[1])/2);
    $y{$try}->add(($v[2]+$v[3])/2);
    $p{$try}->add($v[6]);
}

for (sort keys %s)
{
    printf "%10s ", $_;
    printf ("N=%d TNT Mean=%4d Stdev=%7.2f ",
	    $s{$_}->N, $s{$_}->mean, $s{$_}->stdev);
    printf ("X Mean=%4d Stdev=%7.2f ",
	    $x{$_}->mean, $x{$_}->stdev);
    printf ("Y Mean=%4d Stdev=%7.2f",
	    $y{$_}->mean, $y{$_}->stdev);
    printf ("P Mean=%4d Stdev=%7.2f\n",
	    $p{$_}->mean, $p{$_}->stdev);
}
