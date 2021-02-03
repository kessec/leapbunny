#!/usr/bin/perl

use lib "../../..";
use stats;

@labels=qw(x1 x2 y1 y2 t1 t2 pr);

$TNT_VS_MAD = 0;
$TNTMM_VS_CAP = 0;
$TNT_VS_CAP = 1;

for $f (@ARGV)
{
    my $F = $f;
    $F = sprintf "zcat %s |", $f if $f =~ /\.gz/;
    open (FIN, $F) || die;
    while (<FIN>)
    {
	$weight = $1, next if /weight=(.*)/;
	if (/tnt_mode=(\d+)/)
	{
	    $tnt_mode = $1;
	    # $try = "t$tnt_mode-w$weight";
	    $try = sprintf ("w%02d", $weight);
	    if (!exists $s{$try})
	    {
		$s{$try}[$_] = new stats for (0..6);
	    }
	    next
	}
	$n = @v =
	    /x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+).* tnt1=\s*(\d+) tnt2=\s*(\d+).*P=\s*(\d+)/;
	next unless $n == 7;
	$s{$try}[$_]->add($v[$_]) for (0,1,2,3,6);
	if ($tnt_mode == 1) # 0 for exp, 1 for madrid
	{
	    $s{$try}[$_]->add($v[$_]) for (4,5);
	}
    }

    if ($TNT_VS_MAD)
    {
	printf ("%s\n", $f);
	for $try (sort keys %s)
	{
	    next if $s{$try}[4]->mean >=40;
	    printf ("TNT=%4d |x1-x2|=%3d\n",
		    $s{$try}[4]->mean,
		    $s{$try}[0]->mean - $s{$try}[1]->mean);
	}
    }
    if ($TNTMM_VS_CAP || $TNT_VS_CAP)
    {
	$name = $1 if $f =~ /(.*)-tnt/;
	for $try (sort keys %s)
	{
	    $t{$name}{$try} = new stats if (!exists ($t{$name}{$try}));
	    $t{$name}{$try}->add($s{$try}[4]->mean);
	}
    }
    undef %s;
}

if ($TNTMM_VS_CAP)
{
    for $name (sort keys %t)
    {
	printf ("%20s: ", $name);
	for $try (sort keys %{$t{$name}})
	{
	    printf ("%4d-%4d ", 
		    $t{$name}{$try}->min,
		    $t{$name}{$try}->max);
	}
	printf ("\n");
    }
}
if ($TNT_VS_CAP)
{
    for $name (sort keys %t)
    {
	printf ("%20s: ", $name);
	for $try (sort keys %{$t{$name}})
	{
	    # printf ("%4d ", $t{$name}{$try}->max-$t{$name}{"w10"}->max);
	    # printf ("%4d ", $t{$name}{$try}->max);
	    printf ("%4d ", $t{$name}{$try}->mean);
	    # printf ("%4d ", $t{$name}{$try}->max-$t{$name}{$try}->min);
	}
	printf ("\n");
    }
}
