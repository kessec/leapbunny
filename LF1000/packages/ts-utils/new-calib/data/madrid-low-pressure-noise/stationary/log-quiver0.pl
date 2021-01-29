#!/usr/bin/perl

use lib "../../../..";
use stats;

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
	$s{$try}->add($tnt) if $tnt_mode == 1;
	$x{$try}->add(($v[0]+$v[1])/2);
	$y{$try}->add(($v[2]+$v[3])/2);
	$p{$try}->add($v[6]);
    }

    die unless exists $s{"w05"};
    $lop = "w00";
    $lop++ while defined $s{$lop} && $s{$lop}->mean > 100;
    $h = 10;
    $h-- while !exists $s{sprintf("w%02d",$h)};
    $hip = sprintf("w%02d",$h);
    $dx = $x{$lop}->mean-$x{$hip}->mean;
    $dy = $y{$lop}->mean-$y{$hip}->mean;
    push @X, $x{$hip}->mean;
    push @Y, $y{$hip}->mean;
    push @U, $dx;
    push @V, $dy;
    push @SX, $x{$hip}->stdev;
    push @SY, $y{$hip}->stdev;
    undef %s;
    undef %x;
    undef %y;
    undef %p;
}

printf "x=[%s];\n", join (',', @X);
printf "y=[%s];\n", join (',', @Y);
printf "u=[%s];\n", join (',', @U);
printf "v=[%s];\n", join (',', @V);

printf ("quiver (x,y,u,v,'b-');\n");
printf ("hold on;\n");
printf ("r=0:.1:2*pi;\n");

for (0..$#X)
{
    # printf ("plot (%.1f*cos(r)+%.1f, %.1f*sin(r)+%.1f, 'r-');\n",
    # 	    $SX[$_], $X[$_], $SY[$_], $Y[$_]);
    printf ("plot (%.1f*cos(r)+%.1f, %.1f*sin(r)+%.1f, 'r.');\n",
	    2*$SX[$_], $X[$_], 2*$SY[$_], $Y[$_]);
}

printf ("axis([68-30,644+30,108-30,575+30]);\n");

