#!/usr/bin/perl

while (<>)
{
    # x1=1023 x2=1023 y1= 306 y2= 334 p1=1023 p2=   0 tnt1=1014 tnt2=1016 S=U X=246 Y=199 P=1854 LP= 10    0,   0
    $n = ($x1, $x2, $y1, $y2, $p1, $p1, $t1, $t2) = 
	/x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+) p1=\s*(\d+) p2=\s*(\d+) tnt1=\s*(\d+) tnt2=\s*(\d+)/;
    next unless $n == 8;
    printf ("x1=%4d x2=%4d y1=%4d y2=%4d t1=%4d t2=%4d\n", $x1, $x2, $y1, $y2, $t1, $t2);
}
