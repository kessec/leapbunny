package stats;

# Minimal statistics
#
'(C) Copyright 2005 Robert Dowling.';
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
# $o = new stats;
# $o->add (1);
# $o->add (2);
# $o->add (3);
# print "N=",$o->N, "\n";
# print "mean=",$o->mean, "\n";
# print "stdev=",$o->stdev, "\n";
# print "min=",$o->min, "\n";
# print "max=",$o->max, "\n";

require Exporter;
our @ISA = qw(Exporter); 
our @EXPORT = qw(new add mean stdev sum N min max);

sub new
{
    my $self = {};
    $self->{N} = 0;
    $self->{SX} = 0;
    $self->{SXX} = 0;
    bless $self;
    $self;
}

sub N ()
{
    my $self = shift;
    $self->{N};
}

sub sum ()
{
    my $self = shift;
    $self->{SX};
}

sub add ($)
{
    my $self = shift;
    my $x = shift;
    $self->{N}++;
    $self->{SX}+=$x;
    $self->{SXX}+=$x*$x;
    $self->{MAX}=$x if $self->{N}==1 || $self->{MAX}<$x;
    $self->{MIN}=$x if $self->{N}==1 || $self->{MIN}>$x;
}

sub mean ()
{
    my $self = shift;
    $self->{N} ? $self->{SX}/$self->{N} : "?";
}

sub stdev ()
{
    my $self = shift;
    $self->{N} ? 
	sqrt (($self->{SXX}-$self->{SX}*$self->{SX}/$self->{N})/(1+$self->{N}))
	: "?";
}

sub min ()
{
    my $self = shift;
    $self->{N} ? $self->{MIN} : "?";
}

sub max ()
{
    my $self = shift;
    $self->{N} ? $self->{MAX} : "?";
}

1;
