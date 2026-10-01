//PenteX.h

void InitPenteX(
	long boardHalfSize		/* e.g., 9 for a 19x19 board */
							/* all coordinates between -boardHalfSize
								and +boardHalfSize */
);

void PenteX(
	Point opponentsMove,		/* your opponent moved here */
	Boolean playingFirst,		/* ignore opponentMove */
	Point *yourMove,			/* return your move here */
	Capture claimCaptures[],	/* return coordinates of captured pairs here */
	long *numCaptures,			/* return number of claimCaptures here */
	Boolean *claimVictory		/* return true if you claim victory with this move */
);

void TermPenteX(void);	/* deallocate any dynamic storage */
