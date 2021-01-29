#!/usr/bin/perl

$PAGES=1;
$X = 1350;
$Y = 700;

$FONTSIZE = 12;
$WIDTH = 32;
$END_TIME = 22;

@colors = qw(white blue orange pink purple black lightblue green yellow lightgreen green pink red darkred
	           lightblue blue yellow orange lightgrey grey grey);

@spans192 = (
	   [HW,               1, 3, 0, 0.2],
	   [NOR,              2, 3, 0.2, 1.29],
	   ["First Screen",   3, 0, 0.75, 2.30+4.49],
	   [KDcmp,            4, 3, 1.29, 2.230],
	   [Kernel,           5, 3, 2.230,     2.30+1.69],
	   [InitScripts,      6, 3, 2.30+1.69, 2.30+6.44],
	   [Networking,       7, 3, 2.30+13.66, 2.30+15.79],
	   [Versions,         8, 3, 2.30+18.7,  2.30+19.75],
	   ["Legal Screen",   9, 0, 2.30+4.49, 2.30+8.00],
	   [Chime,            10, 1, 2.30+4.49, 2.30+7.11],
	   [AppManagerLoad,   11, 4, 2.30+3.65, 2.30+3.65+3],
	   [AppManagerInit,   12, 4, 2.30+3.65+3, 2.30+9.21],
	   [AppManager,       13, 4, 2.30+9.21, $END_TIME],
	   ["SplashVideo Screen",14, 0, 2.30+8.00, 2.30+13.16],
	   ["SplashVideo Sound", 15, 1, 2.30+8.00, 2.30+13.16],
	   [FlashPlayerInit,  16, 5, 2.30+9.20, 2.30+9.66],
	   [FlashPlayer,      17, 5, 2.30+9.66, $END_TIME],
	   ["UI Screen",      18, 0, 2.30+11.58,$END_TIME],
	   ["UI Sound",       19, 1, 2.30+11.58,$END_TIME],
	   ["UI SWF",         20, 6, 2.30+9.66,$END_TIME],
);
@milestones192 = (
	   [POW,   1, 0],
	   [Backlight,    2, 0.56],
	   ["BulkReady", 1, 2.30+6.44],
	   ["UI Ready", 1, 2.30+11.58],
	   ["UI Visible", 1, 2.30+13.19],
);

@spans195 = (
	   [HW,               1, 3, 0,   0.2],
	   [NOR,              2, 3, 0.2, 0.680],
	   [Kernel,           5, 3, 0.680, 1.4920],
	[ "AppManager", 13, 4, 9.2220, 22.0000 ],
	[ "AppManager Init", 12, 4, 5.8920, 9.2220 ],
	[ "AppManager Load", 11, 4, 3.1720, 5.8920 ],
	[ "Chime", 10, 1, 3.7720, 6.1620 ],
	[ "First Screen", 3, 0, 0.7500, 3.1720 ],
	[ "FlashPlayer", 17, 5, 9.2220, 22.0000 ],
	[ "FlashPlayer Init", 16, 5, 8.8220, 9.2220 ],
	[ "InitScripts", 6, 3, 1.4920, 6.1620 ],
	[ "Legal Screen", 9, 0, 3.1720, 7.3520 ],
	[ "Networking", 7, 3, 13.1720, 15.2920 ],
	[ "Splash Video Screen", 14, 0, 7.3520, 12.7220 ],
	[ "Splash Video Sound", 15, 1, 7.3520, 12.7220 ],
	[ "UI SWF", 20, 6, 9.2220, 22.0000 ],
	[ "UI Screen", 18, 0, 10.3020, 22.0000 ],
	[ "UI Sound", 19, 1, 10.3020, 22.0000 ],
	[ "Versions", 8, 3, 18.1920, 19.2420 ],
);
@milestones195 = (
	[ "Backlight", 2, 0.221 ],
	[ "BulkReady", 1, 2.2220 ],
	[ "ON", 1, 0.0000 ],
	[ "UI Ready", 1, 10.3020 ],
	[ "UI Visble", 1, 12.7220 ],
);

@spans196_201 = (
	   [HW,               1, 3, 0,   0.2],
	   [NOR,              2, 3, 0.2, 0.680],
	   [Kernel,           5, 3, 0.680, 1.500],
	[ "AppManager", 13, 4, 11.3900, 22.0000 ],
	[ "AppManager Init", 12, 4, 8.7800, 11.3900 ],
	[ "AppManager Load", 11, 4, 4.2000, 8.7800 ],
	[ "Chime", 10, 1, 5.3500, 8.1900 ],
	[ "First Screen", 3, 0, 0.7500, 5.1600 ],
	[ "FlashPlayer", 17, 5, 11.3900, 22.0000 ],
	[ "FlashPlayer Init", 16, 5, 11.0000, 11.3900 ],
	[ "InitScripts", 6, 3, 1.5000, 8.1900 ],
	[ "Legal Screen", 9, 0, 5.1600, 8.8800 ],
	[ "Networking", 7, 3, 13.6000, 17.3600 ],
	[ "Splash Video Screen", 14, 0, 8.8800, 13.8800 ],
	[ "Splash Video Sound", 15, 1, 8.8800, 13.8800 ],
	[ "UI SWF", 20, 6, 11.3900, 22.0000 ],
	[ "UI Screen", 18, 0, 12.6200, 22.0000 ],
	[ "UI Sound", 19, 1, 12.6200, 22.0000 ],
	[ "Versions", 8, 3, 18.6100, 19.7100 ],
);
@milestones196_201 = (
	[ "Backlight", 2, 0.2210 ],
	[ "BulkReady", 1, 2.5500 ],
	[ "ON", 1, 0.0000 ],
	[ "UI Ready", 1, 12.6200 ],
	[ "UI Visble", 1, 13.8800 ],
);

@spans196_203 = (
	   [HW,               1, 3, 0,   0.2],
	   [NOR,              2, 3, 0.2, 0.680],
	   [Kernel,           5, 3, 0.680, 1.500],
	[ "AppManager", 13, 4, 11.9400, 22.0000 ],
	[ "AppManager Init", 12, 4, 9.3300, 11.9400 ],
	[ "AppManager Load", 11, 4, 4.5900, 9.3300 ],
	[ "Chime", 10, 1, 5.9900, 8.9300 ],
	[ "First Screen", 3, 0, 0.7500, 5.8000 ],
	[ "FlashPlayer", 17, 5, 11.9400, 22.0000 ],
	[ "FlashPlayer Init", 16, 5, 11.5300, 11.9400 ],
	[ "InitScripts", 6, 3, 1.5000, 8.9300 ],
	[ "Legal Screen", 9, 0, 5.8000, 9.3800 ],
	[ "Networking", 7, 3, 14.3400, 18.3300 ],
	[ "Splash Video Screen", 14, 0, 9.3800, 14.3800 ],
	[ "Splash Video Sound", 15, 1, 9.3800, 14.3800 ],
	[ "UI SWF", 20, 6, 11.9400, 22.0000 ],
	[ "UI Screen", 18, 0, 13.1700, 22.0000 ],
	[ "UI Sound", 19, 1, 13.1700, 22.0000 ],
	[ "Versions", 8, 3, 19.3700, 20.4800 ],
);
@milestones196_203 = (
	[ "Backlight", 2, 0.2210 ],
	[ "BulkReady", 1, 6.5700 ],
	[ "ON", 1, 0.0000 ],
	[ "UI Ready", 1, 13.1700 ],
	[ "UI Visble", 1, 14.3800 ],
);

sub posn($)
{
    my $t = shift;
    $X0 + ($X-2*$X0) * $t / $END_TIME;
}

sub chart ($$$)
{
    my ($heading, $rspans, $rmilestones) = @_;

    for (@{$rspans})
    {
	($label, $ci, $layer, $start, $end) = @{$_};
	# print "$label, $layer, $start, $end\n";
	$color = $colors[$ci];

	$x1 = posn($start);
	$x2 = posn($end);

	$y1 = $Y0 + $WIDTH*$layer;
	$y2 = $y1;

	print ("<line x1=\"$x1\" y1=\"$y1\" x2=\"$x2\" y2=\"$y2\" style=\"stroke:$color;stroke-width:$WIDTH;opacity:0.5\"/>\n");

	$x3 = ($x1+$x2)/2;
	$y3 = ($y1+$y2)/2;
	print ("<text x=\"$x3\" y=\"$y3\" style=\"font-family:Verdana;font-size:$FONTSIZE;text-anchor:middle\">$label</text>\n");
    }
    for (@{$rmilestones})
    {
	($label, $layer, $point) = @{$_};

	$x1 = posn($point);
	$x2 = $x1;

	$y1 = $Y0 - $WIDTH/2;
	$y2 = $Y0 - $WIDTH/2 - $WIDTH*$layer;

	print ("<line x1=\"$x1\" y1=\"$y1\" x2=\"$x2\" y2=\"$y2\" style=\"stroke:black;stroke-width:2;opacity:0.5\"/>\n");

	$x3 = ($x1+$x2)/2;
	$y3 = $y2 - $FONTSIZE;
	print ("<text x=\"$x3\" y=\"$y3\" style=\"font-family:Verdana;font-size:$FONTSIZE;text-anchor:middle\">$label</text>\n");
    }
    # Timeline
    for (0..$END_TIME)
    {
	$x1 = posn($_);
	$x2 = $x1;

	$y1 = $Y0 - 1*$WIDTH/2;
	$y2 = $Y0 - 2*$WIDTH/3;
	print ("<line x1=\"$x1\" y1=\"$y1\" x2=\"$x2\" y2=\"$y2\" style=\"stroke:gray;stroke-width:2;opacity:0.5\"/>\n");

	$x3 = ($x1+$x2)/2;
	$y3 = $y2 - $FONTSIZE;
	print ("<text x=\"$x3\" y=\"$y3\" style=\"stroke:gray;font-family:Verdana;font-size:$FONTSIZE;text-anchor:middle\">$_</text>\n");
    }
    $x3 = $X/2;
    $y3 = $Y0 - $WIDTH*4 + 40;
    print ("<text x=\"$x3\" y=\"$y3\" style=\"font-family:Verdana;font-size:40;text-anchor:middle\">$heading</text>\n");
}

print <<EOF;
<?xml version="1.0" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg width="$X" height="$Y" version="1.1" xmlns="http://www.w3.org/2000/svg">
<rect width="$X" height="$Y" style="fill:white;stroke-width:5; stroke:black"/>
EOF

$ci = 0;
$X0 = 20;
$Y0 = $WIDTH*4;
#chart ("Madrid Firmware Boot 1.9.2", \@spans192, \@milestones192);
chart ("1.9.6-2127 Build on 201-EP", \@spans196_201, \@milestones196_201);

$ci = 0;
$X0 = 20;
$Y0 = $Y/2+$WIDTH*4;
# chart ("1.9.5-2070 MadridOptimize BRANCH", \@spans195, \@milestones195);
# chart ("1.9.6-2127 Build on 201-EP", \@spans196_201, \@milestones196_201);
chart ("1.9.6-2127 Build on 203-EP", \@spans196_203, \@milestones196_203);

print <<EOF;
</svg>
EOF
