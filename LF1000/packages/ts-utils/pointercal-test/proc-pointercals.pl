#!/usr/bin/perl

$TDET = 0.01;
$TDOT = 0.05;

while (<>)
{
    # A = a b
    #     d e
    # A*[1,0] = [a,d]
    # A*[0,1] = [b,e]
    # ($a, $b, $c, $d, $e, $f, $g) = split;
    ($a, $b, $c, $d, $e, $f, $g) = 
	/(-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (65536)/;
    next unless $g;
    $fail = cal_test ($a, $b, $c, $d, $e, $f, $g);
}

sub cal_test ()
{
    my ($a, $b, $c, $d, $e, $f, $g) = @_;

    my $mx = sqrt ($a * $a + $d * $d);
    my $my = sqrt ($b * $b + $e * $e);

    my $det = $a * $e - $b * $d;
    my $dot = $a * $b + $d * $e;
    
    $det /= $mx*$my;
    $dot /= $mx*$my;

    my $baddet = $det > 1+$TDET || $det < 1-$TDET;
    my $baddot = $dot > 0+$TDOT || $dot < 0-$TDOT;

    my $fail = $baddet || $baddot;

    printf "Det=%6.2f %s ", $det, $baddet ? "*" : " ";
    printf "Dot=%6.2f %s ", $dot, $baddot ? "*" : " ";
    print $fail ? "FAIL " : "     ";
    print;

    return $fail;
}
