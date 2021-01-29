#!/usr/bin/perl

use lib "../../../..";
use stats;

$LATEX = 1;
$OCTAVE = 0;
$HUMAN = 0;

for $f (@ARGV)
{
    printf ("%s\n", $f);
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
		$s{$try}=new stats;
		$x{$try}=new stats;
		$y{$try}=new stats;
		$p{$try}=new stats;
	    }
	    next
	}
	$n = @v =
	    /x1=\s*(\d+) x2=\s*(\d+) y1=\s*(\d+) y2=\s*(\d+).* tnt1=\s*(\d+) tnt2=\s*(\d+).*P=\s*(\d+)/;
	next unless $n == 7;
	$tnt = ($v[4]+$v[5])/2;
	$s{$try}->add($tnt) if $tnt_mode == 1; # 0 for exp, 1 for madrid
	$x{$try}->add(($v[0]+$v[1])/2);
	$y{$try}->add(($v[2]+$v[3])/2);
	$p{$try}->add($v[6]);
    }

    for (sort keys %s)
    {
	# next if /w00/;
	if ($HUMAN)
	{
	    # Human Readable
	    printf "%10s ", $_;
	    $t0 = 0; # $s{"w10"}->mean;
	    printf ("TNT N=%d Mean=%5.1f Stdev=%7.2f ",
		    $s{$_}->N, $s{$_}->mean-$t0, $s{$_}->stdev);
	    $x0 = 0; # $x{"w10"}->mean;
	    printf ("X N=%d Mean=%5.1f Stdev=%7.2f ",
		    $x{$_}->N, $x{$_}->mean-$x0, $x{$_}->stdev);
	    $y0 = 0; # $y{"w10"}->mean;
	    printf ("Y Mean=%5.1f Stdev=%7.2f",
		    $y{$_}->mean-$y0, $y{$_}->stdev);
	    $p0 = 0; # $p{"w10"}->mean;
	    printf ("P Mean=%6.1f Stdev=%7.2f\n",
		    $p{$_}->mean-$p0, $p{$_}->stdev);
	}
	if (0)
	{
	    # Latex Simple
	    ($n) = /w(\d+)/;
	    $l{$_} = sprintf "%2d & ", $n unless exists $l{$w};
	    $w = "w10";
	    $w = "w07" unless exists $s{$w};
	    $t0 = $s{$w}->mean;
	    $l{$_} .= sprintf ("%4.0f & ", $s{$_}->mean);
	    $x0 = $x{$w}->mean;
	    $l{$_} .= sprintf ("%4.1f & ", $x{$_}->mean-$x0);
	    $y0 = $y{$w}->mean;
	    $l{$_} .= sprintf ("%4.1f & ", $y{$_}->mean-$y0);
	}
	if ($LATEX)
	{
	    # Latex Radius and TNT
	    ($n) = /w(\d+)/;
	    $l{$_} = sprintf "%2d & ", $n unless exists $l{$w};
	    $w = "w10";
	    $w = "w07" unless exists $s{$w};
	    $t0 = $s{$w}->mean;
	    $l{$_} .= sprintf ("%4.0f & ", $s{$_}->mean);
	    $dx = $x{$w}->mean-$x{$_}->mean;
	    $dy = $y{$w}->mean-$y{$_}->mean;
	    # Madrid m19 scaling
	    $dx /= 1.1; $dy /= 1.6;
	    # Explorer scaling
	    # $dx /= 2.3; $dy /= 2.9;
	    $d = sqrt ($dx*$dx+$dy*$dy);
	    $l{$_} .= sprintf ("%4.1f & ", $d);
	}
	if ($OCTAVE)
	{
	    # octave
	    push @p, $s{$_}->mean;
	}
    }
    if ($OCTAVE)
    {
	# octave
	printf ("%s;\n", join (',', @p));
	undef @p;
    }
    undef %s;
    undef %x;
    undef %y;
    undef %p;
}

if ($LATEX)
{
    # latex
    for ($i="w00"; $i ne "w11"; $i++)
    {
	$_ = $l{$i};
	s/& $//;
	printf ("%s \\\\ \\hline\n", $_);
    }
}

