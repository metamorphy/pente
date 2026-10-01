//
// Pente.c
// Copyright © 1997 J. Robert Boonstra II
//
#include "Pente.h"

#include "TestUtility.h"

#define kMyBoardHalfSize 16

static Board myBoard;
static Boolean playedFirst;
static Boolean playingSecondMove;
static long myBoardHalfSize;
static long cumCaptures;

/*	This file implements an extrordinarily stupid Pente player, 
	who moves to a random empty location.
	It serves only to provide an opponent for the human player in PenteHuman.c.
	You should replace this entire file with your solution. */

void InitPente(
	long boardHalfSize		/* e.g., 9 for a 19x19 board */
							/* all coordinates between -boardHalfSize
								and +boardHalfSize */
) {
	long i,j;
	for (i=0; i<2*kMyBoardHalfSize; i++) {
		for (j=0; j<2*kMyBoardHalfSize; j++) {
			myBoard[i][j] = kEmpty;
		}
	}
	playedFirst = false;
	playingSecondMove = false;
	myBoardHalfSize = boardHalfSize;
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
	long startDist,dist,i,myCaptures,oppCaptures;
	Capture opponentCaptures[8];
	Boolean fiveInARow;
	
	*numCaptures = 0;
	*claimVictory = false;

	if (playingFirst) {
		move.h = move.v = 0;
		BoardValue(myBoard,move.v,move.h) = kMyPiece;
		*yourMove = move;
		*claimVictory = false;
		playedFirst = true;
		playingSecondMove = true;
		return;
	}
	BoardValue(myBoard,opponentsMove.v,opponentsMove.h) = kOpponentPiece;
	oppCaptures = FindCaptures(myBoard,myBoardHalfSize,opponentCaptures,opponentsMove,kOpponentPiece,kMyPiece);
	if (oppCaptures>0) {
		for (i=0; i<oppCaptures; i++) {
			BoardValue(myBoard,opponentCaptures[i].stone1.v,opponentCaptures[i].stone1.h) = kEmpty;
			BoardValue(myBoard,opponentCaptures[i].stone2.v,opponentCaptures[i].stone2.h) = kEmpty;
		}
	}
	if (playingSecondMove) {
		playingSecondMove = false;
		startDist = 3;
	} else {
		startDist = 1;
	}
	
	for (dist = startDist; dist<=myBoardHalfSize; dist++) {
		move.h = dist;
		move.v = dist;
		for (i=0; i<4*dist; i++) {
			if (BoardValue(myBoard,move.v,move.h) == kEmpty) {
				BoardValue(myBoard,move.v,move.h) = kMyPiece;
				*yourMove = move;
				goto foundMove;
			}
			if ( (move.h > -dist) && (move.v==dist) ) --move.h;
			else if ( (move.h==-dist) && (move.v > -dist)) --move.v;
			else if ( (move.h<dist) && (move.v == -dist)) ++move.h;
			else if ( (move.h==dist) && (move.v<dist) ) ++move.v;
			else break;
		}
	}

	DebugStr("\p no move found - Pente");
	return;  //no move
foundMove:

	/* find captures */
	myCaptures = FindCaptures(myBoard,myBoardHalfSize,claimCaptures,move,kMyPiece,kOpponentPiece);
	for (i=0; i<myCaptures; i++) {
		BoardValue(myBoard,claimCaptures[i].stone1.v,claimCaptures[i].stone1.h) = kEmpty;
		BoardValue(myBoard,claimCaptures[i].stone2.v,claimCaptures[i].stone2.h) = kEmpty;
	}

	//PrintBoard(myBoard,myBoardHalfSize,"Pente move:");
		
	cumCaptures += myCaptures;
	*numCaptures = myCaptures;
	if (cumCaptures>=5) {
		*claimVictory = true;
		return;
	}

	fiveInARow = FindFive(myBoard,myBoardHalfSize,move,kMyPiece);
	*claimVictory = fiveInARow;
}

void TermPente(void) {
}

