/*
 PACMAN GAME
 
 0 1 2 3 4 5 6 7 8 9 101112131415161718192021222324252627
 0  =======================================================
 1  | o o o o o o o o o o o o |=| o o o o o o o o o o o o |
 2  | o ======= o ========= o |=| o ========= o ======= o |
 3  | o |=====| o |=======| o |=| o |=======| o |=====| o |
 4  | o ======= o ========= o === o ========= o ======= o |
 5  | o o o o o o o o o o o o o o o o o o o o o o o o o o |
 6  | o ======= o === o =============== o === o ======= o |
 7  | o ======= o |=| o =============== o |=| o ======= o |
 8  | o o o o o o |=| o o o o |=| o o o o |=| o o o o o o |
 9  =========== o |========   |=|   ========| o ===========
 10 ==========| o |========   ===   ========| o |==========
 11 ==========| o |=|                     |=| o |==========
 12 ==========| o |=|   ======---=====708 |=| o |==========
 13 =========== o ===   |=============764 === o ===========
 14 |           o       |=============820    26    32  36 |
 15 =========== o ===   |=============876 === o ===========
 16 ==========| o |=|   ==============932 |=| o |==========
 17 ==========| o |=|       978  82   988 |=| o |==========
 18 ==========| o |=|   =============1044 |=| o |==========
 19 =========== o ===   ======= =====1100 === o ===========
 20 | o o o o o o o o o o o o |=| o o1156 o o o o o o o o |
 21 | o ======= o =========   |=|   ========= o ======= o |
 23 | o ======| o =========   ===   ========= o |====== o |
 24 | o o o |=| o                             o |=| o o o |
 25 ===== o |=| o === o =============== o === o |=| o =====
 26 ===== o === o |=| o =============== o |=| o === o =====
 27 | o o o o o o |=| o o o o |=| o o o o |=| o o o o o o |
 28 | o =================== o |=| o =================== o |
 29 | o =================== o === o =================== o |
 30 | o o o o o o o o o o o o o o o o o o o o o o o o o o |
 31 =======================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h> // for psix threads, multithreading
#include <semaphore.h> // semaphores with up = 1 -> same as mutex
#include <math.h>
#include "salesman.h"

#define	BG_LINE_CNT      31   // number of lines in Background
#define BG_LINE_CHR_CNT  56   // number of chars per line in Background
#define BG_CHR_CNT       1736 // number of chars in Background
#define WALL_CHR_CNT     3

#define DOWN             56
#define RIGHT			 2
#define UP				 -56
#define LEFT             -2

#define CHR_PACMAN      'P'   //character constant used for pacman
#define CHR_FOOD        '.'   //character constant used for food
#define CHR_GHOST       'W'   //character constant used for ghost character
#define CHR_CNT          2    //NUMBER OF CHARACTERS including pacman
#define GHOSTS_START_K   1
#define PACMAN_K         0    //where is pacman stored in chrs array

#define getAbs(x, y) ( x ) + ( ( y ) * BG_LINE_CHR_CNT )
#define getNewAbs(absChrPos, dir) ( absChrPos ) + ( dir ) //get the absolute postion after move in dir

//defines characters moving in the game ground
typedef struct {
	char chr;
	stp_sPath path;
	int newDirection;
	int lastPosition;
	int position;
} s_character ;

//keep character information during game
s_character chrs[CHR_CNT];

/* GLOBALS */
char bg[BG_CHR_CNT]; //game bg
char refBg[BG_CHR_CNT]; //reference bg never changes after load
char wallChr[3] = {'=', '|', '-'};
int direction = 0; //direction for pacman, value changes upon keypress through to keyLogger
int directionOrder[4]; //used in setGhostBestMovement()
s_character ghost;
pthread_mutex_t mutexDirection;
pthread_t pth;//thread identifier

/* FUNCTION DECLARATIONS */

void   loadBg(char c[]);
void   rasterize();
int    getDirection(char * c);
void   * keyLogger(void * arg);
void   initCharacters();
int    moveEveryone(int dir);
int    moveGhosts(int);
void   moveCharacter(int chrKey, int newPos); 
void   placeChrInBg(int chrK);
void   s_character_print(s_character * chr);


int main (int argc, const char * argv[]) {
	
	/* VARIABLE DECLARATIONS */
	int dir = 0;
	
	/* GLOBAL INITIALIZATIONS */
	loadBg(bg);
	loadBg(refBg);
	setBackground(bg, BG_CHR_CNT, BG_LINE_CHR_CNT);
	setXStep(RIGHT);
	setWallChrs(wallChr, WALL_CHR_CNT);
	
	initCharacters();
	
	/* INITIALIZE VARIABLES */
	pthread_attr_t attr;
	
	//void * status;
	
	/* Multithreading */
	pthread_mutex_init(&mutexDirection, NULL);
	
	/* Create thread to get the keystrokes */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	
	pthread_create(&pth, &attr, keyLogger, "processing...");//create separate thread for keystroke logging (avoids game to pause on wait)
	
	pthread_attr_destroy(&attr);
	
	pthread_detach(pth);
	
	/* GAME LOOP */
	while (1) {
		
		/* Bring pacman direction into game loop */
		pthread_mutex_lock(&mutexDirection);
		dir = direction;
		direction = 0; //avoid constant moving (only use input once: otherwise the direction would be used until a new keypress)
		pthread_mutex_unlock(&mutexDirection);
		
		/* Move everyone */
		if (1 == moveEveryone(dir)) {
			printf("GAME OVER\n");
		}
		
		/* Print game */
		system("stty cooked");
		rasterize();
		system("stty raw");
		
		sleep(1);
		
	}
	pthread_mutex_destroy(&mutexDirection);
	pthread_exit(NULL);
	return 0;
}

/**
 * Set every characters new movement info in their struct "chrs" 
 * DONT FORGET GAME OVER
 */
int moveEveryone(int pacmanDir)
{
	int np, i, ghostNp; //np new pos
	
	//Move pacman with user input...
	if ( isPlaceable( np =  getNewAbs(chrs[PACMAN_K].position, pacmanDir)) ) {
		moveCharacter(PACMAN_K, np);
	}
	
	//Move Ghosts with computer
	/*for (*/i = GHOSTS_START_K; /*i < CHR_CNT; i++) {*/
		//First get the next position
		s_character_print(&(chrs[i]));
		printf("stp_getPath (chrs[%d]position = %d, %d)\n", i, chrs[i].position, np);
		chrs[i].path = stp_getPath(chrs[i].position, np);
		ghostNp = chrs[i].path.absPositions[1];
		printf("ghostNp = %d\n", ghostNp);
		moveCharacter(i, ghostNp);
	/*}*/
	return 0;
}

/**
 * Set new position information in character and place it in bg
 */
void moveCharacter(int chrKey, int newPos)
{
	chrs[chrKey].newDirection = chrs[chrKey].lastPosition - newPos;
	chrs[chrKey].lastPosition = chrs[chrKey].position;
	chrs[chrKey].position = newPos;
	placeChrInBg(chrKey);
}

/* Sets initial values for each character */
void initCharacters()
{
	int i;
	int ghostInitPos[2] = { getAbs(2, 14), getAbs(52, 14) };
	
	/* PACMAN */
	chrs[PACMAN_K].chr = CHR_PACMAN;
	chrs[PACMAN_K].newDirection = 0;
	chrs[PACMAN_K].position = getAbs(26, 17);
	chrs[PACMAN_K].lastPosition = chrs[PACMAN_K].position;
	
	/* GHOSTS */
	/*for (*/i = GHOSTS_START_K;/* i < CHR_CNT; i++) {*/
		chrs[i].chr = CHR_GHOST;
		chrs[i].newDirection = 0;
		chrs[i].position = ghostInitPos[(rand() % 2)]; //initialize ghosts randomly in possible positions defined in array
		chrs[i].lastPosition = chrs[i].position;
	/*}*/
}

/* get single character (Launched in separate thread) */
void * keyLogger(void * arg)
{
	int dir;
	int chr;
	
	while (1) {
		//set terminal to raw mode (avoid pressing enter)
		system("stty raw");
		chr = getchar();
		//restet terminal to cooked mode (set back directive)
		system("stty cooked");
		//printf("\n---- GOT CHAR %c\n", chr);
		switch (chr) {
			case 'j':
				dir = LEFT;
				break;
			case 'k':
				dir = DOWN;
				break;
			case 'l':
				dir = RIGHT;
				break;
			case 'i':
				dir = UP;
				break;
		}
		pthread_mutex_lock(&mutexDirection);
		direction = dir;
		pthread_mutex_unlock(&mutexDirection);
	}
	return NULL;
}

/* Put char in bg and set back the char the bg char that was here before */
void placeChrInBg(int chrK)
{
	//decide what to do with old position
	if (CHR_PACMAN == chrs[chrK].chr && refBg[chrs[chrK].lastPosition] == CHR_FOOD) {
		bg[chrs[chrK].lastPosition] = ' ';//replace food with blank
	} else {
		bg[chrs[chrK].lastPosition] = refBg[chrs[chrK].lastPosition]; //put back the char that was here before char was moved in place
	}
	//place char in new pos
	bg[chrs[chrK].position] = chrs[chrK].chr;
}

/* Clear terminal and draw new background */
void rasterize()
{
	int i;
	system("clear");
	printf("\n");
	for (i = 0; i <= BG_CHR_CNT; i++) {
		printf("%c", bg[i]);
	}
	printf("\n");
}

/* clears all chars from terminal screen shell */
void clearScreen()
{
	system("clear");
}

/* Load Background into array */
void loadBg(char c[])
{
	int i;
	FILE * f;//file pointer
	
	if(NULL == (f = fopen("/Users/gui/Dropbox/Dev/C/CCommand/background.txt", "r"))){
		printf("fuck file not found");
	} //stream
	
	i = 0;
	while (fread(c+i, sizeof(char), 1, f) && ! feof(f)) {
		i++;
	}
}


void s_character_print(s_character * chr)
{
	/*	
	 char chr;
	 stp_sPath path;
	 int newDirection;
	 int lastPosition;
	 int position;*/
	printf("\ns_character : \n char chr : %c\n int newDirection : %d\n int lastPosition : %d\n int position : %d\n", chr->chr, chr->newDirection, chr->lastPosition, chr->position);
	stp_pathPrint(&(chr->path));
}
