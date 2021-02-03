#!/usr/bin/perl

# input: 174,92,1281;
#                 tslib: 174,92,1281;
# input: 170,91,1199;

while (<>)
{
    push @I, [$1-2,$2-2,$3] if /input: (\d+),(\d+),(\d+);/ && $1;
    push @T, [$1,$2,$3] if /tslib: (\d+),(\d+),(\d+);/; # && $3;
}


printf ("i=[%s];\n", join (';', map { join (',', @{$_}) } @I));
printf ("t=[%s];\n", join (';', map { join (',', @{$_}) } @T));
printf ("hold on; plot (i(:,1),i(:,2),'b+-'); plot (t(:,1),t(:,2),'ro-');\n");
printf ("legend('Raw Input', 'Tslib');\n");
printf ("a=gca(); set (a, 'visible', 'off'); print -deps a.eps\n");
