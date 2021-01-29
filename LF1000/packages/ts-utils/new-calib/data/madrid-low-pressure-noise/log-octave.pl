#!/usr/bin/perl

while (<>)
{
    # x1=1023 x2=1023 y1= 306 y2= 334 p1=1023 p2=   0 tnt1=1014 tnt2=1016 S=U X=246 Y=199 P=1854 LP= 10    0,   0
    $n = ($x1, $x2, $y1, $y2, $t1, $t2) = 
	/x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+).* tnt1=\s*(\d+) tnt2=\s*(\d+)/;
    next unless $n == 6;
    push @X, ($x1+$x2)/2;
    push @Y, ($y1+$y2)/2;
    push @T, ($t1+$t2)/2;
}

@ts=(100, 40, 30, 25, 20, 18, 16, 14);
$dx = 0; $dy = 100;
$ox = 0;  $oy = 0;
for $t (@ts)
{
    $sep="";
    printf ("x%d=[", $t);
    for (0..$#X)
    {
	next unless $T[$_] <= $t;
	printf ("%s%d,%d", $sep, $X[$_]+$ox, $Y[$_]+$oy);
	$sep=";";
    }
    $ox += $dx; $oy += $dy;
    printf ("];\n");
}
print <<EOF
hold on
plot (x100(:,1), x100(:,2), 'y+-')
plot (x40(:,1), x40(:,2), 'r+-')
plot (x30(:,1), x30(:,2), 'g+-')
plot (x25(:,1), x25(:,2), 'b+-')
plot (x20(:,1), x20(:,2), 'c+-')
plot (x18(:,1), x18(:,2), 'm+-')
plot (x16(:,1), x16(:,2), 'k+-')
plot (x14(:,1), x14(:,2), 'y+-')
# axis([50,650,750,900])
EOF
