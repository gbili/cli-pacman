/**
 *  salesman.h
 *  SalesMan
 *
 *  Created by gui on 5/28/12.
 *  Copyright 2012 SalesMan . All rights reserved.
 *
 */

#define MAX_BG_CHRS        10000
#define MAX_WALL_CHRS      10

#define DIRECTION_CNT      4

#define PATH_MAX_STEPS     2000
#define PATH_MAX_NODES	   200
#define NOT_SET            -1

/* STRUCTURE DEFINITIONS */	

typedef struct {
	int absPos; // center of node absolute position
	int exitArm; // contains key of the exit arm in arms[] or NOT_SET
	int enterArm; // contains key of the enter arm in arms[] or NOT_SET
	int armCount; // number of arms in arms[]
	int arms[4]; // contains absolute positions
	int lockedArms[4]; // when value is set to 1 key in arms is locked
} stp_sNode;

typedef struct {
	int stepCount;
	int absPositions[PATH_MAX_STEPS];
	stp_sNode nodes [PATH_MAX_NODES];
	int maxAbsPosKey; //max element in absPositions array
	int maxNodeKey; //max node position in array
} stp_sPath;


/* FUNCTION DECLARATIONS */
extern int           getX          ( int abs);
extern int           getY          ( int abs);
extern void          setXStep      ( int cnt);
extern void          setWallChrs   ( char wall[MAX_WALL_CHRS],         int chrCnt);
extern void          setBackground ( char background[MAX_BG_CHRS],     int chrCnt, int lineChrCnt);
extern int           isPlaceable   ( int newPos );
extern stp_sPath        stp_pathInit             ( stp_sNode * node);
extern stp_sNode        stp_nodeInit             ( int absPos,               int enterArmAbsPos);
extern stp_sPath        stp_getPath              ( int start,                int target);
extern void             stp_pathRecShortener     ( stp_sPath * bestPath,     int rollBackToNodeKey, int rollBackToAbsPosKey, int maxSteps);
extern int              stp_pathFinder           ( stp_sPath * path,         int startingNodeKey, int startingAbsPosKey, int maxSteps, int target);
extern stp_sNode        stp_nodeSetNewExitArm    ( stp_sNode * node,         int target);
extern int              stp_setDirectionPriority ( int stepDirPriority[4],    int position, int target);
extern void             stp_pathReplace          ( stp_sPath * from,         stp_sPath * to, int startingNodeKey, int startingAbsPosKey);
extern int              stp_nodeIsBlank          ( stp_sNode node);
extern void             stp_nodePrint            ( stp_sNode * node);
extern void             stp_pathPrint            ( stp_sPath * path);
extern int              stp_nodeArmsExhausted    ( stp_sNode * node);
