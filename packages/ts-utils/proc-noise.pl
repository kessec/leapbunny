#!/usr/bin/perl

use stats;

$X1=new stats;
$X2=new stats;
$Y1=new stats;
$Y2=new stats;
$P1=new stats;
$P2=new stats;
$X=new stats;
$Y=new stats;
$P=new stats;

while (<>)
{
    # x1=297 x2=799 y2=349 y2=864 p1=824 p2=210 tnt1=10 tnt2=13 P=944
    my ($x1, $x2, $y1, $y2, $p1, $p2, $x, $y, $p) = 
	/x1=(\d+) x2=(\d+) y1=(\d+) y2=(\d+) p1=(\d+) p2=(\d+).*X=(\d+) Y=(\d+) P=(\d+)/;
    $X1->add($x1);
    $X2->add($x2);
    $Y1->add($y1);
    $Y2->add($y2);
    $P1->add($p1);
    $P2->add($p2);
    $X->add($x);
    $Y->add($y);
    $P->add($p);
}

# $A=$X1; printf ("X1: %d/%d/%d %f\n", $A->min, $A->mean, $A->max, $A->stdev);
# $A=$X2; printf ("X2: %d/%d/%d %f\n", $A->min, $A->mean, $A->max, $A->stdev);
# $A=$Y1; printf ("Y1: %d/%d/%d %f\n", $A->min, $A->mean, $A->max, $A->stdev);
# $A=$Y2; printf ("Y2: %d/%d/%d %f\n", $A->min, $A->mean, $A->max, $A->stdev);
# $A=$P1; printf ("P1: %d/%d/%d %f\n", $A->min, $A->mean, $A->max, $A->stdev);
# $A=$P2; printf ("P2: %d/%d/%d %f\n", $A->min, $A->mean, $A->max, $A->stdev);

$A=$X1; printf ("X1: %.2f ", $A->stdev);
$A=$X2; printf ("X2: %.2f ", $A->stdev);
$A=$Y1; printf ("Y1: %.2f ", $A->stdev);
$A=$Y2; printf ("Y2: %.2f ", $A->stdev);
$A=$P1; printf ("P1: %.2f ", $A->stdev);
$A=$P2; printf ("P2: %.2f ", $A->stdev);
$A=$X; printf ("X: %.2f ", $A->stdev);
$A=$Y; printf ("Y: %.2f ", $A->stdev);
$A=$P; printf ("P: %.2f ", $A->stdev);
printf (" x1-x2: %.2f y1-y2: %.2f\n", abs($X1->mean-$X2->mean),
	abs($Y1->mean-$Y2->mean));


