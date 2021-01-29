#!/usr/bin/perl

while (<>)
{
    print if /LoadNewApp path = /;
    chomp;
    ($date, $hr, $min, $sec, $rest) = /(... .. (..):(..):(..)) (.*)/;
    $count++;
    $time=($hr*60+$min)*60+$sec;
    $delta=$time-$last;
    if ($delta >=1)
    {
	printf ("%s: %3d/%4d=%8.2f %s|\n", $date, $count, $delta,
		$count/$delta, '*'x($count/$delta));
	$count =0;
	$last=$time;
    }
}

for (sort {$f{$b} <=> $f{$a} || $a cmp $b} keys %f)
{
    printf "%5d: %s\n", $f{$_}, $_;
}

print ("-"x70, "\n");

for (sort {$x{$b} <=> $x{$a} || $a cmp $b} keys %x)
{
    printf "%5d: %s\n", $x{$_}, $_;
}
