/**
 * Water Stream Plane
 *
 *
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
s_node s_node_init(int absPos, int enterArmAbsPos)
{
	s_node node;
	int i, armAbsPos;
	if (DEBUGING) {
		printf("\n<s_node_init>(int absPos = %d, enterArmAbsPos = %d) : \n", absPos, enterArmAbsPos);
	}
	
	node.absPos = absPos;
	node.exitArm = NOT_SET;
	node.enterArm = NOT_SET;
	
	node.armCount = 0;
	
	for (i = 0; i < DIRECTION_CNT; i++) {
		armAbsPos = node.absPos + directions[i];
		if (DEBUGING) {
			printf("\narmAbsPos = %d\n", armAbsPos);
		}
		if (isPlaceable(armAbsPos)) {
			node.arms[node.armCount] = armAbsPos;
			node.armCount++;
		}
	}
	
	for (i = 0; i < node.armCount; i++) {
		if (enterArmAbsPos == node.arms[i]) {
			node.enterArm = i;
		}
	}
	
	for (i = 0; i < DIRECTION_CNT; i++) {
		if (i >= node.armCount) {
			node.arms[i] = 0;
		}
		node.lockedArms[i] = 0;
	}
	
	if (DEBUGING) {
		s_node_print(&node);
		printf("</s_node_init>\n");
	}
	
	return node;
}

/**
 * Guess A Valid Path
 * @return absPosition of next best step
 */
s_path s_path_init(int pos, int target)
{	
	s_path path;
	s_node node;
	
	if (DEBUGING) {
		printf("\ns_path_init(int pos = %d, int target = %d) : \n", pos, target);
	}
	
	node = s_node_init(pos, NOT_SET);
	
	/* Find path from node to target */
	path = s_path_guess(node, target, PATH_MAX_STEPS);
	
	/* Try to find a better match */
	path = s_path_guessRec(path, target);
	
	return path;
}

/**
 * Checks all possible combinations of paths for a shorter one
 * uses recursivity
 */
s_path s_path_guessRec(s_path path, int target)
{
	int i, j, backSteps;
	s_path subPath;
	
	if (NOT_SET == path.stepCount) {
		if (DEBUGING) {
			printf("\ns_path_guessRec : path.stepCount = 0\n");
		}
		return path; //path cannot be found return stepCount will be NOT_SET
	}
	
	i = path.maxAbsPosKey;// taken out of loop to avoid reset
	/* Path has been found in number of stepCount steps try to find a better solution */
	for (j = path.maxNodeKey; j >= 0; j--) { // Go through nodes backwards one at a time
		for (; i >= 0; i--) { //Go through every step backwards
			if ( (path.nodes[j].absPos == path.absPositions[i]) && (backSteps = path.maxAbsPosKey - i)) { // until we find the node that corresponds to the step //keep record of backSteps because we are trying to find a path that is shorter than backSteps
				subPath = s_path_guess(path.nodes[j], target, backSteps);//from that node search for a shorter path starting at the next arm (limit size of path with backsteps (= maxSteps))
				if (DEBUGING) {
					printf("\nBACK TO --> s_path_guessRec\n");
				}
				//keep trying until one is found or no more arms are available in that node
				if (NOT_SET == subPath.stepCount) {// if not found returned path.stepCount will be NOT_SET (-1)
					if (DEBUGING_BIS) {
						printf("BROKE BECAUSE OF MAX STEPS, the call to s_path_guess, returned a bad subPath, because it had steps restriction, stop looking for better\n");
					}
					//keep track of the node arms that have been tried
					break;//in that case exit absPos loop and roll back next node and repeat
				}
				s_path_replace( &subPath, &path, i);//replace path starting at i with tmpPath
			}
		}
	}
	return path;
}

void s_path_updateSNodes(s_path * from, s_path * to)
{
	int i, j;
	s_node deAddress;
	
	for (i = 0; i < from->maxNodeKey; i++) {
		for (j = 0; j <= to->maxNodeKey; j++) {
			if (from->nodes[i].absPos == to->nodes[j].absPos) {
				deAddress = from->nodes[i];
				to->nodes[j] = deAddress;
			}
		}
	}
}

/**
 * Append the shortest path (from) to the base path (to) and erase excess in (to)
 * @return void
 */
void s_path_replace(s_path * from, s_path * to, int startReplaceAtKey)
{
	s_node nodeSwap, nodeBlank;
	
	int i, j, stopErasing;
	
	/* Erase until node that corresponds to the startReplaceAtKey absPosition */
	for (i = to->maxNodeKey; i >= 0 && to->nodes[i].absPos != to->absPositions[startReplaceAtKey]; i--)
		to->nodes[i] = nodeBlank; //blank it out
	
	/* Append the new nodes including the starting node */
	for (j = 0; i <= from->maxNodeKey; i++, j++) {
		nodeSwap = from->nodes[j]; //direct array assignement copies address rather than content
		to->nodes[i] = nodeSwap;
	}
	
	/* Then replace absPositions with from.absPositions and erase excess in to.absPositions */
	stopErasing = to->maxAbsPosKey + 1;
	for (i = startReplaceAtKey, j = 0; i < stopErasing ; i++, j++) {
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
	to->stepCount = (startReplaceAtKey + 1) + from->stepCount;
}

/**
 * Given node from another path, try to reach target in less than maxSteps
 * @return number of steps to target or 0
 */
s_path s_path_guess(s_node node, int target, int maxSteps)
{	
	int g, i, j, nextAbsPos;
	s_path path;
	if (DEBUGING) {
		printf("\n<s_path_guess><head>(s_node node = printing node...\n");
		s_node_print(&node);
		printf(", int target = %d, int maxSteps = %d)s_path_guess</head>\n", target, maxSteps);
	}
	
	/* Makes no sense to try to find another path within less than 2 steps
	 if (2 > maxSteps) {
	 path.stepCount = NOT_SET;
	 return path; //Current path is short enough
	 }*/
	
	/* Try to find path from node (s_node_guessNewExitArm(), automatically tries a new arm when node.exitArm != NOT_SET)*/
	for (i = 0, j = 0; i < maxSteps; i++) {
		/* Save step in Path */
		path.absPositions[i] = node.absPos;
		if (path.absPositions[i] == target) {
			if (DEBUGING_BIS) {
				printf("FOUND TARGET, path.absPositions[%d] = %d, target = %d\n", i, path.absPositions[i], target);
				for (g = 0; g <= i; g++) {
					printf("%d,", path.absPositions[g]);
				}
			}
			break;
		}
		
		/* Let s_node_guessNewExitArm find the best node.exitArm (next step) */
		node = s_node_guessNewExitArm(node, target);
		if (NOT_SET == node.exitArm ) {
			/* When s_node_guessNewExitArm return node has no exitArm target has been reached*/
			break;//found target
		}
		/* Save node information in path when real node */
		if (node.armCount > 2) { // if node is a real node keep it in path.nodes[]			
			path.nodes[j++] = node;
			if (DEBUGING) {
				printf("if node.armCount = %d > 2 then add it to path nodes\n", node.armCount);
				printf("(making sure addition to path.nodes is right : node addr = %ld, path.nodes[j] addr = )\n", &node, &(path.nodes[j-1]));
			}
		}
		
		if (DEBUGING) {
			printf("got out of s_node_guessNewExitArm returned in s_path_guess");
			s_node_print(&node);
		}
		
		/* Initialize next node absPos with current node.exitArm */
		if (DEBUGING) {
			printf("s_node_init(node.arms[node.exitArm = %d] = %d, enterArm = %d);\n", node.exitArm, node.arms[node.exitArm], path.absPositions[i]);
		}
		nextAbsPos = node.arms[node.exitArm]
		/* and node.enterArm with current absPos */;
		node = s_node_init( nextAbsPos, path.absPositions[i] );//path.absPositions[i] contains key of current step it will be set as node.enterArm in next node init
		
	}
	path.maxAbsPosKey = i;
	path.maxNodeKey = (0 < j)? (j - 1) : 0; //j is incrementer after every node is saved in path.nodes, it is possible that no node is saved, make sure there is at least one node saved so j > 0
	if (DEBUGING_BIS) {
		s_path_print(&path);
	}
	if (DEBUGING) {
		printf("</s_path_guess>\n\n");
	}
	/* Return number of steps or 0 if not shorter than maxSteps */
	path.stepCount = i + 1;
	if (path.stepCount == maxSteps) {
		path.stepCount = NOT_SET;
	}
	return path; // found in i+1 steps
}

/**
 * Finds the next "fly best step" given a node and a target absolute position
 * "fly best step" : if there were no walls it would be the best step...
 * @return resulting node with new exit arm
 */
s_node s_node_guessNewExitArm( s_node node, int target )
{
	
	int tx, ty, dx, dy;
	int stepDirPriority[4];
	int a, b, i, j;
	int nextStepAbs;
	
	if (DEBUGING) {
		printf("\n<s_node_guessNewExitArm><head>(s_node * node = printing node...\n");
		s_node_print(&node);
		printf(", int target = %d)</head>\n", target);
	}
	
	/* get x y coordinates of target and node abs pos and difference between both*/
	tx = getX(target);
	ty = getY(target);
	dx = getX(node.absPos) - tx;
	dy = getY(node.absPos) - ty;
	
	/* Make sure not Already in target */
	if (0 == dx && 0 == dy) {
		if (DEBUGING) {
			printf("...hmm not possible to find target ALLREADY IN TARGET = %d, node.absPos = %d</s_node_guessNewExitArm>\n", target, node.absPos);
		}
		node.exitArm = NOT_SET;
		return node; // NOT POSSIBLE TO FIND NEXT STEP _ALLREADY IN TARGET
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
	
	/* Convert s_node_guessNewExitArmPriority directions to absolutePositions
	 and confront theory with reallity of the ground */
	for (i = 0; i < 4; i++) {
		nextStepAbs = node.absPos + stepDirPriority[i]; //make it absPos
		for (j = 0; j < node.armCount; j++) {
			/* if the arm is available and not locked and not the current exit arm */
			if (node.arms[j] == nextStepAbs 
				&& ! node.lockedArms[j] 
				&& j != node.exitArm 
				&& j != node.enterArm ) { //FOUND NEXT STEP
				if (NOT_SET != node.exitArm) { // lock the old exitArm
					printf("locking current exitArm = %d, and setting newExitArm = %d \n", node.exitArm,  j);
					node.lockedArms[node.exitArm] = 1;
				}
				node.exitArm = j; // set the new exit arm
				if (DEBUGING_BIS) {
					s_node_print(&node);
					printf("...everything cool, found step , armKey is %d</s_node_guessNewExitArm>\n", j);
				}
				return node; // EVERYTHING IS COOL
			}
		}
	}
	if (DEBUGING) {
		printf("...not possible to find next step DEAD END</s_node_guessNewExitArm>\n");
	}
	node.exitArm = NOT_SET;
	return node; // NOT POSSIBLE TO FIND NEXT STEP _DEAD END
}

/**
 * Determine whether the node is usefull or not
 */
int s_node_isBlank(s_node node)
{
	return (0 == node.armCount);
}

/**
 * Node print
 */
void s_node_print(s_node * node)
{
	int i;
	
	printf("\n NODE  %ld : \n absPos : %d\n exitArm : %d\n enterArm : %d\n armCount : %d\n", node, node->absPos, node->exitArm, node->enterArm, node->armCount);
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
void s_path_print(s_path * path)
{
	int i;
	printf("\nPATH  %ld : \n", &path);
	printf("absPositions[] : ");
	for (i = 0; i <= path->maxAbsPosKey; i++)
		printf("%d ", path->absPositions[i]);
	
	printf("\nnodes[] : ");
	for (i = 0; i <= path->maxNodeKey; i++)
		s_node_print(&(path->nodes[i]));
	printf("\nmaxAbsPosKey : %d", path->maxAbsPosKey);
	printf("\nmaxNodeKey : %d", path->maxNodeKey);
}
