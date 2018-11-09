//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

// Evaluates agents on how many '1's they can output. This is a purely fixed task
// that requires to reactivity to stimuli.
// Each correct '1' confers 1.0 point to score, or the decimal output determined by 'mode'.

#include "CoopWorld.h"

shared_ptr<ParameterLink<int>> CoopWorld::evaluationsPerGenerationPL = Parameters::register_parameter("WORLD_COOP-evaluationsPerGeneration", 1, "Number of times to test each Genome per generation (useful with non-deterministic brains)");
shared_ptr<ParameterLink<int>> CoopWorld::subgroupSizePL = Parameters::register_parameter("WORLD_COOP-subgroupSize", 5, "number of agents who will play in each game");
shared_ptr<ParameterLink<int>> CoopWorld::gamesPerSubgroupPL = Parameters::register_parameter("WORLD_COOP-gamesPerSubgroup", 1, "number of times each subgroup will play");
shared_ptr<ParameterLink<string>> CoopWorld::groupHuntSucceedThreasholdPL = Parameters::register_parameter("WORLD_COOP-groupHuntSucceedThreashold", (string)"4", "group hunt will succeed if atleast this number of agents choose action GroupHunt\nvalue should be between 1 and groupSize.\nif two numbers are provided *i.e. [n,m]) then GroupHunt success will be probabilistic\nif fewer then n GroupHunt, hunt will fail, if m or more GroupHunt will succeede.");

shared_ptr<ParameterLink<bool>> CoopWorld::useRealRankForGroupHuntScorePL = Parameters::register_parameter("WORLD_COOP_COSTS-useRealRankForGroupHuntScore", true, "if true, use agents real rank to apportion group hunt payoff");


shared_ptr<ParameterLink<double>> CoopWorld::rankInfluenceOnGroupHuntScorePL = Parameters::register_parameter("WORLD_COOP_COSTS-rankInfluenceOnGroupHuntScore", 1.0, "how much are GroupHunt payoffs skewed by rank? (0.0 = no effect, 1.0 = full effect)");
shared_ptr<ParameterLink<double>> CoopWorld::soloHuntPayoffPL = Parameters::register_parameter("WORLD_COOP_COSTS-soloHuntPayoff", 5.0, "payoff for SoloHunt action");
shared_ptr<ParameterLink<double>> CoopWorld::groupHuntPayoffPL = Parameters::register_parameter("WORLD_COOP_COSTS-groupHuntPayoff", 10.0, "average payoff for GroupHunt action if hunt is successful");
shared_ptr<ParameterLink<double>> CoopWorld::groupHuntFailPayoffPL = Parameters::register_parameter("WORLD_COOP_COSTS-groupHuntFailPayoff", 0.0, "payoff for GroupHunt action if unt fails");
shared_ptr<ParameterLink<double>> CoopWorld::noActionPayoffPL = Parameters::register_parameter("WORLD_COOP_COSTS-noActionPayoff", -1.0, "payoff for No action");

shared_ptr<ParameterLink<string>> CoopWorld::lifespanPL = Parameters::register_parameter("WORLD_COOP-lifespan", (string)"[10,25]", "[min,max] agents will live between min and max updates (unless they are replaced by a birth first.");

shared_ptr<ParameterLink<double>> CoopWorld::reproCostPL = Parameters::register_parameter("WORLD_COOP-reproCost", 15.0, "when an agent has collected this amount of energy they will reproduce");
shared_ptr<ParameterLink<int>> CoopWorld::reproDistancePL = Parameters::register_parameter("WORLD_COOP-reproDistance", 2, "offspring will replace an agent with in a square area with sides (2*distance+1) around parent.");

shared_ptr<ParameterLink<int>> CoopWorld::clansInXPL = Parameters::register_parameter("WORLD_COOP-clansInX", 1, "population is divided into clans so that total clans in clansInX * clansInY");
shared_ptr<ParameterLink<int>> CoopWorld::clansInYPL = Parameters::register_parameter("WORLD_COOP-clansInY", 1, "population is divided into clans so that total clans in clansInX * clansInY");

shared_ptr<ParameterLink<int>> CoopWorld::saveMapsStepPL = Parameters::register_parameter("WORLD_COOP-saveMapsStep", 10, "visualization maps will be saved on this update step");

shared_ptr<ParameterLink<string>> CoopWorld::outputBehaviorsPL = Parameters::register_parameter("WORLD_COOP-outputBehaviors", (string)"[NoAction,GroupHunt,NoAction,SoloHunt]", "maps brain output to actions (order is output values converted from binary)");
shared_ptr<ParameterLink<int>> CoopWorld::outputBitsPL = Parameters::register_parameter("WORLD_COOP-outputBits", 2, "number of bits of output provided by brain");

shared_ptr<ParameterLink<bool>> CoopWorld::publicGoodsPL = Parameters::register_parameter("WORLD_COOP-groupHuntIsPublicGoods", false, "if true, solo hunters will share in payoff of successful GroupHunt");

shared_ptr<ParameterLink<string>> CoopWorld::groupNamePL = Parameters::register_parameter("WORLD_COOP_NAMES-groupNameSpace", (string)"root::", "namespace of group to be evaluated");
shared_ptr<ParameterLink<string>> CoopWorld::brainNamePL = Parameters::register_parameter("WORLD_COOP_NAMES-brainNameSpace", (string)"root::", "namespace for parameters used to define brain");

shared_ptr<ParameterLink<bool>> CoopWorld::detectGroupHuntPL = Parameters::register_parameter("WORLD_COOP-detectGroupHunt", true, "If true, agents can detect how many others in group took groupHunt action last update");
shared_ptr<ParameterLink<bool>> CoopWorld::detectSoloHuntPL = Parameters::register_parameter("WORLD_COOP-detectSoloHunt", true, "If true, agents can detect how many others in group took soloHunt action last update");
shared_ptr<ParameterLink<bool>> CoopWorld::detectRankPL = Parameters::register_parameter("WORLD_COOP-detectRank", true, "If true, agents can detect their rank relative to group");

shared_ptr<ParameterLink<bool>> CoopWorld::replaceOnDeathPL = Parameters::register_parameter("WORLD_COOP-replaceOnDeath", true,
	"If true, when an agent dies from old age it will be replace by a mutated copy with the same rank"
	"\nIf false, when an agent dies from old age an organism will be born with in reproDistance of that agent will be select and produce an offspring here"
	"\nthe papameter payForReplacement will determin if this offspring is free");
shared_ptr<ParameterLink<bool>> CoopWorld::payForRepacementPL = Parameters::register_parameter("WORLD_COOP-payForRepacement", false,
	"If true, then when an offspring is created by an old age replacement then:"
	"\nif replaceOnDeath is true, the offspring will start with an energy of reproCost - parent energy"
	"\nif replaceOnDeath is false, the reprocost will be deducted from the parent, if the parent can not cover the cost, the offspring will start with reproCost - parent energy");

// return Average value from a vector of double
double vectorAve(vector<double> vect) {
	double ave = 0;
	for (auto val : vect) {
		ave += val;
	}
	return(ave / (double)vect.size());
}

CoopWorld::CoopWorld(shared_ptr<ParametersTable> _PT) :
	AbstractWorld(_PT) {
	cout << "in CoopWorld Constructor" << endl;

	// columns to be added to ave file
	popFileColumns.clear();
	popFileColumns.push_back("score");
	//popFileColumns.push_back("score_VAR"); // specifies to also record the variance (performed automatically because _VAR)
	popFileColumns.push_back("actionGroupHunt");
	popFileColumns.push_back("actionSoloHunt");
	popFileColumns.push_back("actionNo");

	popFileColumns.push_back("resultGroupHuntSuccess");
	popFileColumns.push_back("resultGroupHuntFail");
	popFileColumns.push_back("resultSoloHuntSuccess");
	popFileColumns.push_back("resultNo");

	popFileColumns.push_back("rank");
	popFileColumns.push_back("offspringCount");
}

void CoopWorld::evaluate(map<string, shared_ptr<Group>>& groups, int analyze, int visualize, int debug) {
	// namespace for group and brain to use in this world
	auto groupName = groupNamePL->get(PT);
	auto brainName = brainNamePL->get(PT);

	// localize some parameters
	auto subgroupSize = subgroupSizePL->get(PT);
	auto gamesPerSubgroup = gamesPerSubgroupPL->get(PT);
	auto rankInfluenceOnGroupHuntScore = rankInfluenceOnGroupHuntScorePL->get(PT);
	auto evaluationsPerGeneration = evaluationsPerGenerationPL->get(PT);
	auto useRealRankForGroupHuntScore = useRealRankForGroupHuntScorePL->get(PT);
	auto soloHuntPayoff = soloHuntPayoffPL->get(PT);
	auto groupHuntPayoff = groupHuntPayoffPL->get(PT);
	auto groupHuntFailPayoff = groupHuntFailPayoffPL->get(PT);
	auto noActionPayoff = noActionPayoffPL->get(PT);
	auto publicGoods = publicGoodsPL->get(PT);
	double reproCost = reproCostPL->get(PT);
	int reproDistance = reproDistancePL->get(PT);
	vector<double> lifespan; // lifespan is a list (min,max) so we must use convertCSVListToVector
	convertCSVListToVector(lifespanPL->get(PT), lifespan);


	auto outputBits = outputBitsPL->get(PT);

	auto detectGroupHunt = detectGroupHuntPL->get(PT);
	auto detectSoloHunt = detectSoloHuntPL->get(PT);
	auto detectRank = detectRankPL->get(PT);

	// localize groupHuntSucceedThreashold - this may be a list or a single number
	if (groupHuntSucceedThreasholdPL->get(PT)[0] == '[') {
		convertCSVListToVector(groupHuntSucceedThreasholdPL->get(PT), groupHuntSucceedThreashold);
	}
	else { // only one number
		groupHuntSucceedThreashold.resize(1);
		convertString(groupHuntSucceedThreasholdPL->get(PT), groupHuntSucceedThreashold[0]);
	}

	// localize outputBehaviors - convert csv string to string of Action enum class
	vector<string> outputBehaviorsStrings;
	convertCSVListToVector(outputBehaviorsPL->get(PT), outputBehaviorsStrings);
	vector<Actions> outputBehaviors;
	for (int s = 0; s < (int)outputBehaviorsStrings.size(); s++) {
		if (outputBehaviorsStrings[s] == "NoAction") {
			outputBehaviors.push_back(Actions::No);
		}
		else if (outputBehaviorsStrings[s] == "GroupHunt") {
			outputBehaviors.push_back(Actions::GroupHunt);
		}
		else if (outputBehaviorsStrings[s] == "SoloHunt") {
			outputBehaviors.push_back(Actions::SoloHunt);
		}
		else {
			cout << "  In CoopWorld::evaluate :: while reading outputBehaviors, found indefined action \"" << outputBehaviorsStrings[s] << "\".\nexiting." << endl;
			exit(1);
		}
	}
	if (outputBehaviors.size() != pow(2, outputBits)) {
		cout << "outputBits (" << outputBits << ") does not match number of outputBehaviors (" << outputBehaviors.size() << ") please fix.\n  exiting." << endl;
		exit(1);
	}



	bool replaceOnDeath = replaceOnDeathPL->get(PT);
	bool payForRepacement = payForRepacementPL->get(PT);

	// will track number of briths and deaths per update
	int meritBirthCount = 0;
	int replacementBirthCount = 0;

	// used when creating visualizations
	string visualizeData;

	double numGamesPlayedPerMatch = subgroupSize * gamesPerSubgroup;

	auto popSize = groups[groupName]->population.size();
	if (sqrt(popSize) != (int)sqrt(popSize)) {
		cout << "  in CoopWorld :: population size (" << popSize << ") does not fit in a square.\n  exiting." << endl;
		exit(1);
	}

	// playerOrder defines how subGroups are constructed (i.e. groupSize 5 will use the first 5 entires from list)
	// consider replacing (or ammending) with config defined group lists...
	vector<CoopPoint2d> playerOrder;
	playerOrder.push_back(CoopPoint2d(0, 0));
	playerOrder.push_back(CoopPoint2d(0, -1));
	playerOrder.push_back(CoopPoint2d(0, 1));
	playerOrder.push_back(CoopPoint2d(-1, 0));
	playerOrder.push_back(CoopPoint2d(1, 0));
	playerOrder.push_back(CoopPoint2d(-1, -1));
	playerOrder.push_back(CoopPoint2d(-1, 1));
	playerOrder.push_back(CoopPoint2d(1, -1));
	playerOrder.push_back(CoopPoint2d(1, 1));
	playerOrder.push_back(CoopPoint2d(0, -2));
	playerOrder.push_back(CoopPoint2d(0, 2));
	playerOrder.push_back(CoopPoint2d(-2, 0));
	playerOrder.push_back(CoopPoint2d(2, 0));
	playerOrder.push_back(CoopPoint2d(-2, -1));
	playerOrder.push_back(CoopPoint2d(-2, 1));
	playerOrder.push_back(CoopPoint2d(2, -1));
	playerOrder.push_back(CoopPoint2d(2, 1));
	playerOrder.push_back(CoopPoint2d(-1, -2));
	playerOrder.push_back(CoopPoint2d(1, -2));
	playerOrder.push_back(CoopPoint2d(-1, 2));
	playerOrder.push_back(CoopPoint2d(1, 2));
	playerOrder.push_back(CoopPoint2d(-2, -2));
	playerOrder.push_back(CoopPoint2d(2, -2));
	playerOrder.push_back(CoopPoint2d(-2, 2));
	playerOrder.push_back(CoopPoint2d(2, 2));

	int worldX = sqrt(popSize);
	int worldY = sqrt(popSize);
	// clans define areas where agents will only play with other clan members.
	// this makes each clan area a tourus (as apposed to the whole world)
	// reproduction takes place as normal and allows for migration between clans
	int clansInX = clansInXPL->get(PT);
	int clansInY = clansInYPL->get(PT);
	int clanSizeInX = worldX / clansInX;
	int clanSizeInY = worldY / clansInY;
	if ((double)worldX / (double)clansInX != clanSizeInX || (double)worldY / (double)clansInY != clanSizeInY) {
		cout << "clansInX or clansInY does not fit in World cleanly. exiting." << endl;
		exit(1);
	}

	CoopVector2d<shared_ptr<Agent>> worldGrid(worldX, worldY);

	// allLocations is created so that we can pull random locations from a list.
	vector<CoopPoint2d> allLocations;
	for (int x = 0; x < worldX; x++) {
		for (int y = 0; y < worldY; y++) {
			allLocations.push_back(CoopPoint2d(x, y));
		}
	}

	// highRank will be used to track agent with highest rank
	// the rank reasignment is preformed by starting at the
	// high rank individual and moving by rank though all agents.
	shared_ptr<Agent> highRank;

	// last agent is only used durring inital setup and is the last
	// agent added to the world (durring setup only!); 
	shared_ptr<Agent> lastAgent;

	// IDCount (probably not needed!) is used to assign a unique ID to each
	// agent (diffrent from Organism->ID).
	int IDcount = 0;
	// for each organism in the original population, create an agent for
	// that org. assign a rank based on agentID and then place randomly in
	// world. Also each agent has a junior and senior. for first agent
	// (lowest rank) set junior to self. best set senior to self.
	for(auto ORG : groups[groupName]->population){
		auto newAgent = make_shared<Agent>();
		newAgent->rank = IDcount + 1; // this could be a unique random generator 
		newAgent->agentID = IDcount;  // this should = index in allAgents
		newAgent->org = ORG;
		newAgent->brain = ORG->brains[brainName];
		auto pick = Random::getIndex(allLocations.size());
		auto thisLocation = allLocations[pick];
		allLocations[pick] = allLocations.back();
		allLocations.pop_back();
		worldGrid(thisLocation) = newAgent;
		if (IDcount == 0) { // first agent, lowest rank
			newAgent->junior = newAgent;
		}
		else if (IDcount < popSize - 1) { // agent in the middle
			newAgent->junior = lastAgent;
			lastAgent->senior = newAgent;
		}
		else { // last agent, highest rank
			highRank = newAgent;
			newAgent->junior = lastAgent;
			newAgent->senior = newAgent;
			lastAgent->senior = newAgent;
		}
		lastAgent = newAgent;
		IDcount++;
	}

	// define some variables we will be using over and over...
	int whichXClan;
	int whichYClan;
	int clanXmin, clanYmin, clanXmax, clanYmax;

	int numGroupHunt = 0;
	int numSoloHunt = 0;
	int numNoAction = 0;
	int inputCount;

	// while the archivist for this group says we are not done...
	while (!groups[groupName]->archivist->finished_) {
		for (int r = 0; r < evaluationsPerGeneration; r++) {
			for (int i = 0; i < popSize; i++) {
				// iterate over agents in world - making each focal agent in turn
				CoopPoint2d focalLoc = CoopPoint2d(i%worldX, (int)(i / worldX));
				// figure out with clan this agent is in.
				whichXClan = focalLoc.x / clanSizeInX;
				whichYClan = focalLoc.y / clanSizeInY;
				clanXmin = whichXClan * clanSizeInX;
				clanXmax = ((whichXClan+1) * clanSizeInX);
				clanYmin = whichYClan * clanSizeInY;
				clanYmax = ((whichYClan + 1) * clanSizeInY);
				if (debug) {
					cout << "world size = " << worldX << "," << worldY << "   agent at: ";
					cout << focalLoc.x << "," << focalLoc.y << " clan: " << whichXClan << "," << whichYClan << "   xXyY: " << clanXmin << "," << clanXmax << "," << clanYmin << "," << clanYmax << endl;
				}

				// make subgroup
				vector<shared_ptr<Agent>> subgroup;
				// add groupSize agents to subgroup. these agents are pulled from the
				// local area around the focal agent. each location must be checked to make
				// sure that we do not run off the edge of the clan.
				for (int playerIndex = 0; playerIndex < subgroupSize; playerIndex++) {
					auto newLoc = focalLoc + playerOrder[playerIndex];
					if (newLoc.x < clanXmin) {
						newLoc.x = clanXmax + (newLoc.x - clanXmin);;
					}
					else if (newLoc.x >= clanXmax) {
						newLoc.x = newLoc.x - (clanSizeInX);
					}
					if (newLoc.y < clanYmin) {
						newLoc.y = clanYmax + (newLoc.y - clanYmin);
					}
					else if (newLoc.y >= clanYmax) {
						newLoc.y = newLoc.y - (clanSizeInY);
					}
					subgroup.push_back(worldGrid(newLoc));
					if (debug) {
						cout << "adding agent: " << worldGrid(newLoc) << " at " << newLoc.x << "," << newLoc.y << endl;
					}
				}
				if (debug) {
					cout << " ----- " << endl;
				}

				// get subgroup local ranks for each agent
				vector<double> groupRanks;
				for (auto agent : subgroup) {
					groupRanks.push_back(agent->rank);
				}
				sort(begin(groupRanks), end(groupRanks));
				for (auto agent : subgroup) {
					agent->brain->resetBrain();
					agent->relativeRank = 1 + distance(begin(groupRanks), find(begin(groupRanks), end(groupRanks), agent->rank));
				}

				// play games
				for (int plays = 0; plays < gamesPerSubgroup; plays++) {
					// set inputs and update brains
					for (auto agent : subgroup) {
						inputCount = 0; // used to keep track of which brain input we are setting
						// set inputs 0 to (groupSize-1) to be relitive rank for this agent in this subgroup

						if (detectRank) {
							for (int i = subgroupSize; i > 1; i--) {
								agent->brain->setInput(inputCount++, i <= agent->relativeRank);
							}
						}
						// old way, setting relativeRank as int (as apposed to a list of bool)
						//agent->brain->setInput(0, agent->relativeRank);
						if (plays == 0) {
							// next input indicates that this is first play
							agent->brain->setInput(inputCount++, 1);
							// inputs are 0 because there is no actions from the last play
							if (detectGroupHunt) {
								for (int i = subgroupSize; i > 1; i--) {
									agent->brain->setInput(inputCount++, 0);
								}
							}
							if (detectSoloHunt) {
								for (int i = subgroupSize; i > 1; i--) {
									agent->brain->setInput(inputCount++, 0);
								}
							}
						}
						else {
							// next input indicates that this is NOT first play
							agent->brain->setInput(inputCount++, 0);
							// set bits based on how many other agents chose GroupHunt.
							if (detectGroupHunt) {
								int howManyOthersGroupHunt = numGroupHunt - (agent->action == Actions::GroupHunt);
								for (int i = subgroupSize; i > 1; i--) {
									agent->brain->setInput(inputCount++, i <= howManyOthersGroupHunt);
								}
							}
							if (detectSoloHunt) {
								int howManyOthersSoloHunt = numSoloHunt - (agent->action == Actions::SoloHunt);
								for (int i = subgroupSize; i > 1; i--) {
									agent->brain->setInput(inputCount++, i <= howManyOthersSoloHunt);
								}
							}
						}
						// call update on this brain
						agent->brain->update();
					}

					// read outputs


					numGroupHunt = 0;
					numSoloHunt = 0;
					numNoAction = 0;
					double totalRealRankOfGroupHunters = 0;
					double totalRelativeRankOfGroupHunters = 0;
					double totalRealRankOfActiveHunters = 0;
					double totalRelativeRankOfActiveHunters = 0;

					// for each agent in subgroup, collect action value, convert to action and update states
					for (auto agent : subgroup) {
						int outputValue = 0;
						
						for (int outputCount = 0; outputCount < outputBits; outputCount++){
							outputValue += (Bit(agent->brain->readOutput(outputCount)) * pow(2, outputCount));
						}
						if (outputBehaviors[outputValue]==Actions::GroupHunt) {
							agent->action = Actions::GroupHunt;
							agent->actionCounts[Actions::GroupHunt]++;
							totalRealRankOfGroupHunters += agent->rank;
							totalRelativeRankOfGroupHunters += agent->relativeRank;
							totalRealRankOfActiveHunters += agent->rank;
							totalRelativeRankOfActiveHunters += agent->relativeRank;
							numGroupHunt++;
						}
						else if (outputBehaviors[outputValue] == Actions::SoloHunt) {
							agent->action = Actions::SoloHunt;
							agent->actionCounts[Actions::SoloHunt]++;
							totalRealRankOfActiveHunters += agent->rank;
							totalRelativeRankOfActiveHunters += agent->relativeRank;
							numSoloHunt++;
						}
						else {
							agent->action = Actions::No;
							agent->actionCounts[Actions::No]++;
							numNoAction++;
						}

					}

					// did group hunt succeed?
					bool successfulHunt = false;
					if (groupHuntSucceedThreashold.size() == 1) { // if groupHuntSucceedThreashold is a single value
						successfulHunt = numGroupHunt >= groupHuntSucceedThreashold[0];
					}
					else { // if groupHuntSucceedThreashold is a single value (probablistic success)
						successfulHunt = numGroupHunt >= Random::getInt(groupHuntSucceedThreashold[0], groupHuntSucceedThreashold[1]);
					}
					// calculate groupHuntPayoff
					double groupHuntTotalPayoff = groupHuntPayoff * numGroupHunt * successfulHunt; // if none group hunt, there is no payoff

					// calculate shares for group hunt
					double realRankShare = groupHuntTotalPayoff / totalRealRankOfGroupHunters;
					double relativeRankShare = groupHuntTotalPayoff / totalRelativeRankOfGroupHunters;
					double publicGoodsRealRankShare = groupHuntTotalPayoff / totalRealRankOfActiveHunters;
					double publicGoodsRelativeRankShare = groupHuntTotalPayoff / totalRelativeRankOfActiveHunters;

					// for each agent, update score based on action
					for (auto agent : subgroup) {
						if (agent->action == Actions::SoloHunt) {
							agent->resultCounts[Results::SoloHuntSuccess]++;
							agent->result = Results::SoloHuntSuccess;
							if (publicGoods) {
								if (useRealRankForGroupHuntScore) {
									agent->scores.push_back(soloHuntPayoff +
										((agent->rank * publicGoodsRealRankShare) * rankInfluenceOnGroupHuntScore) +
										((groupHuntTotalPayoff/(numGroupHunt+numSoloHunt)) * (1.0 - rankInfluenceOnGroupHuntScore)));
								}
								else {
									agent->scores.push_back(soloHuntPayoff + 
										((agent->relativeRank * publicGoodsRelativeRankShare) * rankInfluenceOnGroupHuntScore) +
										((groupHuntTotalPayoff / (numGroupHunt + numSoloHunt)) * (1.0 - rankInfluenceOnGroupHuntScore)));
								}
							}
							else {
								agent->scores.push_back(soloHuntPayoff);
							}
						}
						else if (agent->action == Actions::GroupHunt) {
							if (successfulHunt) {
								agent->resultCounts[Results::GroupHuntSuccess]++;
								agent->result = Results::GroupHuntSuccess;
								if (publicGoods) {
									if (useRealRankForGroupHuntScore) {
										auto thisPayoff = ((agent->rank * publicGoodsRealRankShare) * rankInfluenceOnGroupHuntScore)
											+ ((groupHuntTotalPayoff / (numGroupHunt + numSoloHunt)) * (1.0 - rankInfluenceOnGroupHuntScore));
										agent->scores.push_back(thisPayoff);
									}
									else {
										auto thisPayoff = ((agent->relativeRank * publicGoodsRelativeRankShare) * rankInfluenceOnGroupHuntScore)
											+ ((groupHuntTotalPayoff / (numGroupHunt + numSoloHunt)) * (1.0 - rankInfluenceOnGroupHuntScore));
										agent->scores.push_back(thisPayoff);
									}
								} else { // not public goods
									if (useRealRankForGroupHuntScore) {
										auto thisPayoff = ((agent->rank * realRankShare) * rankInfluenceOnGroupHuntScore)
											+ ((groupHuntPayoff) * (1.0 - rankInfluenceOnGroupHuntScore));
										agent->scores.push_back(thisPayoff);
									}
									else {
										auto thisPayoff = ((agent->relativeRank * relativeRankShare) * rankInfluenceOnGroupHuntScore)
											+ ((groupHuntPayoff) * (1.0 - rankInfluenceOnGroupHuntScore));
										agent->scores.push_back(thisPayoff);
									}
								}
							}
							else {
								agent->resultCounts[Results::GroupHuntFail]++;
								agent->result = Results::GroupHuntFail;
								agent->scores.push_back(groupHuntFailPayoff);
							}
						}
						else { // no action
							agent->resultCounts[Results::No]++;
							agent->result = Results::No;
							agent->scores.push_back(noActionPayoff);
						}
					} // END update agent score based on action
				} // end subGroup game for-loop
			} // end evaluations per generation for-loop 
		} // END of whole population evaluation (all agents have been focal agent evaluationsPerGeneration times

		
		CoopVector2d<double> scoreGrid(worldX, worldY); // quick lookup for scores (when determining reproduction sites)

		vector<CoopPoint2d> reproList; // lists where agents have enough energy for repro 
		vector<CoopPoint2d> killList; // locations of orgs that need to be replaced if still alive after repro.

		// fill in repro and kill lists - i.e. who has enough to reproduce, who dies of old age
		// fill in scoreFGrid
		for (int x = 0; x < worldX; x++) {
			for (int y = 0; y < worldY; y++) {
				scoreGrid(x, y) = vectorAve(worldGrid(x, y)->scores);
				if (worldGrid(x, y)->energy > reproCost) {
					reproList.push_back(CoopPoint2d(x, y));
				}
				if ((Global::update - worldGrid(x, y)->org->timeOfBirth) > lifespan[0]) {
					if ((Global::update - worldGrid(x, y)->org->timeOfBirth) > Random::getInt(lifespan[0], lifespan[1])) {
						killList.push_back(CoopPoint2d(x, y));
					}
				}
			}
		}

		// update Organisms data maps
		double maxScore = 0;
		double aveScore = 0;
		for (int x = 0; x < worldX; x++) {
			for (int y = 0; y < worldY; y++) {
				auto agent = worldGrid(x, y);
				double agentAveScore = vectorAve(agent->scores);
				aveScore += agentAveScore;
				if (agentAveScore > maxScore) {
					maxScore = agentAveScore;
				}
				agent->energy += agentAveScore;
				agent->aveScore = agentAveScore;
				agent->org->dataMap.append("score", agent->scores);
				agent->org->dataMap.append("optimizeValue", agentAveScore);
				agent->org->dataMap.append("actionGroupHunt", (double)agent->actionCounts[Actions::GroupHunt] / numGamesPlayedPerMatch);
				agent->org->dataMap.append("actionSoloHunt", (double)agent->actionCounts[Actions::SoloHunt] / numGamesPlayedPerMatch);
				agent->org->dataMap.append("actionNo", (double)agent->actionCounts[Actions::No] / numGamesPlayedPerMatch);

				agent->org->dataMap.append("resultGroupHuntSuccess", (double)agent->resultCounts[Results::GroupHuntSuccess] / numGamesPlayedPerMatch);
				agent->org->dataMap.append("resultGroupHuntFail", (double)agent->resultCounts[Results::GroupHuntFail] / numGamesPlayedPerMatch);
				agent->org->dataMap.append("resultSoloHuntSuccess", (double)agent->resultCounts[Results::SoloHuntSuccess] / numGamesPlayedPerMatch);
				agent->org->dataMap.append("resultNo", (double)agent->resultCounts[Results::No] / numGamesPlayedPerMatch);

				agent->org->dataMap.append("rank", agent->rank);
				agent->org->dataMap.append("offspringCount", agent->offspringCount);
			}
		}
		aveScore /= popSize;

		// save the visualization file data.
		if (Global::update%saveMapsStepPL->get(PT) == 0) {
			visualizeData = "**ScoreMap**\n";
			visualizeData += to_string(worldX) + "," + to_string(worldY) + "\n";
			for (int y = 0; y < worldY; y++) {
				for (int x = 0; x < worldX; x++) {
					visualizeData += to_string(worldGrid(x, y)->aveScore);
					if (x % worldX == worldX - 1) {
						visualizeData += "\n";
					}
					else {
						visualizeData += ",";
					}
				}
			}
			visualizeData += "\n**ColorMap**\n";
			visualizeData += to_string(worldX) + "," + to_string(worldY) + "\n";
			for (int y = 0; y < worldY; y++) {
				for (int x = 0; x < worldX; x++) {
					visualizeData += to_string(worldGrid(x, y)->colorRed) + ":" + to_string(worldGrid(x, y)->colorGreen) + ":" + to_string(worldGrid(x, y)->colorBlue);
					if (x % worldX == worldX - 1) {
						visualizeData += "\n";
					}
					else {
						visualizeData += ",";
					}
				}
			}
			visualizeData += "\n**rankMap**\n";
			visualizeData += to_string(worldX) + "," + to_string(worldY) + "\n";
			for (int y = 0; y < worldY; y++) {
				for (int x = 0; x < worldX; x++) {
					visualizeData += to_string(worldGrid(x, y)->rank);
					if (x % worldX == worldX - 1) {
						visualizeData += "\n";
					}
					else {
						visualizeData += ",";
					}
				}
			}
			visualizeData += "\n**offspringCountMap**\n";
			visualizeData += to_string(worldX) + "," + to_string(worldY) + "\n";
			for (int y = 0; y < worldY; y++) {
				for (int x = 0; x < worldX; x++) {
					visualizeData += to_string(worldGrid(x, y)->offspringCount);
					if (x % worldX == worldX - 1) {
						visualizeData += "\n";
					}
					else {
						visualizeData += ",";
					}
				}
			}
			visualizeData += "\n**actionMap**\n";
			visualizeData += to_string(worldX) + "," + to_string(worldY) + "\n";
			for (int y = 0; y < worldY; y++) {
				for (int x = 0; x < worldX; x++) {
					visualizeData += to_string((int)worldGrid(x, y)->actionCounts[Actions::GroupHunt]) + ":" + to_string((int)worldGrid(x, y)->actionCounts[Actions::SoloHunt]) + ":" + to_string((int)worldGrid(x, y)->actionCounts[Actions::No]);
					if (x % worldX == worldX - 1) {
						visualizeData += "\n";
					}
					else {
						visualizeData += ",";
					}
				}
			}
			visualizeData += "\n**ageMap**\n";
			visualizeData += to_string(worldX) + "," + to_string(worldY) + "\n";
			for (int y = 0; y < worldY; y++) {
				for (int x = 0; x < worldX; x++) {
					visualizeData += to_string(Global::update - worldGrid(x, y)->org->timeOfBirth);
					if (x % worldX == worldX - 1) {
						visualizeData += "\n";
					}
					else {
						visualizeData += ",";
					}
				}
			}
			visualizeData += "\nupdate," + to_string(Global::update) + "\n";
			if (groups[groupNamePL->get(PT)]->archivist->finished_) {
				visualizeData += "\nEOF";
			}
			FileManager::writeToFile("CoopWorldData.txt", visualizeData);
		} // end save visualization file data

		// archive and advance update
		groups[groupNamePL->get(PT)]->archive();
		cout << "Update: " << Global::update << "   maxScore : " << maxScore << "   aveScore: " << aveScore;
		Global::update++;

		// reset Birth counters
		meritBirthCount = 0; // births resulting from score
		replacementBirthCount = 0; // births resulting from old age replacement

		// birth based on score - offspring will be in a cell within reproDistance with
		// the lowest score. (if more then one low score, a random cell is selected from
		// the low score cells. The new org is wrapped in an agent and this placed in the
		// agent linked list behind the parent (the order in the linked list is rank).
		// rank is updated later, after all birth and death events.
		// agents have a color (used in visualization) this is randomly jiggled on the offspring
		while (reproList.size() > 0) {
			auto pick = Random::getIndex(reproList.size());
			auto thisLoc = reproList[pick];
			auto thisAgent = worldGrid(thisLoc);
			reproList[pick] = reproList.back();
			reproList.pop_back();
			if (thisAgent->energy > reproCost) { // if this agent was not replaced by repro
				thisAgent->energy -= reproCost;
				thisAgent->offspringCount++;
				// select target cell (based on score)
				auto offspringCell = scoreGrid.pickInArea(thisLoc, reproDistance,1); // method 1 is proportional pick
				auto offspringWillReplace = worldGrid(offspringCell); // the agent to be replaced by offspring
				auto newOrg = thisAgent->org->makeMutatedOffspringFrom(thisAgent->org);

				auto newAgent = make_shared<Agent>();
				newAgent->agentID = IDcount++;  // this should = index in allAgents
				newAgent->org = newOrg;
				newAgent->brain = newOrg->brains[brainName];
				newAgent->colorRed = min(1.0, max(0.0, thisAgent->colorRed + Random::getDouble(-.025, .025)));
				newAgent->colorGreen = min(1.0, max(0.0, thisAgent->colorGreen + Random::getDouble(-.025, .025)));
				newAgent->colorBlue = min(1.0, max(0.0, thisAgent->colorBlue + Random::getDouble(-.025, .025)));
				newAgent->action = Actions::undefined;

				auto popIndex = distance(begin(groups[groupName]->population), find(begin(groups[groupName]->population), end(groups[groupName]->population), offspringWillReplace->org));
				groups[groupName]->population[popIndex] = newOrg;

				worldGrid(offspringCell) = newAgent;

				//cout << endl << thisAgent->senior->rank << " : repaced agent senior rank" << endl;
				//cout << thisAgent->rank << " : repaced agent rank (" << thisAgent->agentID << "," << thisAgent->org->ID << ")" << endl;
				//cout << thisAgent->junior->rank << " : repaced agent junior rank" << endl << endl;
				//cout << "   with: " << newAgent->agentID << "," << newAgent->org->ID << endl;
				//cout << "   was: " << offspringWillReplace->agentID << "," << offspringWillReplace->org->ID << endl;

				// put newAgent in after of thisAgent in rank
				if (thisAgent->junior == thisAgent) { // parent is low rank
					newAgent->junior = newAgent;
					newAgent->senior = thisAgent;
					thisAgent->junior = newAgent;
				}
				else {
					newAgent->junior = thisAgent->junior;
					newAgent->senior = thisAgent;
					thisAgent->junior->senior = newAgent;
					thisAgent->junior = newAgent;
				}

				// remove offspringWillReplace from rank
				if (offspringWillReplace->junior == offspringWillReplace) { // is low rank
					offspringWillReplace->senior->junior = offspringWillReplace->senior;
					offspringWillReplace->senior = nullptr;
					offspringWillReplace->junior = nullptr;
				}
				else if (offspringWillReplace->senior == offspringWillReplace) { // is hi rank
					highRank = offspringWillReplace->junior;
					offspringWillReplace->junior->senior = offspringWillReplace->junior;
					offspringWillReplace->senior = nullptr;
					offspringWillReplace->junior = nullptr;
				}
				else {
					offspringWillReplace->senior->junior = offspringWillReplace->junior;
					offspringWillReplace->junior->senior = offspringWillReplace->senior;
					offspringWillReplace->senior = nullptr;
					offspringWillReplace->junior = nullptr;
				}
				offspringWillReplace->org->kill();
				meritBirthCount += 1;
			}
		}

		// death from old age
		// replace agent at each location in killList if new org was not born there this update.
		// new agent is mutated copy, with same rank and color.
		for (auto loc : killList) {
			if (worldGrid(loc)->org->timeOfBirth != Global::update) { // if this org was not just born
				auto thisAgent = worldGrid(loc);
				auto newParent = thisAgent;

				auto newAgent = make_shared<Agent>();
				newAgent->agentID = IDcount++;  // this should = index in allAgents

				if (replaceOnDeath) {
					auto newOrg = thisAgent->org->makeMutatedOffspringFrom(thisAgent->org);
					newAgent->org = newOrg;
					if (payForRepacement) {
						newAgent->energy = -1 * reproCost;
					}
				} else {
					auto parentCell = scoreGrid.pickInArea(loc, reproDistance, 3, false); // method 3 (random), pickLeast = false (pick max)
					newParent = worldGrid(parentCell); // the agent to be replaced by offspring
					auto newOrg = newParent->org->makeMutatedOffspringFrom(newParent->org);
					newAgent->org = newOrg;
					if (payForRepacement) {
						if (newParent->energy < reproCost) {
							newAgent->energy = newParent->energy - reproCost;
						}
						newParent->energy = max(0.0,newParent->energy-reproCost);
					}
				}


				newAgent->brain = newAgent->org->brains[brainName];
				newAgent->colorRed = thisAgent->colorRed;
				newAgent->colorGreen = thisAgent->colorGreen;
				newAgent->colorBlue = thisAgent->colorBlue;
				newAgent->action = Actions::undefined;
				worldGrid(loc) = newAgent;

				//cout << endl << thisAgent->senior->rank << " : repaced agent senior rank" << endl;
				//cout << thisAgent->rank << " : repaced agent rank (" << thisAgent->agentID << "," << thisAgent->org->ID << ")" << endl;
				//cout << thisAgent->junior->rank << " : repaced agent junior rank" << endl << endl;
				//cout << "   with: " << newAgent->agentID << "," << newAgent->org->ID << endl;
				auto popIndex = distance(begin(groups[groupName]->population), find(begin(groups[groupName]->population), end(groups[groupName]->population), thisAgent->org));
				groups[groupName]->population[popIndex] = newAgent->org;

				if (replaceOnDeath) {
					// put newAgent in place of thisAgent in rank
					if (thisAgent->junior == thisAgent) { // low rank
						newAgent->junior = newAgent;
						newAgent->senior = thisAgent->senior;
						thisAgent->senior->junior = newAgent;
					}
					else if (thisAgent->senior == thisAgent) { // high rank
						newAgent->senior = newAgent;
						newAgent->junior = thisAgent->junior;
						thisAgent->junior->senior = newAgent;
						highRank = newAgent;
					}
					else { // in the middle
						thisAgent->junior->senior = newAgent;
						thisAgent->senior->junior = newAgent;
						newAgent->junior = thisAgent->junior;
						newAgent->senior = thisAgent->senior;
					}
					thisAgent->senior = nullptr;
					thisAgent->junior = nullptr;
				}
				else {
					// put newAgent in after newParent
					if (newParent->junior == newParent) { // low rank
						newAgent->junior = newAgent;
						newAgent->senior = newParent;
						newParent->junior = newAgent;
					}
					else if (newParent->senior == newParent) { // high rank
						newAgent->senior = newParent;
						newAgent->junior = newParent->junior;
						newParent->junior->senior = newAgent;
						newParent->junior = newAgent;
					}
					else { // in the middle
						newParent->junior->senior = newAgent;
						newAgent->junior = newParent->junior;
						newParent->junior = newAgent;
						newAgent->senior = newParent;
					}
					//Now cut out thisAgent
					if (thisAgent->junior == thisAgent) { // low rank
						thisAgent->senior->junior = thisAgent->senior;
					}
					else if (thisAgent->senior == thisAgent) { // high rank
						thisAgent->junior->senior = thisAgent->junior;
						highRank = thisAgent->junior;
					}
					else { // in the middle
						thisAgent->junior->senior = thisAgent->senior;
						thisAgent->senior->junior = thisAgent->junior;
					}
					thisAgent->senior = nullptr;
					thisAgent->junior = nullptr;
				}
				thisAgent->org->kill();
				replacementBirthCount += 1;
			}
		}

		cout << "          meritBirths: " << meritBirthCount << "   replacementBirths: " << replacementBirthCount << endl;

		// update rank values for all agents starting from highRank.
		int currentRank = popSize;
		shared_ptr<Agent> currentAgent = highRank;
		while (currentAgent->junior != currentAgent) {
			//cout << currentRank << " " << currentAgent->agentID << " : ";
			currentAgent->rank = currentRank--;
			currentAgent->scores.clear();
			currentAgent->actionCounts.clear();
			currentAgent->resultCounts.clear();
			currentAgent = currentAgent->junior;

		}
		// set rank on lowrank (not handled by while loop)
		currentAgent->rank = currentRank;
		currentAgent->scores.clear();
		currentAgent->actionCounts.clear();
		currentAgent->resultCounts.clear();
		//cout << endl;

		// if the archivistsays we are finished add an EOF to the end of the visualization file
		if (groups[groupNamePL->get(PT)]->archivist->finished_) {
			visualizeData = "\nEOF";
			FileManager::writeToFile("CoopWorldData.txt", visualizeData);
		}
	}
}

