#!/usr/bin/perl

#use lib '../../../new-calib';
#use histo;
use lib '../../..';
use stats;

use constant SVG=>0;

for $f (@ARGV)
{
    open (FIN, $f) || die;
    while (<FIN>)
    {
	# Mode  50g: P= 598 T=16
	$sn = $1 if /Serial Number='...............(.....)'/;
	if (/Mode\s*(\d+)g: P=\s*(\d+) T=\s*(\d+)/)
	{
	    $p{$sn}{$1}=new stats unless exists $p{$sn}{$1};
	    $p{$sn}{$1}->add($2);
	    $w{$1}++;
	}
	if (@x = /pressure_curve:  (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)/)
	{
	    $y=0;
	    for (@x)
	    {
		$c{$sn}{$y}=new stats unless exists $c{$sn}{$y};
		$c{$sn}{$y}->add($_);
		$y++;
	    }
	}
    }
}

@c=qw(red orange yellow green blue violet black brown pink);
$y1=20;
$y1=10;
@l = (sort { $a <=> $b } keys %w);
# @l = (reverse (0..8));
unless (SVG)
{
    printf ("%5s: ", '');
    for $w (@l)
    {
	printf ("%9s ", $w);
    }
    printf "\n";
}
for $sn (sort keys %p)
{
    printf ("%5s: ", $sn) unless SVG;
    $c = 0;
    for $w (@l)
    {
	unless (SVG)
	{
	    printf ("%4d-%4d ", $p{$sn}{$w}->min, $p{$sn}{$w}->max);
	    next;
	}
	$x1=2*($p{$sn}{$w}->min-400); $x2=2*($p{$sn}{$w}->max-400);
	# $x1=2*($c{$sn}{$w}->min-400); $x2=2*($c{$sn}{$w}->max-400);
	$y2 = $y1;
	printf ("<line x1=\"$x1\" y1=\"$y1\" x2=\"$x2\" y2=\"$y2\" style=\"stroke:$c[$c];stroke-width:8;opacity:0.5\"/>\n")
	    if $x1 < 1000 & $x2 < 1000;
	$y1+=2;
	$c++;
    }
    printf "\n" unless SVG;
    $y1 += 20;
}
