/*
 * Medical Image Registration ToolKit (MIRTK)
 *
 * Copyright 2016 Imperial College London
 * Copyright 2016 Andreas Schuh
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mirtk/SymmetricLinearSurfaceMapper.h"

#include "mirtk/EdgeTable.h"

#include "Eigen/SparseCore"
#include "Eigen/IterativeLinearSolvers"


namespace mirtk {


// Global flags (cf. mirtk/Options.h)
MIRTK_Common_EXPORT extern int verbose;


// =============================================================================
// Construction/destruction
// =============================================================================

// -----------------------------------------------------------------------------
void SymmetricLinearSurfaceMapper::CopyAttributes(const SymmetricLinearSurfaceMapper &other)
{
}

// -----------------------------------------------------------------------------
SymmetricLinearSurfaceMapper::SymmetricLinearSurfaceMapper()
{
}

// -----------------------------------------------------------------------------
SymmetricLinearSurfaceMapper::SymmetricLinearSurfaceMapper(const SymmetricLinearSurfaceMapper &other)
:
  LinearSurfaceMapper(other)
{
  CopyAttributes(other);
}

// -----------------------------------------------------------------------------
SymmetricLinearSurfaceMapper &SymmetricLinearSurfaceMapper::operator =(const SymmetricLinearSurfaceMapper &other)
{
  if (this != &other) {
    LinearSurfaceMapper::operator =(other);
    CopyAttributes(other);
  }
  return *this;
}

// -----------------------------------------------------------------------------
SymmetricLinearSurfaceMapper::~SymmetricLinearSurfaceMapper()
{
}

// =============================================================================
// Execution
// =============================================================================

// -----------------------------------------------------------------------------
void SymmetricLinearSurfaceMapper::Solve()
{
  typedef Eigen::MatrixXd             Values;
  typedef Eigen::SparseMatrix<double> Matrix;
  typedef Eigen::Triplet<double>      NZEntry;

  const int n = NumberOfFreePoints();
  const int m = NumberOfComponents();

  int i, j, r, c, l;

  Matrix A(n, n);
  Values b(n, m);
  {
    EdgeTable edgeTable(_Surface);
    EdgeIterator edgeIt(edgeTable);

    b.setZero();
    Array<NZEntry> w;
    Array<double>  w_ii(n, .0);
    w.reserve(2 * edgeTable.NumberOfEdges() + n);

    double w_ij;
    for (edgeIt.InitTraversal(); edgeIt.GetNextEdge(i, j) != -1;) {
      r = FreePointIndex(i);
      c = FreePointIndex(j);
      if (r >= 0 || c >= 0) {
        w_ij = Weight(i, j);
        if (r >= 0 && c >= 0) {
          w.push_back(NZEntry(r, c, -w_ij));
          w.push_back(NZEntry(c, r, -w_ij));
        } else if (r >= 0) {
          for (l = 0; l < m; ++l) {
            b(r, l) += w_ij * GetValue(j, l);
          }
        } else if (c >= 0) {
          for (l = 0; l < m; ++l) {
            b(c, l) += w_ij * GetValue(i, l);
          }
        }
        if (r >= 0) {
          w_ii[r] += w_ij;
        }
        if (c >= 0) {
          w_ii[c] += w_ij;
        }
      }
    }
    for (r = 0; r < n; ++r) {
      w.push_back(NZEntry(r, r, w_ii[r]));
    }

    A.setFromTriplets(w.begin(), w.end());
  }

  if (verbose) {
    cout << "\n";
    cout << "  No. of surface points             = " << _Surface->GetNumberOfPoints() << "\n";
    cout << "  No. of free points                = " << n << "\n";
    cout << "  No. of non-zero stiffness values  = " << A.nonZeros() << "\n";
    cout << "  Dimension of surface map codomain = " << m << "\n";
    cout.flush();
  }

  Values x(n, m);
  for (r = 0; r < n; ++r) {
    i = FreePointId(r);
    for (l = 0; l < m; ++l) {
      x(r, l) = GetValue(i, l);
    }
  }

  int    niter = 0;
  double error = .0;

  Eigen::ConjugateGradient<Matrix> solver(A);
  if (_NumberOfIterations >  0) solver.setMaxIterations(_NumberOfIterations);
  if (_Tolerance          > .0) solver.setTolerance(_Tolerance);
  x = solver.solveWithGuess(b, x);
  niter = solver.iterations();
  error = solver.error();

  if (verbose) {
    cout << "  No. of iterations                 = " << niter << "\n";
    cout << "  Estimated error                   = " << error << "\n";
    cout.flush();
  }

  for (r = 0; r < n; ++r) {
    i = FreePointId(r);
    for (l = 0; l < m; ++l) {
      SetValue(i, l, x(r, l));
    }
  }
}


} // namespace mirtk
