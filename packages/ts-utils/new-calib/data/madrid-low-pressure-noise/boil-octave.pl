#!/usr/bin/perl

printf ("x=[");
$sep="";
while (<>)
{
    # x1=1023 x2=1023 y1= 306 y2= 334 p1=1023 p2=   0 tnt1=1014 tnt2=1016 S=U X=246 Y=199 P=1854 LP= 10    0,   0
    $n = ($x1, $x2, $y1, $y2, $t1, $t2) = 
	/x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+) t1=\s*(\d+) t2=\s*(\d+)/;
    next unless $n == 6;
    printf ("%s%s", $sep, join (',',(($x1+$x2)/2-350, ($y1+$y2)/2-340, ($t1+$t2)/2)));
    $sep = ';';
}
printf ("];\n");
