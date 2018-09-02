//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#include "TPGOptimizer.h"

std::shared_ptr<ParameterLink<std::string>> TPGOptimizer::optimizeValuePL =
    Parameters::register_parameter("OPTIMIZER_TPG-optimizeValue",
                                   (std::string) "DM_AVE[score]",
                                   "value to optimize (MTree)");
std::shared_ptr<ParameterLink<int>> TPGOptimizer::newNodesTargetPL =
Parameters::register_parameter("OPTIMIZER_TPG-newNodesTarget",
	100, "this many nodes will be generated from previous root nodes each generation (up to maxNodesAllowed)");
std::shared_ptr<ParameterLink<int>> TPGOptimizer::maxNodesAllowedPL =
Parameters::register_parameter("OPTIMIZER_TPG-maxNodesAllowed",
	500, "maximum number of nodes allowed at any time (root and non-root)");
std::shared_ptr<ParameterLink<int>> TPGOptimizer::saveReportOnPL =
Parameters::register_parameter("OPTIMIZER_TPG-saveReportOn",
	1000, "a report with info on nodes and programs will be saved to output when update%saveReportOn == 0");
std::shared_ptr<ParameterLink<int>> TPGOptimizer::saveFullGraphOnPL =
Parameters::register_parameter("OPTIMIZER_TPG-saveFullGraphOn",
	1000, "save a graph with all nodes, programs and, atomics when update%saveFullGraphOn == 0");
std::shared_ptr<ParameterLink<int>> TPGOptimizer::saveBestOnPL =
Parameters::register_parameter("OPTIMIZER_TPG-saveBestOn",
	1000, "save a graph of nodes,programs and, atomics associated with high score node when update%saveBestOn == 0");
std::shared_ptr<ParameterLink<int>> TPGOptimizer::saveBest3OnPL =
Parameters::register_parameter("OPTIMIZER_TPG-saveBest3On",
	1000, "save a graph of nodes,programs and, atomics associated with high score node when update%saveBest3On == 0");

TPGOptimizer::TPGOptimizer(std::shared_ptr<ParametersTable> PT_)
    : AbstractOptimizer(PT_) {

	newNodesTarget = newNodesTargetPL->get(PT);
	maxNodesAllowed = maxNodesAllowedPL->get(PT);

	saveReportOn = saveReportOnPL->get(PT);
	saveFullGraphOn = saveFullGraphOnPL->get(PT);
	saveBestOn = saveBestOnPL->get(PT);
	saveBest3On = saveBest3OnPL->get(PT);

	optimizeValueMT = stringToMTree(optimizeValuePL->get(PT));
	optimizeFormula = optimizeValueMT;

	popFileColumns.clear();
	popFileColumns.push_back("optimizeValue");
}

void TPGOptimizer::optimize(std::vector<std::shared_ptr<Organism>> &population) {
	aveScore = 0;
	orgScores.clear();
	maxScore = optimizeValueMT->eval(population[0]->dataMap, PT)[0];
	for (size_t i = 0; i < population.size(); i++) {


		double opVal = optimizeValueMT->eval(population[i]->dataMap, PT)[0];
		population[i]->dataMap.set("optimizeValue", opVal);
		orgScores.push_back({ i, opVal });
		aveScore += opVal;
		maxScore = std::max(maxScore, opVal);
		//std::cout << i << ": " << opVal << std::endl;
	}
	aveScore /= population.size();

	// we will do all the rest of the work work in cleanup
}

void saveSomeNodes(std::vector<std::shared_ptr<Organism>> orgs, std::vector<int> rankOrder, std::string graphFileName) {
	std::string graphEdges;
	std::string graphProps;
	std::set<std::string> visitedElements;
	graphEdges = "digraph graphname {\n";
	for (auto org : orgs) {
		auto b = std::dynamic_pointer_cast<TPGBrain>(org->brains["root::"]);
		auto n = b->rootNode;
		b->graphTPG(n, visitedElements, graphEdges, graphProps);
	}
	graphProps += "}\n";

	//std::string graphFileName = "best_NaP_" + std::to_string(Global::update) + ".dot";
	FileManager::writeToFile(graphFileName, graphEdges);
	FileManager::writeToFile(graphFileName, graphProps);
}

void saveAllNodesAndPrograms(
	std::shared_ptr<std::vector<std::shared_ptr<TPGBrain::Node>>> allNodes,
	std::shared_ptr<std::vector<std::shared_ptr<TPGBrain::Program>>> allPrograms,
	std::string graphFileName) {
	std::string graphEdges;
	std::string graphProps;
	graphEdges = "digraph graphname {\n";
	for (auto n : (*allNodes)) {
		graphProps += "N" + std::to_string(n->ID) + " [color=yellow,shape=ellipse,style=filled]\n";
		for (auto p : n->programs) {
			graphEdges += "  N" + std::to_string(n->ID) + " -> P" + std::to_string(p->ID) + "\n";
		}
	}
	for (auto p : (*allPrograms)) {
		graphProps += "P" + std::to_string(p->ID) + " [color=springGreen,shape=ellipse,style=filled]\n";
		if (p->actionType == 0) {
			graphProps += "A" + std::to_string(p->targetAtomic) + "h" + std::to_string(p->targetHidden) + " [color=lightgrey,shape=component,style=filled]\n";
			graphEdges += "  P" + std::to_string(p->ID) + " -> A" + std::to_string(p->targetAtomic) + "h" + std::to_string(p->targetHidden) + "\n";
		}
		else {
			graphEdges += "  P" + std::to_string(p->ID) + " -> N" + std::to_string(p->targetNode->ID) + "\n";
		}
	}
	graphProps += "}\n";
	FileManager::writeToFile(graphFileName, graphEdges);
	FileManager::writeToFile(graphFileName, graphProps);
}

void TPGOptimizer::cleanup(std::vector<std::shared_ptr<Organism>> &population) {
	std::vector<std::shared_ptr<Organism>> newPopulation;
	
	
	// dynamicly cast org 0s brain, and get
	auto exampleBrain = std::dynamic_pointer_cast<TPGBrain>(population[0]->brains["root::"]);
	if (exampleBrain->rootNode == nullptr) { // if eampleBrain is nullptr there was an error in setup.
		std::cout << "in TPG Optimizer, example brain is nullptr. This is bad." << std::endl;
		std::exit(1);
	}
	// get allNodes and allProgrms
	auto allPrograms = exampleBrain->rootNode->allPrograms;
	auto allNodes = exampleBrain->rootNode->programs[0]->allNodes;


	// get rank order for all orgs (with a really terrible algorithom)
	std::vector<int> rankOrder;
	for (size_t i = 0; i < population.size(); i++) {
		double currMaxScore = orgScores[0].second;
		size_t maxScoreIndex = 0;
		for (size_t j = 1; j < orgScores.size(); j++) { 
			if (orgScores[j].second > currMaxScore) {
				currMaxScore = orgScores[j].second;
				maxScoreIndex = j;
			}
		}
		rankOrder.push_back(orgScores[maxScoreIndex].first);
		orgScores[maxScoreIndex] = orgScores.back();
		orgScores.pop_back();
	}

	// see if we need to save any graphs.
	if (Global::update % saveBestOn == 0) {  // save graph of best rootNode
		auto orgs = { population[rankOrder[0]] };
		saveSomeNodes(orgs, rankOrder, "best_NaP_" + std::to_string(Global::update) + ".dot");
	}
	if (Global::update % saveBest3On == 0) {  // save graph of best 3 rootNodes
		auto orgs = { population[rankOrder[0]] , population[rankOrder[1]]  , population[rankOrder[2]] };
		saveSomeNodes(orgs, rankOrder, "best3_NaP_" + std::to_string(Global::update) + ".dot");
	}
	if (Global::update % saveFullGraphOn == 0) {  // save graph of all nodes and programs
		saveAllNodesAndPrograms(allNodes, allPrograms, "All_NaP_" + std::to_string(Global::update) + ".dot");
	}

	int newNodesCount = 0;

	// remove all root nodes from allNodes. some will be put back (and cloned/mutated)
	for (size_t i = 0; i < (*allNodes).size(); ) {
		auto node = (*allNodes)[i];
		if (node->parentCount == 0) {// this is a root node
			for (auto p : node->programs) {
				//std::cout << " " << n->ID << ":" << p->ID << ":" << p->parentCount << "   ->   ";
				p->parentCount--;
				//std::cout << " " << n->ID << ":" << p->ID << ":" << p->parentCount << std::endl;
			}
			(*allNodes)[i] = (*allNodes).back();
			(*allNodes).pop_back();
		}
		else { // if not a root node, advance to next node in allNodes
			i++;
		}
	}

	// now we will use ranked orgs in population to get parents. for each parent, clone and then clone and mutate each 
	while (newNodesCount < newNodesTarget && (*allNodes).size() < maxNodesAllowed) {
		// get a brain
		auto subjectBrain = std::dynamic_pointer_cast<TPGBrain>(population[rankOrder[(int)(std::pow(Random::getDouble(1.0), 1.5) * rankOrder.size())]]->brains["root::"]);
		// add this brains node and a mutated clone to allNodes
		(*allNodes).push_back(subjectBrain->rootNode->clone());
		newNodesCount++;
		(*allNodes).push_back(subjectBrain->rootNode->cloneAndMutate());
		newNodesCount++;
	}

	// delete unused programs
	int programsDeleted = 0;
	for (size_t i = 0; i < (*allPrograms).size();) {
		if ((*allPrograms)[i]->parentCount == 0) { // noone is using this program, we can delete it.
			if ((*allPrograms)[i]->actionType == 1) {
				(*allPrograms)[i]->targetNode->parentCount--;
			}
			(*allPrograms)[i] = (*allPrograms).back();
			(*allPrograms).pop_back();
			programsDeleted++;
		}
		else {
			i++; // a node is pointing at this, so we need to keep it.
		}
	}

	// make a new population from new root nodes
	int rootNodeCount = 0;
	population.clear();
	for (size_t i = 0; i < (*allNodes).size(); ) {
		auto node = (*allNodes)[i];
		if (node->parentCount == 0) {// this is a root node
			auto newOrg = std::make_shared<Organism>(PT);
			newOrg->brains["root::"] = std::make_shared<TPGBrain>(exampleBrain->nrInputValues, exampleBrain->nrOutputValues, exampleBrain->nrHidden, node, 0, PT);
			population.push_back(newOrg);
			rootNodeCount++;
		}
		i++;
	}

	if (Global::update % saveReportOn == 0) {
		for (auto n : (*allNodes)) {
			std::cout << "node: " << n->ID << " has " << n->programs.size() << " programs (";
			for (auto p : n->programs) {
				std::cout << p->ID << " ";
			}
			std::cout << ") and " << n->parentCount << " parents." << std::endl;
		}
		for (auto p : (*allPrograms)) {
			std::cout << "program: " << p->ID << " has " << p->parentCount << " parents. output is ";
			if (p->actionType == 0) {
				std::cout << "atomic: " << p->targetAtomic << " " << p->targetHidden << std::endl;
			}
			else {
				std::cout << "node: " << p->targetNode->ID << std::endl;
			}
		}
	}
	std::cout << "\n    maxScore: " << maxScore << "    aveScore: " << aveScore << std::endl;
	std::cout << "    nodes: " << (*allNodes).size() << "   rootNodes: " << rootNodeCount << "   programs: " << (*allPrograms).size() << "   after " << programsDeleted << " were deleted." << std::endl;
}

