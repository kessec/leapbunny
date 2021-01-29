#!/usr/bin/perl

$A = 0;
$T = 500;
for $a (@ARGV)
{
    open (FIN, $a) || die;
    $state=-1;
    @a=();
    while (<FIN>)
    {
	$state=$1, next if /target state=(\d+)/;
	if ($state>=0)
	{
	    if (($t1, $t2) = /X=.*Y=.*P1=.*P2=.*T1=\s*(\d+) T2=\s*(\d+)/)
	    {
		$avg = ($t1 + $t2)/2;
		push @a, $avg if $avg < $T;
	    }
	}
	if (/Remove Pin/)
	{
	    $median = (sort @a)[(scalar @a)/2];
	    $m[$state] = $median;
	    @a=();
	    $state=-1;
	    next
	}
	# X= 991 X2= 987  Y= 953 Y2=1022 P1=  -1 P2=  -1 T1=1023 T2= 862  P=  -1  D=  -1 
    }
    $hi = (sort @m)[$#m];
    $lo = (sort @m)[0];
    $delta = $hi - $lo;
    $flag = $hi >= 25 ? "Bad" : "";
    printf "%s Medians=%s Range=%3d-%3d=%3d %s\n", 
	$a, join(' ',map {sprintf "%2d", $_} @m), $lo, $hi, $delta, $flag;
    # $a =~ /(...-).................(...)$/;
    # $N[$A] = $1 . $2;
    # $s = scalar @{$X[$A]};
    # $m = $s if $m < $s;
    # # printf ("m=%d\n", $m);
    # $A++;
}

# for $r (0..$m-1)
# {
#     printf "%s\n", join (',', map { $X[$_][$r] } (0..$A-1));
# }
