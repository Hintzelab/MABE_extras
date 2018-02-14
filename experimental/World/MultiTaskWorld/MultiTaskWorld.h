//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/ahnt/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/ahnt/MABE/wiki/License

#pragma once

#include "../AbstractWorld.h"

#include <stdlib.h>
#include <vector>

using namespace std;

class MiniGameBaseClass{
public:
	vector<int> inputAddresses,outputAddresses;
	MiniGameBaseClass(){}

	~MiniGameBaseClass(){}

	void setup(vector<int> &_InputAdresses,vector<int> &_OutputAdresses){
		inputAddresses=_InputAdresses;
		outputAddresses=_OutputAdresses;
	}

	virtual void reset(){
	}
	virtual void createInput(shared_ptr<AbstractBrain> brain){
	}
	virtual void executeOutput(shared_ptr<AbstractBrain> brain){
	}
	virtual double returnScore(){
		return 0.0;
	}

	virtual int requiredInputs(){
		return 1;
	}
	virtual int requiredOutputs(){
		return 1;
	}
	virtual string name()=0;

};

class nBackMiniGame: public MiniGameBaseClass{
private:
	const int Iwidth = 4;
	//old
	//const vector<int> nBack={0,1,2,3};
	//new
	const int nBack = 5;
	vector<int> sequence;
	int trueRight=0;
	int falseRight=0;
	int trueTotal=0;
	int falseTotal=0;
public:
	//better define these two here:
	nBackMiniGame(){}
	~nBackMiniGame(){}
	virtual string name(){ return "nBack";}

	//the others migrate to the cpp file for readability reasons.
	virtual void createInput(shared_ptr<AbstractBrain> brain);
	virtual void executeOutput(shared_ptr<AbstractBrain> brain);
	virtual double returnScore();
	virtual void reset();
	virtual int requiredInputs();
	virtual int requiredOutputs();
};

class areaRestrictedSearchMiniGame: public MiniGameBaseClass{
private:
	const int xDim=32;
	const int yDim=32;
	const int patchSize=8;
	const int patchNr=10;
	vector<vector<int>> area;
	int xPos,yPos,dir;
	int foodCollected,maxFood;
public:
	areaRestrictedSearchMiniGame(){}
	~areaRestrictedSearchMiniGame(){}
	virtual string name(){ return "areaRestrictedSearch";}

	//the others migrate to the cpp file for readability reasons.
	virtual void createInput(shared_ptr<AbstractBrain> brain);
	virtual void executeOutput(shared_ptr<AbstractBrain> brain);
	virtual double returnScore();
	virtual void reset();
	virtual int requiredInputs();
	virtual int requiredOutputs();
};

class valueJudgementMiniGame: public MiniGameBaseClass{
private:
	int t;
	int right,maxRight;
	int leftOrRight;
public:
	valueJudgementMiniGame(){}
	~valueJudgementMiniGame(){}
	virtual string name(){ return "valueJudgement";}

	//the others migrate to the cpp file for readability reasons.
	virtual void createInput(shared_ptr<AbstractBrain> brain);
	virtual void executeOutput(shared_ptr<AbstractBrain> brain);
	virtual double returnScore();
	virtual void reset();
	virtual int requiredInputs();
	virtual int requiredOutputs();
};

class mazeMiniGame: public MiniGameBaseClass{
private:
	const int xDim=15;
	const int yDim=15;
	int xPos,yPos,dir;
	vector<vector<int>> maze;
	vector<vector<double>> dist;
	int targetDist=0;
	double score,maxScore, partialScore;
	//int stepMax,currentStep
	int timeCounter;
public:
	mazeMiniGame();
	~mazeMiniGame(){}
	virtual string name(){ return "maze";}

	//the others migrate to the cpp file for readability reasons.
	virtual void createInput(shared_ptr<AbstractBrain> brain);
	virtual void executeOutput(shared_ptr<AbstractBrain> brain);
	virtual double returnScore();
	virtual void reset();
	virtual int requiredInputs();
	virtual int requiredOutputs();

	void makeMaze();
	void showMaze(int x=0, int y=0, int facing = -1);
	void fillInDists(int x,int y);
};

class confidenceMiniGame : public MiniGameBaseClass {
private:
	int t;
	int right, maxRight;
	int leftOrRight;
public:
	confidenceMiniGame() {}
	~confidenceMiniGame() {}
	virtual string name() { return "confidenceJudgement"; }

	//the others migrate to the cpp file for readability reasons.
	virtual void createInput(shared_ptr<AbstractBrain> brain);
	virtual void executeOutput(shared_ptr<AbstractBrain> brain);
	virtual double returnScore();
	virtual void reset();
	virtual int requiredInputs();
	virtual int requiredOutputs();
};

class descriptionMiniGame : public MiniGameBaseClass {
private:
	int right, maxRight;
	int sourceIsOne;
	int t;
	vector<int> sampleBuffer;
public:
	descriptionMiniGame() {}
	~descriptionMiniGame() {}
	virtual string name() { return "description"; }

	//the others migrate to the cpp file for readability reasons.
	virtual void createInput(shared_ptr<AbstractBrain> brain);
	virtual void executeOutput(shared_ptr<AbstractBrain> brain);
	virtual double returnScore();
	virtual void reset();
	virtual int requiredInputs();
	virtual int requiredOutputs();
};


class MultiTaskWorld : public AbstractWorld {
private:
    int reqI;
    int reqO;
public:

	static shared_ptr<ParameterLink<string>> gamesAllowedPL;
	vector<bool> gamesAllowed;
	static shared_ptr<ParameterLink<bool>> sequentialBrainPL;
	bool sequentialBrain;
	static shared_ptr<ParameterLink<int>> evaluationsPL;
	int evaluations;

	vector<shared_ptr<MiniGameBaseClass>> miniGames;
	MultiTaskWorld(shared_ptr<ParametersTable> _PT = nullptr);
	virtual ~MultiTaskWorld() = default;

	virtual void evaluateSolo(shared_ptr<Organism> org, int analyse, int visualize, int debug);
	virtual void evaluate(map<string, shared_ptr<Group>>& groups, int analyze, int visualize, int debug) {
		int popSize = groups[groupNamePL->get(PT)]->population.size();
		for (int i = 0; i < popSize; i++) {
			evaluateSolo(groups[groupNamePL->get(PT)]->population[i], analyze, visualize, debug);
		}
	}

	static shared_ptr<ParameterLink<string>> groupNamePL;
	static shared_ptr<ParameterLink<string>> brainNamePL;

	virtual unordered_map<string, unordered_set<string>> requiredGroups() override {
		return { { groupNamePL->get(PT),{ "B:" + brainNamePL->get(PT) + "," + to_string(reqI) + "," + to_string(reqO) } } };
		// requires a root group and a brain (in root namespace) and no addtional genome,
		// the brain must have 1 input, and the variable numberOfOutputs outputs
	}
};
