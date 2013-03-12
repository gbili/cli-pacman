/**
 * SalesmanTravelingP
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "salesman.h"

#define DOWN               0 //positive
#define RIGHT              1 //positive
#define UP                 2 //negative
#define LEFT               3 //negative

#define DEBUGING           1
#define DEBUGING_BIS       1

static int  wallChrCnt;
static char wallChrs[MAX_WALL_CHRS];

static int  backgroundChrCnt;
static char backgroundChrs[MAX_BG_CHRS];
static int  backgroundLineChrCnt;

static int directions[DIRECTION_CNT];

/**
 * Set the wall chars
 */
void setWallChrs(char wall[MAX_WALL_CHRS], int chrCnt)
{
	int i;
	for (i = 0; i < chrCnt; i++) {
		wallChrs[i] = wall[i];
	}
	wallChrCnt = chrCnt;
}

/**
 * Set the background and directions
 */
void setBackground(char background[MAX_BG_CHRS], int chrCnt, int lineChrCnt)
{
	int i;
	for (i = 0; i < chrCnt; i++) {
		backgroundChrs[i] = background[i];
	}
	backgroundChrCnt = chrCnt;
	backgroundLineChrCnt = lineChrCnt;

	directions[DOWN] = lineChrCnt;
	directions[RIGHT]= 1;
	directions[UP]	 = -lineChrCnt;
	directions[LEFT] = -1;
}

/**
 * Set possible directions as their absolute value addition
 * They vary depending on backgroud that's why they must be specified
 */
void setXStep(int cnt)
{
	if (! backgroundChrCnt) {
		printf("Cannot increase x step before background has been set, call setBackground(bg[chrCnt], int chrCnt, int lineChrCnt )\n");
		return;
	}

	directions[RIGHT] = cnt;
	directions[LEFT] = -cnt;
}

/**
 * Return the x coordinate
 */
int getX(int abs)
{
	if (! backgroundLineChrCnt) {
		printf("Cannot call getX(abs) or getY(abs) before setBackground(bg[chrCnt], int chrCnt, int lineChrCnt )\n");
		return 0;
	}
	return abs % backgroundLineChrCnt;
}

/**
 * Return the y coordinate
 */
int getY(int abs)
{
	return ( abs - getX(abs) ) / backgroundLineChrCnt;
}

/**
 * Make sure chars stick to the roads 
 */
int isPlaceable( int newPos )
{
	int i;
	for (i = 0; i < wallChrCnt; i++) {
		if (wallChrs[i] == backgroundChrs[newPos]) {
			return 0;
		}
	}
	return 1;
}

/**
 * Sets initial node information:
 * - absolute position
 * - valid arms (where no walls)
 * - number of arms
 */
stp_sNode stp_nodeInit(int absPos, int enterArmAbsPos)
{
	stp_sNode node;
	int i, armAbsPos;
	
	node.absPos = absPos;
	node.exitArm = NOT_SET;
	node.enterArm = NOT_SET;
	
	node.armCount = 0;

	for (i = 0; i < DIRECTION_CNT; i++) {
		armAbsPos = node.absPos + directions[i];
		if (isPlaceable(armAbsPos)) {
			node.arms[node.armCount] = armAbsPos;
			node.armCount++;
		}
		node.lockedArms[i] = 0; //unlock every arm
	}
	
	for (i = 0; i < DIRECTION_CNT; i++) {
		if (i >= node.armCount) {//only set to 0 inexistant arms (wall arms)
			node.arms[i] = 0;
		}
	}
	
	/* 
	 * IMPORTANT LOCK THE ENTER ARM FOR stp_nodeSetNewExitArm()
	 * the only node whose enterArm wont be locked is the first
	 * node because enterArmAbsPos is set to NOT_SET
	 * This is usful when rolling back , the not set enter arm
	 * will be usable as an exit arm.
	 */
	for (i = 0; i < node.armCount; i++) {
		if(enterArmAbsPos == node.arms[i]) {
			node.enterArm = i;
			node.lockedArms[i] = 1;
		}
	}
	
	return node;
}

/*
 * Sets the first node, first abs pos from node
 * and a default value for the other fields
 */
stp_sPath stp_pathInit(stp_sNode * node)
{
	stp_sPath path;

	path.maxNodeKey = 0;
	path.maxAbsPosKey = 0;
	path.nodes[0] = *(node);
	path.absPositions[0] = node->absPos;
	
	return path;
}

/*
 * Finds an initial path, that is not known to be optimal
 * even if it may be (by accident).
 * Then stp_pathRecShortener, tries all possible permutations
 * of paths to return the shortest one from start to target
 */
stp_sPath stp_getPath(int start, int target)
{
	stp_sPath seed;
	stp_sNode node;
	
	node = stp_nodeInit(start, NOT_SET);
	seed = stp_pathInit(&node);
	stp_pathFinder( &seed, 0, 0, PATH_MAX_STEPS, target);
	stp_pathRecShortener( &seed, 0, 0, PATH_MAX_STEPS);
	return seed;
}

/*
 * Takes a path rolls back the nodes and at each node,
 * it tries to get to target (with the help of stp_pathFinder())
 * with a different arm than the one used in argument path.
 * If a shorter path is found, it calls itself to check for
 * an even shorter on that new shorter path.
 * If no shorter path is found, the node is updated and the
 * arm that has been tried is locked. (done by stp_pathFinder)
 * Once all arms have been tried, it rollsback to the next node
 * 
 */
void stp_pathRecShortener(stp_sPath * path, int rollBackToNodeKey, int rollBackToAbsPosKey, int maxSteps)
{
	printf("<stp_pathRecShortener rBNodeK = %d, rBAbsK = %d >>>>>>>>>>>\n", rollBackToNodeKey, rollBackToAbsPosKey);
	int rollbackStepCnt;
	int oldStepCnt;
	int i, j;
	j = path->maxNodeKey;
	//Rollback every step
	for (i = path->maxAbsPosKey; i >= rollBackToAbsPosKey; i--) {
		//Until step abs pos is same as node abs pos && rollback node limit has not been exceeded
		if (path->absPositions[i] == path->nodes[j].absPos 
		 && j >= rollBackToNodeKey
		 && (rollbackStepCnt = path->maxAbsPosKey - i)) { 
			//call find path on next testeable node arm
			oldStepCnt = path->stepCount;
			if (oldStepCnt > stp_pathFinder(path, j, i, rollbackStepCnt, path->absPositions[path->maxAbsPosKey]/*=target*/)) {
				printf("stp_pathRecShortener inside loop calling recursion with j = %d and i = %d, maxSteps = %d\n", j, i, path->maxAbsPosKey - i);
				stp_pathRecShortener(path, j, i, path->maxAbsPosKey - i /*new maxSteps*/);
			}
			j--;
		}
	}
	printf("</stp_pathRecShortener rBNodeK = %d, rBAbsK = %d >\n", rollBackToNodeKey, rollBackToAbsPosKey);
}

/* 
 * Make sure n arms - 1 have been locked
 * @return 1 if true 0 when false
 */
int stp_nodeArmsExhausted(stp_sNode * node)
{
	int i, lockedCnt;
	lockedCnt = 0;
	for (i = node->armCount - 1; i >= 0; i--) {
		if (node->lockedArms[i]) {//= 1 when locked
			++lockedCnt;
		}
	}
	return ((node->armCount - lockedCnt) < 2);
}

/* 
 * Tries to get to the target from the path node indicated by startingNodeKey 
 * sets path to the first node arm that gives a path to target
 * returns new stepCount
 */
int stp_pathFinder(stp_sPath * path, int startingNodeKey, int startingAbsPosKey, int maxSteps, int target)
{
	/*
	 * TODO if startingNodeKey = 0 we are at the beggining of path
	 * then modify the node such that node.enterArm is not locked to allow 
	 * stp_nodeSetNewExitArm() to set node.exitArm = node.enterArm
	 *
	 */
	
	int nextAbsPos;
	int i; //absPositions[i]
	int j; //nodes[j]
	stp_sPath pathGuess;
	stp_sNode node, nodeNextArmLocked;
	
	/* the path will start at node, so set it to the node from the larger path
	   from which we are told to start the new path
	   we are ensured that it will be different because stp_nodeSetNewExitArm
	   will automatically change the arm */
	node = path->nodes[startingNodeKey];
	
	/* Now that we have an initial node for our path, try to make steps and save
	   nodes when encountered 
	   note: to make steps only nodes are used, but only those with > 2 arms will
	   be saved in path nodes. we will refer to 2 >= nodes as "nodes" */
	for (i = 0, j = 0; i < maxSteps; i++) {
		/* Our "node" has an absPos wich is allways saved in path as a step */
		pathGuess.absPositions[i] = node.absPos;
		/* stop trying to reach target once the "node" is on target*/
		if (pathGuess.absPositions[i] == target) {
			//printf("Dont call nodeNextArmLocked, allready in target");
			break;
		}
		
		/* if target has not been reached, make a decision upon the "node" arm to be used
		   for path flow (aka the next node abs pos see end of loop)
		   Because there are walls, and we have set a maxSteps limit, it is possible that
		   no next node is initialized, that is why we must save an nodeNextArmLocked which
		   correspond to the current node but with the next exit arm locked.
		   We only want to save it when its the first node of new path, to have it if
		   we find out that the new path is not viable */
		if (0 == j && i == 0) { //only store oppositeNode if first node
			nodeNextArmLocked = stp_nodeSetNewExitArm( &node, target);
		} else { //otherwise dont bother
			stp_nodeSetNewExitArm( &node, target);
		}
		
		/*
		 * When exit arm is not set, it means that all arms have already been tried
		 * i.e. it's a dead end, or every other permutation has already been tried
		 * Anyways it means we must use the nodeNextArmLocked and throw away the new path
		 */
		if (node.exitArm == NOT_SET) {
			printf("EXIT ARM NOT SET ALL ARMS HAVE BEEN TRIED; DEAD END");
			break;
		}
		
		/* Save node information in path when real node */
		//if ((node.armCount > 2)) {
		//CRASHES || (path->absPositions[0] == node.absPos)) { // if node is a real node keep it in pathGuess.nodes[]			
			pathGuess.nodes[j++] = node;
		//}
		
		/* Initialize next node absPos with current node.exitArm */
		nextAbsPos = node.arms[node.exitArm];// and node.enterArm with current absPos
		node = stp_nodeInit( nextAbsPos, pathGuess.absPositions[i] );//pathGuess.absPositions[i] contains key of current step it will be set as node.enterArm in next node init
	}
	
	pathGuess.maxAbsPosKey = i;
	pathGuess.maxNodeKey = (0 < j)? (j - 1) : 0; //j is incrementer after every node is saved in pathGuess.nodes, it is possible that no node is saved, make sure there is at least one node saved so j > 0
	pathGuess.stepCount = i + 1;
	
	//printf("<<<<<<<<<<< path guess . abspositions [max = i = %d] = %d\n", i, pathGuess.absPositions[i]);
	
	/*
	 * Fucking found target in less than maxSteps mission accomplished, replace path with guessed path
	 */
	if (pathGuess.stepCount < maxSteps
	 && pathGuess.absPositions[pathGuess.maxAbsPosKey] == target) {
		printf("Replacing this: \n");
		 stp_pathPrint(path);
		 printf("with:\n");
		 stp_pathPrint(&pathGuess);
		 printf("and we get this:\n");
		stp_pathReplace( &pathGuess, path, startingNodeKey, startingAbsPosKey);
		stp_pathPrint(path);
	/* No better path is to be found set replace initial node with next arm locked to avoid trying again */
	} else {
		path->nodes[startingNodeKey] = nodeNextArmLocked;
	}

	return path->stepCount;
}

/**
 * Finds the next "fly best step" given a node and a target absolute position
 * "fly best step" : if there were no walls it would be the best step...
 * The node passed by reference is modified
 * 
 * @return the nodeNextArmLocked is the current node with the would be new exit arm locked
 * it is used in case the subpath is not viable
 */
stp_sNode stp_nodeSetNewExitArm( stp_sNode * node, int target)
{
	int stepDirPriority[4];
	int i, j;
	int nextStepAbs;
	//next node arms are set as exit arms, and old exit arm is locked
	stp_sNode nodeNextArmLocked;//nodeNextArmLocked contains a node where next node is locked and exit arm is kept;
	
	nodeNextArmLocked = *(node);//copy content
	
	/* stp_setDirectionPriority is only conceptual, we dont try to apply it to the backgroud */
	if ( ! stp_setDirectionPriority ( stepDirPriority, node->absPos, target) ) { //when false, already in target
		printf("Allready in target"); // NOT POSSIBLE TO FIND NEXT STEP _ALLREADY IN TARGET
		return nodeNextArmLocked;
	}

	/* now go through every direction in order of priority and see if the direction is compatible with node */
	for (i = 0; i < 4; i++) {
		nextStepAbs = node->absPos + stepDirPriority[i]; //make direction an absPos
		/* test the current direction with every arm to see if it matches */
		for (j = 0; j < node->armCount; j++) {
			/* if the arm is available in node */
			if ( node->arms[j] == nextStepAbs ){
				/* make sure the arm is not locked AND its not the current exit arm in which case we can skip to next direction */
				if (1 == node->lockedArms[j] 
				 || j == node->exitArm)
					break;//skip to next direction
				
				/* However if it is not locked nor exit arm, lock the old exit arm only if already set*/
				if (NOT_SET != node->exitArm)//only set when rolling back
					node->lockedArms[node->exitArm] = 1;
				
				/* lock the supposed new exit arm in nodeNextArmLocked */
				nodeNextArmLocked.lockedArms[j] = 1;
				/* set the new one */
				node->exitArm = j;
				return nodeNextArmLocked; // EVERYTHING IS COOL
			}
		}
	}
	node->exitArm = NOT_SET;
	return nodeNextArmLocked; // NOT POSSIBLE TO FIND NEXT STEP _DEAD END
}

/* 
 * Finds the order in which directions are to be tried in
 * order to get to target (from position) in the minimum steps
 * The guess is made hypothetically by flight
 * returns 0 if already in target or 1 if directions found 
 */
int stp_setDirectionPriority(int stepDirPriority[4], int position, int target) 
{
	int tx, ty, dx, dy;
	int a, b;
	
	/* get x y coordinates of target and node abs pos and difference between both*/
	tx = getX(target);
	ty = getY(target);
	dx = getX(position) - tx;
	dy = getY(position) - ty;
	
	/* Make sure not Already in target */
	if (0 == dx && 0 == dy) {
		return 0;
	}
	
	/* Get x and y "best dir" to approach target */
	/* "best dir" : if we were to fly to target... ;) */
	
	/* When any of the two is not in the same x or y coordinate, getting it one step closer by x or y is equally good */
	if (0 != dx) { // x is best 0 and worst 3
		stepDirPriority[0] = (0 < dx)? directions[LEFT] : directions[RIGHT]; //best
		stepDirPriority[3] = (0 < dx)? directions[RIGHT] : directions[LEFT]; //worst
		a = 1;
		b = 2;
	} else { //same priority 1 and 2
		stepDirPriority[1] = directions[LEFT];  // vanilla
		stepDirPriority[2] = directions[RIGHT]; // vanilla
		a = 0;
		b = 3;
	}
	if (0 != dy) {
		stepDirPriority[a] = (0 < dy)? directions[UP] : directions[DOWN]; //best
		stepDirPriority[b] = (0 < dy)? directions[DOWN] : directions[UP]; //worst
	} else {
		stepDirPriority[a] = directions[UP]; //vanilla
		stepDirPriority[b] = directions[DOWN]; //vanilla
	}
	
	return 1;
}


/**
 * Append the shortest path (from) to the base path (to) and erase excess in (to)
 * @return void
 */
void stp_pathReplace(stp_sPath * from, stp_sPath * to, int startingNodeKey, int startingAbsPosKey)
{
	stp_sNode nodeSwap, nodeBlank;
	int i, j, stopErasing;
	
	/* Erase until node that corresponds to the startReplaceAtKey absPosition */
	for (i = to->maxNodeKey; i >= startingNodeKey; i--) {
		to->nodes[i] = nodeBlank; //blank it out
	}
	++i; //compensate decrement before exit of loop
	
	/* Append the new nodes */
	for (j = 0; j <= from->maxNodeKey; i++, j++) {
		nodeSwap = from->nodes[j]; //direct array assignement copies address rather than content
		to->nodes[i] = nodeSwap;
	}
	to->maxNodeKey = from->maxNodeKey;
	
	/* Then replace absPositions with from.absPositions and erase excess in to.absPositions */
	stopErasing = to->maxAbsPosKey + 1;
	for (i = startingAbsPosKey, j = 0; (i < stopErasing) || (j <= from->maxAbsPosKey); i++, j++) { // || is used when * to path is a seed path
		if (j <= from->maxAbsPosKey) { //only keep replacing if there are steps in from
			/* Replace absPostions */
			to->absPositions[i] = from->absPositions[j];
			/* Update new maxAbsPosKey in to */
			if (j == from->maxAbsPosKey) {
				to->maxAbsPosKey = i;
			}
		} else { //after that, only erase
			to->absPositions[i] = 0;
		}
	}
	// Update to stepCount
	to->stepCount = (startingAbsPosKey + 1) + from->stepCount;
}

/**
 * Determine whether the node is usefull or not
 */
int stp_nodeIsBlank(stp_sNode node)
{
	return (0 == node.armCount);
}

/**
 * Node print
 */
void stp_nodePrint(stp_sNode * node)
{
	int i;
	
	printf("\n NODE  %ld : \n absPos : %d\n exitArm : %d\n enterArm : %d\n armCount : %d\n", (long) node, node->absPos, node->exitArm, node->enterArm, node->armCount);
	printf(" arms[] : ");
	for (i = 0; i < 4; i++)
		printf("%d ", node->arms[i]);
	
	printf("\n lockedArms[] : ");
	for (i = 0; i < 4; i++)
		printf("%d ", node->lockedArms[i]);
	printf("\n");
}

/**
 * Path print
 */
void stp_pathPrint(stp_sPath * path)
{
	int i;
	/*printf("-------- PATH -------- %ld : \n", (long) path);
	printf("maxAbsPosKey : %d\n", path->maxAbsPosKey);
	printf("maxNodeKey : %d\n", path->maxNodeKey);*/
	printf("absPositions[] : ");
	for (i = 0; i <= path->maxAbsPosKey; i++)
		printf("%d ", path->absPositions[i]);
	printf("\n");
	/*printf("\nnodes[] : ");
	for (i = 0; i <= path->maxNodeKey; i++)
		stp_nodePrint(&(path->nodes[i]));
	printf("\n-------HTAP-------\n");*/
}
