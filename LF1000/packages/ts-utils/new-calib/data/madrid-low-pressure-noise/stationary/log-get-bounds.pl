#!/usr/bin/perl

use lib "../../../..";
use stats;

my $f = shift;
my $F = $f;
$F = sprintf "zcat %s |", $f if $f =~ /\.gz/;
open (FIN, $F) || die;
$x=new stats;
$y=new stats;
while (<FIN>)
{
    $n = @v =
	/x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+).* tnt1=\s*(\d+) tnt2=\s*(\d+).*P=\s*(\d+)/;
    next unless $n == 7;
    $tnt = ($v[4]+$v[5])/2;
    $x->add(($v[0]+$v[1])/2) if $tnt < 30;
    $y->add(($v[2]+$v[3])/2) if $tnt < 30;
}

printf ("axis([%d,%d,%d,%d]);\n",
	$x->min, $x->max, $y->min, $y->max);
