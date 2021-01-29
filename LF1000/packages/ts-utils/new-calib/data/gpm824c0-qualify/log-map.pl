#!/usr/bin/perl

use lib "../../..";
use stats;

chomp($TMP=`mktemp`);
open (FOUT, ">$TMP") || die;
for $f (@ARGV)
{
    my $F = $f;
    $F = sprintf "zcat %s |", $f if $f =~ /\.gz/;
    open (FIN, $F) || die;
    while (<FIN>)
    {
	if (/point=(.*)/)
	{
	    $point = $1;
	    $s[$_] = new stats for (0..6);
	    next;
	}
	if (/done/)
	{
	    push @X, $s[0]->mean;
	    push @Y, $s[2]->mean;
	    push @T, $s[4]->mean;
	    push @P, $s[6]->mean;
	    printf FOUT "%d %d\n", $s[0]->mean, $s[2]->mean;
	    undef @s;
	    next;
	}
	$n = @v =
	    /x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+).* tnt1=\s*(\d+) tnt2=\s*(\d+).*P=\s*(\d+)/;
	next unless $n == 7;
	$s[$_]->add($v[$_]) for (0..6);
    }
}

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
    # open (FOUT, "|gnuplot -persist") || die;
    # printf FOUT "plot '$TMP' with lines\n";
    printf FOUT "plot '$TMP'\n";
    close FOUT;
}

open (FOUT, ">tntmap.m") || die;
printf FOUT ("x=[%s];\n", join(',', @X));
printf FOUT ("y=[%s];\n", join(',', @Y));
printf FOUT ("t=[%s];\n", join(',', @T));
printf FOUT ("p=[%s];\n", join(',', @P));
