#!/usr/bin/perl

$PAGES=1;
$PAGE=1;

$XPP = 1350;
$Y = 500;
$CLIP = 25; # Time before which we display.. FIXME
$LITE = 0; # Skip similar time stamps by 1/$LITE

$FONTSIZE = 12;
$WIDTH = 40;
$LABEL_WIDTH = 4*$FONTSIZE;
$TAIL_LEN = $FONTSIZE;

$X = $XPP * $PAGES;
$X0 = ($PAGE-1)*$XPP;
$X1 = $X0 + $XPP;

@colors = qw(white blue orange green red black pink);
$type = 1;
$color = $colors[$type];
$width = 1;
$onepager = 0;
undef $max; $max=21.48;
undef $min;

sub onepager
{
    return 1 if /"Power on/;
    return 1 if /"End NOR/;
    return 1 if /"End Kernel/;
    return 1 if /"End Uncompress/;
    return 1 if /"jiffies=/;
    return 1 if /"Linux version 2.6.31-leapfrog/;
    return 1 if /"nand_get_flash_type/;
    return 1 if /"lf1000-rtc lf1000-rtc:/;
    return 1 if /"Waiting for root device/;
    return 1 if /"VFS: Mounted/;
    return 1 if /"End /;
    return 1 if /"Bgn /;
    return 1 if /"AppManager:.*splash/;
    return 1 if /"AppManager:.*Load/;
    return 1 if /"AppManager: UI/;
    return 0;
}

while (@ARGV)
{
    $arg = shift @ARGV;
    if ($arg =~ /^-h/)
    {
	print <<EOF;
$0 [-h] [-p PAGES] [-o TIME_OFFSET] [-s] [-t TYPE] infile.csv ... > outfile.svg
EOF
	exit 0;
    }
    if ($arg =~ /^-p/)
    {
	$_ = shift @ARGV;
	if (/(\d+)\/(\d+)/)
	{
	    $PAGE=$1;
	    $PAGES=$2;
	    $X = $XPP * $PAGES;
	    $X0 = ($PAGE-1)*$XPP;
	    $X1 = $X0 + $XPP;
	}
	next;
    }
    if ($arg =~ /^-t/)
    {
	$type = shift @ARGV;
	$color = $colors[$type] || die;
	$width = $type;
	next;
    }
    if ($arg =~ /^-l/)
    {
	$LITE = shift @ARGV;
	next;
    }
    $shut = 1, next if ($arg =~ /^-s/);
    $onepager = 1, next if ($arg =~ /^-1/);
    $offset = shift @ARGV, next if ($arg =~ /^-o/);

    # Process a file
    $last_time=-1;
    open (FIN, $arg) || die "Can't open $arg";
    while (<FIN>)
    {
	next if /^#/; # Comments
	next if $onepager && !onepager();
	@rest = split /,/;
	$label = shift @rest;
	$label =~ tr[<>&][{}+];
	next unless @rest;
	# printf "l=%s %s\n", $label, join (';',@rest);
	my $avg = 0;
	$avg += $_ for (@rest);
	$avg /= scalar @rest;
	next if $LITE && int($avg*$LITE) == int($last_time*$LITE);
	$last_time = $avg;
	next if $avg > $CLIP && !$shut;
	next if $avg < $CLIP && $shut;
	$avg += $offset;
	$abs{$label}=$avg;
	$color{$label}=$color;
	$start{$label}= 0;
	$width{$label}=$width;
	$min = $avg if !defined $min || $min > $avg;
	$max = $avg if !defined $max || $max < $avg;
    }
    close FIN;
}


print <<EOF;
<?xml version="1.0" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg width="$XPP" height="$Y" version="1.1" xmlns="http://www.w3.org/2000/svg">
<rect width="$XPP" height="$Y" style="fill:white;stroke-width:5; stroke:black"/>
EOF


$span = $max-$min;
die "max=$max min=$min" unless $span;
$lastx=0;
for (sort {$abs{$a}<=>$abs{$b}} keys %abs)
{
    $abstime = sprintf ("%5.2f", $abs{$_});
    $color = $color{$_};
    $x0 = 10 + ($X-20) * ($abs{$_}-$min) / $span - $X0;
    $x1 = $x2 = $x0;

    $y1 = $WIDTH*$start{$_};
    $y2 = $y1 + $WIDTH*$width{$_};
    printf ("<line x1=\"$x1\" y1=\"$y1\" x2=\"$x2\" y2=\"$y2\" style=\"stroke:$color;stroke-width:2;opacity:0.5\"/>\n");

    $x = $x0 - $FONTSIZE/4;
    $x = $lastx if $x<$lastx && $x>=0;
    $lastx = $x + $FONTSIZE*1.3;

    # $y = 10;
    # printf ("<text x=\"$x\" y=\"$y\" style=\"font-family:Verdana;font-size:$FONTSIZE\" transform=\"rotate(90 $x $y)\">$abstime</text>\n");
    $x2 = $x0;
    $y2 = $y2;
    $x1 = $x + $FONTSIZE/4;
    $y1 = $y2 + $TAIL_LEN;
    printf ("<line x1=\"$x1\" y1=\"$y1\" x2=\"$x2\" y2=\"$y2\" style=\"stroke:$color;stroke-width:2;opacity:0.5\"/>\n");
    $y = $y2 + $TAIL_LEN + $FONTSIZE/2;
    printf ("<text x=\"$x\" y=\"$y\" style=\"font-family:Verdana;font-size:$FONTSIZE\" transform=\"rotate(90 $x $y)\">$abstime $_</text>\n");
}

# End mount -a,1.67,1.67,1.68
# End mdev,2.59,2.60,2.59
# Bgn app,3.64,3.63,3.62

print <<EOF;
</svg>
EOF

