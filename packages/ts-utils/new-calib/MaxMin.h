class CMaxMin
{
public:
	CMaxMin ();
	~CMaxMin ();
	void Init (int N);
	void Add(int x);
	int Max(int i);
	int Min(int i);
	int Avg ();
	void Dump (FILE *f=stdout);
private:
	int N;
	int *max, *min;
	int sum;
	int n;
	int n_max, n_min;
};

