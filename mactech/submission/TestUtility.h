//
// PenteUtility.h
// Copyright © 1997 J. Robert Boonstra II
//
#include "Pente.h"

#define kAppleMenu 128
#define kPenteMenu 129

typedef enum {kEmpty=0,kWhite,kBlack,kMyPiece,kOpponentPiece} PlayerColor;

enum {kPlayWhiteItem=1,kPlayBlackItem,kQuitItem};

#define kBoardOffset 15
#define kBoardMaxSize (2*kBoardOffset+2)

typedef PlayerColor Board[kBoardMaxSize][kBoardMaxSize];

extern long rowDir[],colDir[];
extern char pieceSymbolOrig[];

typedef void InitPenteProc(
	long boardHalfSize		/* e.g., 9 for a 19x19 board */
							/* all coordinates between -boardHalfSize
								and +boardHalfSize */
);

typedef void PenteProc(
	Point opponentsMove,		/* your opponent moved here */
	Boolean playingFirst,		/* ignore opponentMove */
	Point *yourMove,			/* return your move here */
	Capture claimCaptures[],	/* return coordinates of captured pairs here */
	long *numCaptures,			/* return number of claimCaptures here */
	Boolean *claimVictory		/* return true if you claim victory with this move */
);

typedef void TermPenteProc(void);	/* deallocate any dynamic storage */

#define BoardValue(board,row,col) \
  ((board)[kBoardOffset+row][kBoardOffset+col])

void PrintBoard(Board board, long boardHalfSize, char *s);
long FindCaptures(Board board, long boardHalfSize, Capture capture[], Point move,
		PlayerColor myPiece, PlayerColor opponentPiece);
Boolean FindFive(Board board, long halfSize, Point move, PlayerColor player);
Boolean LegalSquare(Point move,long halfSize);
