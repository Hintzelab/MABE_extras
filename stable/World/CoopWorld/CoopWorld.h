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

#include "../AbstractWorld.h"

#include <stdlib.h>
#include <thread>
#include <vector>
#include "Utilities/CoopVectorNd.h"

using namespace std;

class CoopWorld : public AbstractWorld {

public:

	static shared_ptr<ParameterLink<int>> evaluationsPerGenerationPL;
	static shared_ptr<ParameterLink<int>> subgroupSizePL;
	static shared_ptr<ParameterLink<int>> gamesPerSubgroupPL;
	static shared_ptr<ParameterLink<string>> groupHuntSucceedThreasholdPL;
	static shared_ptr<ParameterLink<bool>> useRealRankForGroupHuntScorePL;
	static shared_ptr<ParameterLink<double>> rankInfluenceOnGroupHuntScorePL;
	static shared_ptr<ParameterLink<double>> soloHuntPayoffPL;
	static shared_ptr<ParameterLink<double>> groupHuntPayoffPL;
	static shared_ptr<ParameterLink<double>> groupHuntFailPayoffPL;
	static shared_ptr<ParameterLink<double>> noActionPayoffPL;

	vector<double> groupHuntSucceedThreashold;

	static shared_ptr<ParameterLink<string>> lifespanPL;
	static shared_ptr<ParameterLink<double>> reproCostPL;
	static shared_ptr<ParameterLink<int>> reproDistancePL;

	static shared_ptr<ParameterLink<int>> clansInXPL;
	static shared_ptr<ParameterLink<int>> clansInYPL;

	static shared_ptr<ParameterLink<int>> saveMapsStepPL;

	static shared_ptr<ParameterLink<int>> outputBitsPL;
	static shared_ptr<ParameterLink<string>> outputBehaviorsPL;

	static shared_ptr<ParameterLink<bool>> publicGoodsPL;

	static shared_ptr<ParameterLink<bool>> detectGroupHuntPL;
	static shared_ptr<ParameterLink<bool>> detectSoloHuntPL;
	static shared_ptr<ParameterLink<bool>> detectRankPL;

	//int mode;
	//int numberOfOutputs;
	//int evaluationsPerGeneration;

	static shared_ptr<ParameterLink<string>> groupNamePL;
	static shared_ptr<ParameterLink<string>> brainNamePL;
	//string groupName;
	//string brainName;


	static shared_ptr<ParameterLink<bool>> replaceOnDeathPL;
	static shared_ptr<ParameterLink<bool>> payForRepacementPL;

	enum class Actions {
		GroupHunt=2,
		SoloHunt=1,
		No=0,
		undefined = 3
		};

	enum class Results {
		GroupHuntSuccess,
		GroupHuntFail,
		SoloHuntSuccess,
		No
	};

	class Agent {
	public:
		int agentID;
		vector<double> scores;
		double aveScore = 0;
		double energy = 0;
		shared_ptr<Organism> org;
		shared_ptr<AbstractBrain> brain;
		int offspringCount = 0;
		double rank;
		double relativeRank;
		Actions action;
		Results result;
		map<Actions, int> actionCounts;
		map<Results, int> resultCounts;
		double colorRed = Random::getDouble(1.0);
		double colorBlue = Random::getDouble(1.0);
		double colorGreen = Random::getDouble(1.0);
		shared_ptr<Agent> senior;
		shared_ptr<Agent> junior;

	};

	CoopWorld(shared_ptr<ParametersTable> _PT = nullptr);
	virtual ~CoopWorld() = default;


	virtual void evaluate(map<string, shared_ptr<Group>>& groups, int analyze, int visualize, int debug);

	virtual unordered_map<string, unordered_set<string>> requiredGroups() override {
		int detectionCount = (int)detectGroupHuntPL->get(PT) + (int)detectSoloHuntPL->get(PT) + (int)detectRankPL->get(PT);
		cout << detectGroupHuntPL->get(PT) << "  " << (int)detectGroupHuntPL->get(PT) << endl;

		int numberOfInputs = 1+((subgroupSizePL->get()-1)*detectionCount); // first move(1), rank relitive(groupSize-1), num GroupHunt(groupSize-1)
		int numberOfOutputs = outputBitsPL->get(PT);
		return { { groupNamePL->get(PT),{ "B:" + brainNamePL->get(PT) + "," + to_string(numberOfInputs) + "," + to_string(numberOfOutputs) } } };
		// requires a root group and a brain (in root namespace) and no addtional genome,
		// the brain must have 1 input, and the variable numberOfOutputs outputs
	}


};

