#!/usr/bin/perl

$T = 25;

while (<>)
{
    for (split /;/)
    {
	next unless ($x, $y, $t, $p) = /(\d+),(\d+),(\d+),(\d+)/;
	push @{$Y[$y/$T]}, $t;
	push @{$X[$x/$T]}, $t;
    }
}

sub median
{
    my @x = @_;
    my $n = scalar @x;
    return (sort @x)[$n/2];
}

sub mode
{
    my @x;
    $x[$_]++ for (@_);
    my $mode = 0;
    my $mi = 0;
    for (0.. $#x)
    {
	$mi = $_, $mode=$x[$_] if $mode < $x[$_];
    }
    return $mi;
}


# X
for $x (0.. 1000/$T)
# for $x (150..900)
{
    @a = @{$X[$x]};
    next unless scalar @a;
    $n1 = median(@a);
    $n2 = mode(@a);
    printf ("X=%4d N=%4d TNT Median=%3d Mode=%3d\n", $x*$T, scalar @a, $n1, $n2);
}

# Y
for $y (0.. 1000/$T)
# for $y (300..800)
{
    @a = @{$Y[$y]};
    next unless scalar @a;
    $n1 = median(@a);
    $n2 = mode(@a);
    printf ("Y=%4d N=%4d TNT Median=%3d Mode=%3d\n", $y*$T, scalar @a, $n1, $n2);
}
