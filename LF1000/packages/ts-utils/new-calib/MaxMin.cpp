#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include "MaxMin.h"

CMaxMin::CMaxMin ()
{
	N = 0;
	max=min=NULL;
}

CMaxMin::~CMaxMin ()
{
	if (max) free (max);
	if (min) free (min);
}

void CMaxMin::Init (int newN)
{
	N = newN;
	if (max) free (max);
	if (min) free (min);
	max = (int *)malloc (sizeof(int)*N);
	min = (int *)malloc (sizeof(int)*N);
	if (!max || !min)
	{
		perror ("Can't malloc for MaxMin\n");
		exit (1);
	}
	n = 0;
	sum = 0;
	n_max = n_min = 0;
}

void CMaxMin::Add (int x)
{
	int i, j;
	if (n_max == 0)
	{
		max[0] = x;
		n_max = 1;
	}
	else
	{
		for (i=0; i<n_max; i++)
		{
			if (max[i]<x)
			{
				for (j=N-1; j>i; j--)
					max[j] = max[j-1];
				max[i] = x;
				if (n_max < N)
					n_max++;
				i=N;
				break;
			}
		}
		if (i<N)
		{
			max[i] = x;
			n_max++;
		}
	}
	if (n_min == 0)
	{
		min[0] = x;
		n_min = 1;
	}
	else
	{
		for (i=0; i<n_min; i++)
		{
			if (min[i]>x)
			{
				for (j=N-1; j>i; j--)
					min[j] = min[j-1];
				min[i] = x;
				if (n_min < N)
					n_min++;
				i=N;
				break;
			}
		}
		if (i<N)
		{
			min[i] = x;
			n_min++;
		}
	}
	sum += x;
	n++;
}

int CMaxMin::Max (int i) { return i > 0 && i < N ? max[i] : -1; }
int CMaxMin::Min (int i) { return i > 0 && i < N ? min[i] : -1; }
int CMaxMin::Avg () { return n ? sum / n : 0; }

void CMaxMin::Dump (FILE *f)
{
	int i;
	fprintf (f, "N=%d Avg=%d\n", n, Avg());
	fprintf (f, "min: ");
	for (i=0; i<n_min; i++)
		fprintf (f,"%d ", min[i]);
	fprintf (f, "\nmax: ");
	for (i=0; i<n_max; i++)
		fprintf (f, "%d ", max[i]);
	fprintf (f, "\n");
}

#ifdef TEST
main ()
{
	CMaxMin t;
	int i;
	for (i=1; i<10; i+=2)
	{
		t.Add(i);
		t.Dump ();
	}
	for (i=11; i>0; i-=2)
	{
		t.Add(i);
		t.Dump ();
	}
}
#endif
