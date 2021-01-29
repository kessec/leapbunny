#!/usr/bin/perl

use stats;

$s1=new stats;
$s10=new stats;
$s11=new stats;

$s1->add(1); $s1->add(2); $s1->add(3); $s1->add(4); $s1->add(5);
$s10->add(10); $s10->add(20); $s10->add(30); $s10->add(40); $s10->add(50);
$s11->add(11); $s11->add(12); $s11->add(13); $s11->add(14); $s11->add(15);

printf ("%d/%d/%d %f\n", $s1->min, $s1->mean, $s1->max, $s1->stdev);
printf ("%d/%d/%d %f\n", $s10->min, $s10->mean, $s10->max, $s10->stdev);
printf ("%d/%d/%d %f\n", $s11->min, $s11->mean, $s11->max, $s11->stdev);
