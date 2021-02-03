#!/usr/bin/perl
while (<>)
{
    $n = ($x1, $x2, $y1, $y2, $t1, $t2) = 
	/x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+) t1=\s*(\d+) t2=\s*(\d+)/;
    next unless $n == 6;
    $x = ($x1+$x2)/2;
    $y = ($y1+$y2)/2;
    $t = ($t1+$t2)/2;
    push @X, $x;
    push @Y, $y;
    push @T, $t;
}
@S = sort {$a<=>$b} @T;
$N = scalar @S;
$min=$S[0];
$max=$S[-1];
$median=$S[$#S/2];
$mean = 0; $mean += $_ for (@S); $mean /= $N;
printf ("S: min=%d / median=%d / mean=%d / max=%d\n", $min, $median, $mean, $max);
for (5, 10, 25, 50, 75, 90, 95)
{
    printf ("%d%%=%d\n", $_, $S[$N*$_/100]);
}
# Chop off top 10% of values...
$chop = $S[$N*.90];
$chop = 1000 if $chop > 1000;
@P = ();
for (@S)
{
    push @P, $_ if $_ <  $chop;
}
@S = sort {$a<=>$b} @P;
$N = scalar @S;
$min=$S[0];
$max=$S[-1];
$median=$S[$#S/2];
$mean = 0; $mean += $_ for (@S); $mean /= $N;
printf ("P: min=%d / median=%d / mean=%d / max=%d\n", $min, $median, $mean, $max);
for (5, 10, 25, 50, 75, 90, 95)
{
    printf ("%d%%=%d\n", $_, $S[$N*$_/100]);
}
