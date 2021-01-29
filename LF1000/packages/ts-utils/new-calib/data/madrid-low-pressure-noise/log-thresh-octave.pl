#!/usr/bin/perl

$T = 1000;
$a = shift @ARGV;
for $T (1000, 40, 30, 20, 18, 16, 14)
{
    $N = 0;
    printf ("x%d=[", $T);
    open (FIN, $a) || die;
    while (<FIN>)
    {
	$n = @v =
	    /x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+).* tnt1=\s*(\d+) tnt2=\s*(\d+)/;
	next unless $n == 6;
	if ($v[4] <= $T)
	{
	    printf ("%s%d,%d", $N ? ";" : "",
		    ($v[0]+$v[1])/2, ($v[2]+$v[3])/2);
	    $N++;
	}
    }
    close FIN;
    printf ("];\n");
}
