#!/usr/bin/perl

#use lib '../../../new-calib';
#use histo;
use lib '../../..';
use stats;

for $f (@ARGV)
{
    open (FIN, $f) || die;
    while (<FIN>)
    {
	# Mode  50g: P= 598 T=16
	$sn = $1 if /Serial Number='.................(...)'/;
	if (/Mode\s*(\d+)g: P=\s*(\d+) T=\s*(\d+)/)
	{
	    $sn{$f}=$sn;
	    $p{$f}{$1} = $2;
	    $t{$f}{$1} = $3;
	    $w{$1}++;
	    $h{$sn}{$1}=new histo unless exists $h{$sn}{$1};
	    $h{$sn}{$1}->add($2);
	}
	if (@x = /pressure_curve:  (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)/)
	{
	    $c{$f} = [@x];
	}

    }
}

for $sn (sort keys %h)
{
    printf ("%s: ", $sn);
    for $w (sort { $a <=> $b } keys %w)
    {
	printf ("%4d(%5.0f) ", $h{$sn}{$w}->mean, $h{$sn}{$w}->stdev);
    }
    printf ("\n");
}
printf ("\n");

for $f (sort keys %p)
{
    printf ("%4s ", $sn{$f});
}
printf ("\n\n");
for $w (sort { $a <=> $b } keys %w)
{
    for $f (sort keys %p)
    {
	printf ("%4s ", $p{$f}{$w});
    }
    printf ("\n");
}
for $w (sort { $a <=> $b } keys %w)
{
    for $f (sort keys %p)
    {
	printf ("%4s ", $t{$f}{$w});
    }
    printf ("\n");
}
for $w (reverse (0..8))
{
    for $f (sort keys %p)
    {
	printf ("%4s ", $c{$f}[$w]);
    }
    printf ("\n");
}
