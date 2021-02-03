#!/usr/bin/perl

use histo;

$S=10;
$ht = new histo;
for $a (@ARGV)
{
    open (FIN, $a) || die;
    printf ("%s ---------\n", $a);
    # $ht = new histo;
    while (<FIN>)
    {
	# last if /Entering get_four_points/;
	$n = ($t1, $t2, $d) = /T1=\s*(\d+)\s+T2=\s*(\d+).*D=\s*(\d+)/;
	next unless $n;
	$ht->add($t1);
	# printf ("%4d %4d %2d\n", $t1, $t2, $d);
	$k{$d}++;
	push @{$T1{$d}}, $t1;
	push @{$T2{$d}}, $t2;
    }

    if (0)
    {
	$m = $0;
	for $d (sort keys %k)
	{
	    $m = $k{$d} if $m < $k{$d};
	}
	for $i (0..$m-1)
	{
	    printf ("%4d: ", $i);
	    for $d (sort { $a <=> $b } keys %k)
	    {
		printf ("%4d %4d %2d | ", $T1{$d}[$i], $T2{$d}[$i], $d);
	    }
	    printf ("\n");
	}
    }
}
{
    @b = $ht->buckets($S);
    if (0)
    {
	for (0..$#b)
	{
	    printf ("%4d-%4d: %d\n",
		    $_*$S, $_*$S+$S-1, $b[$_]) if $b[$_];
	}
    }
    push @A, $a;
    push @B, [@b];
}


for $i (0..$#A)
{
    printf ("%s%s%s\n", " "x10, " "x($i*5),$A[$i]);
}
for $b (0.. 1024/$S)
{
    printf ("%4d-%4d:", $b*$S, $b*$S+$S-1);
    for $i (0..$#A)
    {
	printf (" %4d", $B[$i][$b]) if $B[$i][$b];
	printf (" %4s", "") unless $B[$i][$b];
    }
    printf ("\n");
}
