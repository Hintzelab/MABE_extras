//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#include "../AbstractOptimizer.h"
#include "../../Brain/TPGBrain/TPGBrain.h"
#include "../../Utilities/MTree.h"

#include <iostream>
#include <sstream>

class TPGOptimizer : public AbstractOptimizer {
public:
	static std::shared_ptr<ParameterLink<std::string>> optimizeValuePL; // what value is used to generate

	static std::shared_ptr<ParameterLink<int>> saveReportOnPL;
	static std::shared_ptr<ParameterLink<int>> saveFullGraphOnPL;
	static std::shared_ptr<ParameterLink<int>> saveBestOnPL;
	static std::shared_ptr<ParameterLink<int>> saveBest3OnPL;
	static std::shared_ptr<ParameterLink<int>> newNodesTargetPL;
	static std::shared_ptr<ParameterLink<int>> maxNodesAllowedPL;

  std::shared_ptr<Abstract_MTree> optimizeValueMT;


  int newNodesTarget;
  int maxNodesAllowed;

  int saveReportOn;
  int saveFullGraphOn;
  int saveBestOn;
  int saveBest3On;

  std::vector<std::pair<size_t, double>> orgScores;
  double aveScore;
  double maxScore;

  TPGOptimizer(std::shared_ptr<ParametersTable> PT_ = nullptr);

  virtual void optimize(std::vector<std::shared_ptr<Organism>> &population) override;


  virtual void cleanup(std::vector<std::shared_ptr<Organism>> &population);

  // virtual string maxValueName() override {
  //	return (PT == nullptr) ? optimizeValuePL->lookup() :
  //PT->lookupString("OPTIMIZER_Simple-optimizeValue");
  //}
};

