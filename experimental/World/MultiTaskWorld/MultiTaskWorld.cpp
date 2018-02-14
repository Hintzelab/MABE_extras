//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#include "MultiTaskWorld.h"
#include <unistd.h>

shared_ptr<ParameterLink<string>> MultiTaskWorld::gamesAllowedPL = Parameters::register_parameter("WORLD_MULTITASK-gamesAllowed", (string)"1_0_0_0_0_0", "list of the currently active mini games 0=false (off) 1=true (on)");
shared_ptr<ParameterLink<bool>> MultiTaskWorld::sequentialBrainPL = Parameters::register_parameter("WORLD_MULTITASK-sequential", true, "brain should be processed sequentially instead of parallel");
shared_ptr<ParameterLink<int>> MultiTaskWorld::evaluationsPL = Parameters::register_parameter("WORLD_MULTITASK-evaluations", 1, "number of world evaluations to average over before reporting score (1 is normal)");

shared_ptr<ParameterLink<string>> MultiTaskWorld::groupNamePL = Parameters::register_parameter("MULTITASK_NAMES-groupNameSpace", (string)"root::", "namespace for group to be evaluated");
shared_ptr<ParameterLink<string>> MultiTaskWorld::brainNamePL = Parameters::register_parameter("MULTITASK_NAMES-brainNameSpace", (string)"root::", "namespace for parameters used to define brain");


MultiTaskWorld::MultiTaskWorld(shared_ptr<ParametersTable> _PT) : AbstractWorld(_PT) {
    //read in parameters
    string S = gamesAllowedPL->get(PT);
    sequentialBrain = sequentialBrainPL->get(PT);
    evaluations = evaluationsPL->get(PT);

    //parse active mini-games string and set game true or false
	vector<string> listS=parseCSVLine(S,'_');
	gamesAllowed.clear();
	for(int i=0;i<6;i++){
		if(listS[i].compare("0")==0){
			gamesAllowed.push_back(false);
		} else {
			gamesAllowed.push_back(true);
		}
	}

	//games are added here, to change, right now removing games removed IO nodes, that will be fixed
	miniGames.push_back(dynamic_pointer_cast<MiniGameBaseClass>(make_shared<nBackMiniGame>()));
	miniGames.push_back(dynamic_pointer_cast<MiniGameBaseClass>(make_shared<areaRestrictedSearchMiniGame>()));
	miniGames.push_back(dynamic_pointer_cast<MiniGameBaseClass>(make_shared<valueJudgementMiniGame>()));
	miniGames.push_back(dynamic_pointer_cast<MiniGameBaseClass>(make_shared<mazeMiniGame>()));
	miniGames.push_back(dynamic_pointer_cast<MiniGameBaseClass>(make_shared<confidenceMiniGame>()));
	miniGames.push_back(dynamic_pointer_cast<MiniGameBaseClass>(make_shared<descriptionMiniGame>()));

    //set up each mini-game's input registers and output registers dynamically (i.e. removing mini game changes brain architecture)
    reqI=0;
    reqO=0;
    for(int i=0;(int)i<miniGames.size();i++){
        //get size of game's io buffer
        int localI=miniGames[i]->requiredInputs();
        int localO=miniGames[i]->requiredOutputs();
        vector<int> vI,vO;
        //assign input register and increment register count
        for(int j=0;j<localI;j++){
            vI.push_back(reqI);
            reqI++;
        }
        //assign and increment output register count
        for(int j=0;j<localO;j++){
            vO.push_back(reqO);
            reqO++;
        }
        //push value range to mini-game class
        miniGames[i]->setup(vI, vO);
    }

	// columns to be added to ave file
	popFileColumns.clear();
	popFileColumns.push_back("score");
}

// score is number of outputs set to 1 (i.e. output > 0) squared
void MultiTaskWorld::evaluateSolo(shared_ptr<Organism> org, int analyse, int visualize, int debug) {
    //reset is also used as an init function for the games so it must be called at the beginning even if there are no previous runs
    for (int evals = 0; evals < evaluations; evals ++){
        for(auto G : miniGames)
            G->reset();
        if(sequentialBrain){
            //run each game sequentially
            for(int i=0;i<miniGames.size();i++){
                //clear each brain so there is no interference between games
                org->brain->resetBrain();
                //if game is allowed run in brain for 1000 time steps
                if(gamesAllowed[i]){
                    for(int t=0;t<1000;t++){
                            /*
                        if (Global::modePL->get() == "visualize"){
                            printf("%i\n",t);
                            printf("DEBUG SCORE: %f",miniGames[i]->returnScore());
                        }
                        */
                        org->brain->resetInputs();
                        org->brain->resetOutputs();

                        miniGames[i]->createInput(org->brain);
                        org->brain->update();
                        miniGames[i]->executeOutput(org->brain);
                    }
                }
            }
        } else {
            //insert each input for all games together
            for(int t=0;t<1000;t++){
                org->brain->resetInputs();
                org->brain->resetOutputs();

                for(int i=0;i<miniGames.size();i++){
                    if(gamesAllowed[i])
                        miniGames[i]->createInput(org->brain);
                }
                //run inputs through brain together
                org->brain->update();
                //read each game's outputs together
                for(int i=0;i<miniGames.size();i++){
                    if(gamesAllowed[i])
                        miniGames[i]->executeOutput(org->brain);
                }
            }
        }
        double score=1.0;
        //save each game's score while summing(multiplying) a cumulative score
        for(int i=0;i<miniGames.size();i++){
            double localScore=1.0;
            if(gamesAllowed[i]){
                localScore=miniGames[i]->returnScore();
            }
            org->dataMap.append(to_string((string)"score_"+miniGames[i]->name()), localScore);
            score*=localScore;
        }
        org->dataMap.append("score",score);
    }
}

/*** mini games here : -------------------------------------------------------------------------------------------------------------------------------------------------------------------------******/
// *********** nBack **********
void nBackMiniGame::createInput(shared_ptr<AbstractBrain> brain){
	int I=0; //this stores the value put into the brain for a single time step.
	//the input number is allowed to be larger than a single bit. when it is, this loop will generate bits until a number with Iwidth bits is generated.
	for(int i=0;i<Iwidth;i++){
		int value=Random::getIndex(2);
		brain->setInput(inputAddresses[i], value);
		I+=value<<i;
	}
	//even though the brain takes the number input as individual bits, it is stored as a single number in binary, I.
	sequence.push_back(I);
}

void nBackMiniGame::executeOutput(shared_ptr<AbstractBrain> brain){
	//int localRight=0;
	//old nback
	/*
	for(int i=0;i<(int)nBack.size();i++){ // loop over partials of nBack.size()
		if(sequence.size()>=nBack[i]){//if sequence is long enough to ask about N back
			int seq=sequence[sequence.size()-1-nBack[i]];//get the nback answer
			for(int j=0;j<Iwidth;j++){//loop over the output bits that will form a single number in binary
				int o=Bit(brain->readOutput(outputAddresses[(i*Iwidth)+j]));//multiplying by i means each loop of i is asking for a single value of N
				if(o==((seq>>j)&1))//compare output bit with correct bit in stored number
					localRight++;//tally correct
				maxRight++;//tally total number of questions to get right
			}
		}
	}
	*/
	//begin rewrite of nback
	if(sequence.size()>=nBack){
        int nbackSeq=sequence[sequence.size()-1-nBack];
        int currentSeq = sequence[sequence.size()-1];
        int o=Bit(brain->readOutput(outputAddresses[0]));
        if (nbackSeq == currentSeq){
            trueTotal++;
            if (o == 1){
                trueRight++;
            }
        }
        else{//now != nback
            falseTotal++;
            if (o == 0){
                falseRight++;
            }
        }
	}
	if (Global::modePL->get() == "visualize"){
        //printf("score %f\n", returnScore());
	}
	//old
	//right+=localRight;//this value is a class variable of the nback game
}

double nBackMiniGame::returnScore(){
    double T = (double)trueRight/(double)trueTotal;
    double F = (double)falseRight/(double)falseTotal;
    return 1.0 + (T+F)/2.0;
}

void nBackMiniGame::reset(){
	trueRight=0;
	falseRight=0;
	trueTotal=0;
	falseTotal=0;
	sequence.clear();
}

int nBackMiniGame::requiredInputs(){
	//return Iwidth;
	return 2*Iwidth;
}

int nBackMiniGame::requiredOutputs(){
	//return Iwidth*(int)nBack.size();
	return 1;
}

// *********** area restricted search minigame----------------------------------------------------------------------------------------------------------------------------------------------------------
void areaRestrictedSearchMiniGame::createInput(shared_ptr<AbstractBrain> brain){
	brain->setInput(inputAddresses[0], area[xPos][yPos]);
}

void areaRestrictedSearchMiniGame::executeOutput(shared_ptr<AbstractBrain> brain){
	int xm[8]={0,1,1,1,0,-1,-1,-1};//xm and ym define a pattern of grid movement with positive x towards the right and positive y towards the bottom
	int ym[8]={-1,-1,0,1,1,1,0,-1};//TODO why are these created locally upon every call?
	foodCollected+=area[xPos][yPos];
	area[xPos][yPos]=0;
	//*
	//turn
	if (Bit(brain->readOutput(outputAddresses[0]))==1){
		dir=Random::getIndex(8);
	}
	//forward
	if (Bit(brain->readOutput(outputAddresses[1]))==1){
		xPos=(xPos+xm[dir])&(xDim-1);//bitwise and is used to create a torus ONLY ON GRIDS OF SIZE 2^x for integer x
		yPos=(yPos+ym[dir])&(yDim-1);
	}
}

double areaRestrictedSearchMiniGame::returnScore(){
	return 1.0+((double)foodCollected/(double)maxFood);
}

void areaRestrictedSearchMiniGame::reset(){
	foodCollected=0;
	maxFood=0;
	area.resize(xDim);//TODO why is resize called? if the size of the grid is designed to change there should be a function for this
	for(int i=0;i<xDim;i++){
		area[i].resize(yDim);//TODO are these here so that the reset method can double as an init method?
		for(int j=0;j<yDim;j++)
			area[i][j]=0;
	}
	//create patchNr number of patches of size patchSize and set maxFood appropriately
	for(int i=0;i<patchNr;i++){
		int x=Random::getIndex(xDim);
		int y=Random::getIndex(yDim);
		for(int a=0;a<patchSize;a++)
			for(int b=0;b<patchSize;b++){
				if(area[(x+a)%xDim][(y+b)%yDim]==0){
					area[(x+a)%xDim][(y+b)%yDim]=1;
					maxFood++;
				}
			}
	}
	//randomize start location
	xPos=Random::getIndex(xDim);
	yPos=Random::getIndex(yDim);
	dir=Random::getIndex(8);
}

int areaRestrictedSearchMiniGame::requiredInputs(){
	return 1;
}
int areaRestrictedSearchMiniGame::requiredOutputs(){
	return 2;
}

//********** value Judgment Task---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void valueJudgementMiniGame::createInput(shared_ptr<AbstractBrain> brain){
	int b=Random::P(0.75);
	brain->setInput(inputAddresses[leftOrRight],b);
	brain->setInput(inputAddresses[1-leftOrRight],1-b);
}

void valueJudgementMiniGame::executeOutput(shared_ptr<AbstractBrain> brain){
	t++;
	//every 5 time steps do:
	if(t==5){
		maxRight++;
		t=0;
		//v = veto
		int b1=Bit(brain->readOutput(outputAddresses[0]));
		int v1=Bit(brain->readOutput(outputAddresses[1]));
		int b2=Bit(brain->readOutput(outputAddresses[2]));
		int v2=Bit(brain->readOutput(outputAddresses[3]));
		//if v, b=0, v signals b to be set to 0 (but why not just set b to zero then?)TODO
		if(v1) b1=0;//TODO due to the Bit cast on the previous lines of code, the >=1 check is redundant, change to if v?
		if(v2) b2=0;
		//action depends on combination of B values (0,0)=0, (0,1)=2, (1,0)=1, (1,1)=3
		int action=b1+(2*b2);
		switch(action){
			//case 0: case 3: break;
			case 3:
			case 1:
				if(leftOrRight==0)
					right++;
				break;
			case 0:
			case 2:
				if(leftOrRight==1)
					right++;
				break;
		}
		leftOrRight=Random::getIndex(2);
	}
}

double valueJudgementMiniGame::returnScore(){
	return 1.0+((double)right/(double)maxRight);
}

void valueJudgementMiniGame::reset(){
	t=0;
	leftOrRight=Random::getIndex(2);
	right=0;
	maxRight=0;
}

int valueJudgementMiniGame::requiredInputs(){
	return 2;
}

int valueJudgementMiniGame::requiredOutputs(){
	return 4;
}

/********** MAZE *********-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
struct cell{
	int x,y;
	int count;
	cell(int _x,int _y,int _count){ x=_x; y=_y; count=_count;};
};

mazeMiniGame::mazeMiniGame(){
	makeMaze();
	showMaze();
}
//Dijkstra
void mazeMiniGame::fillInDists(int x,int y){
	int xm[4]={0,1,0,-1};//make a class var? TODO
	int ym[4]={-1,0,1,0};
	vector<cell> currentStack,newStack;
	currentStack.push_back(cell(x,y,1));
	dist[x][y]=1.0;
	while(currentStack.size()!=0){
		for(auto C:currentStack){
			for(int i=0;i<4;i++){
				int tx=C.x+xm[i];
				int ty=C.y+ym[i];
				// printf("%i %i %i\n",tx,ty,C.count);
				if((maze[tx][ty]==0)&&(dist[tx][ty]==0)){
					cell NC=cell(tx,ty,C.count+1);
					dist[NC.x][NC.y]=1.0/(double)NC.count;
					newStack.push_back(NC);
				}
			}
		}
		currentStack=newStack;
		newStack.clear();
	}
}

void mazeMiniGame::makeMaze(){
	int xm[4]={0,1,0,-1};//TODO class var?
	int ym[4]={-1,0,1,0};
	//initialize maze to correct size, set all locations to walls and all distances to 0.0
	//this stack will keep track of each "intersection" and allow for backtracking when a dead end is reached
	vector<cell> stack;
	maze.resize(xDim);
	dist.resize(xDim);
	for(int i=0;i<xDim;i++){
		maze[i].resize(yDim);
		dist[i].resize(yDim);
		for(int j=0;j<yDim;j++){
			maze[i][j]=1;
			dist[i][j]=0.0;
		}
	}
	//one down and one right from the top left most cell, w/ count = 0
	cell currentCell=cell(1,1,0);
	//set number of empty squares
	int possibleCells=((xDim-1)*(yDim-1))/4;//((xDim-1)/2)*((yDim-1)/2)
	//make (1,1) empty
	maze[1][1]=0;
	//reduce number of empty squares by one
	possibleCells--;
    //while there is still room for more empty cells:
	while(possibleCells>0){
		vector<cell> possibleNB;
		//for each direction, N,S,E,W :
		for(int i=0;i<4;i++){
            //check if a 2x1 extension in the i-direction would violate a boundary condition
			if((currentCell.x+(2*xm[i])>0) && (currentCell.x+(2*xm[i])<xDim) && (currentCell.y+(2*ym[i])>0) && (currentCell.y+(2*ym[i])<yDim)){
				//if not, now check if the addition would create a graph cycle i.e. intersect an already empty cell
				if(maze[currentCell.x+(2*xm[i])][currentCell.y+(2*ym[i])] == 1){
                    //if not, include the 2x1 extension as a candidate extension, only one candidate will be chosen later
					possibleNB.push_back(cell(currentCell.x+(2*xm[i]),currentCell.y+(2*ym[i]),currentCell.count+1));
				}
			}
		}
		//if not in a dead end:
		if(possibleNB.size()>0){
            //choose one of the possible next blocks randomly
			cell targetCell=possibleNB[Random::getIndex((int)possibleNB.size())];
            //push current cell onto the stack in order to back track to it later if needed
			stack.push_back(currentCell);
			//create the 2x1 block
			maze[targetCell.x][targetCell.y]=0;
			maze[(currentCell.x+targetCell.x)/2][(currentCell.y+targetCell.y)/2]=0;
			//set the new current cell to be the endpoint of the newly added 2x1 block
			currentCell=targetCell;
			//reduce number of cells allowed to create
			possibleCells--;
        //else, if in a dead end
		} else {
		    //check that the stack (containing previous locations on the path) is not empty
			if(stack.size()>0){
                //backtrack to the previous point on the path
				currentCell=stack.back();
                //remove the proper data from the stack
				stack.pop_back();
			}
		}
	}
	//use Dijkstra to define a fitness function based on the distance from the maze exit
	fillInDists(xDim-2,yDim-2);//-2, one for 0 index, one to offset the border
	//stepMax=(1.0/dist[1][1])-1;
}

void mazeMiniGame::showMaze(int x, int y, int facing ){
	for(int j=0;j<yDim;j++){
		for(int i=0;i<xDim;i++){
            if (facing != -1 && x==i && y==j){
                switch(facing){
                case 0:
                    printf(" ^");
                    break;
                case 1:
                    printf(" >");
                    break;
                case 2:
                    printf(" v");
                    break;
                case 3:
                    printf(" <");
                    break;
                }
                continue;
            }
			printf("%s",maze[i][j] ? "██":"  ");
		}
		printf("\n");
	}
	if (facing == -1){
        printf("\n");
        for(int i=0;i<xDim;i++){
            for(int j=0;j<yDim;j++){
                printf("%2i ",dist[i][j] == 0.0 ? 0: (int)(1.0/dist[i][j]));
            }
            printf("\n");
        }
    }
}

void mazeMiniGame::createInput(shared_ptr<AbstractBrain> brain){
    //input is a vision cone, 0 is one space forward to the left, 1 is one space forward, and 2 is one space forward to the right.
    //TODO adjust vision cone to be at 90 degree angles instead of 45
	int xm[8]={0,1,1,1,0,-1,-1,-1};
	int ym[8]={-1,-1,0,1,1,1,0,-1};
	brain->setInput(inputAddresses[0], maze[xPos+xm[(dir-1)&7]][yPos+ym[(dir-1)&7]]);
	brain->setInput(inputAddresses[1], maze[xPos+xm[dir]][yPos+ym[dir]]);
	brain->setInput(inputAddresses[2], maze[xPos+xm[(dir+1)&7]][yPos+ym[(dir+1)&7]]);
}

void mazeMiniGame::executeOutput(shared_ptr<AbstractBrain> brain){//TODO redo fitness function so that there is a "committed score" and an "in progress score" to allow for points to be rewarded after the game has ended
	int xm[8]={0,1,1,1,0,-1,-1,-1};
	int ym[8]={-1,-1,0,1,1,1,0,-1};
	int action=Bit(brain->readOutput(outputAddresses[0]))+(2*Bit(brain->readOutput(outputAddresses[1])));
	timeCounter++;
	switch(action){
		case 0: break;
		case 1: dir=(dir+2)&7; break;//org can see left and right 45 degrees but can only turn its body 90 degrees at a time
		case 2: dir=(dir-2)&7; break;
		case 3:
			if(maze[xPos+xm[dir]][yPos+ym[dir]]==0){//if the space ahead is empty, move there
				xPos+=xm[dir];
				yPos+=ym[dir];
			}
			if((xPos==xDim-2)&&(yPos==yDim-2)){//if reached the exit, reset for another run
				//home
				score++;
				xPos=1;
				yPos=1;
			}
			partialScore = dist[xPos][yPos];
			break;
	}
	if (Global::modePL->get() == "visualize"){
        printf("maxScore: %f\ncurrentStep: %f\nscore: %f\n",maxScore,score);
        mazeMiniGame::showMaze(xPos, yPos, dir/2);
        char dummy;
        //cin >> dummy;
        usleep(50000);
	}
}

double mazeMiniGame::returnScore(){
    int maxPL = (int)(1.0/dist[1][1]) + 1;//1 is the minimum number of turns. //+xDim; //rough calculations put the maximum number of turns in a maze's shortest path at the x or y dim.
    maxScore=(timeCounter/maxPL) + 1.0/(maxPL-(timeCounter%maxPL));
    //printf("max score: %f\tinteger: %i\tfloat: %f\n",maxScore,(int)(timeCounter*dist[1][1]),1.0/((1.0/(dist[1][1]))-1.0/(timeCounter%(int)(1.0/dist[1][1]))));
	return 1.0+((score+partialScore)/maxScore);
}

void mazeMiniGame::reset(){
	xPos=1;
	yPos=1;
	dir=Random::getIndex(4)*2;
	score=0.0;
	timeCounter = 0;
	maxScore=0.0;
	//currentStep=stepMax;
	partialScore=dist[1][1];
}

int mazeMiniGame::requiredInputs(){
	return 3;
}

int mazeMiniGame::requiredOutputs(){
	return 2;
}

//********** Confidence Judgement Task------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void confidenceMiniGame::createInput(shared_ptr<AbstractBrain> brain) {
	int b = Random::P(0.7);
	brain->setInput(inputAddresses[leftOrRight], b);
	brain->setInput(inputAddresses[1 - leftOrRight], 1 - b);
}

void confidenceMiniGame::executeOutput(shared_ptr<AbstractBrain> brain) {
	t++;
	if (t == 10) {
		maxRight+=3;
		t = 0;
		int action = Bit(brain->readOutput(outputAddresses[0])) + (2 * Bit(brain->readOutput(outputAddresses[1]))) + (4* Bit(brain->readOutput(outputAddresses[2]))) + (8 * Bit(brain->readOutput(outputAddresses[3])));
		switch (action) {
			case 1:
				if (leftOrRight == 0)
					right = right + 3;
				else if(leftOrRight == 1)
					right = right - 3;
				else
					break;
			case 2:
				if (leftOrRight == 0)
					right = right + 1;
				else if (leftOrRight == 1)
					right = right - 1;
				break;
			case 4:
				if (leftOrRight == 1)
					right = right + 1;
				else if (leftOrRight == 0)
					right = right - 1;
				break;
			case 8:
				if (leftOrRight == 1)
					right = right + 3;
				else if (leftOrRight == 0)
					right = right - 3;
				else
					break;
			default:
				//these are all others
				break;
		}
		leftOrRight = Random::getIndex(2);
	}
}

double confidenceMiniGame::returnScore() {
	if(right<0.0)
		return 1.0;
	return 1.0 + ((double)right / (double)maxRight);
}

void confidenceMiniGame::reset() {
	t = 0;
	leftOrRight = Random::getIndex(2);
	right = 0;
	maxRight = 0;
}

int confidenceMiniGame::requiredInputs() {
	return 2;
}

int confidenceMiniGame::requiredOutputs() {
	return 4;
}

//********** Decision from Description Task------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void descriptionMiniGame::createInput(shared_ptr<AbstractBrain> brain){
	int inputNum;
	if(t==0){
		sampleBuffer.clear();
		for (inputNum = 0; inputNum < 6; inputNum++) {
			int b = Random::P(0.85);
			// defines 1111-0.5220 1110-0.3684 1100-0.0975 1000-0.0116 0000-0.0005
			// means: majority >50% 0.8904 , ambiguous 50/50 0.0975 , misleading majority <50% 0.0121
			// expected best score when playing perfectly should be 1*0.8904 + 0.5*0.0975 + 0*0.0121 = 0.93915
			// any score greater than 0.93915 should be considered suspiciously
			b = sourceIsOne*b + (1-sourceIsOne)*(1-b); // flips the input bits if the source is 0
			sampleBuffer.push_back(b);
		}
	}
	for (inputNum = 0; inputNum < 6; inputNum++) {
		brain->setInput(inputAddresses[inputNum], sampleBuffer[inputNum]);
	}
}

void descriptionMiniGame::executeOutput(shared_ptr<AbstractBrain> brain){
	t++;
	if(t>5){
		t=0;
		maxRight++;
		int action=Bit(brain->readOutput(outputAddresses[0]))+(2*Bit(brain->readOutput(outputAddresses[1])));
		//action=Random::getIndex(4);
		switch(action){
			//case 0:
			case 1:
				if(sourceIsOne==0) // 01 / right = source is 0, 10 / left = source is 1
					right++;
				break;
			//case 3:
			case 2:
				if(sourceIsOne==1)
					right++;
				break;
		}
		sourceIsOne =Random::getIndex(2);
	}
 //   }
}

double descriptionMiniGame::returnScore(){
	return 1.0+((double)right/(double)maxRight);
}

void descriptionMiniGame::reset(){
	sourceIsOne =Random::getIndex(2);
	right=0;
	maxRight=0;
	t = 0;
}

int descriptionMiniGame::requiredInputs(){
	return 6;
}

int descriptionMiniGame::requiredOutputs(){
	return 2;
}

