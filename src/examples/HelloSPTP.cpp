/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2013-2015, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
 * following terms and conditions apply:
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 *
 * http://numenta.org/licenses/
 * ---------------------------------------------------------------------
 */

#include <algorithm> // std::generate
#include <cmath>     // pow
#include <ctime>     // std::time
#include <iostream>
#include <vector>

#include "nupic/algorithms/Anomaly.hpp"
#include "nupic/algorithms/Cells4.hpp"  //TODO use TM instead
#include "nupic/algorithms/SpatialPooler.hpp"

#include "nupic/os/Timer.hpp"
#include "nupic/utils/VectorHelpers.hpp"
#include "nupic/utils/Random.hpp" 

namespace examples {

using namespace std;
using namespace nupic;
using namespace nupic::utils;
using nupic::algorithms::spatial_pooler::SpatialPooler;
using nupic::algorithms::Cells4::Cells4;
using nupic::algorithms::anomaly::Anomaly;
using nupic::algorithms::anomaly::AnomalyMode;

// work-load 
void run() {
  const UInt COLS = 2048; // number of columns in SP, TP
  const UInt DIM_INPUT = 10000;
  const UInt CELLS = 10; // cells per column in TP
  const UInt EPOCHS = (UInt)pow(10, 3); // number of iterations (calls to SP/TP compute() )
  std::cout << "starting test. DIM_INPUT=" << DIM_INPUT
  		<< ", DIM=" << COLS << ", CELLS=" << CELLS << std::endl;
  std::cout << "EPOCHS = " << EPOCHS << std::endl;


  // initialize SP, TP, Anomaly, AnomalyLikelihood
  SpatialPooler sp(vector<UInt>{DIM_INPUT}, vector<UInt>{COLS});
  Cells4 tp(COLS, CELLS, 12, 8, 15, 5, .5f, .8f, 1.0f, .1f, .1f, 0.0f,
            false, 42, true, false);
  Anomaly an(5, AnomalyMode::LIKELIHOOD);

  // data for processing input
  vector<UInt> input(DIM_INPUT);
  vector<UInt> outSP(COLS); // active array, output of SP/TP
  vector<UInt> outTP(tp.nCells());
  vector<Real> rIn(COLS); // input for TP (must be Reals)
  vector<Real> rOut(tp.nCells());
  Real res = 0.0; //for anomaly:
  vector<UInt> prevPred_(outSP.size());
  Random rnd;
  
  // Start a stopwatch timer
  printf("starting:  %d iterations.", EPOCHS);
  Timer stopwatch(true);


  //run
  for (UInt e = 0; e < EPOCHS; e++) {
    generate(input.begin(), input.end(), [&] () { return rnd.getUInt32(2); });

    fill(outSP.begin(), outSP.end(), 0);
    sp.compute(input.data(), true, outSP.data());
    sp.stripUnlearnedColumns(outSP.data());

    rIn = VectorHelpers::castVectorType<UInt, Real>(outSP);
    tp.compute(rIn.data(), rOut.data(), true, true);
    outTP = VectorHelpers::castVectorType<Real, UInt>(rOut);

    res = an.compute(outSP /*active*/, prevPred_ /*prev predicted*/); 
    prevPred_ = outTP; //to be used as predicted T-1
    cout << res << " "; 

    // print
    if (e == EPOCHS - 1) {
      cout << "Epoch = " << e << endl;
      VectorHelpers::print_vector(VectorHelpers::binaryToSparse<UInt>(outSP), ",", "SP= ");
      VectorHelpers::print_vector(VectorHelpers::binaryToSparse<UInt>(VectorHelpers::cellsToColumns(outTP, CELLS)), ",", "TP= ");
      NTA_CHECK(outSP[69] == 0) << "A value in SP computed incorrectly";
      NTA_CHECK(outTP[42] == 0) << "Incorrect value in TP";
    }
  }

  stopwatch.stop();
  const size_t timeTotal = stopwatch.getElapsed();
  const size_t CI_avg_time = 45; //sec
  cout << "Total elapsed time = " << timeTotal << " seconds" << endl;
  NTA_CHECK(timeTotal <= CI_avg_time) << //we'll see how stable the time result in CI is, if usable
	  "HelloSPTP test slower than expected! (" << timeTotal << ",should be "<< CI_avg_time;

}
} //-ns
