#!/usr/bin/perl

#use lib '../../../new-calib';
#use histo;
use lib '../../..';
use stats;

use constant MAXP => 850;
use constant MINP => 450;

print <<EOF;
<?xml version="1.0" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg viewBox="0 0 1200 700" id="cont" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" xml:space="preserve">
<text x="200" y="50" style="font-family:Verdana;font-size:24">Pressure Observations</text>
<g>
EOF

@w=qw(20 25 30 40 50 75 100 150 200);

for $f (@ARGV)
{
    open (FIN, $f) || die;
    while (<FIN>)
    {
	$sn = $1, $sn{$sn}++ if /Serial Number='...............(.....)'/;
	# Mode  50g: P= 598 T=16
	if (/Mode\s*(\d+)g: P=\s*(\d+) T=\s*(\d+)/)
	{
	    # $p{$1}=$2;
	}
	if (@x = /pressure_curve:  (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)/)
	{
	    @x = reverse @x;
	    $l = $x[3];
	    $p{$w[$_]} = $x[$_] for (0..$#w);
	}
    }
    $d{$l}={%p};
    undef %p;
}

@c=qw(#ff9f30 #df0000 #00df00 #0000df #ffdf00 #dfdfdf #60bfff #df00df #010101);
@w=qw(20 25 30 40 50 75 100 150 200);
$c{$w[$_]} = $c[$_] for (0..$#c);
$x0 = 0;
$y0 = 80;
$y1 = $y0;
for $l (sort { $a <=> $b } keys %d)
# for $l (keys %d)
{
    for $w (@w)
    {
	if ($d{$l}{$w})
	{
	    $y2 = $y1;
	    # $x1 = ($d{$l}{$w}- MINP)*2;
	    $x1 = 500000 / $d{$l}{$w} - 500;
	    $x2 = $x1+20;
	    printf ("<line x1=\"$x1\" y1=\"$y1\" x2=\"$x2\" y2=\"$y2\" style=\"stroke:$c{$w};stroke-width:8;opacity:0.5\"/>\n");
	}
    }
    $y1 += 10;
}
print <<EOF;
</g></svg>
EOF
