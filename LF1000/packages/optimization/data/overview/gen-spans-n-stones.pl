#!/usr/bin/perl

# A span has
#  label
#  bgn time
#  end time
#  layer number
#  color number

$JIFFIES=shift;
$END = 22.0;

sub get_time ($)
{
    local $_ = shift;
    0+$_;
}

sub add_span
{
    my ($label, $side, $time, $layer, $color) = @_;
    $span{$label}{bgn_time} = $time if $side =~ /bgn/i;
    $span{$label}{end_time} = $time if $side =~ /end/i;
    $span{$label}{layer} = $layer if defined $layer;
    $span{$label}{color} = $color if defined $color;
}

sub join_spans
{
    my ($start, $end) = @_;
    $span{$start}{end_time} = $span{$end}{bgn_time};
}

sub end_span
{
    my ($label) = @_;
    $span{$label}{end_time} = $END - $JIFFIES;
}

sub add_stone
{
    my ($label, $layer, $time) = @_;
    $stone{$label}{time} = $time;
    $stone{$label}{layer} = $layer;
}


while (<>)
{
    add_span ("InitScripts", "bgn", get_time ($1), 3, 6), next
	if /"Freeing init memory.*,(.*)/;
    add_span ("Networking", $1, get_time ($2), 3, 7), next
	if /"(...) Rest of networking.*,(.*)/;
    add_span ("Versions", $1, get_time ($2), 3, 8), next
	if /"(...) Version Logging.*,(.*)/;
    add_span ("Legal Screen", 'bgn', get_time ($1), 0, 9), next
	if /display legal.*,(.*)/;
    add_span ("Chime", $1, get_time ($2), 1, 10), next
	if /"(...) sounds\/startup.wav.*,(.*)/;
    add_span ("AppManager Load", $1, get_time ($2), 4, 11), next
	if /"(...) app.*,(.*)/;
#    add_span ("AppManager Init", "bgn", get_time ($1), 4, 12), next
#	if /"AppManager: UI loaded.*,(.*)/;
    add_span ("AppManager Init", "bgn", get_time ($1), 4, 12), next
	if /"AppManager: Entered main function.*,(.*)/;
    add_span ("AppManager", "bgn", get_time ($1), 4, 13), next
	if /"AppManager: UI entered.*,(.*)/;
    add_span ("Splash Video Screen", "Bgn", get_time ($1), 0, 14),
    add_span ("Splash Video Sound", "Bgn", get_time ($1), 1, 15), next
	if /Start splash video.*,(.*)/;
    add_span ("Splash Video Screen", "End", get_time ($1), 0, 14),
    add_span ("Splash Video Sound", "End", get_time ($1), 1, 15), next
	if /End splash video.*,(.*)/;
    add_span ("FlashPlayer Init", "bgn", get_time ($1), 5, 16), next
	if /Constructing DidjPlayer.*,(.*)/;
    add_span ("FlashPlayer Init", $1, get_time ($2), 5, 16), next
	if /Flash: (...) CreatePlayer.*,(.*)/;
    add_span ("FlashPlayer", 'bgn', get_time ($1), 5, 17),
    add_span ("UI SWF", 'bgn', get_time ($1), 6, 20), next
	if /End Load ?Movie.*,(.*)/;
    add_span ("UI Screen", 'bgn', get_time ($1), 0, 18),
    add_span ("UI Sound", 'bgn', get_time ($1), 1, 19),
    add_stone ("UI Ready", 1, get_time ($1)), next
	if /"AppManager: UI updated.*,(.*)/;
    add_stone ("BulkReady", 1, get_time ($1)), next
	if /"End mounts.*,(.*)/;
}


add_span ("First Screen", 'bgn', 0.75-$JIFFIES, 0, 3);
join_spans ("First Screen", "Legal Screen");
join_spans ("AppManager Load", "AppManager Init");
join_spans ("AppManager Init", "AppManager");
join_spans ("Legal Screen", "Splash Video Screen");
join_spans ("FlashPlayer Init", "FlashPlayer");
$span{"InitScripts"}{end_time} = $span{"Chime"}{end_time};

end_span ("AppManager");
end_span ("FlashPlayer");
end_span ("UI Screen");
end_span ("UI Sound");
end_span ("UI SWF");

add_stone ("UI Visble", 1, $span{"Splash Video Screen"}{end_time}) if
    $span{"Splash Video Screen"}{end_time} > $span{"UI Screen"}{bgn_time};
add_stone ("ON", 1, -$JIFFIES);
add_stone ("Backlight", 2, 0.221-$JIFFIES);


for (sort keys %span)
{
    printf ("\t[ \"%s\", %d, %d, %.4f, %.4f ],\n",
	    $_,
	    $span{$_}{color},
	    $span{$_}{layer},
	    $JIFFIES+$span{$_}{bgn_time},
	    $JIFFIES+$span{$_}{end_time});
}
print (");\n");
print ("\@milestones = (\n");
for (sort keys %stone)
{
    printf ("\t[ \"%s\", %d, %.4f ],\n",
	    $_,
	    $stone{$_}{layer},
	    $JIFFIES+$stone{$_}{time});
}
print (");\n");

__END__

@spans192 = (
1	   [HW,               3, 0, 0.2],
2	   [NOR,              3, 0.2, 1.29],
3	   ["First Screen",   0, 0.75, 2.30+4.49],
4	   [KDcmp,            3, 1.29, 2.230],
5	   [Kernel,           3, 2.230,     2.30+1.69],
6be	   [InitScripts,      3, 2.30+1.69, 2.30+6.44],
7be	   [Networking,       3, 2.30+13.66, 2.30+15.79],
8be	   [Versions,         3, 2.30+18.7,  2.30+19.75],
9	   ["Legal Screen",   0, 2.30+4.49, 2.30+8.00],
10	   [Chime,            1, 2.30+4.49, 2.30+7.11],
11	   [AppManagerLoad,   4, 2.30+3.65, 2.30+3.65+3],
12	   [AppManagerInit,   4, 2.30+3.65+3, 2.30+9.21],
13	   [AppManager,       4, 2.30+9.21, $END_TIME],
14	   ["SplashVideo Screen",0, 2.30+8.00, 2.30+13.16],
15	   ["SplashVideo Sound", 1, 2.30+8.00, 2.30+13.16],
16	   [FlashPlayerInit,  5, 2.30+9.20, 2.30+9.66],
17	   [FlashPlayer,      5, 2.30+9.66, $END_TIME],
18	   ["UI Screen",      0, 2.30+11.58,$END_TIME],
19	   ["UI Sound",       1, 2.30+11.58,$END_TIME],
20	   ["UI SWF",         6, 2.30+9.66,$END_TIME],
);
@milestones192 = (
	   [POW,   1, 0],
	   [Backlight,    2, 0.56],
	   ["BulkReady", 1, 2.30+6.44],
	   ["UI Ready", 1, 2.30+11.58],
	   ["UI Visible", 1, 2.30+13.19],
);
