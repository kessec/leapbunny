#!/usr/bin/perl

#use lib '../../../new-calib';
#use histo;
use lib '../../..';
use stats;

@w = qw(20 25 30 40 50 75 100 150 200);

sub fit_line ($$)
{
    my ($rx, $ry) = @_;
    my @x = @{$rx};
    my @y = @{$ry};
    #Compute M
    my ($sx, $sy, $sxx, $sxy, $s1);
    for (0..$#x)
    {
	next unless $y[$_] < 99;
	$s1 += 1;
	$sx += $x[$_];
	$sy += $y[$_];
	$sxx += $x[$_]*$x[$_];
	$sxy += $x[$_]*$y[$_];
    }
    # Thanks www.pdc.kth.se/publications/talks/mpi/pt2pt-i-lab/least-squares.c
    # who credits Dora Abdullah (Fortran version, 11/96)
    my $dm = ($sx*$sy - $s1*$sxy) / ($sx*$sx - $s1*$sxx);
    my $db = ($sy - $dm*$sx) / $s1;

    # Verify our fit...
    # printf ("mx=%9.4f b=%9.4f\n", $dm, $db);
    printf ("Fit      ");
    my $e = 0;
    for (0..$#x)
    {
	# printf ("Fit x=%9.5f y=%3.0f -> %3.0f\n",
	#	$x[$_], $y[$_], (.5+$dm*$x[$_]+$db));
	printf ("%2d ", $dm*$x[$_]+$db);
	next unless $y[$_] < 99;
	my $d = $dm*$x[$_]+$db - $y[$_];
	$e += $d * $d;
    }
    $p20g = $dm*$x[0]+$db;
    printf ("20gPick=%2d Err=%9.5f\n", , $p20g, $e);
    $ee += $e;
}


for $f (@ARGV)
{
    open (FIN, $f) || die;
    undef %t;
    while (<FIN>)
    {
	# Mode  50g: P= 598 T=16
	$sn = $1, $sn{$sn}++ if /Serial Number='...............(.....)'/;
	if (/Mode\s*(\d+)g: P=\s*(\d+) T=\s*(\d+)/)
	{
	    $t=$3, $w=$1 unless $t || $3>100;
	    $t{$1} = $3 < 100 ? $3 : 99;
	    # $t20=$3 if $1 == 20 && $3 < 100;
	    # $t25=$3 if $1 == 25 && $3 < 100;
	    # $t30=$3 if $1 == 30 && $3 < 100;
	    # $t40=$3 if $1 == 40 && $3 < 100;
	}
	if (@x = /pressure_curve:  (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)/)
	{
	    @c = @x;
	}
	if (/Setting for max_tnt_down: (\d+)/)
	{
	    $s=$1;
	}
    }
    # printf ("sn=%s s=%2d t=%2d t30=%2d w=%3d\n", $sn, $s, $t, $t30, $w);
    # printf ("sn=%s s=%2d t=%2d w=%3d delta=%2d\n", $sn, $s, $t, $w, $t-$s);
    if (0)
    {
	printf ("sn=%s 3.5pick=%2d t20=%2d t25=%2d t30=%2d t40=%2d d20=%3d d25=%3d d30=%3d d40=%3d tlow=%2d wlow=%3d delta=%2d\n",
		#	    $sn, $s, $t20, $t25, $t30, $t40, $t20-$s, $t25-$s, $t30-$s, $t40-$s, $t, $w, $t-$s);
		$sn, $s, $c[8], $c[7], $c[6], $c[5], $c[8]-$s, $c[7]-$s, $c[6]-$s, $c[5]-$s, $t, $w, $t-$s);
    }
    if (1)
    {
	printf ("sn=%s %2d %2d %2d %2d %2d %2d %2d %2d %2d",
		$sn, $t{20}, $t{25}, $t{30}, $t{40}, $t{50}, $t{75},
		$t{100}, $t{150}, $t{200});
	printf (" 3.5pick=%2d\n", $s);
    }
    if (1)
    {
	my @x;
	my @y;
	for (0..$#w)
	{
	    # next unless $t{$w[$_]} < 100-10;
	    push @x, 1/$w[$_];
	    # push @x, $w[$_];
	    push @y, $t{$w[$_]};
	}
	fit_line (\@x, \@y);
    }
    printf ("ee=%9.5f\n", $ee);
}
