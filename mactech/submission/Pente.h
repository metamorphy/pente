//
// Pente.h
// Copyright © 1997 J. Robert Boonstra II
//
#pragma once

typedef enum {kFirst=1,kSecond=2} PlayerNumber;

typedef struct Capture {
	Point stone1;
	Point stone2;
} Capture;

void InitPente(
	long boardHalfSize		/* e.g., 9 for a 19x19 board */
							/* all coordinates between -boardHalfSize
								and +boardHalfSize */
);

void Pente(
	Point opponentsMove,		/* your opponent moved here */
	Boolean playingFirst,		/* ignore opponentMove */
	Point *yourMove,			/* return your move here */
	Capture claimCaptures[],	/* return coordinates of captured pairs here */
	long *numCaptures,			/* return number of claimCaptures here */
	Boolean *claimVictory		/* return true if you claim victory with this move */
);

void TermPente(void);	/* deallocate any dynamic storage */