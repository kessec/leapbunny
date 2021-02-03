// Minimal statistics
//
// (C) Copyright 2005 Robert Dowling.
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
// Usage
// 
// CStats o = new CStats;
// o->add (1);
// o->add (2);
// o->add (3);
// printf ("N=%d\n", o->N());
// printf ("mean=%f\n", o->Mean());
// printf ("stdev=%f\n", o->Stdev());
// printf ("min=%f\n", o->Min());
// printf ("max=%f\n", o->Max());

#include "Stats.h"
#include <math.h>
#define NaN (1/m_N)

CStats::CStats()
{
  Init ();
}

CStats::~CStats()
{
}

void CStats::Init ()
{
  m_N = 0;
  m_SX = 0;
  m_SXX = 0;
}	

void CStats::Add (double x)
{
  m_N++;
  m_SX += x;
  m_SXX += x*x;
  if (m_N==1 || m_MAX < x)
    m_MAX = x;
  if (m_N==1 || m_MIN > x)
    m_MIN = x;
}

double CStats::N()
{
  return m_N;
}

double CStats::Sum()
{
  return m_SX;
}

double CStats::Mean ()
{
  return m_N ? m_SX/m_N : NaN;
}

double CStats::Stdev ()
{
  if (m_N)
  {
    return sqrt ((m_SXX - m_SX*m_SX/m_N) / (1+m_N));
  }
  return 0.0;
}

double CStats::Min ()
{
  return m_N ? m_MIN : NaN;
}

double CStats::Max ()
{
  return m_N ? m_MAX : NaN;
}
