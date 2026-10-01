//
// Pente.c
// Copyright © 1997 J. Robert Boonstra II
//
#include "Pente.h"
#include "TestUtility.h"
#include <stdio.h>

#define kMyBoardHalfSize 16
#define kSquareSize 20
#define kPieceSize (kSquareSize/3)
#define kBoardHOffset (10+kSquareSize)
#define kBoardVOffset (10+kSquareSize)

static Board myBoard;
static Boolean playedFirst;
static Boolean playingSecondMove;
static long myBoardHalfSize;
static long cumCaptures;
static CGrafPtr wind;

InitPenteProc InitPenteHuman;
PenteProc PenteHuman;
TermPenteProc TermPenteHuman;

static void PlotOffsetPiece(PlayerColor value,Rect *pieceRect,long row,long col,long halfSize) 							
{ 																					
	OffsetRect(pieceRect,kSquareSize*(col+halfSize),kSquareSize*(row+halfSize)); 
	if (value==kEmpty) {
		EraseRect(pieceRect);
	} else if (((value==kMyPiece) && !playedFirst) || 
			   ((value==kOpponentPiece) && playedFirst)) {	
		PaintOval(pieceRect); 														
	} else {																		
		PenSize(2,2);																
		EraseOval(pieceRect); 														
		FrameOval(pieceRect); 														
		PenNormal();																
	} 																				
	OffsetRect(pieceRect,-kSquareSize*(col+halfSize),-kSquareSize*(row+halfSize)); 	
}

static void InvalOffsetPiece(Rect *pieceRect,long row,long col,long halfSize) 							
{ 																					
	OffsetRect(pieceRect,kSquareSize*(col+halfSize),kSquareSize*(row+halfSize)); 
	EraseRect(pieceRect);
	InvalRect(pieceRect);
	OffsetRect(pieceRect,-kSquareSize*(col+halfSize),-kSquareSize*(row+halfSize)); 	
}

static void PlotBoard(Board theBoard,long boardHalfSize)
{
	long row,col;
	PlayerColor player;
	Rect pieceRect = {-kPieceSize,-kPieceSize,kPieceSize+1,kPieceSize+1};
	Rect dotRect = {-2,-2,3,3};
	long boardSize = boardHalfSize*2;
	
	OffsetRect(&pieceRect,kBoardHOffset,kBoardVOffset);
	/* draw grid */
	for (row=0; row<boardSize+1; row++) {
		MoveTo(kBoardHOffset,kBoardVOffset+row*kSquareSize);
		Line(boardSize*kSquareSize,0);
		MoveTo(kBoardHOffset+row*kSquareSize,kBoardVOffset);
		Line(0,boardSize*kSquareSize);
	}
	OffsetRect(&dotRect,kBoardHOffset+(boardHalfSize-3)*kSquareSize,kBoardVOffset+(boardHalfSize-3)*kSquareSize);
	PaintOval(&dotRect);
	OffsetRect(&dotRect,6*kSquareSize,0);
	PaintOval(&dotRect);
	OffsetRect(&dotRect,0,6*kSquareSize);
	PaintOval(&dotRect);
	OffsetRect(&dotRect,-6*kSquareSize,0);
	PaintOval(&dotRect);
	OffsetRect(&dotRect,3*kSquareSize,-3*kSquareSize);
	FrameOval(&dotRect);
	/* draw pieces */
	for (row=-boardHalfSize; row<=boardHalfSize; row++) {
		for (col=-boardHalfSize; col<=boardHalfSize; col++) {
			player = BoardValue(theBoard,row,col);
			if (player>0) {
				PlotOffsetPiece(player,&pieceRect,row,col,boardHalfSize);
			}
		}
	}
}

static void DoUpdate(CWindowPtr wind,EventRecord *theEvent,Board theBoard,long halfSize) {
	if (wind == (CWindowPtr)theEvent->message) {
		BeginUpdate((WindowPtr)wind);
		PlotBoard(theBoard,halfSize);
		EndUpdate((WindowPtr)wind);
	}
}

static Boolean DoCommand(long mResult)
{
	long menu,item;
	Boolean notDone = true;
	
	menu = HiWord(mResult);
	item = LoWord(mResult);
	HiliteMenu(0);
	switch (menu) {
	case kPenteMenu:
		switch (item) {
		case kQuitItem:
			notDone = false;
			break;
		}
		break;
	}
	return notDone;
}

void InitPenteHuman(
	long boardHalfSize		/* e.g., 9 for a 19x19 board */
							/* all coordinates between -boardHalfSize
								and +boardHalfSize */
) {
	GrafPtr savePort;
	Rect rBounds;
	long i,j;
	SetRect(&rBounds,kBoardHOffset,40+kBoardVOffset,
			3*kBoardHOffset+2*boardHalfSize*kSquareSize,40+4*kBoardVOffset+2*boardHalfSize*kSquareSize);
	for (i=-kMyBoardHalfSize; i<kMyBoardHalfSize; i++) {
		for (j=-kMyBoardHalfSize; j<kMyBoardHalfSize; j++) {
			BoardValue(myBoard,i+1,j+1) = kEmpty; // ### added +1s -- jrm
		}
	}
	playedFirst = false;
	playingSecondMove = false;
	myBoardHalfSize = boardHalfSize;

	wind = (CWindowPtr)NewCWindow(nil, &rBounds, "\pPente", true, documentProc, (WindowPtr)-1, 0, 0);
	GetPort(&savePort);
	SetPort((GrafPtr)wind);
	TextFont(geneva);
	TextFace(bold);
	TextSize(9);
	
	PlotBoard(myBoard,boardHalfSize);
}

static void MyDrawString(char *s,long halfSize,long line)
{
	Rect r;
	SetRect(&r,kBoardHOffset,kBoardVOffset-8,kBoardHOffset+2*halfSize*kSquareSize,kBoardVOffset+4);
	OffsetRect(&r,0,2*halfSize*kSquareSize+line*12);
	EraseRect(&r);
	MoveTo(r.left,r.bottom-3);
	c2pstr(s);
	DrawString((unsigned char *)s);
}

static void SetBoardValue(Board myBoard,long boardHalfSize,long row,long col,PlayerColor value)
{
	Rect pieceRect = {-kPieceSize,-kPieceSize,kPieceSize+1,kPieceSize+1};
	OffsetRect(&pieceRect,kBoardHOffset,kBoardVOffset);
	BoardValue(myBoard,row,col) = value;
	if (value != kEmpty)
		PlotOffsetPiece(value,&pieceRect,row,col,boardHalfSize);
	else
		InvalOffsetPiece(&pieceRect,row,col,boardHalfSize);
}

void PenteHuman(
	Point opponentsMove,		/* your opponent moved here */
	Boolean playingFirst,		/* ignore opponentMove */
	Point *yourMove,			/* return your move here */
	Capture claimCaptures[],	/* return coordinates of captured pairs here */
	long *numCaptures,			/* return number of claimCaptures here */
	Boolean *claimVictory		/* return true if you claim victory with this move */
) {
	Point move;
	long i,myCaptures,oppCaptures;
	Capture opponentCaptures[8];
	char s[256];
	Boolean fiveInARow,notDone;
	
	*numCaptures = 0;
	*claimVictory = false;

	if (playingFirst) {
		move.h = move.v = 0;
		SetBoardValue(myBoard,myBoardHalfSize,move.v,move.h,kMyPiece);
		*yourMove = move;
		*claimVictory = false;
		playedFirst = true;
		playingSecondMove = true;
		return;
	}
	SetBoardValue(myBoard,myBoardHalfSize,opponentsMove.v,opponentsMove.h,kOpponentPiece);
	oppCaptures = FindCaptures(myBoard,myBoardHalfSize,opponentCaptures,opponentsMove,kOpponentPiece,kMyPiece);
	sprintf(s,"OPPONENT move (%d,%d)",opponentsMove.h,opponentsMove.v);
	MyDrawString(s,myBoardHalfSize,1);
	if (oppCaptures>0) {
		for (i=0; i<oppCaptures; i++) {
			SetBoardValue(myBoard,myBoardHalfSize,opponentCaptures[i].stone1.v,opponentCaptures[i].stone1.h,kEmpty);
			SetBoardValue(myBoard,myBoardHalfSize,opponentCaptures[i].stone2.v,opponentCaptures[i].stone2.h,kEmpty);
		}
		sprintf(s,"OPPONENT captures %d",oppCaptures);
	} else {
		*s=0;
	}
	MyDrawString(s,myBoardHalfSize,2);
	notDone = true;
	while (notDone) {
		EventRecord theEvent;
		WindowPtr theWind;
		long thePart,mResult,modifiers;
		Point where;
		char theChar;
		Boolean mouseMoved,completed;
		if(WaitNextEvent(everyEvent,&theEvent,0,0)) {
			modifiers = theEvent.modifiers;
			switch (theEvent.what) {
				Point pt;
			case mouseDown:
				thePart = FindWindow(theEvent.where,&theWind);
				if ((theWind!=0) && ((WindowPtr)wind != theWind)) break;
				SetPort((WindowPtr)wind);
				pt = *(Point *)&theEvent.where;
				GlobalToLocal(&pt);
				switch (thePart) {
				case inContent:
					mouseMoved = WaitMouseUp();
					move.h = (pt.h-kBoardHOffset+kSquareSize/2) / kSquareSize - myBoardHalfSize;
					move.v = (pt.v-kBoardVOffset+kSquareSize/2) / kSquareSize - myBoardHalfSize;
					if (!LegalSquare(move,myBoardHalfSize)) {
						SysBeep(1);
					} else if (BoardValue(myBoard,move.v,move.h) != kEmpty) {
						SysBeep(1);
					} else if (playingSecondMove && ( (move.h<3)&&(move.h>-3)&&(move.v<3)&&move.v>-3)) {
						SysBeep(1);
					} else {
						SetBoardValue(myBoard,myBoardHalfSize,move.v,move.h,kMyPiece);
						*yourMove = move;
						notDone = false;
					}
					break;
				case inMenuBar:
					where = theEvent.where;
					mResult = MenuSelect(where);
					completed = !DoCommand(mResult);
					if (completed) {
						notDone=false;
						yourMove->h = yourMove->v = 0;
						*claimVictory=true;
					}
					break;
				}
				break;
			case keyDown:
				theChar = theEvent.message & charCodeMask;
				if (modifiers&cmdKey) {
					mResult = MenuKey(theChar);
					completed = !DoCommand(mResult);
					if (completed) notDone=false;
				}
				break;
			case updateEvt:
				DoUpdate(wind,&theEvent,myBoard,myBoardHalfSize);
				break;
			}
		}
	}	
foundMove:
	if (playingSecondMove) {
		playingSecondMove = false;
	}

	sprintf(s,"YOU move (%d,%d)",move.h,move.v);
	MyDrawString(s,myBoardHalfSize,3);
	/* find captures */
	myCaptures = FindCaptures(myBoard,myBoardHalfSize,claimCaptures,move,kMyPiece,kOpponentPiece);
	if (myCaptures>0) {
		for (i=0; i<myCaptures; i++) {
			SetBoardValue(myBoard,myBoardHalfSize,claimCaptures[i].stone1.v,claimCaptures[i].stone1.h,kEmpty);
			SetBoardValue(myBoard,myBoardHalfSize,claimCaptures[i].stone2.v,claimCaptures[i].stone2.h,kEmpty);
		}
		sprintf(s,"YOU capture %d",myCaptures);
	} else {
		*s=0;
	}
	MyDrawString(s,myBoardHalfSize,4);
	
	cumCaptures += myCaptures;
	*numCaptures = myCaptures;
	
	if (cumCaptures>=5) {
		*claimVictory = true;
	}

	fiveInARow = FindFive(myBoard,myBoardHalfSize,move,kMyPiece);
	if (fiveInARow) *claimVictory = true;
	
	if (*claimVictory) {
		long timeVal;
		sprintf(s,"YOU claim victory");
		MyDrawString(s,myBoardHalfSize,4);
		Delay(120,&timeVal);
	}
}

void TermPenteHuman(void) {
}

