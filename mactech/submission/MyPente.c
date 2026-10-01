//
// MyPente.c
// Copyright © 1997 Jeff Mallett
//
// Looking for game AIs?  Please contact me at:
//
//    jeffm@zillions-of-games.com
//
// This is my solution to the Nov. '97 Programmer's Challenge.
// It plays nxn Pente¨.
//
// Do a depth=1 search to sort moves.  Then do a higher
//  depth alpha-beta search to select the best move.
//
// Plays for score rather than to win.  For example,
//  doesn't try to win by getting 5 captures since
//  there is no score incentive for this.  It could be
//  retuned to play for a win instead.

#include "Pente.h"

#define SEARCH_DEPTH 4

//#define XPLAYER
//#define WRITE

#ifdef XPLAYER
	#include "PenteX.h"
#endif

#ifdef WRITE
	#include <stdio.h>
	#include <stdlib.h>
	extern FILE *outFile;
	static Boolean gWrite = false;
#endif

enum {
	_FRIEND = 0x0001,
	_ENEMY  = 0x0002,
	_EMPTY  = 0x0004,
	_WALL   = 0x0008
};

#define BORDER				1
#define BORDERS				2
#define INFINITY			32600
#define NA						0
#define MAX_CELLS			(31 + BORDERS) * (31 + BORDERS)

#define ESTIMATE_PLUS	1
#define WIN						2000
#define CAPTURE_SCORE	240
#define VULNERABLE    150

static short CHAIN_SCORE2[3][3] = { //[color][open]
	{ NA, NA, NA },
	{ -1, -9, -2 }, // FRIEND
	{  1,  9,  2 }  // ENEMY
};
static short CHAIN_SCORE3[3][3] = { //[color][open]
	{ NA, NA, NA },
	{  3, 16, 40 }, // FRIEND
	{ -2,-10,-30 }  // ENEMY
};
static short CHAIN_SCORE4[3][3] = { //[color][open]
	{   NA,  NA,  NA },
	{  230, 240, 250 }, // FRIEND
	{ -150,-155,-160 }  // ENEMY
};
static short CHAIN_SCORE5[3] = { //[color]
	NA, WIN, -WIN
};
//                                  1   2   3   4
static short BLOCK_SCORE[5] = { NA, 0, 14, 14, 22};
static short THREATS[] = {
  NA, NA,
  25, // 2: tria
  30, // 3: half-open four
  350, // 4: double trias
  370, // 5: tria + half-open four
  450, // 6: tessera or 2 half-open fours
  500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};

static short gPreScore[MAX_CELLS], gEstimates[MAX_CELLS];
static short *gEstimatesStart, *gEstimatesEnd;
static short gBoard[MAX_CELLS], *gBoardStart, *gBoardEnd;
static short *gFirstStone, *gLastStone;
static short *gKillers[SEARCH_DEPTH], gPreviousThreats;
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

#define OCCUPIED(x) 		((x) & (_FRIEND | _ENEMY))
#define EMPTY(x)				((x) == _EMPTY)

// gChanges -- Array of unsigned longs containing data to
//  undo moves
//     list of:
//          <pointer to position> <old position value>
//     terminated by a OL
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
		*pSq = _WALL;
	} while (++pSq != gBoardStart);
	do {
		*pSq = _EMPTY;
	} while (++pSq != gBoardEnd);
	do {
		*pSq = _WALL;
	} while (++pSq < &gBoard[gBoardMax]);
	for (i = BORDER + boardHalfSize * 2 + 1;
				i<gBoardMax-gSideLength; i += gSideLength)
		for (j=0; j<BORDERS; ++j)
			gBoard[i+j] = _WALL;
		
	gCumCapturesFriend = gCumCapturesEnemy = gMoveNum = 0;
	gAdjustment = gBoardHalfSize + BORDER;
	
	gStartDepth = SEARCH_DEPTH - 1;
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
	
	short *pSq, *pNewSq, *d;
	short score, bestScore, i;
	short *pBestMove;
	
	*numCaptures = 0;
	*claimVictory = false;

	if (playingFirst) { // *** MOVE 1
		move.h = move.v = 0;
		pSq = &gBoard[TRANSLATE(0, 0)];
		*pSq = _FRIEND;
		UPDATE_ENDPOINTS(pSq, gFirstStone, gLastStone);
		*yourMove = move;
		gMoveNum = 1;
		return;
	}
	
	i = TRANSLATE(opponentsMove.h, opponentsMove.v);
	pSq = &gBoard[i];
	*pSq = _ENEMY;
	UPDATE_ENDPOINTS(pSq, gFirstStone, gLastStone);
	++gMoveNum;

	if (gMoveNum == 1) { // *** MOVE 2
		move.h = move.v = -2;
		pSq = &gBoard[TRANSLATE(-2, -2)];
		*pSq = _FRIEND;
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
		*pSq = _FRIEND;
		UPDATE_ENDPOINTS(pSq, gFirstStone, gLastStone);
		*yourMove = move;
		gMoveNum = 3;
		return;
	}
	
	gChangesEnd = gChanges;

	gCumCapturesEnemy +=
			MyFindCaptures(opponentCaptures, &gBoard[i],
											_ENEMY, _FRIEND);

	for (pSq = gEstimatesStart; pSq != gEstimatesEnd; ++pSq)
		*pSq = 0;
	for (pSq = gFirstStone; pSq <= gLastStone; ++pSq)
		if (OCCUPIED(*pSq)) {
			d = gDirections;
			do {
				pNewSq = pSq + *d;
				if (EMPTY(*pNewSq)) {
					gEstimates[GET_INDEX(pNewSq)] += ESTIMATE_PLUS;
					pNewSq += *d;
					if (EMPTY(*pNewSq)) {
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
												_FRIEND, 0, gCumCapturesFriend,
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
		score = AddStone(-INFINITY, -bestScore, pSq, _FRIEND,
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
			if (EMPTY(*pSq) && !gEstimates[GET_INDEX(pSq)]) {
				gScore = 0;
				score = AddStone(-INFINITY, -bestScore, pSq, _FRIEND,
									gStartDepth, gCumCapturesFriend,
									gCumCapturesEnemy, gFirstStone, gLastStone);
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
	
	*pBestMove = _FRIEND;
	UPDATE_ENDPOINTS(pBestMove, gFirstStone, gLastStone);
	i = GET_INDEX(pBestMove);
	move.h = GET_X(i);
	move.v = GET_Y(i);
	*yourMove = move;
	++gMoveNum;

	// find captures
	*numCaptures = MyFindCaptures(claimCaptures, pBestMove,
										_FRIEND, _ENEMY);
	gCumCapturesFriend += *numCaptures;
	if (gCumCapturesFriend >= 5) {
		*claimVictory = true;
	} else {
		*claimVictory = MyFindFive(pBestMove, _FRIEND);
	}
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
// Alpha-beta search routine
// returns score of how good it is for color
// alpha and beta apply for the opponent after the move
//   is made
// gScore is absolute: + for _FRIEND
short AddStone(short alpha, short beta, short *pSq,
				short color, short depth, short capturesFriend,
				short capturesEnemy, short *firstStone,
				short *lastStone)
{
	register short *pNewSq, *d;
	short x, bestScore, t, *pEnd;
	short open, *killer;
	short threats, blocks, vulnerable;
	short opponent = OPPONENT(color);
	short saveScore = gScore;
	
#ifdef WRITE
	if (gWrite) {
		short i;
		for (i=0; i<gStartDepth-depth; i++)
			fprintf(outFile, "  ");
		i = GET_INDEX(pSq);
		fprintf(outFile,"d=%d c=%d s=%d a=%d b=%d xy=%d,%d\n",
						depth, color, gScore, alpha, beta,
						(short)GET_X(i), (short)GET_Y(i));
		fflush(outFile);
	}
#endif	
	
	START_SAVE;
	threats = blocks = vulnerable = 0;
	d = gDirections;
	do {
		pNewSq = pSq + *d;
		
		if (*pNewSq == color) { // Next to friend
			if (d <= &gDirections[3]) {
				x = 1;
				open = 0;
				for (pNewSq += *d; *pNewSq == color; pNewSq += *d)
					++x;
				if (EMPTY(*pNewSq))
					open = 1;
				for (pNewSq = pSq + *(d+4); *pNewSq == color;
							pNewSq += *(d+4))
					++x;
				if (EMPTY(*pNewSq))
					++open;
			} else {
				t = *(pSq + *(d-4));
				if (t == color)
					continue;
				x = 1;
				open = 0;
				if (EMPTY(t))
					open = 1;
				for (pNewSq += *d; *pNewSq == color; pNewSq += *d)
					++x;
				if (EMPTY(*pNewSq))
					++open;
			}
			
			switch (x) {
				case 1: // 2-in-a-row
					gScore += CHAIN_SCORE2[color][open];
					if (open == 1)
						++vulnerable;
					break;
				case 2: // 3-in-a-row
					gScore += CHAIN_SCORE3[color][open];
					if (open == 2)
						threats += 2; // tria = 2
					break;
				case 3: // 4-in-a-row
					gScore += CHAIN_SCORE4[color][open];
					threats += open * 3; // tessera = 6, half-open = 3
					break;
				default:  // 5-in-a-row (or more)
					gScore += CHAIN_SCORE5[color];
					threats = -99; // game over
					break;
			}
			
		} else if (*pNewSq == opponent) { // Next to enemy
			x = 1;
			for (pNewSq += *d; *pNewSq == opponent; pNewSq += *d)
				++x;
				
			if (EMPTY(*pNewSq)) {
				blocks += BLOCK_SCORE[x];
				
			} else if (x == 2 && *pNewSq == color) {
				t = CAPTURE_SCORE;
				if (color != _FRIEND)
					t = -t;
				gScore += t;
				
				if (color == _FRIEND) {
					if (++capturesFriend >= 5) {
						threats = -99; // game over
					}
				} else { // color == _ENEMY
					if (++capturesEnemy >= 5) {
						threats = -99; // game over
					}
				}
				if (depth) {
					pNewSq = pSq + *d;
					PUSH_SQ(pNewSq);
					*pNewSq = _EMPTY;
					pNewSq += *d;
					PUSH_SQ(pNewSq);
					*pNewSq = _EMPTY;
				}
			}
		}
	} while (++d != gDirectionsEnd);
		
	if (threats < 0) {
		// Game over
		bestScore = gScore;
		if (color != _FRIEND)
			bestScore = -bestScore;
		goto RESTORE;
	}
			
	// Not forced to make a bad move
	if (color == _FRIEND) {
		if (gScore < saveScore)
			gScore = saveScore;
	} else if (gScore > saveScore)
		gScore = saveScore;
	
	if (depth) {
		// Add stone
		PUSH_SQ(pSq);
		*pSq = color;
		UPDATE_ENDPOINTS(pSq, firstStone, lastStone);

		--depth; 
		gPreviousThreats = threats;
		bestScore = -INFINITY;
		
		// Killer move?
		killer = gKillers[depth];
		if (killer && EMPTY(*killer)) {
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
			if (EMPTY(*pSq) && pSq != killer) {
				d = gDirections;
				do {
					pNewSq = pSq + *d;
					if (OCCUPIED(*pNewSq) &&
								(*(pNewSq + *d) == *pNewSq ||
								 *(pSq    - *d) == *pNewSq))
						break;
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
							if (gWrite) {
								fprintf(outFile, "       ....%d > %s\n",
											t, (t>beta) ? "beta" : "alpha");
							}
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

		if (bestScore == -INFINITY) {
			++depth;
			goto TERMINAL;
		}
		bestScore = -bestScore;
#ifdef WRITE
		++depth;
#endif
		
	} else { // !depth
TERMINAL:
		bestScore = gScore;
		if (color != _FRIEND)
			bestScore = -bestScore;

		// Winning threats
		if (gPreviousThreats >= 5) {
			bestScore -= THREATS[gPreviousThreats];
		} else if (threats >= 5) {
			bestScore += THREATS[threats];
		} else if (gPreviousThreats == 4) {
			bestScore -= THREATS[gPreviousThreats];
		} else if (threats == 4) {
			bestScore += THREATS[threats];
		} else if (gPreviousThreats) {
			bestScore -= THREATS[gPreviousThreats];
		} else if (threats) {
			bestScore += THREATS[threats];
		}
		bestScore += blocks - vulnerable * VULNERABLE;
	}
	
RESTORE:	
	while (POP) {
		pSq = (short *)TOP;
		*pSq = POP;
	}
		
#ifdef WRITE
	if (gWrite) {
		short i;
		for (i=0; i<gStartDepth-depth; i++)
			fprintf(outFile, "  ");
		fprintf(outFile,"==%d\n", bestScore);
		fflush(outFile);
	}
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
				*p1 = *p2 = _EMPTY;
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
		x = 1;
		for (pNewSq = pSq + *d; *pNewSq == color; pNewSq += *d)
			++x;
		for (pNewSq = pSq + *(d+4); *pNewSq == color;
					pNewSq += *(d+4))
			++x;
		if (x >= 5)
			return true;
		++d;
	} while (++i < 4);
	return false;
}
