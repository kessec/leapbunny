#!/usr/bin/perl

for $a (@ARGV)
{
    if ($a =~ /.gz$/)
    {
	open (FIN, "zcat $a| tail -n 1|") || die;
    }
    else
    {
	open (FIN, "tail -n 1 $a|") || die;
    }
    $_ = <FIN>;
    ($dev, $mass) = $a =~ /new-calib-(...-...-.*)-(\d+g)-/;
    $d{$dev}++;
    $m{$mass}++;
    $D{$dev}{$mass} = "Pass", next if /Setting for tnt_plane:/;
    ($hi, $avg, $range) = undef;
    if (/Failed heuristic checks: /)
    {
	@r = ();
	push @r, $1 if /(hi:\d+>25)/;
	push @r, $1 if /(avg:\d+>20)/;
	push @r, $1 if /(hi-lo:\d+>10)/;
	$D{$dev}{$mass} = join (" ", @r);
	next;
    }
    $D{$dev}{$mass} = "Timeout", next if /Timeout: /;
}

@m = qw(20g 25g 30g 40g 50g 75g 100g 150g 200g);
@m = qw(20g 25g 30g 40g);
printf ("\"Device\",\"%s\"\n", join ("\",\"", @m));
for $d (sort keys %d)
{
    printf ("\"%s\"", $d);
    for $m (@m)
    {
	printf (",\"%s\"", $D{$d}{$m});
    }
    printf ("\n");
}
