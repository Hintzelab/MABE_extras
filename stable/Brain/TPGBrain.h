//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#pragma once

#include <cmath>
#include <memory>
#include <iostream>
#include <set>
#include <vector>

#include "../../Genome/AbstractGenome.h"

#include "../../Utilities/Random.h"

#include "../AbstractBrain.h"

class TPGBrain : public AbstractBrain {
public:

	static std::shared_ptr<ParameterLink<int>> hiddenCountPL;
	static std::shared_ptr<ParameterLink<int>> initalProgramsPL;
	static std::shared_ptr<ParameterLink<int>> initalNodesPL;

	class Node;

	void graphTPG(std::shared_ptr<Node> n, std::set<std::string> &visitedElements, std::string &graphEdges, std::string &graphProps) {
		auto nName = "N" + std::to_string(n->ID);
		visitedElements.insert(nName);
		graphProps += nName + " [color=yellow,shape=ellipse,style=filled]\n";
		for (auto p : n->programs) {
			auto pName = "P" + std::to_string(p->ID);
			graphEdges += "  " + nName + " -> " + pName + "\n";
			if (visitedElements.find(pName) == visitedElements.end()) { // if this program is not known
				visitedElements.insert(pName);
				graphProps += pName + " [color=springGreen,shape=ellipse,style=filled]\n";

				if (p->actionType == 0) {
					auto aName = "A" + std::to_string(p->targetAtomic) + "h" + std::to_string(p->targetHidden);
					graphEdges += "  " + pName + " -> " + aName + "\n";
					if (visitedElements.find(aName) == visitedElements.end()) {
						visitedElements.insert(aName);
						graphProps += aName + " [color=lightgrey,shape=component,style=filled]\n";
					}
				}
				else {
					auto tnName = "N" + std::to_string(p->targetNode->ID);
					graphEdges += "  " + pName + " -> " + tnName + "\n";
					if (visitedElements.find(tnName) == visitedElements.end()) {
						visitedElements.insert(tnName);
						graphTPG(p->targetNode, visitedElements, graphEdges, graphProps); // repeat process for this node;
					}
				}
			}
		}
	}

	std::vector <bool> intToBoolVector(int value, bool flipOrder = false) {
		std::vector <bool> result;
		while (value > 0) { // pull out one bit at a time creating vector with least signifigent bit in the first position
			result.push_back(value & 1);
			value >>= 1;
			//std::cout << value << "  " << result.size() << "  " << result.back() << std::endl;
		}
		if (flipOrder) {
			for (int i = 0; i < int(result.size()) / 2; i++) { // flip vector (edcba -> abdce)
				auto temp = result[i];
				result[i] = result[(result.size() - 1) - i];
				result[(result.size() - 1) - i] = temp;
			}
		}
		return result;
	}

	class Program {
		// okay here we go. fast cgp...
		// 20 values (5 actions)
		// 8 registers (initally values 0 -> 1, and then point mutated)
		// operations:
		// 0 : +, 1 : -, 2 : *, 3 : /, 4 : sin, 5 : if, 6 : * -1, 7 : random()
		// operands inputValues.size()+hiddenValues.size()+registers.size();
		// output registers.size()
		// operator, operand, operand, output, operator, operand, operand, output, ... operator, operand, operand
		// last value generated is output.

	public:
		long ID;
		int inputCount, outputCount, hiddenCount;
		// code here
		int actionType; // 0 = atomic, 1 = node
		std::shared_ptr<Node> targetNode = nullptr;
		std::shared_ptr<std::vector<std::shared_ptr<Node>>> allNodes;

		long long targetAtomic; // either actual value of output, or index into atomics list
		long long targetHidden; // if action is atomic, then set hidden to this.
		int parentCount = 0; // number of Nodes referencing this Program

		std::vector<int> instructionCodes;
		std::vector<double> registerPresets;

		static std::shared_ptr<ParameterLink<int>> numInstructionPL;
		static std::shared_ptr<ParameterLink<int>> registersSizePL;
		static std::shared_ptr<ParameterLink<bool>> allowRandomOpPL;

		static std::shared_ptr<ParameterLink<double>> mutateInstructionCodeChancePL;
		static std::shared_ptr<ParameterLink<double>> mutateRegisterPresetChancePL;
		static std::shared_ptr<ParameterLink<double>> mutateActionChancePL;


		int numInstructions;
		int numInstructionsCodes; // 4 values op,in1,in2,out per instruction
		int registersSize;
		bool allowRandomOp;

		int numOps; // change to 8 to allow random op

		double mutateInstructionCodeChance;
		double mutateRegisterPresetChance;
		double mutateActionChance;

		static long nextProgramID;

		Program(int inputCount_, int outputCount_, int hiddenCount_, std::shared_ptr<std::vector<std::shared_ptr<Node>>> allNodes_)
			:inputCount(inputCount_), outputCount(outputCount_), hiddenCount(hiddenCount_), allNodes(allNodes_) {

			mutateInstructionCodeChance = mutateInstructionCodeChancePL->get();
			mutateRegisterPresetChance = mutateRegisterPresetChancePL->get();
			mutateActionChance = mutateActionChancePL->get();
			registersSize = registersSizePL->get();

			numInstructions = numInstructionPL->get();
			numInstructionsCodes = numInstructions * 4; // 4 values op,in1,in2,out per instruction

			allowRandomOp = allowRandomOpPL->get();
			numOps = allowRandomOp ? 8 : 7; // change to 8 to allow random op

			ID = nextProgramID++;
			for (int i = 0; i < numInstructionsCodes; i++) {
				instructionCodes.push_back(Random::getIndex(256));
			}
			for (int i = 0; i < registersSize; i++) {
				registerPresets.push_back(Random::getDouble(1.0));
			}
			actionType = 0; // initaly all programs have atomic actions
			targetAtomic = Random::getIndex(std::pow(2.0, (double(outputCount))));
			targetHidden = Random::getIndex(std::pow(2.0, (double(hiddenCount)))); // set this to the bit size or index from list
		}

		std::shared_ptr<Program> clone() {
			auto newProgram = std::make_shared<Program>(inputCount, outputCount, hiddenCount, allNodes);
			newProgram->actionType = actionType;
			newProgram->targetAtomic = targetAtomic;
			newProgram->targetHidden = targetHidden;
			newProgram->targetNode = targetNode;
			newProgram->instructionCodes = instructionCodes;
			newProgram->registerPresets = registerPresets;
			if (newProgram->actionType == 1) {
				newProgram->targetNode->parentCount++;
			}
			return newProgram;
		}

		void mutate() {
			bool mutated = false;

			while (!mutated) {
				// change an instructionCode
				if (Random::P(mutateInstructionCodeChance)) {
					instructionCodes[Random::getIndex(instructionCodes.size())] = Random::getIndex(256);
					mutated = true;
				}
				// change a registerPreset
				if (Random::P(mutateRegisterPresetChance)) {
					registerPresets[Random::getIndex(registerPresets.size())] = Random::getDouble(1.0);
					mutated = true;
				}
				// change actionType / targetNode / targetAtomic / targetHidden
				if (Random::P(mutateActionChance)) {
					if (actionType == 1) { // if action type was node, then we must decrese the parent Count before we unlink
						targetNode->parentCount--;
						targetNode = nullptr;
					}
					actionType = Random::getIndex(2);
					if (actionType == 0) { // get new atomic values
						targetAtomic = Random::getIndex(std::pow(2.0, (double(outputCount)))); // set this to the bit size or index from list
						targetHidden = Random::getIndex(std::pow(2.0, (double(hiddenCount)))); // set this to the bit size or index from list
					}
					else { // get a new node
						   // why not clone targetNode if it's root?
						   // (because it would require clone node and it's programs, but not mutation and we would end up with perfect copies...)

						   // this should not be needed, we already dropped this above if actionType was 1
						   //if (targetNode != nullptr) {
						   //    targetNode->parentCount--;
						   //}
						targetNode = (*allNodes)[Random::getIndex((*allNodes).size())];
						targetNode->parentCount++;
					}
					mutated = true;
				}
			}
		}

		std::shared_ptr<Program> cloneAndMutate() {
			auto newProgram = clone();
			newProgram->mutate();
			return newProgram;
		}

		double evaluate(std::vector<double> inputValues, std::vector<double> hiddenValues) {
			std::vector<double> registers = registerPresets; // load registers with 1.0
			for (size_t i = 0; i < inputValues.size(); i++) {
				registers.push_back(inputValues[i]);
			}
			for (size_t i = 0; i < hiddenCount; i++) {
				registers.push_back(hiddenValues[i]);
			}
			for (size_t i = 0; i < numInstructions; i++) {
				double operand1 = registers[instructionCodes[(i * 4) + 1] % registers.size()];
				double operand2 = registers[instructionCodes[(i * 4) + 2] % registers.size()];
				int outputIndex = instructionCodes[(i * 4) + 3] % registerPresets.size();

				switch (instructionCodes[(i * 4)] % numOps) {
				case 0: // +
					registers[outputIndex] = operand1 + operand2;
					break;
				case 1: // -
					registers[outputIndex] = operand1 - operand2;
					break;
				case 2: // *
					registers[outputIndex] = operand1 * operand2;
					break;
				case 3: // /
					if (operand2 < (std::numeric_limits<double>::min() * 2)) {
						registers[outputIndex] = 0;
					}
					else {
						registers[outputIndex] = operand1 / operand2;
					}
					break;
				case 4: // sin+cos
					registers[outputIndex] = std::sin(operand1) + std::cos(operand2);
					break;
				case 5: // if o1 > o2 ? 1 : 0
					registers[outputIndex] = (operand1 > operand2) ? 1 : 0;
					break;
				case 6: // * -1 o1 (ignore o2)
					registers[outputIndex] = -1 * operand1;
					break;
				case 7: // random in range of (o1,o2)
					registers[outputIndex] = Random::getDouble(std::min(operand1, operand2), std::max(operand1, operand2));
					break;
				} // end switch
			} // end operations loop
			return registers[instructionCodes.back() % registersSize];
		} // end evaluate function
	}; // end program

	class Node {
	public:
		int ID;
		std::shared_ptr<std::vector<std::shared_ptr<Program>>> allPrograms;
		std::vector<std::shared_ptr<Program>> programs;
		std::vector<int> programOrder; // used to store program indexes ordered by their 'score'
		int visitCounter = 0; // number of times this node has been visted in this brain update
		int parentCount = 0; // number of Actions referencing this Node

		static std::shared_ptr<ParameterLink<double>> mutateAddProgramChancePL;
		static std::shared_ptr<ParameterLink<double>> mutateTradeProgramChancePL;
		static std::shared_ptr<ParameterLink<double>> mutateMutateProgramChancePL;
		static std::shared_ptr<ParameterLink<double>> mutateTradeAndMutateProgramChancePL;
		static std::shared_ptr<ParameterLink<double>> mutateDeleteProgramChancePL;

		static std::shared_ptr<ParameterLink<int>> maxProgramsPL;
		static std::shared_ptr<ParameterLink<int>> minProgramsPL;

		double mutateAddProgramChance;
		double mutateTradeProgramChance;
		double mutateMutateProgramChance;
		double mutateTradeAndMutateProgramChance;
		double mutateDeleteProgramChance;

		int maxPrograms;
		int minPrograms;

		static long nextNodeID;

		Node(std::shared_ptr<std::vector<std::shared_ptr<Program>>> allPrograms_)
			:allPrograms(allPrograms_) {

			mutateAddProgramChance = mutateAddProgramChancePL->get();
			mutateTradeProgramChance = mutateTradeProgramChancePL->get();
			mutateMutateProgramChance = mutateMutateProgramChancePL->get();
			mutateTradeAndMutateProgramChance = mutateTradeAndMutateProgramChancePL->get();
			mutateDeleteProgramChance = mutateDeleteProgramChancePL->get();
			maxPrograms = maxProgramsPL->get();
			minPrograms = minProgramsPL->get();

			ID = nextNodeID++;
		}

		std::shared_ptr<Node> clone() {
			auto newNode = std::make_shared<Node>(allPrograms);
			newNode->programs = programs;
			for (auto p : newNode->programs) {
				p->parentCount++;
			}
			return newNode;
		}

		void mutate() {
			bool mutated = false;
			while (!mutated) {
				if (Random::P(mutateAddProgramChance)) {
					if (programs.size() < maxPrograms) {
						programs.push_back((*allPrograms)[Random::getIndex((*allPrograms).size())]);
						programs.back()->parentCount++;
						mutated = true;
					}
				}
				if (Random::P(mutateTradeProgramChance)) {
					int whichProgram = Random::getIndex(programs.size());
					programs[whichProgram]->parentCount--;
					programs[whichProgram] = (*allPrograms)[Random::getIndex((*allPrograms).size())];
					programs[whichProgram]->parentCount++;
					mutated = true;
				}
				if (Random::P(mutateMutateProgramChance)) {
					int whichProgram = Random::getIndex(programs.size());
					programs[whichProgram]->parentCount--;
					programs[whichProgram] = programs[whichProgram]->cloneAndMutate();
					programs[whichProgram]->parentCount++;
					(*allPrograms).push_back(programs[whichProgram]);
					mutated = true;
				}
				if (Random::P(mutateTradeAndMutateProgramChance)) {
					int whichProgram = Random::getIndex(programs.size());
					programs[whichProgram]->parentCount--;
					programs[whichProgram] = (*allPrograms)[Random::getIndex((*allPrograms).size())]->cloneAndMutate();
					programs[whichProgram]->parentCount++;
					(*allPrograms).push_back(programs[whichProgram]);
					mutated = true;
				}
				if (Random::P(mutateDeleteProgramChance)) {
					if (programs.size() > minPrograms) {
						int whichProgram = Random::getIndex(programs.size());
						programs[whichProgram]->parentCount--;
						programs[whichProgram] = programs.back();
						programs.pop_back();
						mutated = true;
					}
				}
				//if (Random::P(newProgramChance)) {
				//
				//}
			} // end while !mutated
		} // end mutate()

		std::shared_ptr<Node> cloneAndMutate() {
			auto newNode = clone();
			newNode->mutate();
			return newNode;
		}

	}; // end node



	std::shared_ptr<Node> rootNode;

	int nrHidden;
	std::vector<double> hiddenValues;



	TPGBrain() = delete;

	// this costructor used only to generate progenitor it will
	// need to create inital nodes and inital population (how the
	// heck is that going to work?)
	// inital population will be bullshit - they will be empty/fast
	// optimizer will know on update 0 to simply generate a
	// population from nodes (i.e. bypass scoring.)
	TPGBrain(int nrIn_, int nrOut_,
		std::shared_ptr<ParametersTable> PT_)
		: AbstractBrain(nrIn_, nrOut_, PT_) {
		
		// we assume that if this constructor is being called that we
		// are starting a new TPG object.
		// we will need to generate nodes and programs
		
		nrHidden = hiddenCountPL->get(PT);
		auto initalPrograms = initalProgramsPL->get(PT);
		auto initalNodes = initalNodesPL->get(PT);

		auto AllPrograms = std::make_shared<std::vector<std::shared_ptr<Program>>>();
		auto AllNodes = std::make_shared<std::vector<std::shared_ptr<Node>>>();

		// generate initial programs (these will be atomic)
		for (int i = 0; i < initalPrograms; i++) {
			auto newProgram = std::make_shared<Program>(nrIn_, nrOut_, nrHidden, AllNodes);
			AllPrograms->push_back(newProgram);
		}

		// generate initial nodes
		//for (int i = 0; i < initalNodes; i++) {
			auto newNode = std::make_shared<Node>(AllPrograms);
			// initalize with 2 programs
			newNode->programs.push_back((*AllPrograms)[Random::getIndex(AllPrograms->size())]);
			newNode->programs.back()->parentCount++;
			newNode->programs.push_back((*AllPrograms)[Random::getIndex(AllPrograms->size())]);
			newNode->programs.back()->parentCount++;
			AllNodes->push_back(newNode);
		//}

		(*AllNodes)[0]->allPrograms = AllPrograms;
		(*AllPrograms)[0]->allNodes = AllNodes;
		rootNode = (*AllNodes)[Random::getIndex((*AllNodes).size())];
		std::cout << "  built a new projenitor TPG Brain." << std::endl;
		std::cout << "     total nodes: " << (*(*rootNode->allPrograms)[0]->allNodes).size() << "  total programs : " << (*rootNode->allPrograms).size() << std::endl;
	}

	TPGBrain(int nrIn_, int nrOut_, int nrHidden_,
		std::shared_ptr<ParametersTable> PT_)
		: AbstractBrain(nrIn_, nrOut_, PT_) {

		std::cout << "TPG brain 'nomal' constructor is called, but should not be called."
			"\n  perhaps this should be a deleted function." << std::endl;
		exit(1);
		nrHidden = nrHidden_;
		hiddenValues.resize(nrHidden);
		std::cout << "done normal Constructor" << std::endl;

	}

	TPGBrain(int nrIn_, int nrOut_, int nrHidden_,
		std::shared_ptr<Node> rootNode_,
		std::shared_ptr<ParametersTable> PT_)
		: AbstractBrain(nrIn_, nrOut_, PT_) {

		// this constructor is used to generate the first population only
		// it makes new nodes.

		//rootNode = (*(*rootNode_->allPrograms)[0]->allNodes)[(*(*rootNode_->allPrograms)[0]->allNodes).size()];

		rootNode = std::make_shared<Node>(rootNode_->allPrograms);
		// initalize with 2 programs
		rootNode->programs.push_back((*rootNode_->allPrograms)[Random::getIndex((*rootNode_->allPrograms).size())]);
		rootNode->programs.back()->parentCount++;
		rootNode->programs.push_back((*rootNode_->allPrograms)[Random::getIndex((*rootNode_->allPrograms).size())]);
		rootNode->programs.back()->parentCount++;
		(*(*rootNode_->allPrograms)[0]->allNodes).push_back(rootNode);

		nrHidden = nrHidden_;
		hiddenValues.resize(nrHidden);
	}

	TPGBrain(int nrIn_, int nrOut_, int nrHidden_,
		std::shared_ptr<Node> rootNode_,
		int pID,
		std::shared_ptr<ParametersTable> PT_)
		: AbstractBrain(nrIn_, nrOut_, PT_) {

		//rootNode = (*(*rootNode_->allPrograms)[0]->allNodes)[(*(*rootNode_->allPrograms)[0]->allNodes).size()];

		rootNode = rootNode_;
		// initalize with 2 programs
		nrHidden = nrHidden_;
		hiddenValues.resize(nrHidden);
	}

	virtual void resetBrain() override {
		resetInputs();
		resetOutputs();
		std::fill(hiddenValues.begin(), hiddenValues.end(), 0);
	}

  virtual ~TPGBrain() = default;

  virtual void update() override;

  virtual std::shared_ptr<AbstractBrain>
  makeBrain(std::unordered_map<std::string,
	  std::shared_ptr<AbstractGenome>> &_genomes) override;

  virtual std::unordered_set<std::string> requiredGenomes() override {
    return {};
  }

  virtual std::string description() override { return "TPGBrain\n"; }
  virtual DataMap getStats(std::string &prefix) override {
	  DataMap dataMap;
	  return dataMap;
  }
  virtual std::string getType() override { return "TPG"; }

  virtual std::shared_ptr<AbstractBrain> makeCopy(std::shared_ptr<ParametersTable> PT_ = nullptr) override;

  void initializeGenomes(
	  std::unordered_map<std::string,
	  std::shared_ptr<AbstractGenome>> &_genomes) {
	  // no genomes used here
  }


  void DetectParts(const std::shared_ptr<Node> &n, 
	  std::set<std::shared_ptr<Node>> &saveNodes,
	  std::set<std::shared_ptr <Program>> &savePrograms,
	  std::set<std::string> &visitedElements);

  // convert a brain into data map with data that can be saved to file
  virtual DataMap serialize(std::string &name) override;

  // given an unordered_map<string, string> and PT, load data into this brain
  virtual void deserialize(std::shared_ptr<ParametersTable> PT,
	  std::unordered_map<std::string, std::string> &orgData,
	  std::string &name) override;

};



inline std::shared_ptr<AbstractBrain>
TPGBrain_brainFactory(int ins, int outs, std::shared_ptr<ParametersTable> PT) {
  return std::make_shared<TPGBrain>(ins, outs, PT);
}

