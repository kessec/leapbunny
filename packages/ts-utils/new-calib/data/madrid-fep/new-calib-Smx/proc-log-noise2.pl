#!/usr/bin/perl

$CSV=0;

if ($ARGV[0] eq '-5')
{
    shift;
    $dash5 = 1;
}

if ($CSV)
{
    printf ('"Device","Center","Avg","Medians",');
    printf ('"","","","Hi","Lo","Hi-Lo"' . "\n")
}

$T = 500;
for $a (@ARGV)
{
    if ($a =~ /.gz$/)
    {
	open (FIN, "zcat $a|") || die;
    }
    else
    {
	open (FIN, $a) || die;
    }
    $state=-1;
    @a=();
    $cen = -1;
    $avg = $Navg = 0;
    while (<FIN>)
    {
	$state=$1, @a=(), next if /target state=(\d+)/;
	if ($state>=0)
	{
	    if (($t1, $t2) = /X=.*Y=.*P1=.*P2=.*T1=\s*(\d+) T2=\s*(\d+)/)
	    {
		my $t = ($t1 + $t2)/2;
		push @a, $t if $t < $T;
	    }
	}
	$cen = $1, next if /Mode  40g: P=\s*\d+ T=(\d+)/;
	if (/Remove Pin/)
	{
	    next unless $state >= 0;
# printf ("a=%s\n", join(',', (sort {$a<=>$b} @a)));
	    $median = (sort {$a<=>$b} @a)[(scalar @a)/2];
	    $avg += $median;
	    $Navg++;
	    if ($dash5)
	    {
		$state == 0 ? $cen : $m[$state-1] = $median;
	    }
	    else
	    {
		$m[$state] = $median;
	    }
	    @a=();
	    $state=-1;
	    next
	}
	# X= 991 X2= 987  Y= 953 Y2=1022 P1=  -1 P2=  -1 T1=1023 T2= 862  P=  -1  D=  -1 
    }
    @sortm = sort {$a<=>$b} @m;
    $hi = $sortm[-1];
    $lo = $sortm[0];
    $range = $hi - $lo;
    $chi = $cen - $hi;
    $avg /= $Navg;
    $N++;
    $Hhi[$hi]++;
    $Hlo[$lo]++;
    $Havg[$avg]++;
    $Hchi[$chi>=0 ? $chi+1 : 0]++;
    $Hran[$range]++;
    if ($CSV)
    {
	printf "\"%s\",%d,%d,%s,%d,%d,%d\n",
	    $a, $cen, $avg, join(',',map {sprintf "%d", $_} @m),
		$hi, $lo, $range;
    }
    else
    {
	$flag = "";
	$flag .= $hi >= 25 ? "hi " : "   ";
	$flag .= $avg >= 25 ? "avg " : "    ";
	$flag .= $range >= 10 ? "range " : "      ";
	$flag .= $chi > 0 ? "center " : "       ";
	$flag .= "bad" if $flag !~ /^\s*$/;
	printf "%s Cen=%2d Avg=%2d Medians=%s Range=%3d-%3d=%3d %s\n",
	    $a, $cen, $avg, join(' ',map {sprintf "%2d", $_} @m),
		$lo, $hi, $range, $flag;
    }
}

$m=$#Hhi; $m=$#Hchi if $m<$#Hchi; $m=$#Hran if $m<$#Hran;
$m=$#Havg if $m<$#Havg;
$m=$#Hlo if $m<$#Hlo;

sub pct ($)
{
    local $_ = shift;
    return "" unless $_;
    sprintf ("%2d", 100*$_/$N);
}

if ($CSV)
{
    printf ("\"i\",\"Hi\",\"Avg\",\"Cen>Hi\",\"Range\"\n");
    for (0..$m)
    {
	printf ("%d,%s,%s,%s,%s\n", $_, 
		$Hhi[$_], 
		$Havg[$_],
		$Hchi[$_],
		$Hran[$_]);
    }
}
else
{
    for (0..$m)
    {
	printf ("Hhi[%2d]=%2s ", $_, pct($Hhi[$_]));
	printf ("Havg[%2d]=%2s ", $_, pct($Havg[$_]));
	printf ("Hchi[%2d]=%2s ", $_, pct($Hchi[$_]));
	printf ("Hran[%2d]=%2s ", $_, pct($Hran[$_]));
	printf ("Hlo[%2d]=%2s\n", $_, pct($Hlo[$_]));
    }
}
