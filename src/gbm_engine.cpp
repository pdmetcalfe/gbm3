//  GBM by Greg Ridgeway  Copyright (C) 2003
//#define NOISY_DEBUG
#include <algorithm>
#include "gbm_engine.h"

CGBMEngine::CGBMEngine(DataDistParams& datadistparams,
		TreeParams& treeparams)
    : datacontainer_(datadistparams),
      tree_(treeparams),
      residuals_(datacontainer_.get_data().nrow(), 0) {}

CGBMEngine::~CGBMEngine() {}

void CGBMEngine::FitLearner(double* func_estimate, double& trainingerror,
                      double& validationerror, double& outofbag_improvement) {
  trainingerror = 0.0;
  validationerror = 0.0;
  outofbag_improvement = 0.0;

  // Initialize adjustments to function estimate
  std::vector<double> delta_estimates(datacontainer_.get_data().nrow(), 0);

  // Bag data
  datacontainer_.BagData();

  // Set up tree
  tree_.Reset();

  // Compute Residuals and fit tree
  datacontainer_.ComputeResiduals(&func_estimate[0], residuals_);
  tree_.Grow(residuals_, datacontainer_.get_data(), &delta_estimates[0]);

  // Now I have adF, adZ, and vecpTermNodes (new node assignments)
  // Fit the best constant within each terminal node

  // Adjust terminal node predictions and shrink
  datacontainer_.ComputeBestTermNodePreds(&func_estimate[0], residuals_,
                                          tree_);
  tree_.Adjust(&delta_estimates[0]);

  // Compute the error improvement within bag
  outofbag_improvement = datacontainer_.ComputeBagImprovement(
      &func_estimate[0], tree_.get_shrinkage_factor(), &delta_estimates[0]);

  // Update the function estimate
  unsigned long i = 0;
  for (i = 0; i < datacontainer_.get_data().get_trainsize(); i++) {
    func_estimate[i] += tree_.get_shrinkage_factor() * delta_estimates[i];
  }

  // Make validation predictions
  trainingerror = datacontainer_.ComputeDeviance(&func_estimate[0], false);
  tree_.PredictValid(datacontainer_.get_data(),
                     datacontainer_.get_data().get_validsize(),
                     &delta_estimates[0]);

  for (i = datacontainer_.get_data().get_trainsize();
       i < datacontainer_.get_data().get_trainsize() +
               datacontainer_.get_data().get_validsize();
       i++) {
    func_estimate[i] += delta_estimates[i];
  }

  validationerror = datacontainer_.ComputeDeviance(&func_estimate[0], true);
}

void CGBMEngine::GbmTransferTreeToRList(int* splitvar, double* splitvalues,
                                  int* leftnodes, int* rightnodes,
                                  int* missingnodes, double* error_reduction,
                                  double* weights, double* predictions,
                                  VecOfVectorCategories& splitcodes_vec,
                                  int prev_categorical_splits) {
  tree_.TransferTreeToRList(datacontainer_.get_data(), splitvar, splitvalues,
                            leftnodes, rightnodes, missingnodes,
                            error_reduction, weights, predictions,
                            splitcodes_vec, prev_categorical_splits);
}
