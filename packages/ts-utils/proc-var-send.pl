#!/usr/bin/perl

# tslib 100,60;0
#                         input 200,10;10 (-1)
# 
# NOT input: 174,92,1281;
# NOT                 tslib: 174,92,1281;
# NOT input: 170,91,1199;

while (<>)
{
    if (/input (\d+),(\d+);(\d+)/)
    {
	push @I, [$1,$2] if ($3);
	unless ($3 || !@I)
	{
	    printf ("i=[%s];\n", join (';', map { join (',', @{$_}) } @I));
	    @I = ();
	    printf ("hold on; plot (i(:,1),i(:,2),'b+-');\n");
	}
    }
    if (/tslib (\d+),(\d+);(\d+)/)
    {
	push @T, [$1+2,$2];
	unless ($3 || !@T)
	{
	    printf ("t=[%s];\n", join (';', map { join (',', @{$_}) } @T));
	    @T = ();
	    printf ("hold on; plot (t(:,1),t(:,2),'ro-');\n");
	}
    }
}
printf ("legend('Tslib', 'Raw Input');\n");
printf ("a=gca(); set (a, 'visible', 'off'); print -deps a.eps\n");
