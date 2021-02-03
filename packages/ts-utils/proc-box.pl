#!/usr/bin/perl

while (<>)
{
    # x1=297 y1=799 x2=349 y2=864 p1=824 p2=210 tnt1=10 tnt2=13 P=944
    my ($x1, $y1, $x2, $y2) = /x1=(\d+) y1=(\d+) x2=(\d+) y2=(\d+)/;
    push @a, sprintf "%d,%d", $x1, $y1;
    push @b, sprintf "%d,%d", $x2, $y2;
}

printf "a=[%s];\n", join (';', @a);
printf "b=[%s];\n", join (';', @b);

