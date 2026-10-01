//
// PenteTest.c
// Copyright © 1997 J. Robert Boonstra II
//
#include <stdio.h>
#include <stdlib.h>
#include <timer.h>

#include "Pente.h"
#include "TestUtility.h"
#include "PenteHuman.h"
#include "TimingUtilities.h"

#include "PenteX.h" //$$$ jrm

FILE *outFile;
extern InitPenteProc InitPenteHuman;
extern PenteProc PenteHuman;
extern TermPenteProc TermPenteHuman;

static long cumCaptures[3],cumPoints[3];

static void InitMac()
{
	InitGraf(&qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(nil);
	InitCursor();
}

static Boolean InitGlobalBoard(Board board,long boardHalfSize)
{
	long row,col;

	for (row=-boardHalfSize; row<=boardHalfSize; row++) {
		for (col=-boardHalfSize; col<=boardHalfSize; col++) {
			BoardValue(board,row,col) = kEmpty;
		}
	}
	cumCaptures[kWhite] = cumCaptures[kBlack] = 0;
	cumPoints[kWhite] = cumPoints[kBlack] = 0;
	return true;
}

static void SwapStones(Capture *theCapture)
{
	Point pt = theCapture->stone1;
	theCapture->stone1 = theCapture->stone2;
	theCapture->stone2 = pt;
}

static void SortCaptureList(Capture captures[], long numCaptures)
{
/* slow sort, good enough for the small number of possible captures */
	long i,j;
	Capture aCapture;
	for (i=0; i<numCaptures; i++) {
		if (captures[i].stone1.h < captures[i].stone2.h) continue;
		if (captures[i].stone1.h == captures[i].stone2.h) {
			if (captures[i].stone1.v < captures[i].stone2.v) continue;
			SwapStones(&captures[i]);
		}
	}
	if (numCaptures<2) return;
	for (i=0; i<numCaptures-1; i++) {
		for (j=i+1; j<numCaptures; j++) {
			if (captures[i].stone1.h < captures[j].stone1.h) continue;
			if (captures[i].stone1.h == captures[j].stone1.h) {
				if (captures[i].stone1.v < captures[j].stone1.v) continue;
				aCapture = captures[i];
				captures[i] = captures[j];
				captures[j] = aCapture;
			}	
		}
	}
}

static void PrintCaptures(char *str,Capture captures[],long numCaptures)
{
	long i;
	printf("%s %ld\n",str,numCaptures);
	fprintf(outFile,"%s %ld\n",str,numCaptures);
	for (i=0; i<numCaptures; i++) {
		Capture *c = &captures[i];
		fprintf(outFile,"  %ld:  (%d,%d)  (%d,%d)\n",
			i,c->stone1.v,c->stone1.h,c->stone2.v,c->stone2.h);
	}
}

enum {kNoErr,kBadRowCol=1,kIllegalFirstMove,kIllegalSecondMove,kNonEmptySquare,
		kWrongNumCaptures,kBadCaptures,kFalseVictoryClaim,kVictoryClaimMissed};

static long CountFours(Board board,long halfSize,long loopLim,long player,
				long rowStart,long colStart,
				long rDelta,long cDelta,long rowDir,long colDir)
{
	long row,col,numInARow,state,numFours,counter;
	numFours = 0;
	for (counter=0; counter<=loopLim; counter++) {
		row = rowStart + counter*rDelta;
		col = colStart + counter*cDelta;
		state = 0;
		numInARow=0;
		while ( (row<=halfSize) && (col<=halfSize) & (row>=-halfSize) & (col>=-halfSize) ) {
			if (state==0) /* looking for first match */ {
				if (BoardValue(board,row,col) == player) {
					state=1;
					numInARow = 1;
				}
			} else {
				if (BoardValue(board,row,col) == player) {
					++numInARow;
				} else {
					state=0;
					if (numInARow>=4) {
						++numFours;
					}
					numInARow=0;
				}
			}
			row += rowDir;
			col += colDir;
		}
		if (numInARow>=4) {
			++numFours;
		}
	}
//fprintf(outFile,"- found %ld fours for %ld\n",numFours,player);
	return numFours;
}

static long CheckMove(PlayerColor player,Board board,long halfSize,Point move,
						Boolean first,Boolean playingSecondMove,Boolean victoryClaimed,
						Capture captures[],long numCaptures) {
	Capture trueCaptures[8];
	long numTrueCaptures,i;
	Point move1;
	Boolean captureErr,trueVictory,fiveInARow;
	PlayerColor opponent = (player==kWhite) ? kBlack : kWhite ;
	move1 = move;
	
	fprintf(outFile,"Player %c moved to (row,col) = (%ld,%ld) victory=%d, numCaptures=%ld\n",
			pieceSymbolOrig[player],move.v,move.h,victoryClaimed,numCaptures);
	/* is move within board limits */
	if ( (move.v<-halfSize) || (move.v>halfSize) || (move.h<-halfSize) || (move.h>halfSize) ) 
		return kBadRowCol;
	/* is first move legal */
	if (first && ( (move.v!=0) || (move.h!=0) ) ) return kIllegalFirstMove;
	/* is move to an empty square */
	if (BoardValue(board,move.v,move.h) != kEmpty) return kNonEmptySquare;
	/* is second move legal */
	if (playingSecondMove && ( (move.v<3) && (move.v>-3) && (move.h<3) && (move.h>-3)) ) 
		return kIllegalSecondMove;
	
	/* Find captures */
	numTrueCaptures = FindCaptures(board,halfSize,trueCaptures,move,player,opponent);
	if (numCaptures != numTrueCaptures) {
		captureErr = true;
	} else {
		SortCaptureList(trueCaptures,numTrueCaptures);
		SortCaptureList(captures,numCaptures);
		
		captureErr = false;
		for (i=0; i<numCaptures; i++) {
			if ( (*(long *)&captures[i].stone1 != *(long *)&trueCaptures[i].stone1) ||
				 (*(long *)&captures[i].stone2 != *(long *)&trueCaptures[i].stone2) ) {
				captureErr = true;
				break;
			}
		}
	}
	if (captureErr) {
		PrintCaptures("Found these captures:",captures,numCaptures);
		PrintCaptures("Expecting these captures:",trueCaptures,numTrueCaptures);
		if (numCaptures != numTrueCaptures) return kWrongNumCaptures;
		return kBadCaptures;
	}
	
	cumCaptures[player] += numCaptures;
//fprintf(outFile,"-- %ld cumCaptures for %ld\n",cumCaptures[player],player);
	
	/* check for victory */
	fiveInARow = FindFive(board,halfSize,move,player);
	trueVictory = fiveInARow || (cumCaptures[player]>=5);
	
	if (victoryClaimed && !trueVictory) return kFalseVictoryClaim;
	if (!victoryClaimed && trueVictory) return kVictoryClaimMissed;
	
	BoardValue(board,move.v,move.h) = player;
	for (i=0; i<numCaptures; i++) {
		BoardValue(board,captures[i].stone1.v,captures[i].stone1.h) = kEmpty;
		BoardValue(board,captures[i].stone2.v,captures[i].stone2.h) = kEmpty;
	}
	
	if (victoryClaimed) {
		if (fiveInARow) cumCaptures[player] += 5;
	}
		
	return kNoErr;
}

static void PrintErrorMessage(PlayerColor player,Point move,long errNum)
{
	printf("Error when %c moved to row %d col %d.\n", pieceSymbolOrig[player],move.v,move.h);
	switch (errNum) {
	case kBadRowCol:
		fprintf(outFile,"Error when %c moved to row %d col %d: Illegal row/col.\n",
				pieceSymbolOrig[player],move.v,move.h);
		break;
	case kIllegalFirstMove:
		fprintf(outFile,"Error when %c moved to row %d col %d: First move must be to 0,0.\n",
				pieceSymbolOrig[player],move.v,move.h);
		break;
	case kIllegalSecondMove:
		fprintf(outFile,"Error when %c moved to row %d col %d: Second move must be outside 5x5 box.\n",
				pieceSymbolOrig[player],move.v,move.h);
		break;
	case kNonEmptySquare:
		fprintf(outFile,"Error when %c moved to row %d col %d: Move to nonempty square.\n",
				pieceSymbolOrig[player],move.v,move.h);
		break;
	case kWrongNumCaptures:
		fprintf(outFile,"Error when %c moved to row %d col %d: Wrong number of captures.\n",
				pieceSymbolOrig[player],move.v,move.h);
		break;
	case kBadCaptures:
		fprintf(outFile,"Error when %c moved to row %d col %d: Bad list of captures.\n",
				pieceSymbolOrig[player],move.v,move.h);
		break;
	case kFalseVictoryClaim:
		fprintf(outFile,"Error when %c moved to row %d col %d: False claim of victory.\n",
				pieceSymbolOrig[player],move.v,move.h);
		break;
	case kVictoryClaimMissed:
		fprintf(outFile,"Error when %c moved to row %d col %d: Missed victory claim.\n",
				pieceSymbolOrig[player],move.v,move.h);
				break;
	default:
		fprintf(outFile,"Unknown error, value %ld\n",errNum);
	}
	fflush(outFile);
}

static void DoTestCase(PlayerColor firstPlayer,long boardHalfSize,
	InitPenteProc InitPente1, PenteProc Pente1, TermPenteProc TermPente1,
	InitPenteProc InitPente2, PenteProc Pente2, TermPenteProc TermPente2
	)
{
	Board gBoard;
	UnsignedWide startTime,endTime,diffTime,cumTime1,cumTime2;
	 //$$$long finalTick;
	Boolean reverse; //$$$

	Point opponentMove1,opponentMove2;
	Point yourMove1,yourMove2;
	Capture captures1[8],captures2[8];
	long numCaptures1,numCaptures2;
	long result1,result2;
	PlayerColor secondPlayer,player;
	float score;
	Boolean claimVictory1,claimVictory2;
	Boolean illegal,notDone = true;
	Boolean playingFirst = true, playingSecondMove = false;
	
	cumTime1.hi = cumTime1.lo = cumTime2.hi = cumTime2.lo = 0;
	
	secondPlayer = (firstPlayer == kWhite) ? kBlack : kWhite;

	illegal = InitGlobalBoard(gBoard,boardHalfSize);

	Microseconds(&startTime);
	(*InitPente1)(boardHalfSize);
	Microseconds(&endTime);
	SubWide(&endTime,&startTime,&diffTime);
	AddWideTo(&diffTime,&cumTime1);

	Microseconds(&startTime);
	(*InitPente2)(boardHalfSize);
	Microseconds(&endTime);
	SubWide(&endTime,&startTime,&diffTime);
	AddWideTo(&diffTime,&cumTime2);

	while (notDone) {
		 //$$$Delay(60,&finalTick);
		
		/* Play first */
		opponentMove1 = yourMove2;
		numCaptures1 = 0;
		claimVictory1 = false;
		Microseconds(&startTime);
		(*Pente1)(opponentMove1,playingFirst,&yourMove1,captures1,
						&numCaptures1,&claimVictory1);
		Microseconds(&endTime);
		SubWide(&endTime,&startTime,&diffTime);
		AddWideTo(&diffTime,&cumTime1);
		/* check move 1 */
		result1 = CheckMove(firstPlayer,gBoard,boardHalfSize,yourMove1,playingFirst,
			playingSecondMove,claimVictory1,captures1,numCaptures1);
		playingSecondMove = false;
		if (playingFirst) {
			playingFirst = false;
			playingSecondMove = true;
		}
		
		if (result1!=kNoErr) {
			PrintErrorMessage(firstPlayer,yourMove1,result1);
			notDone = false;
		} else if (claimVictory1) {
			fprintf(outFile,"FIRST PLAYER HAS FIVE IN A ROW OR FIVE CAPTURES\n");
			notDone = false;
		} else {
			/* Play second */
			opponentMove2 = yourMove1;
			numCaptures2 = 0;
			claimVictory2 = false;

			Microseconds(&startTime);
			(*Pente2)(opponentMove2,playingFirst,&yourMove2,captures2,
							&numCaptures2,&claimVictory2);
			Microseconds(&endTime);
			SubWide(&endTime,&startTime,&diffTime);
			AddWideTo(&diffTime,&cumTime2);
			/* check move 2 */
			result2 = CheckMove(secondPlayer,gBoard,boardHalfSize,yourMove2,playingFirst,
				false,claimVictory2,captures2,numCaptures2);
			if (result2!=kNoErr) {
				PrintErrorMessage(secondPlayer,yourMove2,result2);
				notDone = false;
			} else if (claimVictory2) {
				fprintf(outFile,"SECOND PLAYER HAS FIVE IN A ROW OR FIVE CAPTURES\n");
				notDone = false;
			}
		}
	}
	
	PrintBoard(gBoard,boardHalfSize,"Final positions:");
	
	Microseconds(&startTime);
	(*TermPente1)();
	Microseconds(&endTime);
	SubWide(&endTime,&startTime,&diffTime);
	AddWideTo(&diffTime,&cumTime1);

	Microseconds(&startTime);
	(*TermPente2)();
	Microseconds(&endTime);
	SubWide(&endTime,&startTime,&diffTime);
	AddWideTo(&diffTime,&cumTime2);

	
	for (player=kWhite; player<=kBlack; ++player) {
		cumPoints[player] = cumCaptures[player];
		/* search for 4 in a row across */
		cumPoints[player] += CountFours(gBoard,boardHalfSize,2*boardHalfSize+1,player,
				/*top left*/-boardHalfSize,-boardHalfSize, /*down rows*/1,0, /*across cols*/0,1);
		/* search for 4 in a row down */
		cumPoints[player] += CountFours(gBoard,boardHalfSize,2*boardHalfSize+1,player,
				/*top left*/-boardHalfSize,-boardHalfSize, /*across cols*/0,1, /*scan down*/1,0);
		/* search for 4 in a row diagonal up from top left */
		cumPoints[player] += CountFours(gBoard,boardHalfSize,2*boardHalfSize+1,player,
				/*top left*/-boardHalfSize,-boardHalfSize, /*down rows*/1,0, -1,1);
		/* search for 4 in a row diagonal up from bottom left */
		cumPoints[player] += CountFours(gBoard,boardHalfSize,2*boardHalfSize,player,
				/*bottom left+1*/boardHalfSize,-boardHalfSize+1, /*across cols*/0,1, -1,1);
		/* search for 4 in a row diagonal down from top right */
		cumPoints[player] += CountFours(gBoard,boardHalfSize,2*boardHalfSize+1,player,
				/*top right*/-boardHalfSize,boardHalfSize, /*back cols*/0,-1, 1,1);
		/* search for 4 in a row diagonal up from top left */
		cumPoints[player] += CountFours(gBoard,boardHalfSize,2*boardHalfSize,player,
				/*top+1 left*/-boardHalfSize+1,-boardHalfSize, /*down rows*/1,0, 1,1);
	}
	
	reverse = firstPlayer == kBlack;
	score = cumPoints[1] - (reverse ? cumTime2.lo : cumTime1.lo)/1000000.0;
 	fprintf(outFile,"Player 1(%c) took %ld : %lu microseconds, points = %ld, score=%f.\n",
				pieceSymbolOrig[1],
				reverse ? cumTime2.hi : cumTime1.hi, //$$$
				reverse ? cumTime2.lo : cumTime1.lo,
				cumPoints[1], //reverse ? cumPoints[2] : cumPoints[1],
				score);
//	printf("Player 1(%c) took %ld : %lu microseconds, points = %ld, score=%f.\n",
//				pieceSymbolOrig[1],cumTime1.hi,cumTime1.lo,cumPoints[1],score);
	score = cumPoints[2] - (reverse ? cumTime1.lo : cumTime2.lo)/1000000.0;
	fprintf(outFile,"Player 2(%c) took %ld : %lu microseconds, points = %ld, score=%f.\n",
				pieceSymbolOrig[2],
				reverse ? cumTime1.hi : cumTime2.hi, //$$$
				reverse ? cumTime1.lo : cumTime2.lo,
				cumPoints[2], // reverse ? cumPoints[1] : cumPoints[2],
				score);
//	printf("Player 2(%c) took %ld : %lu microseconds, points = %ld, score=%f.\n",
//				pieceSymbolOrig[2],cumTime2.hi,cumTime2.lo,cumPoints[2],score);
}

static Boolean DoCommand(long mResult)
{
	GrafPtr port;
	long menu,item;
	Str255 daName;
	Boolean notDone = true;
	
	menu = HiWord(mResult);
	item = LoWord(mResult);
	HiliteMenu(0);
	switch (menu) {
	case kAppleMenu:
		switch (item) {
		default:
			GetPort(&port);
			SetCursor(&qd.arrow);
			GetMenuItemText(GetMenuHandle(kAppleMenu), item, daName);
			OpenDeskAcc(daName);
			SetPort(port);
			break;
		}
		break;
	case kPenteMenu:
		switch (item) {
		case kPlayWhiteItem:
			DoTestCase(kBlack,9,
				//@@@ InitPenteX,PenteX,TermPenteX,
				InitPenteHuman,PenteHuman,TermPenteHuman,
				InitPente,Pente,TermPente);
			notDone = false;
			break;
		case kPlayBlackItem:
			DoTestCase(kWhite,9,
				InitPente,Pente,TermPente,
				//@@@ InitPenteX,PenteX,TermPenteX
				InitPenteHuman,PenteHuman,TermPenteHuman
				);
			notDone = false;
			break;
		case kQuitItem:
			notDone = false;
			break;
		}
		break;
	}
	return notDone;
}

void main(void)
{
	Handle mbar;
	EventRecord theEvent;
	Point where;
	WindowPtr wind;
	long mResult;
	unsigned char theChar;
	short modifiers,part;
	Boolean notDone = true;

	InitMac();
	mbar = GetNewMBar(128);
	SetMenuBar(mbar);
	AppendResMenu(GetMenuHandle(128), 'DRVR');
	EnableItem(GetMenuHandle(kPenteMenu),1);
	EnableItem(GetMenuHandle(kPenteMenu),2);
	DrawMenuBar();
	
	outFile = fopen("PenteDetail.out","w");
	if (0 == outFile) DebugStr("\p problem opening output file");
	
	while (notDone) {
	
		if(WaitNextEvent(everyEvent,&theEvent,60,0)) {
			modifiers = theEvent.modifiers;
			switch (theEvent.what) {
			case mouseDown:
				part = FindWindow(theEvent.where, &wind);
				switch (part) {
				case inMenuBar:
					where = theEvent.where;
					mResult = MenuSelect(where);
					notDone = DoCommand(mResult);
					break;
				}
				break;
			case keyDown:
				theChar = theEvent.message & charCodeMask;
				if (modifiers&cmdKey) {
					mResult = MenuKey(theChar);
					notDone = DoCommand(mResult);
				}
				break;
			}
		}
	}
	fclose(outFile);
}

