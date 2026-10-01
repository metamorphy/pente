# Pente

A [Pente](https://en.wikipedia.org/wiki/Pente) playing engine written in C in the fall of 1997 by Jeff Mallett.

```text
. . . . . . . . . . .
. . O . . . . . . . .
. . X X . . X . . . .
. X . O X O . . . . .
. . . . O X . . . . .
. . . O . O X . . . .
. . O . . O O O . . .
. X . . . X X X X X .
. . . . . . . . . . .
```

Players take turns placing stones. Bracketing two of the opponent's stones captures them, and the game is won with five in a row or five captured pairs. This is the end of a test game from [`misc/p1.out`](misc/p1.out), replayed from its move list: X has just completed five in a row along the bottom row.

It won the [MacTech Magazine](https://en.wikipedia.org/wiki/MacTech) Programmer's Challenge for November 1997 and was published in the February 1998 issue. Entries played a round-robin tournament, and the rules allowed boards from 19×19 up to 31×31. Each game scored 5 points for five in a row, 1 point per capture and 1 point per row of four left on the board, minus 1 point per second of thinking time. Of the three entries, this engine and Randy Boring's each won 3 games, but this engine's faster search lost far fewer points to the clock: 33 points and a score of 32.03, against Randy Boring's 23 points and -374.90.

The search is a 4-ply alpha-beta with killer moves. A depth-1 search sorts the candidate moves first. The evaluation scores chains by length and openness, blocks, captures and threats such as trias (open threes) and tesseras (open fours). It deliberately plays for contest points rather than simply to win. Everything lives in static arrays sized for the largest board.

The engine is [`Pente.c`](Pente.c). Its git history rebuilds the development from the numbered copies saved along the way (`pente3.c`, `Pente2.c`, `Pente5.c`, `Pente6.c`), ending with the submitted `MyPente.c`; each commit carries the copy's original 1997 date.

[`mactech/`](mactech/) has links to the published [challenge](http://preserve.mactech.com/articles/mactech/Vol.13/13.11/Nov97Challenge/index.html) (November 1997) and [results](http://preserve.mactech.com/articles/mactech/Vol.14/14.02/Feb98Challenge/index.html) (February 1998), a scan of the published code, and two folders:

- `submission/` is the submitted package: `MyPente.c`, the Metrowerks CodeWarrior project (`Pente Test.µ`), the test app it built (`Pente`, a classic Mac OS PowerPC executable that won't run on modern macOS), and the contest's test code with a few fixes marked `jrm`.
- `test_code/` is the test code as Bob Boonstra sent it to contestants on October 21, 1997, with its sample player and README.

[`misc/`](misc/) holds:

- Test-game logs (`*.out`). The test app writes each game to `PenteDetail.out`; the others are saved copies. `PD6.out` also contains a trace of the search.
- `PenteX.h`, for playing one version of the engine against another. Defining `XPLAYER` in `Pente.c` renames its entry points to `InitPenteX`, `PenteX` and `TermPenteX`. Build that copy alongside a normal one, and in `mactech/submission/PenteTest.c` swap each `//@@@` line for the human player's line below it; the test app's Play White and Play Black menu items then play the two engines against each other instead of against you.
- `PenteSamplePlayer.c`, the test code's sample player (it moves at random) with its functions renamed to `InitPente0`, `Pente0` and `TermPente0`.

This is historical source. It targets 1990s Macintosh C (Mac Toolbox `Point` and `Boolean`, CodeWarrior) and needs `Pente.h` from `mactech/submission/`; it is not set up as a portable build.
