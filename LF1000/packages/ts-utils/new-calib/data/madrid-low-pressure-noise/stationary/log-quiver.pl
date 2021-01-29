#!/usr/bin/perl

use lib "../../../..";
use stats;

printf ("hold on;\n");
printf ("r=0:.1:2*pi;\n");

# $c{w10} = "[0,0,0]";
# $c{w09} = "[.1,.1,.1]";
# $c{w08} = "[0.8, 0.0, 0.0]";
# $c{w07} = "[0.8, 0.6, 0.0]";
# $c{w06} = "[0.6, 0.8, 0.0]";
# $c{w05} = "[0.0, 0.8, 0.0]";
# $c{w04} = "[0.0, 0.8, 0.6]";
# $c{w03} = "[0.0, 0.6, 0.8]";
# $c{w02} = "[0.0, 0.0, 0.8]";
# $c{w01} = "[0.6, 0.0, 0.8]";
# $c{w00} = "[0.8, 0.0, 0.8]";

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

    $last_lop = undef;
    for $lop (reverse qw (w01 w02 w03 w04 w05 w06 w07 w08 w09 w10))
    {
	next if !exists $x{$lop};
	next if $s{$lop}->mean > 100;
	if ($last_lop)
	{
	    printf ("plot ([%f,%f],[%f,%f],'+-b', 'linewidth', 1);\n",
		    $x{$last_lop}->mean, $x{$lop}->mean,
		    $y{$last_lop}->mean, $y{$lop}->mean);
	    # local $_= $lop;
	    # s/w0?//;
	    # printf ("text (%f, %f, '$_');\n",
	    #	    $x{$lop}->mean+5, $y{$lop}->mean);
	}
	else
	{
	    printf ("plot (%.1f*cos(r)+%.1f, %.1f*sin(r)+%.1f, 'r-');\n",
		    2*$x{$lop}->stdev, 
		    $x{$lop}->mean,
		    2*$y{$lop}->stdev, 
		    $y{$lop}->mean);
	}
	$last_lop = $lop;
    }
    undef %s;
    undef %x;
    undef %y;
    undef %p;
}

$x0 = 68-30;
$y0 = 108-30;
$x1 = 644+30;
$y1 = 575+30;
printf ("axis([$x0-1,$x1+1, $y0-1, $y1+1]);\n");
printf ("plot ([$x0,$x1], [$y0,$y0], 'k-');\n");
printf ("plot ([$x0,$x1], [$y1,$y1], 'k-');\n");
printf ("plot ([$x0,$x0], [$y0,$y1], 'k-');\n");
printf ("plot ([$x1,$x1], [$y0,$y1], 'k-');\n");
printf ("set (gca(), 'visible', 'off');\n");
