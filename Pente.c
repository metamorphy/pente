//
// Pente.c
// Copyright © 1997 Jeff Mallett
//
// For game AIs, contact me at: jeffm@zillions-of-games.com
//
// This is my solution to the Pente¨ Programmer's Challenge
//
// Do a depth=1 search to sort moves.  Then do a higher
//  depth alpha-beta search to select the best move.

#include "Pente.h"

#define XPLAYER
//#define WRITE
#define SEARCH_DEPTH 4

#ifdef XPLAYER
	#include "PenteX.h"
#endif

#ifdef WRITE
	#include <stdio.h>
	#include <stdlib.h>
#endif

enum {
	FRIEND = 0x0001,
	ENEMY  = 0x0002,
	EMPTY  = 0x0004,
	WALL   = 0x0008
};

#define BORDER				1
#define BORDERS				2
#define INFINITY			32600
#define NA						0
#define ESTIMATE_PLUS	1
#define WIN						9999
#define CAPTURE_SCORE	240

static short CHAIN_SCORE[3][3][9] = { //[color][open][count]
 {
	{ NA, NA, NA, NA, NA, NA, NA, NA, NA },
	{ NA, NA, NA, NA, NA, NA, NA, NA, NA },
	{ NA, NA, NA, NA, NA, NA, NA, NA, NA }
 },
 { // FRIEND
	//     2   3   4    5+
	{ NA, -1,  3, 240, WIN, WIN, WIN, WIN, WIN },// 0 open
	{ NA,-15, 16, 280, WIN, WIN, WIN, WIN, WIN },// 1 open
	{ NA, -2, 45, 800, WIN, WIN, WIN, WIN, WIN } // 2 open
 },
 { // ENEMY
	//     2   3   4    5+
	{ NA,  1, -1,-120,-WIN,-WIN,-WIN,-WIN,-WIN },// 0 open
	{ NA, 15, -8,-140,-WIN,-WIN,-WIN,-WIN,-WIN },// 1 open
	{ NA,  2,-40,-800,-WIN,-WIN,-WIN,-WIN,-WIN } // 2 open
 }
};
static short BLOCK_SCORE[3][5] = {
	{ NA, NA, NA, NA, NA },
 //     1   2   3   4
	{ NA, 0, 10, 30, 110 }, // FRIEND
	{ NA, 0,-10,-30,-110 }  // ENEMY
};

static short gPreScore[33*33], gEstimates[33*33];
static short *gEstimatesStart, *gEstimatesEnd;
static short gBoard[33*33], *gBoardStart, *gBoardEnd;
static short *gFirstStone, *gLastStone;
static short *gKillers[SEARCH_DEPTH];
static short gDirections[8], *gDirectionsEnd;
static short gMoveNum, gScore, gStartDepth;
static short gBoardHalfSize, gSideLength, gBoardMax;
static short gCumCapturesFriend, gCumCapturesEnemy;
static short gAdjustment, gSE;

#define ADJUST(a)				( (a) + gAdjustment )
#define TRANSLATE(x, y) \
	(gSideLength * ADJUST(y) + ADJUST(x))

#define GET_INDEX(p)	((p) - gBoard)
#define GET_X(i)			( ((i) % gSideLength) - gAdjustment )
#define GET_Y(i)			( ((i) / gSideLength) - gAdjustment )

#define UPDATE_ENDPOINTS(pSq, a, b) \
	if (pSq < a) a = pSq; \
	if (pSq > b) b = pSq

#define OPPONENT(side)	(3 - (side))

#define VALUE(x) gBoard[x]

#define _ON_BOARD(x) (((x) & WALL) == 0)
#define ON_BOARD(i) _ON_BOARD(gBoard[i])

#define _OCCUPIED(x) ((x) & (FRIEND|ENEMY))
#define OCCUPIED(i) _OCCUPIED(gBoard[i])

#define _EMPTY(x) ((x) & EMPTY)
#define EMPTY(i) _EMPTY(gBoard[i])

#define HAS_STONE_OF_COLOR(x, c) (gBoard[x] == (c))


// gChanges -- Array of unsigned longs containing data to
//  undo moves
//     list of:
//          <pointer to square> <old square value>
//     terminated by a OL
//   The first position will be the drop square and the
//     others will be flips
static unsigned long gChanges[256], *gChangesEnd;
#define PUSH(x)				*(gChangesEnd++) = (x)
#define START_SAVE		PUSH(0L)
#define PUSH_SQ(pSq) \
		{ PUSH((long)*(pSq)); PUSH((unsigned long)(pSq)); }
#define POP						*(--gChangesEnd)
#define TOP						*gChangesEnd


static short * ChooseNextMove();
static short AddStone(short alpha, short beta, short *pSq,
								short color, short depth,
								short capturesFriend, short capturesEnemy,
								short *firstStone, short *lastStone);
static long MyFindCaptures(Capture capture[], short *pSq,
								short us, short them);
static Boolean MyFindFive(short *pSq, short color);


// ***** 
// ***** InitPente
// ***** 
#ifdef XPLAYER
void InitPenteX(
#else
void InitPente(
#endif
	long boardHalfSize		/* e.g., 9 for a 19x19 board */
							/* all coordinates between -boardHalfSize
								and +boardHalfSize */
) {
	short i, j, *pSq;
	
	gBoardHalfSize = boardHalfSize;
	gSideLength = boardHalfSize * 2 + 1 + BORDERS;
	
	gSE = gSideLength + 1;
	gDirections[0] = -gSE;             // NW
	gDirections[1] = -gSideLength;     // N
	gDirections[2] = -gSideLength + 1; // NE
	gDirections[3] = -1;               // W
	gDirections[4] = gSE;              // SE
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
	
	gFirstStone = gBoardEnd;
	gLastStone = gBoardStart;
	
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
	for (i = BORDER + boardHalfSize * 2 + 1;
				i<gBoardMax-gSideLength; i += gSideLength)
		for (j=0; j<BORDERS; ++j)
			gBoard[i+j] = WALL;
		
	gCumCapturesFriend = gCumCapturesEnemy = gMoveNum = 0;
	gAdjustment = gBoardHalfSize + BORDER;
	
	gStartDepth = SEARCH_DEPTH;
	if (boardHalfSize >= 13)
		gStartDepth -= 2;
	if (gStartDepth < 2)
		gStartDepth = 2;
	--gStartDepth;
}

// ***** 
// ***** Pente
// ***** 
#ifdef XPLAYER
void PenteX(
#else
void Pente(
#endif
	Point opponentsMove,		/* your opponent moved here */
	Boolean playingFirst,		/* ignore opponentMove */
	Point *yourMove,				/* return your move here */
	Capture claimCaptures[],/* return captured pairs here */
	long *numCaptures,			/* return # of claimCaptures */
	Boolean *claimVictory		/* return true if you claim
														victory with this move */
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
	pSq = &gBoard[i];
	*pSq = ENEMY;
	UPDATE_ENDPOINTS(pSq, gFirstStone, gLastStone);
	++gMoveNum;

	if (gMoveNum == 1) { // *** MOVE 2
		move.h = move.v = -2;
		pSq = &gBoard[TRANSLATE(-2, -2)];
		*pSq = FRIEND;
		UPDATE_ENDPOINTS(pSq, gFirstStone, gLastStone);
		*yourMove = move;
		gMoveNum = 2;
		return;
	}
	
	if (gMoveNum == 2) { // *** MOVE 3
		if (opponentsMove.v < opponentsMove.h) {
			if (opponentsMove.v < -opponentsMove.h) {
				// top triangle
				move.h = 0;
				move.v = 4;
			} else {
				// right triangle
				move.h = -4;
				move.v = 0;
			}
		} else {
			if (opponentsMove.v >= -opponentsMove.h) {
				// bottom triangle
				move.h = 0;
				move.v = -4;
			} else {
				// left triangle
				move.h = 4;
				move.v = 0;
			}
		}
		pSq = &gBoard[TRANSLATE(move.h, move.v)];
		*pSq = FRIEND;
		UPDATE_ENDPOINTS(pSq, gFirstStone, gLastStone);
		*yourMove = move;
		gMoveNum = 3;
		return;
	}
	
	gChangesEnd = gChanges;

	gCumCapturesEnemy +=
			MyFindCaptures(opponentCaptures, &gBoard[i],
											ENEMY, FRIEND);

	for (pSq = gEstimatesStart; pSq != gEstimatesEnd; ++pSq)
		*pSq = 0;
	for (pSq = gFirstStone; pSq <= gLastStone; ++pSq)
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

	for (pSq = gBoardStart; pSq != gBoardEnd; ++pSq)
		if (gEstimates[GET_INDEX(pSq)]) {
			i = GET_INDEX(pSq);
			gScore = gEstimates[i];
			gPreScore[i] = AddStone(-INFINITY, INFINITY, pSq,
												FRIEND, 0, gCumCapturesFriend,
												gCumCapturesEnemy, gFirstStone,
												gLastStone);
		}

	for (i=0; i<SEARCH_DEPTH; ++i)
		gKillers[i] = NULL;
	bestScore = -INFINITY;
	pBestMove = NULL;
	pSq = ChooseNextMove();
	while (pSq) {
		i = GET_INDEX(pSq);
		gScore = gEstimates[i];
		score = AddStone(-INFINITY, -bestScore, pSq, FRIEND,
							gStartDepth, gCumCapturesFriend,
							gCumCapturesEnemy, gFirstStone, gLastStone);
		gEstimates[i] = -1; // searched
		if (score > bestScore) {
			bestScore = score;
			pBestMove = pSq;
		}
		pSq = ChooseNextMove();
	}
		
	if (bestScore == -INFINITY) {
		for (pSq = gBoardStart; pSq != gBoardEnd; ++pSq)
			if (_EMPTY(*pSq) && !gEstimates[GET_INDEX(pSq)]) {
				gScore = 0;
				score = AddStone(-INFINITY, -bestScore, pSq, FRIEND, gStartDepth,
									gCumCapturesFriend, gCumCapturesEnemy,
									gFirstStone, gLastStone);
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
	*numCaptures = MyFindCaptures(claimCaptures, pBestMove,
										FRIEND, ENEMY);
	gCumCapturesFriend += *numCaptures;
	if (gCumCapturesFriend >= 5) {
		*claimVictory = true;
		return;
	}

	fiveInARow = MyFindFive(pBestMove, FRIEND);
	*claimVictory = fiveInARow;
}

// ***** 
// ***** TermPente
// ***** 
#ifdef XPLAYER
void TermPenteX(void) { }
#else
void TermPente(void) { }
#endif

// ***** 
// ***** ChooseNextMove
// ***** 
short * ChooseNextMove()
{
	short i, *pSq;
	short *found = NULL;
	short high = -INFINITY;
	for (pSq = gBoardStart; pSq != gBoardEnd; ++pSq) {
		i = GET_INDEX(pSq);
		if (gEstimates[i] > 0 && gPreScore[i] > high) {
			found = pSq;
			high = gPreScore[i];
		}
	}
	
	return found;
}

// ***** 
// ***** AddStone
// ***** 
// gScore is absolute: + for FRIEND
// returns score of how good it is for color
// alpha and beta apply for the opponent after the move
//   is made
short AddStone(short alpha, short beta, short *pSq,
				short color, short depth, short capturesFriend,
				short capturesEnemy, short *firstStone,
				short *lastStone)
{
	short x, *pNewSq, bestScore, saveScore, *d, *killer;
	short t, open, *pEnd;
	short count[3];
	short opponent = OPPONENT(color);
	
#ifdef WRITE
	extern FILE *outFile;
	short i;
	for (i=0; i<gStartDepth-depth; i++)
		fprintf(outFile, "  ");
	i = GET_INDEX(pSq);
	fprintf(outFile,"d=%d c=%d s=%d a=%d b=%d xy=%d,%d\n",
					depth, color, gScore, alpha, beta,
					(short)GET_X(i), (short)GET_Y(i));
	fflush(outFile);
#endif	
	START_SAVE;
	PUSH_SQ(pSq);
	*pSq = color; // Add stone
	UPDATE_ENDPOINTS(pSq, firstStone, lastStone);
	
	d = gDirections;
	do {
		pNewSq = pSq + *d;
		
		if (*pNewSq == color) { // Next to friend
			if (d <= &gDirections[3]) {
				x = 1;
				open = 0;
				for (pNewSq += *d; *pNewSq == color; pNewSq += *d)
					++x;
				if (_EMPTY(*pNewSq))
					open = 1;
				for (pNewSq = pSq + *(d+4); *pNewSq == color;
							pNewSq += *(d+4))
					++x;
				if (_EMPTY(*pNewSq))
					++open;
			} else {
				t = *(pSq + *(d-4));
				if (t == color)
					continue;
				x = 1;
				open = 0;
				if (_EMPTY(t))
					open = 1;
				for (pNewSq += *d; *pNewSq == color; pNewSq += *d)
					++x;
				if (_EMPTY(*pNewSq))
					++open;
			}
			gScore += CHAIN_SCORE[color][open][x];
			if (x >= 4) // 5-in-a-row
				depth = 0; // game over
			
		} else if (*pNewSq == opponent) { // Next to enemy
			x = 1;
			for (pNewSq += *d; *pNewSq == opponent; pNewSq += *d)
				++x;
			if (x == 2 && *pNewSq == color) {
				t = CAPTURE_SCORE;
				if (color != FRIEND)
					t = -t;
				gScore += t;
				
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
			} else {
				gScore += BLOCK_SCORE[color][x];
			}
		}
	} while (++d != gDirectionsEnd);
	
	if (depth) {
		--depth;
		bestScore = -INFINITY;
		
		// Killer move?
		killer = gKillers[depth];
		if (killer && _EMPTY(*killer)) {
			saveScore = gScore;
				bestScore = AddStone(-beta, -alpha, killer,
											opponent, depth, capturesFriend,
											capturesEnemy, firstStone, lastStone);
			gScore = saveScore;
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
		
		pEnd = lastStone + gSE;
		if (pEnd >= gBoardEnd)
			pEnd = gBoardEnd - 1;
		pSq = firstStone - gSE;
		if (pSq < gBoardStart)
			pSq = gBoardStart;
		do {
			if (_EMPTY(*pSq) && pSq != killer) {
				count[FRIEND] = count[ENEMY] = 0;
				d = gDirections;
				do {
					pNewSq = pSq + *d;
					if (_OCCUPIED(*pNewSq)) {
						if (count[*pNewSq]++ ||
								*(pNewSq + *d) == *pNewSq)
							break;
					}
				} while (++d != gDirectionsEnd);
				if (d != gDirectionsEnd) {
					saveScore = gScore;
						t = AddStone(-beta, -alpha, pSq, opponent,
									depth, capturesFriend, capturesEnemy,
									firstStone, lastStone);
					gScore = saveScore;
					if (t > bestScore) {
						bestScore = t;
						if (t > alpha) {
#ifdef WRITE
							fprintf(outFile, "       ....%d > %s\n",
									t, (t>beta) ? "beta" : "alpha");
#endif	
							if (t >= beta) {
								gKillers[depth] = pSq;
								break;
							}
							if (depth >= gStartDepth-1)
								gKillers[depth] = pSq;
							alpha = t;
						}
					}
				}
			}
		} while (++pSq <= pEnd);
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
	for (i=0; i<gStartDepth-depth; i++)
		fprintf(outFile, "  ");
	fprintf(outFile,"==%d\n", bestScore);
	fflush(outFile);
#endif	

	return bestScore;
}

// ***** 
// ***** MyFindCaptures
// ***** 
long MyFindCaptures(Capture capture[], short *pSq,
										short us, short them)
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
				++myCaptures;
			}
		}
	} while (++d != gDirectionsEnd);
	return myCaptures;
}

// ***** 
// ***** MyFindFive
// ***** 
Boolean MyFindFive(short *pSq, short color)
{
	short x, *d, *pNewSq, i;
	
	d = gDirections;
	i = 0;
	do {
		x = 0;
		for (pNewSq = pSq + *d; *pNewSq == color; pNewSq += *d)
			++x;
		for (pNewSq = pSq + *(d+4); *pNewSq == color;
					pNewSq += *(d+4))
			++x;
		if (x >= 4)
			return true;
		++d;
	} while (++i < 4);
	return false;
}
