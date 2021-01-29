package histo;

# Minimal statistics
#
'(C) Copyright 2010 Robert Dowling.';
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# Usage
#
# $o = new histo;
# $o->add (1);
# $o->add (2);
# $o->add (3);
# print "N=",$o->N, "\n";
# print "min=",$o->min, "\n";
# print "max=",$o->max, "\n";
# print "buckets of 5:", join(',' $o->buckets(5)), "\n";

require Exporter;
our @ISA = qw(Exporter); 
our @EXPORT = qw(new N add min max buckets);

sub new
{
    my $self = {};
    $self->{x} = [];
    bless $self;
    $self;
}

sub N ()
{
    my $self = shift;
    scalar (@{$self->{x}});
}

sub add ($)
{
    my $self = shift;
    my $x = shift;
    $self->{MAX}=$x if !$self->N || $self->{MAX}<$x;
    $self->{MIN}=$x if !$self->N || $self->{MIN}>$x;
    push @{$self->{x}}, $x;
}

sub min ()
{
    my $self = shift;
    $self->N ? $self->{MIN} : "?";
}

sub max ()
{
    my $self = shift;
    $self->N ? $self->{MAX} : "?";
}

sub mean ()
{
    my $self = shift;
    return 0 unless $self->N;
    my $sum; $sum += $_ for (@{$self->{x}});
    $sum / $self->N;
}

sub median ()
{
    my $self = shift;
    my $n = $self->N;
    return 0 unless $n;
    my @s = sort {$a<=>$b} (@{$self->{x}});
    return $s[$n/2] if @s & 1;
    return ($s[$n/2]+$s[1+$n/2])/2;
}

sub median_pct ($)
{
    my $self = shift;
    my $pct = shift;
    my $n = $self->N;
    return 0 unless $n;
    my @s = sort {$a<=>$b} (@{$self->{x}});
    $pct >= 1 ? $s[-1] : $s[$n*$pct];
}

sub buckets ($)
{
    my $self = shift;
    my $width = shift;
    my @b = ();
    $b[$_/$width]++ for (@{$self->{x}});
    @b;
}

sub mode ()
{
    my $self = shift;
    my $n = $self->N;
    return 0 unless $n;
    my %s;
    $s{$_}++ for (@{$self->{x}});
    my @s = sort {$s{$a}<=>$s{$b}} keys %s;
    return $s[-1];
}

sub modeN ()
{
    my $self = shift;
    my $n = $self->N;
    return 0 unless $n;
    my %s;
    $s{$_}++ for (@{$self->{x}});
    my @s = sort {$s{$a}<=>$s{$b}} keys %s;
    return $s{$s[-1]};
}

1;
