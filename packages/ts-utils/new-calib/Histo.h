// product of HISTO_N * HISTO_BUCKET >= 2000
#include <stdio.h>

#define HISTO_N		40
#define HISTO_BUCKET	50

class CHisto
{
public:
	CHisto ();
	void Init ();
	void Add(int x);
	int Avg ();
	void Dump (FILE *f=stdout);
private:
	int h[HISTO_N];
	int n;
	int max, min, sum;
};

#define MAKEHISTO_N	2000
#define MAKEHISTO_BUCKETS	10

class CMakeHisto
{
 public:
	CMakeHisto ();
	void Init ();
	void Add(int x);
	int Avg ();
	void Dump (FILE *f=stdout);
	void Partition (int p[MAKEHISTO_BUCKETS]);
private:
	int h[MAKEHISTO_N];
	int n;
	int max, min, sum;
};
