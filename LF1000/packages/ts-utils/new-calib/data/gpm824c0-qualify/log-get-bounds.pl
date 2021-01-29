#!/usr/bin/perl

use lib "../../..";
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
	/X=\s*(\d+)\s+X2=\s*(\d+)\s+Y=\s*(\d+)\s+Y2=\s*(\d+).*\s+T1=\s*(\d+)\s+T2=\s*(\d+)\s+P=\s*(\d+)/;
    next unless $n == 7;
    $tnt = ($v[4]+$v[5])/2;
    $x->add(($v[0]+$v[1])/2) if $tnt < 30;
    $y->add(($v[2]+$v[3])/2) if $tnt < 30;
}

printf ("axis([%d,%d,%d,%d]); # %d x %d\n",
	$x->min, $x->max, $y->min, $y->max,
	$x->max-$x->min, $y->max-$y->min);
