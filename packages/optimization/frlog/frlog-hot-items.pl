#!/usr/bin/perl

while (<>)
{
    chomp;
    /fail|cant|can't|could not|unable|error/i ? $f{substr($_, 23)}++ : $x{substr($_, 23)}++;
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
