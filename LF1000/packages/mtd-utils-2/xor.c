#include <stdio.h>

main (int c, char **v)
{
	if (c<2)
	{
		printf ("Usage:\n\txor position\nInvert <position>'th bit in stdin and output to stdout\n");
		return 0;
	}
	int x = atoi (v[1]);
	int xi = x / 8;
	int xm = 1 << (x%8);
	int i=0;
	while ((x=getchar ()) != EOF)
	{
		if (xi==i++)
			x ^= xm;
		putchar (x);
	}
	return 0;
}
