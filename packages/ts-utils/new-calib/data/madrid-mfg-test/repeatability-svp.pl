#!/usr/bin/perl

#use lib '../../../new-calib';
#use histo;
use lib '../../..';
use stats;

use constant MAXP => 850;
use constant MINP => 380;

print <<EOF;
<?xml version="1.0" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg viewBox="0 0 1200 700" id="cont" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" xml:space="preserve">
<text x="200" y="50" style="font-family:Verdana;font-size:24">Repeatability of Pressure Calibration</text>
<g>
EOF

for $f (@ARGV)
{
    open (FIN, $f) || die;
    while (<FIN>)
    {
	# Mode  50g: P= 598 T=16
	$sn = $1, $sn{$sn}++ if /Serial Number='...............(.....)'/;
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

@c=qw(grey brown red orange yellow green blue purple black);
@c=qw(#ff9f30 #df0000	#00df00 #0000df #ffdf00 #dfdfdf #60bfff #df00df #010101);
$y1=90;
@l = (sort { $a <=> $b } keys %w);
# @l = (reverse (0..8));
for $sn (sort keys %p)
{
    $x = 1;
    $y = $y1+8;
    $n = $sn{$sn};
    printf ("<text x=\"$x\" y=\"$y\" style=\"font-family:Verdana;font-size:15\">$sn ($n)</text>\n");
    $c = 0;
    for $w (@l)
    {
	$x1=2*($p{$sn}{$w}->min()- MINP); $x2=2*($p{$sn}{$w}->max()- MINP);
	$x1--, $x2++ if ($x2 == $x1);
	# $x1=2*($c{$sn}{$w}->min()-MINP); $x2=2*($c{$sn}{$w}->max()-MINP);
	$y2 = $y1;
	$x2 = MAXP if $x2 > MAXP;
	printf ("<line x1=\"$x1\" y1=\"$y1\" x2=\"$x2\" y2=\"$y2\" style=\"stroke:$c[$c];stroke-width:8;opacity:0.5\"/>\n")
	    if $x1 < MAXP;
	$y1+=2;
	$c++;
    }
    $y1 += 20;
}

# Color legend at bottom
@w = qw(20g 25g 30g 40g 50g 75g 100g 150g 200g);
$x = 700; 
$c = 0;
for (@w)
{
    $x1 = $x;
    $x2 = $x+60;
    $y = $y1+60;
    printf ("<line x1=\"$x1\" y1=\"$y\" x2=\"$x2\" y2=\"$y\" style=\"stroke:$c[$c];stroke-width:20;opacity:0.5\"/>\n");
    $y = $y+6;
    $x1 = $x+10;
    printf ("<text x=\"$x1\" y=\"$y\" style=\"font-family:Verdana;font-size:15\">$_</text>\n");
    $x -= 70;
    $c++;
}

# Ticks
for ($x=500; $x<=MAXP; $x+=100)
{
    $x1=2*($x- MINP);
    $y = $y1+20;
    printf ("<line x1=\"$x1\" y1=\"80\" x2=\"$x1\" y2=\"$y\" style=\"stroke:black;stroke-width:1;opacity:0.75\"/>\n");
    $x1 -= 12;
    $y = $y+20;
    printf ("<text x=\"$x1\" y=\"$y\" style=\"font-family:Verdana;font-size:15\">$x</text>\n");
}
for ($x=450; $x<=MAXP; $x+=100)
{
    $x1=2*($x- MINP);
    $y = $y1+20;
    printf ("<line x1=\"$x1\" y1=\"80\" x2=\"$x1\" y2=\"$y\" style=\"stroke:black;stroke-width:1;opacity:0.25\"/>\n");
}
printf ("<text x=\"0\" y=\"70\" style=\"font-family:Verdana;font-size:15\">Serial# (N)</text>\n");
print <<EOF;
</g></svg>
EOF
