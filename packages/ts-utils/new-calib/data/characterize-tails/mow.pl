#!/usr/bin/perl

while (<>)
{
    for (split /;/)
    {
	next unless ($x, $y, $t, $p) = /(\d+),(\d+),(\d+),(\d+)/;
	next if $t>25;
	print;
	print ";"
    }
}
