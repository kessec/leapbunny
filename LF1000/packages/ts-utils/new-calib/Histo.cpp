#include <stdio.h>
#include "Histo.h"

CHisto::CHisto ()
{
	Init ();
}

void CHisto::Init ()
{
	int i;
	n = 0;
	for (i=0; i<HISTO_N; i++)
		h[i] = 0;
	sum = 0;
}

void CHisto::Add (int x)
{
	if (n==0)
	{
		sum = x;
		min = max = x;
	}
	else
	{
		if (max < x) max = x;
		if (min > x) min = x;
		sum += x;
	}
	n++;
	int b = x/HISTO_BUCKET;
	if (b > HISTO_N-1)
		b = HISTO_N-1;
	h[b]++;
}

int CHisto::Avg () { return n ? sum / n : 0; }

void CHisto::Dump (FILE *f)
{
	int i;
	fprintf (f, "N=%d %d/%d/%d\n", n, min, Avg(), max);
	if (n<1)
		return;
	for (i=min/HISTO_BUCKET; i<=max/HISTO_BUCKET; i++)
	{
		fprintf (f, "%4d: %5d\n", i*HISTO_BUCKET, h[i]);
	}
}

//////////////////////////////////////////////////////////////////////////////

CMakeHisto::CMakeHisto ()
{
	Init ();
}

void CMakeHisto::Init ()
{
	int i;
	n = 0;
	for (i=0; i<MAKEHISTO_N; i++)
		h[i] = 0;
	sum = 0;
}

void CMakeHisto::Add (int x)
{
	if (n==0)
	{
		sum = x;
		min = max = x;
	}
	else
	{
		if (max < x) max = x;
		if (min > x) min = x;
		sum += x;
	}
	n++;
	int b = x;
	if (b > MAKEHISTO_N-1)
		b = MAKEHISTO_N-1;
	if (b < 0)
		b = 0;
	h[b]++;
}

int CMakeHisto::Avg () { return n ? sum / n : 0; }

void CMakeHisto::Partition (int p[MAKEHISTO_BUCKETS])
{
	int i, j, s=0;
	for (i=0; i<MAKEHISTO_BUCKETS; i++)
		p[i] = 0;
	for (i=0, j=1; j<=MAKEHISTO_BUCKETS && i<MAKEHISTO_N; i++)
	{
		int target = j*n/MAKEHISTO_BUCKETS;
		s += h[i];
		if (s > target)
		{
			p[j-1]=i;
			j++;
		}
	}
}

void CMakeHisto::Dump (FILE *f)
{
	int i, j, s=0;
	int p[MAKEHISTO_BUCKETS];
	Partition (p);
	fprintf (f, "N=%d %d/%d/%d\n", n, min, Avg(), max);
	for (i=0; i<MAKEHISTO_BUCKETS; i++)
		fprintf (f, "%2d: %4d\n", i, p[i]);
}
