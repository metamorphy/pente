//
// Pente.c
// Copyright © 1997 Jeff Mallett
//

#include "Pente.h"

enum { FRIEND = 0x0001,
       ENEMY = 0x0002,
       EMPTY = 0x0004,
       EMPTY_ADJACENT = 0x0008,
       WALL = 0x0010
};

//#define WRITE
#define SEARCH_DEPTH 4

#define BORDER			1
#define BORDERS			2
#define INFINITY		32600
#define NA				0
#define CAPTURE_SCORE	60
#define ESTIMATE_PLUS	1

static short CHAIN_SCORE[3][9] = {
   //      2   3    4   5+
	{ NA, -1,  2,  60, 500, 500, 500, 500, 500 },// 0 open
	{ NA, -6,  8,  80, 500, 500, 500, 500, 500 },// 1 open
	{ NA, -1, 15, 200, 500, 500, 500, 500, 500 } // 2 open
};
static short BLOCK_SCORE[5] = { NA, 0, 4, 20, 100 };

static short gEstimates[32*32], *gEstimatesStart, *gEstimatesEnd;
static short gBoard[32*32], *gBoardStart, *gBoardEnd;
static short gDirections[8], *gDirectionsEnd;

static short gMoveNum, gScore;
static short *gKillers[SEARCH_DEPTH];

static short gAdjustment;
#define ADJUST(a)		( (a) + gAdjustment )
#define TRANSLATE(x, y)	(gSideLength * ADJUST(y) + ADJUST(x))

#define GET_INDEX(p)	((p) - gBoard)
#define GET_X(i)		( ((i) % gSideLength) - gAdjustment )
#define GET_Y(i)		( ((i) / gSideLength) - gAdjustment )

#define OPPONENT(side)	(3 - (side))

#define VALUE(x) gBoard[x]

#define _ON_BOARD(x) (((x) & WALL) == 0)
#define ON_BOARD(i) _ON_BOARD(gBoard[i])

#define _OCCUPIED(x) ((x) & (FRIEND|ENEMY))
#define OCCUPIED(i) _OCCUPIED(gBoard[i])

#define _EMPTY(x) ((x) & (EMPTY|EMPTY_ADJACENT))
#define EMPTY(i) _EMPTY(gBoard[i])

#define HAS_STONE_OF_COLOR(x, c) (gBoard[x] == (c))

static long gBoardHalfSize, gSideLength, gBoardMax;
static long gCumCapturesFriend, gCumCapturesEnemy;

// gChanges -- Array of unsigned longs containing data to undo moves
//     list of:
//          <pointer to square> <old square value>
//     terminated by a OL
//   The first position will be the drop square and the others will be flips
static unsigned long gChanges[1024];
static unsigned long *gChangesEnd; // Pointer into gChanges

#define PUSH(x)			*(gChangesEnd++) = (x)
#define START_SAVE		PUSH(0L)
#define PUSH_SQ(pSq) 	{ PUSH((long)*(pSq)); PUSH((unsigned long)(pSq)); }
#define POP				*(--gChangesEnd)
#define TOP				*gChangesEnd


short AddStone(short alpha, short beta, short *pSq, short color, short depth,
				short capturesFriend, short capturesEnemy);
long MyFindCaptures(Capture capture[], short *pSq, short us, short them);
Boolean MyFindFive(short *pSq, short color);


/*	This file implements an extrordinarily stupid Pente player, 
	who moves to a random empty location.
	It serves only to provide an opponent for the human player in PenteHuman.c.
	You should replace this entire file with your solution. */

void InitPente(
	long boardHalfSize		/* e.g., 9 for a 19x19 board */
							/* all coordinates between -boardHalfSize
								and +boardHalfSize */
) {
	short i, j, *pSq;
	
	gBoardHalfSize = boardHalfSize;
	gSideLength = boardHalfSize * 2 + 1 + BORDERS;
	
	gDirections[0] = -gSideLength - 1; // NW
	gDirections[1] = -gSideLength;     // N
	gDirections[2] = -gSideLength + 1; // NE
	gDirections[3] = -1;               // W
	gDirections[4] = gSideLength + 1;  // SE
	gDirections[5] = gSideLength;      // S
	gDirections[6] = gSideLength - 1;  // SW
	gDirections[7] = 1;                // E
	gDirectionsEnd = &gDirections[8];
	
	gBoardMax = gSideLength * gSideLength;
	i = (gSideLength + 1) * BORDER;
	gBoardStart = &gBoard[i];
	gBoardEnd = &gBoard[gBoardMax - i];
	gEstimatesStart = &gEstimates[i];
	gEstimatesEnd = &gEstimates[gBoardMax - i];
	
	pSq = gBoard;
	do {
		*pSq = WALL;
	} while (++pSq != gBoardStart);
	do {
		*pSq = EMPTY;
	} while (++pSq != gBoardEnd);
	do {
		*pSq = WALL;
	} while (++pSq < &gBoard[gBoardMax]);
	for (i = BORDER + boardHalfSize * 2 + 1; i<gBoardMax-gSideLength; i += gSideLength)
		for (j=0; j<BORDERS; ++j)
			gBoard[i+j] = WALL;
		
	
	gCumCapturesFriend = gCumCapturesEnemy = 0;
	gMoveNum = 0;
	gAdjustment = gBoardHalfSize + BORDER;
}

void Pente(
	Point opponentsMove,		/* your opponent moved here */
	Boolean playingFirst,		/* ignore opponentMove */
	Point *yourMove,			/* return your move here */
	Capture claimCaptures[],	/* return coordinates of captured pairs here */
	long *numCaptures,			/* return number of claimCaptures here */
	Boolean *claimVictory		/* return true if you claim victory with this move */
) {
	Point move;
	Capture opponentCaptures[8];
	Boolean fiveInARow;
	
	short *pSq, *pNewSq, *d;
	short score, bestScore, i;
	short *pBestMove;
	
	*numCaptures = 0;
	*claimVictory = false;

	if (playingFirst) { // *** MOVE 1
		gBoard[TRANSLATE(0, 0)] = FRIEND;
		move.h = move.v = 0;
		*yourMove = move;
		gMoveNum = 1;
		return;
	}
	
	i = TRANSLATE(opponentsMove.h, opponentsMove.v);
	gBoard[i] = ENEMY;
	++gMoveNum;

	if (gMoveNum == 1) { // *** MOVE 2
		move.h = move.v = 2;
		gBoard[TRANSLATE(2, 2)] = FRIEND;
		*yourMove = move;
		gMoveNum = 2;
		return;
	}
	
	if (gMoveNum == 2) { // *** MOVE 3
		if (opponentsMove.h == 3 && opponentsMove.v == 3)
			move.h = move.v = -3;
		else
			move.h = move.v = 3;
		gBoard[TRANSLATE(move.h, move.v)] = FRIEND;
		*yourMove = move;
		gMoveNum = 3;
		return;
	}
	
	gChangesEnd = gChanges;

	gCumCapturesEnemy += MyFindCaptures(opponentCaptures, &gBoard[i], ENEMY, FRIEND);

	for (pSq = gEstimatesStart; pSq != gEstimatesEnd; ++pSq)
		*pSq = 0;
	for (pSq = gBoardStart; pSq != gBoardEnd; ++pSq)
		if (_OCCUPIED(*pSq)) {
			d = gDirections;
			do {
				pNewSq = pSq + *d;
				if (_EMPTY(*pNewSq)) {
					gEstimates[GET_INDEX(pNewSq)] += ESTIMATE_PLUS;
					pNewSq += *d;
					if (_EMPTY(*pNewSq)) {
						gEstimates[GET_INDEX(pNewSq)] += ESTIMATE_PLUS;
					}
				}
			} while (++d != gDirectionsEnd);
		}

	for (i=0; i<SEARCH_DEPTH; ++i)
		gKillers[i] = NULL;
	bestScore = -INFINITY;
	pBestMove = NULL;
	for (pSq = gBoardStart; pSq != gBoardEnd; ++pSq)
		if (gEstimates[GET_INDEX(pSq)]) {
			gScore = gEstimates[GET_INDEX(pSq)];
			score = AddStone(-INFINITY, -bestScore, pSq, FRIEND, SEARCH_DEPTH-1,
							gCumCapturesFriend, gCumCapturesEnemy);
			if (score > bestScore) {
				bestScore = score;
				pBestMove = pSq;
			}
		}
		
	if (bestScore == -INFINITY) {
		for (pSq = gBoardStart; pSq != gBoardEnd; ++pSq)
			if (_EMPTY(*pSq) && !gEstimates[GET_INDEX(pSq)]) {
				gScore = 0;
				score = AddStone(-INFINITY, -bestScore, pSq, FRIEND, SEARCH_DEPTH-1,
								gCumCapturesFriend, gCumCapturesEnemy);
				if (score > bestScore) {
					bestScore = score;
					pBestMove = pSq;
				}
			}
		if (bestScore == -INFINITY) {
			DebugStr("\p no move found - Pente");
			return;  //no move
		}
	}
	
	*pBestMove = FRIEND;
	i = GET_INDEX(pBestMove);
	move.h = GET_X(i);
	move.v = GET_Y(i);
	*yourMove = move;
	++gMoveNum;

	// find captures
	*numCaptures = MyFindCaptures(claimCaptures, pBestMove, FRIEND, ENEMY);
	gCumCapturesFriend += *numCaptures;
	if (gCumCapturesFriend >= 5) {
		*claimVictory = true;
		return;
	}

	fiveInARow = MyFindFive(pBestMove, FRIEND);
	*claimVictory = fiveInARow;
}

void TermPente(void) {
}

#include <stdio.h>
#include <stdlib.h>
// gScore is absolute: + for FRIEND
// returns score of how good it is for color
// alpha and beta apply for the opponent after the move is made
short AddStone(short alpha, short beta, short *pSq, short color, short depth,
				short capturesFriend, short capturesEnemy)
{
	short v, x, *pNewSq, bestScore, saveScore, *d, *killer, t, open;
	short count[3];
	short opponent = OPPONENT(color);
	short score = 0;
	
#ifdef WRITE
	extern FILE *outFile;
	short i;
	for (i=0; i<SEARCH_DEPTH-1-depth; i++)
		fprintf(outFile, "  ");
	i = GET_INDEX(pSq);
	fprintf(outFile,"d=%d c=%d s=%d a=%d b=%d xy=%d,%d\n",
				depth, color, gScore, alpha, beta, (short)GET_X(i), (short)GET_Y(i));
	fflush(outFile);
#endif	
	START_SAVE;
	PUSH_SQ(pSq);
	*pSq = color; // Add stone
	
	d = gDirections;
	do {
		pNewSq = pSq + *d;
		v = *pNewSq;
		
		if (v == EMPTY) {
			//*pSq = EMPTY_ADJACENT;
			
		} else if (v == color) {
			if (d <= &gDirections[3]) {
				x = 1;
				open = 0;
				for (pNewSq += *d; *pNewSq == color; pNewSq += *d)
					++x;
				if (_EMPTY(*pNewSq))
					open = 1;
				for (pNewSq = pSq + *(d+4); *pNewSq == color; pNewSq += *(d+4))
					++x;
				if (_EMPTY(*pNewSq))
					++open;
			} else {
				v = *(pSq + *(d-4));
				if (v == color)
					continue;
				x = 1;
				open = 0;
				if (_EMPTY(v))
					open = 1;
				for (pNewSq += *d; *pNewSq == color; pNewSq += *d)
					++x;
				if (_EMPTY(*pNewSq))
					++open;
			}
			score += CHAIN_SCORE[open][x];
			if (x >= 4) // 5-in-a-row
				depth = 0; // game over
			
		} else if (v == opponent) {
			x = 1;
			for (pNewSq += *d; *pNewSq == opponent; pNewSq += *d)
				++x;
			if (x == 2 && *pNewSq == color) {
				score += CAPTURE_SCORE;
				pNewSq = pSq + *d;
				PUSH_SQ(pNewSq);
				*pNewSq = EMPTY;
				pNewSq += *d;
				PUSH_SQ(pNewSq);
				*pNewSq = EMPTY;
				if (color == FRIEND) {
					if (++capturesFriend >= 5)
						depth = 0; // game over
				} else { // color == ENEMY
					if (++capturesEnemy >= 5)
						depth = 0; // game over
				}
			} else
				score += BLOCK_SCORE[x];
		}
	} while (++d != gDirectionsEnd);
	
	if (color != FRIEND)
		score = -score;
	gScore += score;
	
	if (depth) {
		--depth;
		bestScore = -INFINITY;
		
		// Killer move?
		killer = gKillers[depth];
		if (killer && _EMPTY(*killer)) {
			bestScore = AddStone(-beta, -alpha, killer, opponent, depth, capturesFriend, capturesEnemy);
			if (bestScore > alpha) {
				if (bestScore >= beta) {
#ifdef WRITE
					++depth;
#endif
					bestScore = -bestScore;
					goto RESTORE;
				}
				alpha = bestScore;
			}
		}
		
		for (pSq = gBoardStart; pSq != gBoardEnd; ++pSq) {
			if (_EMPTY(*pSq) && pSq != killer) {
				count[FRIEND] = count[ENEMY] = 0;
				d = gDirections;
				do {
					pNewSq = pSq + *d;
					if (_OCCUPIED(*pNewSq)) {
						if (count[*pNewSq]++ || *(pNewSq + *d) == *pNewSq)
							break;
					}
				} while (++d != gDirectionsEnd);
				if (d != gDirectionsEnd) {
					saveScore = gScore;
						t = AddStone(-beta, -alpha, pSq, opponent, depth, capturesFriend, capturesEnemy);
					gScore = saveScore;
					if (t > bestScore) {
						bestScore = t;
						if (t > alpha) {
#ifdef WRITE
							fprintf(outFile, "       ....%d > %s\n", t, (t>beta) ? "beta" : "alpha");
#endif	
							if (t >= beta) {
								gKillers[depth] = pSq;
								break;
							}
							if (depth >= SEARCH_DEPTH-2)
								gKillers[depth] = pSq;
							alpha = t;
						}
					}
				}
			}
		}
#ifdef WRITE
		++depth;
#endif
		if (bestScore == -INFINITY)
			goto TERMINAL;
		bestScore = -bestScore;
	} else {
TERMINAL:
		bestScore = gScore;
		if (color != FRIEND)
			bestScore = -bestScore;
	}

RESTORE:	
	// Restore
	while (POP) {
		pSq = (short *)TOP;
		*pSq = POP;
	}

#ifdef WRITE
	for (i=0; i<SEARCH_DEPTH-1-depth; i++)
		fprintf(outFile, "  ");
	fprintf(outFile,"==%d\n", bestScore);
	fflush(outFile);
#endif	

	return bestScore;
}

long MyFindCaptures(Capture capture[], short *pSq, short us, short them)
{
	short i, *p1, *p2;
	short myCaptures = 0;
	short *d = gDirections;
	
	do {
		p1 = pSq + *d;
		if (*p1 == them) {
			p2 = p1 + *d;
			if (*p2 == them && *(p2 + *d) == us) {
				*p1 = *p2 = EMPTY;
				i = GET_INDEX(p1);
				capture[myCaptures].stone1.h = GET_X(i);
				capture[myCaptures].stone1.v = GET_Y(i);
				i = GET_INDEX(p2);
				capture[myCaptures].stone2.h = GET_X(i);
				capture[myCaptures].stone2.v = GET_Y(i);
//fprintf(outFile,"removing stone1 (%d,%d)\n",capture[myCaptures].stone1.v,capture[myCaptures].stone1.h);
//fprintf(outFile,"removing stone2 (%d,%d)\n",capture[myCaptures].stone2.v,capture[myCaptures].stone2.h);
				++myCaptures;
			}
		}
	} while (++d != gDirectionsEnd);
	return myCaptures;
}

Boolean MyFindFive(short *pSq, short color)
{
	short x, *d, *pNewSq, i;
	
	d = gDirections;
	i = 0;
	do {
		x = 0;
		for (pNewSq = pSq + *d;     *pNewSq == color; pNewSq += *d)
			++x;
		for (pNewSq = pSq + *(d+4); *pNewSq == color; pNewSq += *(d+4))
			++x;
		if (x >= 4)
			return true;
		++d;
	} while (++i < 4);
	return false;
}
