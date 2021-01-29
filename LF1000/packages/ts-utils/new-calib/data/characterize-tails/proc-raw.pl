#!/usr/bin/perl

while (<>)
{
    $_ = substr ($_, 34);
    next unless ($x1, $x2, $y1, $y2, $p1, $p2, $t1, $t2, $p) = /\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)/;
    next if $t1 > 900 || $t2 > 900;
    $x = ($x1+$x2+1)/2;
    $y = ($y1+$y2+1)/2;
    $t = ($t1+$t2+1)/2;
    $x[$x]++;
    $y[$y]++;
    $t[$t]++;
}

@n = qw(x y t);
for $r ( \@x, \@y, \@t )
{
    $n = shift @n;
    printf ("--------\n%s\n", $n);
    @r=@{$r};
    for $i (0..1023)
    {
	printf ("%4d: %4d\n", $i, $r[$i]) if $r[$i];
    }
}
