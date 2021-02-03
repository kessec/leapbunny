#!/usr/bin/perl

my $a = $ARGV[0];
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

# chomp($TMP=`mktemp`);
chomp($a=`basename "$a"`);
$TMP="/tmp/$a.dat";
open (FOUT, ">$TMP") || die;
$t = 40;
$skip=0;
for (0..$#X)
{
    if ($T[$_] <= $t)
    {
	printf FOUT ("%d %d\n", $X[$_], $Y[$_]);
	$skip=0;
    }
    elsif ($T[$_] > 500)
    {
	if (!$skip)
	{
	    printf FOUT "\n";
	    $skip=1;
	}
    }
}
close FOUT;
if (0)
{
    # PNG output
    open (FOUT, "|gnuplot && rm $TMP") || die;
    printf FOUT "set terminal png\n";
    printf FOUT "set output '$a.png'\n";
    printf FOUT "set xrange [0:1023]\n";
    printf FOUT "set yrange [0:1023]\n";
    printf FOUT "plot '$TMP' with lines\n";
    close FOUT;
}
else
{
    # View only
    open (FOUT, "|gnuplot -persist && rm $TMP") || die;
    printf FOUT "plot '$TMP' with lines\n";
    close FOUT;
}
