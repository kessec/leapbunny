#!/usr/bin/perl

use lib '..';
use histo;
$l=0;
$l=$l<length($_)?length($_):$l for (@ARGV);
$S=5;
@l=(0..9, 200..204);

printf (" "x$l);
printf ("%4d- ", $_*$S) for (@l);
printf ("\n");
printf (" "x$l);
printf (" %4d ", $_*$S+$S-1) for (@l);
printf ("\n");

for $a (@ARGV)
{
    $t = new histo;
    $sp = new stats;
    open (FIN, $a) || die;
    while (<FIN>)
    {
	$t->add($1) if /\s+T1=\s*(\d+)\s+T2=/;
    }
    printf ("%${l}s ", $a);
    @b = $t->buckets($S);
    for (@l)
    {
	printf ("%4d  ", $b[$_]);
    }
    printf ("\n");
    close FIN;
}
