//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#include "../TPGBrain/TPGBrain.h"
#include "../../Utilities/Utilities.h"

long TPGBrain::Node::nextNodeID = 0;
long TPGBrain::Program::nextProgramID = 0;

std::shared_ptr<ParameterLink<int>> TPGBrain::hiddenCountPL =
Parameters::register_parameter("BRAIN_TPG-hiddenCount",
	4, "number of hidden nodes in the TPG brains");
std::shared_ptr<ParameterLink<int>> TPGBrain::initalProgramsPL =
Parameters::register_parameter("BRAIN_TPG-initalPrograms",
	100, "number programs to generate at initialization");
std::shared_ptr<ParameterLink<int>> TPGBrain::initalNodesPL =
Parameters::register_parameter("BRAIN_TPG-initalNodes",
	100, "number nodes to generate at initialization");

std::shared_ptr<ParameterLink<int>> TPGBrain::Program::numInstructionPL =
Parameters::register_parameter("BRAIN_TPG_PROGRAM-numInstruction",
	5, "fixed number of instructions in the psudo-CGP programs");
std::shared_ptr<ParameterLink<int>> TPGBrain::Program::registersSizePL =
Parameters::register_parameter("BRAIN_TPG_PROGRAM-registersSize",
	8, "size of regiter array for the psudo-CGP programs");
std::shared_ptr<ParameterLink<bool>> TPGBrain::Program::allowRandomOpPL =
Parameters::register_parameter("BRAIN_TPG_PROGRAM-allowRandomOp",
	false, "if true, the random op will be avalible in the psudo-CGP programs");

std::shared_ptr<ParameterLink<double>> TPGBrain::Program::mutateInstructionCodeChancePL =
Parameters::register_parameter("BRAIN_TPG_PROGRAM-mutateInstructionCodeChance",
	.2, "chance that an instrcution codon will be altered by mutation");
std::shared_ptr<ParameterLink<double>> TPGBrain::Program::mutateRegisterPresetChancePL =
Parameters::register_parameter("BRAIN_TPG_PROGRAM-mutateRegisterPresetChance",
	.2, "chance that one preset value in the register array will be altered by mutation");
std::shared_ptr<ParameterLink<double>> TPGBrain::Program::mutateActionChancePL =
Parameters::register_parameter("BRAIN_TPG_PROGRAM-mutateActionChance",
	.2, "chance that the action result of this program will be altered by mutation");

std::shared_ptr<ParameterLink<double>> TPGBrain::Node::mutateAddProgramChancePL =
Parameters::register_parameter("BRAIN_TPG_NODE-mutateAddProgramChance",
	.3, "chance mutation will add a program");
std::shared_ptr<ParameterLink<double>> TPGBrain::Node::mutateTradeProgramChancePL =
Parameters::register_parameter("BRAIN_TPG_NODE-mutateTradeProgramChance",
	.3, "chance a mutation will replace on program with another");
std::shared_ptr<ParameterLink<double>> TPGBrain::Node::mutateMutateProgramChancePL =
Parameters::register_parameter("BRAIN_TPG_NODE-mutateMutateProgramChance",
	.1, "chance a program will be replaced by a mutated copy of itself");
std::shared_ptr<ParameterLink<double>> TPGBrain::Node::mutateTradeAndMutateProgramChancePL =
Parameters::register_parameter("BRAIN_TPG_NODE-mutateTradeAndMutateProgramChance",
	.1, "chance a program will be replaced by a mutated copy of a diffrent program");
std::shared_ptr<ParameterLink<double>> TPGBrain::Node::mutateDeleteProgramChancePL =
Parameters::register_parameter("BRAIN_TPG_NODE-mutateDeleteProgramChance",
	.2, "chance a program will be deleted");
std::shared_ptr<ParameterLink<int>> TPGBrain::Node::maxProgramsPL =
Parameters::register_parameter("BRAIN_TPG_NODE-maxPrograms",
	6, "max number of programs referenced by a Node");
std::shared_ptr<ParameterLink<int>> TPGBrain::Node::minProgramsPL =
Parameters::register_parameter("BRAIN_TPG_NODE-minPrograms",
	2, "min number of programs referenced by a Node");

// this will be called by main (which does not know about nodes to create inital population)
std::shared_ptr<AbstractBrain> TPGBrain::makeBrain(
	std::unordered_map<std::string, 
	std::shared_ptr<AbstractGenome>> &_genomes) {
	auto avaliblePrograms = *rootNode->allPrograms;
	auto avalibleNodes = *avaliblePrograms[0]->allNodes;
	auto newRootNode = avalibleNodes[Random::getIndex(avalibleNodes.size())];
	return std::make_shared<TPGBrain>(nrInputValues, nrOutputValues, nrHidden, newRootNode, PT);
}

void TPGBrain::update() {
	std::vector<std::shared_ptr<Node>> visitedNodes; // list of nodes visited this update

	auto currentNode = rootNode;
	bool foundAtomic = false;

	while (!foundAtomic) {
		//std::cout << "in loop    currentNode has " << currentNode->programs.size() << " programs." << std::endl;
		if (currentNode->visitCounter == 0) { // first time at this node in this update. run and rank programs
											  //std::cout << "visit couter is 0" << std::endl;

			visitedNodes.push_back(currentNode); // add to list of nodes to clean up later.

			std::vector<std::pair<size_t, double>> programBids; // index in programs list, value generated by program
			for (size_t i = 0; i < currentNode->programs.size(); i++) {
				programBids.push_back({ i, currentNode->programs[i]->evaluate(inputValues,hiddenValues) });
			}
			//std::cout << "got program bids" << std::endl;

			for (size_t i = 0; i < currentNode->programs.size(); i++) {
				double maxBid = programBids[0].second;
				size_t maxBidIndex = 0;
				for (size_t j = 1; j < programBids.size(); j++) { // find highest in remaining bids
					if (programBids[j].second > maxBid) {
						maxBid = programBids[j].second;
						maxBidIndex = j;
					}
				}

				currentNode->programOrder.push_back(programBids[maxBidIndex].first);
				programBids[maxBidIndex] = programBids.back();
				programBids.pop_back();
			}
			//std::cout << "reorder is done" << std::endl;
			//for (auto po : currentNode->programOrder) {
			//	std::cout << "(" << currentNode->ID << ") program #: "<< po << " (" << currentNode->programs[po]->ID << " has score: " << currentNode->programs[po]->evaluate(inputValues, hiddenValues) << "  and action type: " << currentNode->programs[po]->actionType << std::endl;
			//}
		} // end run and rank programs

		  // follow program indicated by visit
		  // if visit = programs.size() return Atomic 0

		if (currentNode->programs.size() <= currentNode->visitCounter) {
			//std::cout << "no more programs - we are done!" << std::endl;
			resetOutputs();
			foundAtomic = true;
		}
		else { // follow path of wining program
			if (currentNode->programs[currentNode->programOrder[currentNode->visitCounter]]->actionType == 0) { // this program references an atomic action
																												//std::cout << "found atomic!  " << currentNode->programs[currentNode->programOrder[currentNode->visitCounter]]->targetAtomic << " : " << currentNode->programs[currentNode->programOrder[currentNode->visitCounter]]->targetHidden << std::endl;
				auto newOutput = intToBoolVector(currentNode->programs[currentNode->programOrder[currentNode->visitCounter]]->targetAtomic);
				auto newHidden = intToBoolVector(currentNode->programs[currentNode->programOrder[currentNode->visitCounter]]->targetHidden);

				std::fill(outputValues.begin(), outputValues.end(), 0); // insure that they are empty in case newValues don't set all bits
				std::fill(hiddenValues.begin(), hiddenValues.end(), 0);

				//std::cout << newOutput.size() << "  " << newHidden.size() << std::endl;
				//std::cout << outputValues.size() << "  " << hiddenValues.size() << std::endl;
				for (size_t i = 0; i < newOutput.size(); i++) {
					//	std::cout << i << ":" << newOutput[i] << " " << std::flush;
					outputValues[i] = newOutput[i];
				}
				//std::cout << std::endl;
				for (size_t i = 0; i < newHidden.size(); i++) {
					//std::cout << i << ":" << newHidden[i] << " " << std::flush;
					hiddenValues[i] = newHidden[i];
				}
				//std::cout << std::endl;
				//std::cout << "done atomic!" << std::endl;

				foundAtomic = true;
			}
			else { // this program reference a node
				//   std::cout << "found node!  " << currentNode->programs[currentNode->programOrder[currentNode->visitCounter]]->targetNode->ID << std::endl;
				currentNode->visitCounter++;
				currentNode = currentNode->programs[currentNode->programOrder[currentNode->visitCounter - 1]]->targetNode;
				//std::cout << "currentNode advaced!" << std::endl;
			}
		}
	}

	for (auto n : visitedNodes) { // we are done, clean up this mess!
		n->visitCounter = 0;
		n->programOrder.clear();
	}
}

std::shared_ptr<AbstractBrain>
TPGBrain::makeCopy(std::shared_ptr<ParametersTable> PT_) {
  if (PT_ == nullptr) {
    PT_ = PT;
  }
  return std::make_shared<TPGBrain>(nrInputValues, nrOutputValues, hiddenCountPL->get(PT), rootNode, PT_);;
}

void TPGBrain::DetectParts(const std::shared_ptr<Node> &n,
	std::set<std::shared_ptr<Node>> &saveNodes, 
	std::set<std::shared_ptr <Program>> &savePrograms,
	std::set<std::string> &visitedElements) {

	auto nName = "N" + std::to_string(n->ID);
	visitedElements.insert(nName);
	saveNodes.insert(n);
	for (auto p : n->programs) {
		auto pName = "P" + std::to_string(p->ID);
		if (visitedElements.find(pName) == visitedElements.end()) { // if this program is not known
			visitedElements.insert(pName);
			savePrograms.insert(p);
			if (p->actionType == 1) {
				auto tnName = "N" + std::to_string(p->targetNode->ID);
				if (visitedElements.find(tnName) == visitedElements.end()) {
					DetectParts(p->targetNode, saveNodes, savePrograms, visitedElements);
				}
			}
		}
	}


};

// convert a brain into data map with data that can be saved to file
// format
//  nodes = ID#pID:pID:...:|ID-pID:pID:...:...|...
//  programs = ID#aType-[aOut/aHid or nID]#registerPreset:registerPreset:#instructionCode:instructionCodes:|...|...
DataMap TPGBrain::serialize(std::string &name) {
	DataMap dataMap;
	std::shared_ptr<Node> n = rootNode;
	std::set<std::shared_ptr<Node>> saveNodes;
	std::set<std::shared_ptr<Program>> savePrograms;
	std::set<std::string> visitedElements;

	dataMap.set("rootNode", std::to_string(rootNode->ID));

	DetectParts(n, saveNodes, savePrograms, visitedElements);

	std::stringstream ss;
	ss << saveNodes.size() << "#"; // how many nodes?
	for (auto n : saveNodes) {
		ss << n->ID << "#" << n->programs.size() << "#"; // how many programs in this node
		for (auto p : n->programs) {
			ss << p->ID << "#";
		}
	}
	dataMap.set("nodes", ss.str());
	ss.str(std::string()); // clear ss
	ss << savePrograms.size() << "#"; // how many programs
	for (auto p : savePrograms) {
		ss << p->ID << "#" << p->actionType << "#";
		if (p->actionType == 0){ // atomic
			//if atomic then save output value / hidden value
			ss << p->targetAtomic << "#" << p->targetHidden << "#";
		} else { // node
			// if node then node id
			ss << p->targetNode->ID << "#";
		}
		for (auto val : p->registerPresets) {
			// write each preset program value
			ss << val << "#";
		}
		// # to sperate preset values and instruction codes
		for (auto val : p->instructionCodes) {
			// write each instruction code
			ss << val << "#";
		}
	}
	dataMap.set("programs", ss.str());

	return dataMap;
}

// given an unordered_map<string, string> and PT, load data into this brain
void TPGBrain::deserialize(std::shared_ptr<ParametersTable> PT,
	std::unordered_map<std::string, std::string> &orgData,
	std::string &name) {
/*
// this is too much of a pain in the ass to write. it can be done
// if needed, but at the moment, I don't see the value in it.
// below is the begining of the code needed
// the complexity comes from the fact that the nodes
// and programs lists saved with each org are the nodes
// and programs for that org. when being deserialized checks
// must be preformed for each node and program to see if
// they are already in allNodes or allPrograms. Also
// nodes and programs must be correctly linked (this is
// a pain. Perhaps a better way would be to save all
// nodes and programs will every org. Then if !fromFile
// would result in a read of all programs and nodes and
// assignment of rootNode for that first brain. Thereafter
// fromFile == true, so all that would be needed to be set
// is the rootNode for each Brain.

	if (name == "root::") {
		name = "";
	}
	std::string nodesName = name + "nodes";
	std::string programsName = name + "programs";
	std::vector<std::string> nodes_data;
	convertCSVListToVector(orgData[nodesName],nodes_data,'#');
	std::vector<std::string> programs_data;
	convertCSVListToVector(orgData[programsName], programs_data,'#');

	auto temp_allNodes = (*rootNode->allPrograms)[0]->allNodes;
	//if this is the first brain being deserialized, reset programs and brains
	if (!rootNode->fromFile) {
		(*rootNode->allPrograms)[0]->allNodes->clear();
		rootNode->allPrograms->clear();
	}

	size_t read_index = 0; // use this to read nodes and programs
	int nCount, nID, pCount, value;
	stringToValue(nodes_data[read_index++], nCount);
	for (int i = 0; i < nCount; i++) {
		// for each node
		stringToValue(nodes_data[read_index++], nID);
		// if node is in allNodes
		bool new_node = true;
		for (auto const & n : *temp_allNodes) {
			if (n->ID == nID) {
				new_node == false;
			}
		}
		if (new_node == false) {
			// skip to next node. first get number of programs in this node
			stringToValue(nodes_data[read_index++], pCount);
			// now skip that number of values
			read_index += pCount;
		}
		else {
			// this node needs to be added to temp_allNodes;

		}
	}



	*/

	// read out the nodes
	
	//std::cout << "nodes_data: " << nodes_data << std::endl;
	//std::cout << "programs_data: " << programs_data << std::endl;

	//
}

