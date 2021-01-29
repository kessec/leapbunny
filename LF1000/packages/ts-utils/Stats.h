class CStats
{
public:
  CStats();
  virtual ~CStats();
  void Add(double x);
  double N();
  double Sum();
  double Mean ();
  double Stdev ();
  double Min ();
  double Max ();
  void Init ();
private:
  double m_N, m_SX, m_SXX, m_MIN, m_MAX;
};
