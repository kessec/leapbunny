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
# for $x (150..900)
for $x (225/$T.. 800/$T)
{
    @a = @{$X[$x]};
    next unless scalar @a;
    $n1 = median(@a);
    $n2 = mode(@a);
    push @XX, $n2;
}
printf ("x=[%s];\n", join(',', @XX));

# Y
for $y (300/$T.. 725/$T)
# for $y (300..800)
{
    @a = @{$Y[$y]};
    next unless scalar @a;
    $n1 = median(@a);
    $n2 = mode(@a);
    push @YY, $n2;
}
printf ("y=[%s];\n", join(',', @YY));
