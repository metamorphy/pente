//
// PenteUtility.c
// Copyright © 1997 J. Robert Boonstra II
//
#include <stdio.h>
#include <stdlib.h>

#include "Pente.h"
#include "TestUtility.h"

extern FILE *outFile;

char pieceSymbolOrig[] = {' ','O','¥','O','¥'};

long colDir[8] = { 1, 1, 0,-1,-1,-1, 0, 1};
long rowDir[8] = { 0,-1,-1,-1, 0, 1, 1, 1};


Boolean LegalSquare(Point move,long halfSize)
{
	return ((move.h<halfSize) && (move.h>-halfSize) && (move.v<halfSize) && (move.v>-halfSize));
}

void PrintBoard(Board board,long boardHalfSize,char *s)
{
long i,j;
	fprintf(outFile,"%s\n    ",s);
	for (i=-boardHalfSize; i<=boardHalfSize; i++) {
		fprintf(outFile,"%2d ",i);
	}
	fprintf(outFile,"\n");
	for (i=-boardHalfSize; i<=boardHalfSize; i++) {
		fprintf(outFile,"%2d ",i);
		for (j=-boardHalfSize; j<=boardHalfSize; j++) {
			PlayerColor p = BoardValue(board,i,j);
			char outputSymbol;
			if ((p>4) || (p<0)) outputSymbol = 'x';
			else outputSymbol = pieceSymbolOrig[p];
			fprintf(outFile," %2c",outputSymbol);
		}
		fprintf(outFile,"\n");
	}
	fprintf(outFile,"\n");
	fflush(outFile);
}

long FindCaptures(Board board, long boardHalfSize, Capture capture[], Point move,
		PlayerColor myPiece, PlayerColor opponentPiece)
{
	Point pt;
	long dir,myCaptures;
	myCaptures = 0;
	for (dir=0; dir<8; dir++) {
		if (move.v+3*rowDir[dir] >  boardHalfSize) continue;
		if (move.v+3*rowDir[dir] < -boardHalfSize) continue;
		if (move.h+3*colDir[dir] >  boardHalfSize) continue;
		if (move.h+3*colDir[dir] < -boardHalfSize) continue;
		if (BoardValue(board,move.v+rowDir[dir],move.h+colDir[dir])!=opponentPiece) continue;
		if (BoardValue(board,move.v+2*rowDir[dir],move.h+2*colDir[dir])!=opponentPiece) continue;
		if (BoardValue(board,move.v+3*rowDir[dir],move.h+3*colDir[dir])!=myPiece) continue;
		pt.h = move.h + colDir[dir];	pt.v = move.v + rowDir[dir];
		capture[myCaptures].stone1 = pt;
fprintf(outFile,"removing stone1 (%d,%d)\n",capture[myCaptures].stone1.v,capture[myCaptures].stone1.h);
		pt.h = move.h + 2*colDir[dir];	pt.v = move.v + 2*rowDir[dir];
		capture[myCaptures].stone2 = pt;
fprintf(outFile,"removing stone2 (%d,%d)\n",capture[myCaptures].stone2.v,capture[myCaptures].stone2.h);
		myCaptures++;
	}
	return myCaptures;
}

Boolean FindFive(Board board, long halfSize, Point move, PlayerColor player)
{
	long dir,numInARow;
	Point saveMove;
	Boolean victory = false;
	for (dir=0; dir<4; dir++) {
		numInARow = 1;
		saveMove = move;
		move.v += rowDir[dir];
		move.h += colDir[dir];
		while ( (move.v <=  halfSize) && (move.h <=  halfSize) &&
				(move.v >= -halfSize) && (move.h >= -halfSize) ) {
			if (BoardValue(board,move.v,move.h) != player) break;
			++numInARow;
			move.v += rowDir[dir];
			move.h += colDir[dir];
		}
		move = saveMove;
		move.v -= rowDir[dir];
		move.h -= colDir[dir];
		while ( (move.v <=  halfSize) && (move.h <=  halfSize) &&
				(move.v >= -halfSize) && (move.h >= -halfSize) ) {
			if (BoardValue(board,move.v,move.h) != player) break;
			++numInARow;
			move.v -= rowDir[dir];
			move.h -= colDir[dir];
		}
		move = saveMove;
		if (numInARow >= 5) victory = true;
	}
	return victory;
}